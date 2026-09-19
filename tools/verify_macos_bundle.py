"""Check the self-contained Frameworks layout produced by our macOS deployment."""

import pathlib
import subprocess
import sys


def output(*args):
    return subprocess.check_output(args, text=True).strip()


def verify(bundle):
    contents = bundle / "Contents"
    executable_dir = contents / "MacOS"
    frameworks = contents / "Frameworks"
    architectures = output("lipo", "-archs", str(executable_dir / "ChatApp")).split()
    errors = []
    binaries = 0
    magic_numbers = {bytes.fromhex(value) for value in (
        "cffaedfe", "cefaedfe", "cafebabe", "bebafeca", "cafebabf", "bfbafeca"
    )}
    for path in contents.rglob("*"):
        if path.is_symlink():
            if not path.exists() or bundle not in path.resolve().parents:
                errors.append(f"External or broken symlink: {path}")
            continue
        if not path.is_file():
            continue
        with path.open("rb") as stream:
            if stream.read(4) not in magic_numbers:
                continue
        binaries += 1
        available = output("lipo", "-archs", str(path)).split()
        for arch in architectures:
            if arch not in available:
                errors.append(f"Missing {arch} architecture: {path}")
                continue
            for line in output("otool", "-arch", arch, "-L", str(path)).splitlines()[1:]:
                dependency = line.strip().split(" (", 1)[0]
                if dependency.startswith(("/System/Library/", "/usr/lib/")):
                    continue
                target = None
                for prefix, root in (("@rpath/", frameworks),
                                     ("@loader_path/", path.parent),
                                     ("@executable_path/", executable_dir)):
                    if dependency.startswith(prefix):
                        target = root / dependency[len(prefix):]
                        break
                if target is None or not target.exists() or bundle not in target.resolve().parents:
                    errors.append(f"Unbundled dependency: {path}: {dependency}")
    if errors:
        sys.exit("\n".join(errors))
    print(f"Verified {binaries} Mach-O files for {', '.join(architectures)}: all dependencies bundled.")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("Usage: verify_macos_bundle.py /path/to/ChatApp.app")
    verify(pathlib.Path(sys.argv[1]).resolve())
