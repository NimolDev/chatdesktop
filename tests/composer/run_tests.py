"""Exercise the real composer with a fake recorder and isolated popup/theme dependencies."""
import pathlib
import shutil
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="composer-tests-") as directory:
    work = pathlib.Path(directory)
    subject = work / "subject"
    subject.mkdir()
    shutil.copy(root / "src/features/chat/presentation/view/components/MessageComposer.qml", subject)
    shutil.copy(pathlib.Path(__file__).with_name("tst_recording.qml"), work)
    (subject / "Attachment.qml").write_text("import QtQuick.Controls.Basic\nMenu {}\n")
    (subject / "EmojiPopup.qml").write_text("import QtQuick.Controls.Basic\nPopup { property bool hovered: false }\n")
    theme = work / "Theme"
    theme.mkdir()
    shutil.copy(root / "src/shared/theme/Colors.qml", theme)
    (theme / "qmldir").write_text("module Theme\nsingleton Colors 1.0 Colors.qml\nsingleton AppAssets 1.0 AppAssets.qml\n")
    (theme / "AppAssets.qml").write_text('pragma Singleton\nimport QtQuick\nQtObject {\n' + ''.join(
        f' readonly property url {name}: ""\n' for name in ["icAttactment", "icEmoji", "icSend", "icMicrophone"]
    ) + '}\n')
    ui = work / "Shared/UI"
    ui.mkdir(parents=True)
    shutil.copy(root / "src/shared/ui/CircleButton.qml", ui)
    (ui / "qmldir").write_text("module Shared.UI\nCircleButton 1.0 CircleButton.qml\n")
    result = subprocess.run([sys.argv[1], "-platform", "offscreen", "-input", str(work), "-import", str(work)])
    sys.exit(result.returncode)
