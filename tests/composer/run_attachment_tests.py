"""Run attachment dialog checks with isolated theme dependencies."""
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="attachment-dialog-") as directory:
    path = pathlib.Path(directory)
    for folder in ("subject", "Theme", "Shared/UI"):
        (path / folder).mkdir(parents=True)
    source = root / "src/features/chat/presentation/view/components/AttachmentUploadDialog.qml"
    shutil.copy(source, path / "subject")
    names = ["Colors", "Typography", "AppLayouts"]
    for name in names:
        shutil.copy(root / f"src/shared/theme/{name}.qml", path / "Theme")
    (path / "Theme/qmldir").write_text("module Theme\n" + "\n".join(
        f"singleton {name} 1.0 {name}.qml" for name in names + ["AppAssets"]))
    assets = sorted(set(re.findall(r"AppAssets\.(\w+)", source.read_text())))
    (path / "Theme/AppAssets.qml").write_text(
        "pragma Singleton\nimport QtQuick\nQtObject {\n" +
        "\n".join(f'property url {name}: ""' for name in assets) + "\n}")
    shutil.copy(root / "src/shared/ui/CircleButton.qml", path / "Shared/UI")
    (path / "Shared/UI/qmldir").write_text("module Shared.UI\nCircleButton 1.0 CircleButton.qml\n")
    shutil.copy(root / "tests/composer/tst_mixed_attachments.qml", path)
    result = subprocess.run([sys.argv[1], "-platform", "offscreen", "-style", "Basic", "-input", str(path),
                             "-import", str(path)], capture_output=True, text=True)
    print(result.stdout)
    print(result.stderr)
    raise SystemExit(result.returncode)
