#pragma once

#include <QAudioDevice>
#include <QObject>
#include <QUrl>
#include <memory>

namespace core {
namespace media {

// Use on the application thread. Each recording owns a unique output directory.
// Connect MediaDeviceManager::audioInputDeviceChanged to setAudioInputDevice().
class VoiceRecorder final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(QUrl outputLocation READ outputLocation NOTIFY outputLocationChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)

public:
    enum State { Idle, Starting, Recording, Paused, Stopping };
    Q_ENUM(State)

    explicit VoiceRecorder(QObject *parent = nullptr);
    ~VoiceRecorder() override;

    State state() const { return m_state; }
    qint64 duration() const { return m_duration; } // milliseconds, excluding pauses
    QUrl outputLocation() const { return m_outputLocation; }
    QString errorString() const { return m_error; }

    // Empty uses AppLocalDataLocation/voice. Existing files are never overwritten.
    Q_INVOKABLE void start(const QUrl &outputDirectory = {});
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void cancel();

public slots:
    // A change during recording applies to the next recording.
    void setAudioInputDevice(const QAudioDevice &device);

signals:
    void stateChanged();
    void durationChanged();
    void outputLocationChanged();
    void errorChanged();
    // Emitted only after the encoder has finished writing the file.
    void recordingReady(const QUrl &location, qint64 duration);
    void previewReady(const QUrl &location, qint64 duration);
    void recordingFileLocaltionChanged(const QUrl &localion);
    void canceled();
    void errorOccurred(const QString &message);

private:
    struct Session;
    void beginRecording(const QString &directory);
    void finishRecording();
    void completeRecording(bool saved);
    void scheduleFinish();
    void fail(const QString &message);
    void setState(State state);

    std::unique_ptr<Session> m_session;
    QAudioDevice m_device;
    State m_state = Idle;
    qint64 m_duration = 0;
    QUrl m_outputLocation;
    QString m_error;
    quint64 m_request = 0;
    bool m_discard = false;
};

} // namespace media
} // namespace core
