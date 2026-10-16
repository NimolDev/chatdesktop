import QtQuick
import QtTest
import QtMultimedia
import "subject" as Subject
TestCase {
 id: test
 name: "VoicePreview"
 when: windowShown
 width: 640; height: 180; visible: true
 QtObject {
  id: vm
  property bool isVoiceRecordingBusy: false
  property url voiceRecordingFile: ""
  property string voiceRecordingDurationText: "00:03:0"
  function cancelVoiceRecording() { isVoiceRecordingBusy = false; voiceRecordingFile = "" }
 }
 Subject.MessageComposer { id: composer; width: 600; height: implicitHeight; viewmodel: vm }
 function cleanup() { composer.resetState() }
 function test_playPauseSeekClose() {
  verify(!composer.hasVoicePreview)
  vm.voiceRecordingFile = Qt.resolvedUrl("audio.wav")
  verify(composer.hasVoicePreview)
  const player = findChild(composer, "voicePreviewPlayer")
  const play = findChild(composer, "voicePlayButton")
  tryCompare(play, "enabled", true, 10000)
  mouseClick(play)
  tryCompare(player, "playbackState", MediaPlayer.PlayingState)
  mouseClick(play)
  tryCompare(player, "playbackState", MediaPlayer.PausedState)
  const slider = findChild(composer, "voiceSeekSlider")
  verify(slider.enabled)
  slider.value = 1500; slider.moved()
  compare(player.position, 1500)
  mouseClick(play)
  tryCompare(player, "mediaStatus", MediaPlayer.EndOfMedia, 10000)
  mouseClick(play)
  tryCompare(player, "playbackState", MediaPlayer.PlayingState)
  mouseClick(findChild(composer, "voiceCloseButton"))
  verify(!composer.hasVoicePreview)
  compare(player.playbackState, MediaPlayer.StoppedState)
 }
 function test_resetClearsPreview() {
  vm.voiceRecordingFile = Qt.resolvedUrl("audio.wav")
  composer.resetState()
  verify(!composer.hasVoicePreview)
 }
 function test_waitForFinalizedFile() {
  vm.isVoiceRecordingBusy = true
  verify(!composer.hasVoicePreview)
  verify(!findChild(composer, "voicePlayButton").visible)
  vm.isVoiceRecordingBusy = false
  vm.voiceRecordingFile = Qt.resolvedUrl("audio.wav")
  verify(findChild(composer, "voicePlayButton").visible)
 }
}
