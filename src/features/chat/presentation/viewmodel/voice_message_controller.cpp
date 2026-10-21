#include "voice_message_controller.hpp"
#include "utils/voice_duration_formate.hpp"
#include <QCryptographicHash>
#include <QFileInfo>
#include <algorithm>
#include <utility>

VoiceMessageController::VoiceMessageController(
    std::shared_ptr<domain::repository::MediaRepository> mediaRepository,
    std::function<QString(const QUrl &)> messageIdForSource,
    QObject *parent)
    : QObject(parent),
      m_messageIdForSource(std::move(messageIdForSource)),
      m_mediaRepository(std::move(mediaRepository)),
      m_messagePlayback(this),
      m_previewPlayback(this),
      m_voiceRecorder(this)
{
    if (m_mediaRepository) {
        using domain::repository::MediaRepository;
        connect(m_mediaRepository.get(), &MediaRepository::downloadProgress, this,
                [this](const QString &message, qint64 received, qint64 total) {
                    if (message == m_pendingVoiceMessage) {
                        m_voiceDownloadProgress = total > 0
                            ? std::clamp(double(received) / double(total), 0.0, 1.0) : 0.0;
                        emit voicePlaybackChanged();
                    }
                });
        connect(m_mediaRepository.get(), &MediaRepository::downloadCompleted, this,
                [this](const QString &message, const QString &path) {
                    if (message != m_pendingVoiceMessage) {
                        return;
                    }
                    m_pendingVoiceMessage.clear();
                    m_voiceDownloadProgress = 1;
                    const QFileInfo file(path);
                    if (!file.isFile() || file.size() <= 0) {
                        m_voiceDownloadError = tr("Downloaded audio is missing or empty.");
                        emit voicePlaybackChanged();
                        return;
                    }
                    m_messagePlayback.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
                    m_messagePlayback.play();
                    emit voicePlaybackChanged();
                });
        connect(m_mediaRepository.get(), &MediaRepository::downloadFailed, this,
                [this](const QString &message, const QString &error) {
                    if (message == m_pendingVoiceMessage) {
                        m_pendingVoiceMessage.clear();
                        m_voiceDownloadError = error;
                        emit voicePlaybackChanged();
                    }
                });
    }
    connect(this, &VoiceMessageController::voiceRecordingFileChanged, this, [this] {
        m_previewPlayback.setSource(m_voiceRecordingFile);
    });

    // ----  voice message  ----
    using core::media::AudioPlayer;
    connect(&m_messagePlayback,
        &AudioPlayer::sourceChanged,
        this,
        &VoiceMessageController::voicePlaybackChanged);
    connect(&m_messagePlayback,
            &AudioPlayer::positionChanged,
            this,
            &VoiceMessageController::voicePlaybackChanged);
    connect(&m_messagePlayback,
            &AudioPlayer::durationChanged,
            this,
            &VoiceMessageController::voicePlaybackChanged);
    connect(&m_messagePlayback,
            &AudioPlayer::seekableChanged,
            this,
            &VoiceMessageController::voicePlaybackChanged);
    connect(&m_messagePlayback,
            &AudioPlayer::playbackStateChanged,
            this,
            &VoiceMessageController::voicePlaybackChanged);
    connect(
        &m_messagePlayback,
        &AudioPlayer::mediaStatusChanged,
        this,
        [this]() {
            setIsInPlay (m_messagePlayback.playbackState() == QMediaPlayer::PlayingState);
            emit voicePlaybackChanged();
        }
        );
    // End-of-media updates the controls through mediaStatusChanged; keep the
    // bar open until explicitly closed so short clips do not resize the list.
    connect(&m_messagePlayback,
            &AudioPlayer::errorChanged,
            this,
            &VoiceMessageController::voicePlaybackChanged);
    connect(&m_messagePlayback,
            &AudioPlayer::positionChanged,
            this,
            &VoiceMessageController::voicePlaybackDurationTextChanged);

    // --- Preview voice ----
    connect(&m_previewPlayback,
            &AudioPlayer::sourceChanged,
            this,
            &VoiceMessageController::voicePreviewChanged);
    connect(&m_previewPlayback,
            &AudioPlayer::positionChanged,
            this,
            &VoiceMessageController::voicePreviewChanged);
    connect(&m_previewPlayback,
            &AudioPlayer::durationChanged,
            this,
            &VoiceMessageController::voicePreviewChanged);
    connect(&m_previewPlayback,
            &AudioPlayer::seekableChanged,
            this,
            &VoiceMessageController::voicePreviewChanged);
    connect(&m_previewPlayback,
            &AudioPlayer::playbackStateChanged,
            this,
            &VoiceMessageController::voicePreviewChanged);
    connect(
        &m_previewPlayback,
        &AudioPlayer::mediaStatusChanged,
        this,
        [this]() {
            if (m_messagePlayback.mediaStatus () == QMediaPlayer::MediaStatus::EndOfMedia) {
            }
            emit voicePreviewChanged();
        }
        );
    connect(&m_previewPlayback,
            &AudioPlayer::errorChanged,
            this,
            &VoiceMessageController::voicePreviewChanged);

    // --- voice recoder ---
    using core::media::VoiceRecorder;
    connect(&m_voiceRecorder,
            &VoiceRecorder::stateChanged,
            this,
            &VoiceMessageController::voiceRecordingStateChanged);
    connect(&m_voiceRecorder,
            &VoiceRecorder::durationChanged,
            this,
            &VoiceMessageController::voiceRecordingDurationChanged);
    connect(&m_voiceRecorder,
            &VoiceRecorder::canceled,
            this,
            [this] {
        emit voiceRecordingCanceled();
    });
    connect(&m_voiceRecorder,
            &VoiceRecorder::errorOccurred,
            this,
            [this](const QString &message) {
                setVoiceRecordingError(message);
                emit voiceRecordingFailed(message);
            });
    connect(&m_voiceRecorder,
            &VoiceRecorder::previewReady,
            this,
            [this](const QUrl &file, qint64) {
                if (m_recordingConversationId.isEmpty()
                    || m_recordingConversationId != m_activeConversationId) {
                    return;
                }
                m_voiceRecordingFile = file;
                emit voiceRecordingFileChanged();
            });
    connect(&m_voiceRecorder,
            &VoiceRecorder::recordingReady,
            this,
            [this](const QUrl &file, qint64 duration) {
                if (m_recordingConversationId.isEmpty()
                    || m_recordingConversationId != m_activeConversationId) {
                    return;
                }
                m_voiceRecordingFile = file;
                emit voiceRecordingFileChanged();
                m_finalVoiceDuration = duration;
                emit voiceRecordingReady(m_recordingConversationId, file, duration);
            });
    connect(&m_voiceRecorder,
            &VoiceRecorder::recordingFileLocaltionChanged,
            this,
            [this](const QUrl &location) {
                m_voiceRecordingFile = location;
            }
            );

}

