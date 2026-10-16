#include "voice_recorder.hpp"

#include <QAudioSource>
#include "m4a_encoder.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMediaDevices>
#include <QPermission>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>

namespace core {
namespace media {

struct VoiceRecorder::Session
{
    explicit Session(const QString &directory)
        : output(directory + QStringLiteral("/recording-XXXXXX"))
    {
    }

    // Destroy the encoder before removing any unfinished output.
    QTemporaryDir output;
    M4aEncoder encoder;
    QAudioFormat format;
    std::unique_ptr<QAudioSource> source;
    QTimer durationTimer;
    int previewRevision = 0;
    bool encodingFinal = false;


};

VoiceRecorder::VoiceRecorder(QObject *parent) : QObject(parent) {}

VoiceRecorder::~VoiceRecorder()
{
    if (m_session) {
        if (m_session->source) {
            disconnect(m_session->source.get(), nullptr, this, nullptr);
            m_session->source->stop();
        }
        m_session.reset();
    }
}

void VoiceRecorder::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged();
    }
}

void VoiceRecorder::setAudioInputDevice(const QAudioDevice &device)
{
    m_device = device;
}

void VoiceRecorder::start(const QUrl &outputDirectory)
{
    Q_ASSERT(QThread::currentThread() == thread());
    Q_ASSERT(qApp && thread() == qApp->thread());
    if (m_state != Idle) {
        return;
    }
    m_error.clear();
    emit errorChanged();
    m_outputLocation = QUrl();
    emit outputLocationChanged();
    m_duration = 0;
    emit durationChanged();
    m_discard = false;

    if (!outputDirectory.isEmpty() && !outputDirectory.isLocalFile()) {
        fail(tr("The recording directory must be a local folder."));
        return;
    }
    const QString directory = outputDirectory.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/voice")
        : outputDirectory.toLocalFile();
    if (!QDir().mkpath(directory) || !QFileInfo(directory).isDir()
        || !QFileInfo(directory).isWritable()) {
        fail(tr("The recording directory is not writable."));
        return;
    }

    const quint64 request = ++m_request;
    setState(Starting);
    // A stateChanged handler may cancel before permission is requested.
    if (m_state != Starting || request != m_request) {
        return;
    }
    const QMicrophonePermission permission;
    switch (qApp->checkPermission(permission)) {
    case Qt::PermissionStatus::Granted:
        beginRecording(directory);
        break;
    case Qt::PermissionStatus::Denied:
        fail(tr("Microphone access is denied. Enable it in system settings."));
        break;
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(permission, this, [this, directory, request](const QPermission &result) {
            if (request != m_request || m_state != Starting) {
                return;
            }
            if (result.status() == Qt::PermissionStatus::Granted) {
                beginRecording(directory);
            } else {
                fail(tr("Microphone access is required to record voice."));
            }
        });
        break;
    }

}

void VoiceRecorder::beginRecording(const QString &directory)
{
    const QAudioDevice device = m_device.isNull() ? QMediaDevices::defaultAudioInput() : m_device;
    if (device.isNull() || !QMediaDevices::audioInputs().contains(device)) {
        fail(tr("The selected microphone is unavailable."));
        return;
    }
    m_session = std::make_unique<Session>(directory);
    // Capture explicit PCM on both platforms and encode it with the same M4A encoder.
    auto &session = *m_session;
    session.format = device.preferredFormat();
    session.format.setSampleFormat(QAudioFormat::Int16);
    QAudioFormat monoFormat = session.format;
    monoFormat.setChannelCount(1);
    if (device.isFormatSupported(monoFormat)) {
        session.format = monoFormat;
    }
    if (!session.output.isValid() || !device.isFormatSupported(session.format)
        || session.format.channelCount() > 2) {
        fail(tr("The microphone does not support PCM voice recording."));
        return;
    }
    const QString file_path = session.output.filePath(QStringLiteral("voice.m4a"));
    if (!session.encoder.start(file_path,
                               session.format.sampleRate(),
                               session.format.channelCount())) {
        fail(session.encoder.errorString());
        return;
    }
    connect(&session.encoder, &M4aEncoder::encodingFinished, this,
            [this](const QString &path, bool success) {
                if (!m_session) {
                    return;
                }
                if (m_session->encodingFinal) {
                    completeRecording(success && !m_discard);
                } else if (m_state == Paused) {
                    if (success) {
                        emit previewReady(QUrl::fromLocalFile(path), m_duration);
                    } else {
                        m_error = m_session->encoder.errorString();
                        emit errorChanged();
                        emit errorOccurred(m_error);
                    }
                }
            });
    session.source = std::make_unique<QAudioSource>(device, session.format);
    connect(session.source.get(), &QAudioSource::stateChanged, this, [this](QtAudio::State) {
        if (m_state != Stopping && m_session && m_session->source
            && m_session->source->error() != QtAudio::NoError) {
            fail(tr("The microphone stopped capturing audio."));
        }
    });
    session.durationTimer.setInterval(100);
    connect(&session.durationTimer, &QTimer::timeout, this, [this] {
        if (m_session) {
            m_duration = m_session->format.durationForBytes(m_session->encoder.pcmBytes()) / 1000;
            emit durationChanged();
        }
    });
    session.source->start(&session.encoder);
    if (m_state == Stopping) {
        return;
    }
    if (session.source->error() != QtAudio::NoError) {
        fail(tr("Cannot start the microphone."));
        return;
    }
    session.durationTimer.start();
    setState(Recording);
    emit recordingFileLocaltionChanged(QUrl::fromLocalFile(file_path));
}

