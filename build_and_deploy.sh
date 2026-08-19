#!/usr/bin/env bash
#
# build_and_deploy.sh
#
# Configures, builds (Release), and deploys a Qt/CMake macOS app in one go:
#   1. cmake configure (Release)
#   2. cmake build
#   3. macdeployqt (bundles Qt frameworks, scans QML imports)
#   4. optional: strip unused frameworks (calls cleanup_qt_frameworks.sh if present)
#   5. optional: ad-hoc codesign + dmg
#
# Usage:
#   ./build_and_deploy.sh
#   ./build_and_deploy.sh --clean          # wipe build dir first
#   ./build_and_deploy.sh --dmg            # also produce a .dmg
#   ./build_and_deploy.sh --clean --dmg
#
# Configure the variables below for your project before running.

set -euo pipefail

# ------------------------------------------------------------------
# CONFIG — edit these for your project
# ------------------------------------------------------------------
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"   # assumes script lives at repo root; change if not
BUILD_DIR="$PROJECT_ROOT/build"
QT_PREFIX="${QT_PREFIX:-$HOME/Qt/6.11.1/macos}"                  # override: QT_PREFIX=/path/to/Qt ./build_and_deploy.sh
APP_NAME="src/app/ChatApp"                                              # <-- change to your .app target name (without .app)
QML_SOURCE_DIR="$PROJECT_ROOT"                                  # dir macdeployqt scans for QML imports
QUICK_CONTROLS_STYLE="${QUICK_CONTROLS_STYLE:-Basic}"           # only bundle this Quick Controls style
BUILD_ARCHS="${BUILD_ARCHS:-}"                                  # e.g. "arm64;x86_64" for universal; empty = native only

# ------------------------------------------------------------------
# ARGS
# ------------------------------------------------------------------
DO_CLEAN=false
DO_DMG=false
for arg in "$@"; do
    case "$arg" in
        --clean) DO_CLEAN=true ;;
        --dmg)   DO_DMG=true ;;
        *) echo "Unknown option: $arg"; exit 1 ;;
    esac
done

APP_BUNDLE="$BUILD_DIR/$APP_NAME.app"
MACDEPLOYQT="$QT_PREFIX/bin/macdeployqt"

# ------------------------------------------------------------------
# SANITY CHECKS
# ------------------------------------------------------------------
if [[ ! -x "$MACDEPLOYQT" ]]; then
    echo "ERROR: macdeployqt not found at $MACDEPLOYQT"
    echo "Set QT_PREFIX to your Qt install, e.g.:"
    echo "  QT_PREFIX=\$HOME/Qt/6.7.2/macos $0"
    exit 1
fi

if [[ "$DO_CLEAN" == true ]]; then
    echo "== Removing existing build dir =="
    rm -rf "$BUILD_DIR"
fi

# ------------------------------------------------------------------
# 1. CONFIGURE
# ------------------------------------------------------------------
echo "== Configuring (Release) =="
CMAKE_ARGS=(
    -B "$BUILD_DIR"
    -S "$PROJECT_ROOT"
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_PREFIX_PATH="$QT_PREFIX"
)
if [[ -n "$BUILD_ARCHS" ]]; then
    CMAKE_ARGS+=(-DCMAKE_OSX_ARCHITECTURES="$BUILD_ARCHS")
fi
cmake "${CMAKE_ARGS[@]}"

# ------------------------------------------------------------------
# 2. BUILD
# ------------------------------------------------------------------
echo "== Building (Release, $(sysctl -n hw.ncpu) jobs) =="
cmake --build "$BUILD_DIR" --config Release -j"$(sysctl -n hw.ncpu)"

if [[ ! -d "$APP_BUNDLE" ]]; then
    echo "ERROR: expected app bundle not found at $APP_BUNDLE"
    echo "Check APP_NAME in this script matches your CMake target output name."
    exit 1
fi

# ------------------------------------------------------------------
# 3. DEPLOY (bundle Qt frameworks + QML modules)
# ------------------------------------------------------------------
echo "== Running macdeployqt (style=$QUICK_CONTROLS_STYLE) =="
export QT_QUICK_CONTROLS_STYLE="$QUICK_CONTROLS_STYLE"

DEPLOY_ARGS=("$APP_BUNDLE" -qmldir="$QML_SOURCE_DIR")
if [[ "$DO_DMG" == true ]]; then
    DEPLOY_ARGS+=(-dmg)
fi
"$MACDEPLOYQT" "${DEPLOY_ARGS[@]}"

# ------------------------------------------------------------------
# 4. OPTIONAL CLEANUP (unused frameworks) — runs only if the script exists
# ------------------------------------------------------------------
CLEANUP_SCRIPT="$PROJECT_ROOT/cleanup_qt_frameworks.sh"
if [[ -x "$CLEANUP_SCRIPT" ]]; then
    echo "== Scanning for unused bundled frameworks (dry run) =="
    "$CLEANUP_SCRIPT" "$APP_BUNDLE"
    echo
    echo "Review the report above. To actually strip unused frameworks, run:"
    echo "  $CLEANUP_SCRIPT \"$APP_BUNDLE\" --delete"
else
    echo "(tip: place cleanup_qt_frameworks.sh next to this script to auto-scan for unused frameworks)"
fi

# ------------------------------------------------------------------
# 5. AD-HOC CODESIGN (needed for local runs after any file changes to the bundle)
# ------------------------------------------------------------------
echo "== Ad-hoc codesigning =="
codesign --force --deep --sign - "$APP_BUNDLE"

echo
echo "== Done =="
echo "App bundle: $APP_BUNDLE"
echo "Size: $(du -sh "$APP_BUNDLE" | cut -f1)"
if [[ "$DO_DMG" == true ]]; then
    DMG_PATH="$BUILD_DIR/$APP_NAME.dmg"
    if [[ -f "$DMG_PATH" ]]; then
        echo "DMG: $DMG_PATH ($(du -sh "$DMG_PATH" | cut -f1))"
    else
        echo "WARNING: --dmg was requested but no .dmg found at $DMG_PATH"
    fi
fi