bool VoiceMessageController::isVoiceRecording() const
{
    return m_voiceRecorder.state() == core::media::VoiceRecorder::Recording;
}

bool VoiceMessageController::isVoiceRecordingPaused() const
{
    return m_voiceRecorder.state() == core::media::VoiceRecorder::Paused;
}

bool VoiceMessageController::isVoiceRecordingBusy() const
{
    return m_voiceRecorder.state() != core::media::VoiceRecorder::Idle;
}

qint64 VoiceMessageController::voiceRecordingDuration() const
{
    return m_voiceRecorder.duration();
}

QString VoiceMessageController::voiceRecordingDurationText() const
{
    const qint64 milliseconds = std::max<qint64>(0, voiceRecordingDuration());
    return core::utils::voiceRecordingDurationStr (milliseconds);
}

void VoiceMessageController::setVoiceRecordingError(const QString &message)
{
    if (m_voiceRecordingError != message) {
        m_voiceRecordingError = message;
        emit voiceRecordingErrorChanged();
    }
}



void VoiceMessageController::startVoiceRecording(const QUrl &outputDirectory)
{
    if (isVoiceRecordingBusy()) {
        return;
    }
    if (m_activeConversationId.isEmpty()) {
        const QString message = tr("Select a conversation before recording a voice message.");
        setVoiceRecordingError(message);
        emit voiceRecordingFailed(message);
        return;
    }
    ++m_voiceRecordingRevision;
    stopVoicePlayback();
    m_recordingConversationId = m_activeConversationId;
    m_finalVoiceDuration = 0;
    m_voiceRecordingFile = QUrl();
    emit voiceRecordingFileChanged();
    setVoiceRecordingError(QString());
    m_voiceRecorder.start(outputDirectory);
}

void VoiceMessageController::pauseVoiceRecording()
{
    m_voiceRecorder.pause();
}

void VoiceMessageController::resumeVoiceRecording()
{
    if (m_voiceRecorder.state() == core::media::VoiceRecorder::Paused) {
        m_voiceRecordingFile = QUrl();
        emit voiceRecordingFileChanged();
    }
    m_voiceRecorder.resume();
}

void VoiceMessageController::stopVoiceRecording()
{
    m_voiceRecorder.stop();
}

void VoiceMessageController::cancelVoiceRecording()
{
    qDebug() << ("Cancel voice recording");
    ++m_voiceRecordingRevision;
    // Invalidate the recipient before an asynchronous encoder completion.
    m_recordingConversationId.clear();
    m_finalVoiceDuration = 0;
    m_voiceRecorder.cancel();
    m_voiceRecordingFile = QUrl();
    emit voiceRecordingFileChanged();
    setVoiceRecordingError(QString());
}

