# Voice recording

`VoiceRecorder` is a QObject backend; link `core::media` and keep it on the
application thread. It requests microphone permission when `start()` is called.
Creating the object does not activate the microphone.

```cpp
#include "media/media_device_manager.hpp"
#include "media/voice_recorder.hpp"

// Keep these as members of the owning viewmodel.
core::media::MediaDeviceManager devices;
core::media::VoiceRecorder recorder;

recorder.setAudioInputDevice(devices.selectedAudioInputDevice());
QObject::connect(&devices,
                 &core::media::MediaDeviceManager::audioInputDeviceChanged,
                 &recorder,
                 &core::media::VoiceRecorder::setAudioInputDevice);
QObject::connect(&recorder, &core::media::VoiceRecorder::recordingReady,
                 [](const QUrl &file, qint64 durationMs) {
                     // The file is finalized and ready for playback or upload.
                 });
recorder.start();
// Later: recorder.pause(), recorder.resume(), recorder.stop(), recorder.cancel().
```

- `start()` uses `AppLocalDataLocation/voice`; alternatively pass a local folder
  URL. A unique subfolder prevents overwriting previous recordings.
- Capture uses signed 16-bit PCM through `QAudioSource`. Qt Multimedia encodes
  AAC at 128 kbps in a `voice.m4a` container. Mono is preferred; stereo capture
  is accepted when needed. Previews encode independent PCM snapshots, including
  all resumed audio. Encoding is asynchronous; `recordingReady` is emitted only
  after the M4A container has been finalized. Duration comes from captured PCM.
- Observe `state`, `duration` (milliseconds), `errorString`, and their change
  signals. `Stopping` lasts until the encoder finishes. Only `recordingReady`
  means the output is ready to use.
- `cancel()` removes unfinished output. Destruction also discards unfinished
  output. Successful files remain until the caller deletes them (and their
  recording folder) after sending or discarding the voice message.
- Microphone changes during a recording apply to the next recording.
- The host application needs microphone permissions (including
  `NSMicrophoneUsageDescription` on macOS, already present in ChatApp).
- Chat buttons, playback, and message upload are not connected by this module.

Run the hardware-independent tests separately from the app:

```sh
cmake -S tests/media -B build/voice-recorder-tests -DCMAKE_PREFIX_PATH=/path/to/Qt/macos
cmake --build build/voice-recorder-tests
ctest --test-dir build/voice-recorder-tests --output-on-failure
```

A live microphone smoke test should also cover recording, pause/resume, stop,
playback of the returned file, cancellation, and unplugging the selected input.

## MessagingViewModel

The chat singleton owns a recorder and exposes `startVoiceRecording()` (also
available through the existing `startVoiceRecoder()` spelling),
`pauseVoiceRecording()`, `resumeVoiceRecording()`, `stopVoiceRecording()`, and
`cancelVoiceRecording()`. `startVoiceRecording()` optionally accepts a local
output folder URL.

QML can observe `isVoiceRecording`, `isVoiceRecordingPaused`,
`isVoiceRecordingBusy`, `voiceRecordingDuration`, `voiceRecordingFile`, and
`voiceRecordingError`. The busy property includes permission and finalization.
Select a conversation before starting. Switching conversations or resetting
messages cancels in-progress capture and clears the displayed clip reference.

`voiceRecordingReady(receiverId, file, duration)` supplies the captured recipient
and finalized local audio. Successful files remain caller-owned, as described
above; clearing the viewmodel's file property does not delete a clip that an
upload handler may already be using. Connect `voiceRecordingFailed(message)` to
error UI and `voiceRecordingCanceled()` to recording UI cleanup.

Connect an existing device picker to `setVoiceAudioInputDevice(QAudioDevice)`;
otherwise the recorder uses the current default microphone. `MessagingViewModel::sendVoice(duration)` finalizes the recording when necessary, uploads the M4A,
and sends a voice payload through the message repository. Pausing only produces
local previews and never triggers an upload. Observe `isSendingVoice`,
`voiceSendError`, and `voiceMessageSent` on `MessagingViewModel` for progress, retry, and completion.
`VoiceMessageController` handles only recording, preview, playback, and media downloads.
No local path is sent as a text message.