void VoiceRecorder::pause()
{
    if (m_state == Recording && m_session) {
        m_session->source->suspend();
        if (m_state != Stopping) {
            setState(Paused);
            m_session->durationTimer.stop();
            m_duration = m_session->format.durationForBytes(m_session->encoder.pcmBytes()) / 1000;
            emit durationChanged();
            const QString path = m_session->output.filePath(
                QStringLiteral("preview-%1.m4a").arg(++m_session->previewRevision));
            if (!m_session->encoder.savePreview(path)) {
                m_error = m_session->encoder.errorString();
                emit errorChanged();
                emit errorOccurred(m_error);
            }
        }
    }
}

void VoiceRecorder::resume()
{
    if (m_state == Paused && m_session) {
        m_session->source->resume();
        if (m_state != Stopping) {
            m_session->durationTimer.start();
            setState(Recording);
        }
    }
}

void VoiceRecorder::stop()
{
    if (m_state == Idle || m_state == Stopping) {
        return;
    }
    if (!m_session) {
        cancel();
        return;
    }
    setState(Stopping);
    m_session->durationTimer.stop();
    if (m_session->source) {
        m_session->source->stop();
    }
    scheduleFinish();
}

void VoiceRecorder::cancel()
{
    if (m_state == Idle) {
        return;
    }
    m_discard = true;
    if (m_session) {
        if (m_state == Stopping) {
            scheduleFinish();
        } else {
            stop();
        }
    } else {
        ++m_request;
        setState(Idle);
        emit canceled();
    }
}

void VoiceRecorder::fail(const QString &message)
{
    m_error = message;
    m_discard = true;
    if (m_session) {
        setState(Stopping);
        m_session->durationTimer.stop();
        if (m_session->source) {
            m_session->source->stop();
        }
        scheduleFinish();
    } else {
        setState(Idle);
    }
    emit errorChanged();
    emit errorOccurred(message);
}

void VoiceRecorder::scheduleFinish()
{
    const quint64 request = m_request;
    QTimer::singleShot(0, this, [this, request] {
        if (request == m_request) {
            finishRecording();
        }
    });
}

void VoiceRecorder::finishRecording()
{
    if (!m_session || m_state != Stopping) {
        return;
    }
    m_duration = m_session->format.durationForBytes(m_session->encoder.pcmBytes()) / 1000;
    emit durationChanged();
    if (!m_discard) {
        m_session->encodingFinal = true;
        if (m_session->encoder.finish()) {
            return;
        }
    }
    completeRecording(false);
}

void VoiceRecorder::completeRecording(bool saved)
{
    const QUrl location = QUrl::fromLocalFile(m_session->encoder.fileName());
    bool missingRecording = false;
    if (saved) {
        m_session->output.setAutoRemove(false);
        m_outputLocation = location;
    } else if (!m_discard && m_error.isEmpty()) {
        m_error = m_session->encoder.errorString();
        missingRecording = true;
    }
    const QString error = m_error;
    const qint64 duration = m_duration;
    m_session.reset();
    emit outputLocationChanged();
    setState(Idle);
    if (saved) {
        emit recordingReady(location, duration);
    } else if (error.isEmpty()) {
        emit canceled();
    } else {
        emit errorChanged();
        if (missingRecording) {
            emit errorOccurred(error);
        }
    }
}

} // namespace media
} // namespace core