bool VoiceMessageController::voicePreviewReady() const
{
    const auto status = m_previewPlayback.mediaStatus();
    return m_previewPlayback.error() == QMediaPlayer::NoError
        && (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferingMedia
            || status == QMediaPlayer::BufferedMedia || status == QMediaPlayer::EndOfMedia);
}
void VoiceMessageController::playVoicePreview()
{
    stopVoicePlayback();
    m_previewPlayback.togglePlayback();
}

void VoiceMessageController::playVoice(const QUrl &source, const QString &messageId)
{
    if (source.isEmpty()) {
        return;
    }

    m_previewPlayback.stop();
    // setIsInPlay (true);
    const bool sameMessage = !messageId.isEmpty()
        ? messageId == m_voicePlaybackMessageId : source == m_voicePlaybackSource;
    if (sameMessage && voiceDownloading()) {
        return;
    }
    if (sameMessage && !m_messagePlayback.source().isEmpty()
        && m_messagePlayback.error() == QMediaPlayer::NoError
        && m_messagePlayback.mediaStatus() != QMediaPlayer::InvalidMedia) {
        setIsInPlay (true);
        m_messagePlayback.togglePlayback();
        return;
    }
    stopCurrentAudio();
    m_messagePlayback.setSource(QUrl());
    m_voicePlaybackSource = source;
    m_voicePlaybackMessageId = messageId;
    m_voiceDownloadError.clear();
    m_voiceDownloadProgress = 0;

    if (source.isLocalFile()) {
        m_messagePlayback.setSource(source);
        m_messagePlayback.play();
    } else if (m_mediaRepository) {
        QString messageId = m_messageIdForSource(source);
        if (messageId.isEmpty()) {
            messageId = QString::fromLatin1(QCryptographicHash::hash(
                source.toEncoded(), QCryptographicHash::Sha256).toHex());
        }
        m_pendingVoiceMessage = messageId;
        emit voicePlaybackChanged();
        m_mediaRepository->downloadFile(messageId, source);
    } else {
        m_voiceDownloadError = tr("Audio download service is unavailable.");
    }
    emit voicePlaybackChanged();
    setIsInPlay (true);

}

void VoiceMessageController::pauseVoice()
{
    if (!m_messagePlayback.source ().isEmpty ()) {
        m_messagePlayback.pause ();
    }
}

void VoiceMessageController::resumeVoice()
{
    if (!m_messagePlayback.source ().isEmpty ()) {
        m_messagePlayback.togglePlayback ();
    }
}

void VoiceMessageController::seekVoice(qint64 position)
{
    m_messagePlayback.setPosition(position);
}

void VoiceMessageController::stopVoicePlayback()
{
    stopCurrentAudio();
    setIsInPlay(false);
}

void VoiceMessageController::stopCurrentAudio()
{
    const auto pending = m_pendingVoiceMessage;
    m_pendingVoiceMessage.clear();
    if (m_mediaRepository && !pending.isEmpty()) {
        m_mediaRepository->cancelDownload(pending);
    }
    m_messagePlayback.stop();
    emit voicePlaybackChanged();
}


void VoiceMessageController::seekVoicePreview(qint64 position)
{
    m_previewPlayback.setPosition(position);
}

void VoiceMessageController::stopVoicePreview()
{
    m_previewPlayback.stop();
}

void VoiceMessageController::cancelVoicePlayback()
{
    m_messagePlayback.stop ();
    setIsInPlay (false);
}

void VoiceMessageController::setPlaybackRate(qreal rate)
{
    m_messagePlayback.playbackRate (rate);
}


bool VoiceMessageController::isInPlay() const
{
    return m_currentPlay;

}

QString VoiceMessageController::voicePlaybackDurationText()
{
    const qint64 milliseconds = std::max<qint64>(0, voicePlaybackPosition ());
    return core::utils::voiceRecordingDurationStr (
        milliseconds,
        core::utils::DurationFormat::MinutesSeconds
        );
}

void VoiceMessageController::setIsInPlay(bool value)
{
    qDebug() << "====> value" << value;
    if (m_currentPlay == value) {
        return;
    }
    m_currentPlay = value;
    emit isInPlayChanged ();
}

void VoiceMessageController::setVoiceAudioInputDevice(const QAudioDevice &device)
{
    m_voiceRecorder.setAudioInputDevice(device);
}

void VoiceMessageController::setConversation(const QString &conversationId)
{
    if (conversationId != m_activeConversationId) {
        // Recording drafts belong to a conversation; message playback does not.
        cancelVoiceRecording();
        m_activeConversationId = conversationId;
    }
}

void VoiceMessageController::reset()
{
    stopVoicePlayback();
    m_voicePlaybackSource.clear();
    m_voicePlaybackMessageId.clear();
    m_messagePlayback.setSource(QUrl());
    cancelVoiceRecording();
    m_activeConversationId.clear();
    emit voicePlaybackChanged();
}
