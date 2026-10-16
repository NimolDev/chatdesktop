"""Exercise the real message delegates and controls; stub only Bubble painting."""
import pathlib
import os
import shutil
import subprocess
import sys
import tempfile
import wave

root = pathlib.Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="messaging-tests-") as directory:
    work = pathlib.Path(directory)
    subject = work / "subject"
    subject.mkdir()
    for name in ("MessagingBody", "MessageBubble", "TextMessageView", "VoiceMessageView", "ImageMessageView", "FileMessageView", "StickerView"):
        shutil.copy(root / f"src/features/chat/presentation/view/components/{name}.qml", subject)
    shutil.copy(root / "src/features/chat/presentation/view/ImageViewWindow.qml", subject)
    # Bake the production effect for the isolated QML fixture.
    qsb = os.environ.get("QSB") or shutil.which("qsb")
    if not qsb:
        sys.exit("Set QSB to the Qt Shader Baker executable or put qsb on PATH.")
    shader = root / "src/features/chat/presentation/view/shaders/message_dissolve.frag"
    subprocess.run([qsb, "--glsl", "100 es,120,150", "--hlsl", "50", "--msl", "12",
                    "-o", str(subject / "message_dissolve.frag.qsb"), str(shader)], check=True)
    bubble = subject / "MessageBubble.qml"
    bubble.write_text(bubble.read_text().replace(
        "qrc:/chat/shaders/message_dissolve.frag.qsb", "message_dissolve.frag.qsb"))
    test_name = sys.argv[2] if len(sys.argv) > 2 else "tst_pointer_routing.qml"
    shutil.copy(pathlib.Path(__file__).with_name(test_name), work)
    with wave.open(str(work / "audio.wav"), "wb") as audio:
        audio.setnchannels(1)
        audio.setsampwidth(2)
        audio.setframerate(16000)
        audio.writeframes(b"\0\0" * 16000 * 3)
    for name, width, height, color in (("first.svg", 500, 400, "blue"),
                                       ("removed.svg", 100, 500, "red")):
        (work / name).write_text(
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">'
            f'<rect width="100%" height="100%" fill="{color}"/></svg>')
    # Name the real controls in the temporary copy for QtTest lookup.
    voice = subject / "VoiceMessageView.qml"
    voice.write_text(voice.read_text().replace("id: btnDonwload\n", 'id: btnDonwload\n                objectName: "action"\n').replace("id: voiceSlider\n", 'id: voiceSlider\n                    objectName: "slider"\n'))
    voice.write_text(voice.read_text().replace("id: btnDonwload1\n", 'id: btnDonwload1\n                objectName: "outgoingAction"\n'))
    voice.write_text(voice.read_text().replace("id: voiceSlider1\n", 'id: voiceSlider1\n                    objectName: "outgoingSlider"\n'))
    chat = work / "Features/Chat"
    chat.mkdir(parents=True)
    (chat / "qmldir").write_text("module Features.Chat\nBubble 1.0 Bubble.qml\nsingleton MessagingViewModel 1.0 MessagingViewModel.qml\n")
    (chat / "Bubble.qml").write_text('import QtQuick\nRectangle { property bool outgoing; property bool showTail }')
    (chat / "MessagingViewModel.qml").write_text(
        'pragma Singleton\nimport QtQuick\nQtObject {\n'
        '    property var conversationImages: []\n'
        '    property var imageUrls: []\n'
        '    function getImageList() { imageUrls = conversationImages.slice() }\n'
        '}\n')
    ui = work / "Shared/UI"
    ui.mkdir(parents=True)
    names = ("DownloadCircleButton", "MySlider")
    (ui / "qmldir").write_text("module Shared.UI\n" + "".join(f"{name} 1.0 {name}.qml\n" for name in names))
    for name in names:
        shutil.copy(root / f"src/shared/ui/{name}.qml", ui)
    button = ui / "DownloadCircleButton.qml"
    button.write_text(button.read_text().replace("HoverHandler {", 'HoverHandler {\n        objectName: "actionHover"'))
    theme = work / "Theme"
    theme.mkdir()
    names = ("Colors", "AppLayouts", "Typography", "AppAssets")
    (theme / "qmldir").write_text("module Theme\n" + "".join(f"singleton {name} 1.0 {name}.qml\n" for name in names))
    for name in names:
        shutil.copy(root / f"src/shared/theme/{name}.qml", theme)
    environment = dict(os.environ, QT_QUICK_CONTROLS_STYLE="Basic")
    environment["QT_LOGGING_RULES"] = environment.get("QT_LOGGING_RULES", "") + "\nqt.quick.hover.cursor.debug=true"
    result = subprocess.run([sys.argv[1], "-platform", environment.get("QML_TEST_PLATFORM", "offscreen"), "-input", str(work), "-import", str(work)], env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    print(result.stdout, end="")
    # Check the cursor Qt installs, rather than only HoverHandler.hovered.
    for direction in (("incoming", "outgoing") if test_name == "tst_pointer_routing.qml" else ()):
        if not any(f"test_actionHover({direction})" in line and "setting cursor" in line and "PointingHandCursor" in line and "actionHover" in line for line in result.stdout.splitlines()):
            sys.exit(f"No pointing-hand window cursor observed for {direction} voice control")
    sys.exit(result.returncode)