## M4A build dependency

Requires Qt 6.8 or newer with Qt Multimedia's FFmpeg backend and an AAC encoder.
Deploy Qt Multimedia and its bundled FFmpeg runtime using the Qt deployment
tools for the target kit. No system-installed LAME, Homebrew/vcpkg encoder, or
external conversion executable is used. Unsupported AAC/M4A encoding produces
an explicit error instead of silently selecting a different format.

## Openfire audio upload

The Send button uses XEP-0363 HTTP File Upload through QXmpp. Enable Openfire's
[HTTP File Upload plugin](https://github.com/igniterealtime/openfire-httpFileUpload-plugin)
and HTTP Binding (Server → Server Settings → HTTP Binding). The plugin must
advertise HTTPS PUT/GET URLs reachable by all clients, with a trusted certificate.
Configure its upload size limit and persistent file storage for your deployment;
Openfire's default temporary storage is not suitable for durable message history.
The client discovers the service rather than assuming a host or port. TLS errors
are reported; upload certificate verification is not bypassed.

After the PUT succeeds, the app sends its existing JSON message format with
`content.type = "voice"`, `media_url` set to the download URL, `fileSize` in bytes,
and `duration` in milliseconds. XMPP send success triggers the existing message
history save and local insertion. This uses ChatApp's JSON payload protocol;
other XMPP clients need a compatible renderer. It does not add an audio bubble
player to the chat history.

Failed uploads keep the local MP3 for retry and do not insert a successful chat
message. Navigating during upload preserves the original recipient. Disconnection
or a two-minute upload timeout reports an error. Server-side orphan cleanup is
left to Openfire if upload succeeds but the message cannot subsequently be sent.

## Audio playback

`core::media::AudioPlayer` owns a `QMediaPlayer` and `QAudioOutput`. Create it on
the GUI thread and link `core::media`. Sources can be local file URLs or remote
HTTP/HTTPS audio URLs. Each instance has independent playback state.

```cpp
core::media::AudioPlayer player;
player.setSource(QUrl::fromLocalFile("/path/to/voice.m4a"));
player.play();
player.pause();
player.setPosition(1500); // milliseconds; applied once the source is seekable
player.togglePlayback();
player.stop();
```

The C++ getters and change signals cover `source`, `position`, `duration`, `seekable`,
`playbackState`, `mediaStatus`, `error`, `errorString`, `volume` (0–1), and `muted`.
`setAudioOutputDevice()` selects an output device. Changing source stops the old
recording; stopping or reaching the end resets position to zero. `finished()` is
emitted when playback reaches the end.

`AudioPlayer` is C++ only: it has no QML registration, QML properties, or
invokable methods. `MessagingViewModel` owns both core players privately and exposes playback state
as `voicePlayback*` and `voicePreview*` properties. Voice bubbles call
`MessagingViewModel::playVoice(url)`, `seekVoice(position)`, and
`stopVoicePlayback()`. Preview controls call `playVoicePreview()`,
`seekVoicePreview(position)`, and `stopVoicePreview()` on the same viewmodel.
The play commands toggle play/pause. QML never creates or accesses a core player. Message playback and pending downloads continue when switching
conversations. Resetting the messaging model stops playback, and starting either message or preview playback stops the other.

### Playback checks

Configure `tests/media` with your Qt CMake prefix, then build `test_audio_player`
and `audio_qml_runner`. Run the `AudioPlayer` CTest and use the custom runner for
the QML tests so they exercise the real `MessagingViewModel` and its private players:

```sh
python3 tests/messaging/run_tests.py /absolute/build/path/audio_qml_runner tst_voice_playback.qml
python3 tests/messaging/run_tests.py /absolute/build/path/audio_qml_runner
QT_QUICK_CONTROLS_STYLE=Basic python3 tests/composer/run_preview_tests.py /absolute/build/path/audio_qml_runner tst_voice_duration.qml
```
