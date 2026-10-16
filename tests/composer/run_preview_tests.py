"""Run composer playback checks with a generated WAV and isolated UI dependencies."""
import pathlib, tempfile, shutil, subprocess, wave, sys, re
root=pathlib.Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='voice-preview-') as d:
 p=pathlib.Path(d); (p/'subject').mkdir(); (p/'Theme').mkdir(); (p/'Shared/UI').mkdir(parents=True)
 shutil.copy(root/'src/features/chat/presentation/view/components/MessageComposer.qml',p/'subject')
 (p/'subject/Attachment.qml').write_text('import QtQuick.Controls.Basic\nMenu {}')
 (p/'subject/EmojiPopup.qml').write_text('import QtQuick.Controls.Basic\nPopup { property bool hovered: false }')
 for name in ['Colors','Typography','AppLayouts']:
  shutil.copy(root/f'src/shared/theme/{name}.qml',p/'Theme')
 (p/'Theme/qmldir').write_text('module Theme\nsingleton Colors 1.0 Colors.qml\nsingleton Typography 1.0 Typography.qml\nsingleton AppLayouts 1.0 AppLayouts.qml\nsingleton AppAssets 1.0 AppAssets.qml')
 (p/'Theme/AppAssets.qml').write_text('pragma Singleton\nimport QtQuick\nQtObject {'+''.join(f'property url {s}: ""\n' for s in sorted(set(re.findall(r'AppAssets\.(\w+)', (p/'subject/MessageComposer.qml').read_text()))))+'}')
 shutil.copy(root/'src/shared/ui/CircleButton.qml',p/'Shared/UI')
 shutil.copy(root/'src/shared/ui/MySlider.qml',p/'Shared/UI')
 (p/'Shared/UI/qmldir').write_text('module Shared.UI\nCircleButton 1.0 CircleButton.qml\nMySlider 1.0 MySlider.qml')
 with wave.open(str(p/'audio.wav'),'wb') as w:
  w.setnchannels(1); w.setsampwidth(2); w.setframerate(16000); w.writeframes(b'\0\0'*16000*3)
 shutil.copy(root / "tests/composer" / (sys.argv[2] if len(sys.argv) > 2 else "tst_preview.qml"), p / "tst_preview.qml")
 result=subprocess.run([sys.argv[1],'-platform','offscreen','-input',str(p),'-import',str(p)],capture_output=True,text=True)
 print(result.stdout); print(result.stderr); raise SystemExit(result.returncode)
