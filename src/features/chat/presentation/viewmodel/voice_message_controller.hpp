#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <functional>

#include <memory>
#include "domain/repository/media_repository.hpp"
#include "media/audio_player.hpp"
#include "media/voice_recorder.hpp"

class VoiceMessageController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by MessagingViewModel")

public:
    explicit VoiceMessageController(
        std::shared_ptr<domain::repository::MediaRepository> mediaRepository,
        std::function<QString(const QUrl &)> messageIdForSource,
        QObject *parent = nullptr);

    void setConversation(const QString &conversationId);
    void reset();

    Q_PROPERTY(bool isVoiceRecording READ isVoiceRecording NOTIFY voiceRecordingStateChanged)
    Q_PROPERTY(bool isVoiceRecordingPaused READ isVoiceRecordingPaused NOTIFY voiceRecordingStateChanged)
    Q_PROPERTY(bool isVoiceRecordingBusy READ isVoiceRecordingBusy NOTIFY voiceRecordingStateChanged)
    Q_PROPERTY(qint64 voiceRecordingDuration READ voiceRecordingDuration NOTIFY voiceRecordingDurationChanged)
    Q_PROPERTY(QString voiceRecordingDurationText READ voiceRecordingDurationText NOTIFY voiceRecordingDurationChanged)
    Q_PROPERTY(QUrl voiceRecordingFile READ voiceRecordingFile NOTIFY voiceRecordingFileChanged)
    Q_PROPERTY(QString voiceRecordingError READ voiceRecordingError NOTIFY voiceRecordingErrorChanged)


    Q_INVOKABLE void startVoiceRecording(const QUrl &outputDirectory = {});
    Q_INVOKABLE void startVoiceRecoder() { startVoiceRecording(); }
    Q_INVOKABLE void pauseVoiceRecording();
    Q_INVOKABLE void resumeVoiceRecording();
    Q_INVOKABLE void stopVoiceRecording();
    Q_INVOKABLE void cancelVoiceRecording();

    qint64 finalizedRecordingDuration() const { return m_finalVoiceDuration; }
    QString recordingConversationId() const { return m_recordingConversationId; }
    quint64 recordingRevision() const { return m_voiceRecordingRevision; }

    bool isVoiceRecording() const;
    bool isVoiceRecordingPaused() const;
    bool isVoiceRecordingBusy() const;
    qint64 voiceRecordingDuration() const;
    QString voiceRecordingDurationText() const;
    QUrl voiceRecordingFile() const { return m_voiceRecordingFile; }
    QString voiceRecordingError() const { return m_voiceRecordingError; }

    //MARK: - Audio player
public:
    Q_PROPERTY(bool voiceDownloading READ voiceDownloading NOTIFY voicePlaybackChanged)   
    Q_PROPERTY(double voiceDownloadProgress READ voiceDownloadProgress NOTIFY voicePlaybackChanged)   
    Q_PROPERTY(QString voicePlaybackMessageId READ voicePlaybackMessageId NOTIFY voicePlaybackChanged)
    Q_PROPERTY(QUrl voicePlaybackSource READ voicePlaybackSource NOTIFY voicePlaybackChanged)   
    Q_PROPERTY(bool voicePlaybackPlaying READ voicePlaybackPlaying NOTIFY voicePlaybackChanged)   
    Q_PROPERTY(bool voicePlaybackSeekable READ voicePlaybackSeekable NOTIFY voicePlaybackChanged)    
    Q_PROPERTY(qint64 voicePlaybackPosition READ voicePlaybackPosition NOTIFY voicePlaybackChanged)   
    Q_PROPERTY(qint64 voicePlaybackDuration READ voicePlaybackDuration NOTIFY voicePlaybackChanged)   
    Q_PROPERTY(QString voicePlaybackError READ voicePlaybackError NOTIFY voicePlaybackChanged)
    Q_PROPERTY(QUrl voicePreviewSource READ voicePreviewSource NOTIFY voicePreviewChanged)  
    Q_PROPERTY(bool voicePreviewPlaying READ voicePreviewPlaying NOTIFY voicePreviewChanged)   
    Q_PROPERTY(bool voicePreviewSeekable READ voicePreviewSeekable NOTIFY voicePreviewChanged)   
    Q_PROPERTY(qint64 voicePreviewPosition READ voicePreviewPosition NOTIFY voicePreviewChanged)    
    Q_PROPERTY(qint64 voicePreviewDuration READ voicePreviewDuration NOTIFY voicePreviewChanged)   
    Q_PROPERTY(QString voicePreviewError READ voicePreviewError NOTIFY voicePreviewChanged)   
    Q_PROPERTY(bool voicePreviewReady READ voicePreviewReady NOTIFY voicePreviewChanged)  
    Q_PROPERTY(bool isInPlay READ isInPlay WRITE setIsInPlay NOTIFY isInPlayChanged FINAL)
    Q_PROPERTY(QString voicePlaybackDurationText READ voicePlaybackDurationText  NOTIFY voicePlaybackDurationTextChanged FINAL)



    Q_INVOKABLE void playVoice(const QUrl &source, const QString &messageId = {});
    Q_INVOKABLE void pauseVoice();
    Q_INVOKABLE void resumeVoice();
    Q_INVOKABLE void seekVoice(qint64 position);
    Q_INVOKABLE void stopVoicePlayback();
    Q_INVOKABLE void playVoicePreview();
    Q_INVOKABLE void seekVoicePreview(qint64 position);
    Q_INVOKABLE void stopVoicePreview();
    Q_INVOKABLE void cancelVoicePlayback();
    Q_INVOKABLE void setPlaybackRate(qreal rate);

    bool voiceDownloading() const { return !m_pendingVoiceMessage.isEmpty(); }
    double voiceDownloadProgress() const { return m_voiceDownloadProgress; }
    QString voicePlaybackMessageId() const { return m_voicePlaybackMessageId; }
    QUrl voicePlaybackSource() const { return m_voicePlaybackSource; }
    bool voicePlaybackPlaying() const { return m_messagePlayback.playbackState() == QMediaPlayer::PlayingState; }
    bool voicePlaybackSeekable() const { return m_messagePlayback.isSeekable(); }
    qint64 voicePlaybackPosition() const { return m_messagePlayback.position(); }
    qint64 voicePlaybackDuration() const { return m_messagePlayback.duration(); }
    QString voicePlaybackError() const { return m_voiceDownloadError.isEmpty() ? m_messagePlayback.errorString() : m_voiceDownloadError; }
    QUrl voicePreviewSource() const { return m_previewPlayback.source(); }
    bool voicePreviewPlaying() const { return m_previewPlayback.playbackState() == QMediaPlayer::PlayingState; }
    bool voicePreviewSeekable() const { return m_previewPlayback.isSeekable(); }
    qint64 voicePreviewPosition() const { return m_previewPlayback.position(); }
    qint64 voicePreviewDuration() const { return m_previewPlayback.duration(); }
    QString voicePreviewError() const { return m_previewPlayback.errorString(); }
    bool voicePreviewReady() const;

    bool isInPlay() const;
    QString voicePlaybackDurationText();


public slots:
    void setVoiceAudioInputDevice(const QAudioDevice &device);


signals:
    void voicePlaybackChanged();
    void voicePreviewChanged();
    void voiceRecordingStateChanged();
    void voiceRecordingDurationChanged();
    void voiceRecordingFileChanged();
    void voiceRecordingErrorChanged();
    void voiceRecordingFailed(const QString &message);
    void voiceRecordingCanceled();
    // The local file is finalized. Upload handling is separate from text sending.
    void voiceRecordingReady(const QString &receiverId, const QUrl &file, qint64 duration);
    void isInPlayChanged();
    void voicePlaybackDurationTextChanged();


private:
    void stopCurrentAudio();
    void setVoiceRecordingError(const QString &message);
    void setIsInPlay(bool value);

private:
    std::function<QString(const QUrl &)> m_messageIdForSource;
    QString m_activeConversationId;
    std::shared_ptr<domain::repository::MediaRepository> m_mediaRepository;
    QUrl m_voicePlaybackSource;
    QString m_voicePlaybackMessageId;
    QString m_pendingVoiceMessage;
    QString m_voiceDownloadError;
    double m_voiceDownloadProgress = 0;
    core::media::AudioPlayer m_messagePlayback;
    core::media::AudioPlayer m_previewPlayback;
    core::media::VoiceRecorder m_voiceRecorder;
    QString m_recordingConversationId;
    QUrl m_voiceRecordingFile;
    qint64 m_finalVoiceDuration = 0;
    quint64 m_voiceRecordingRevision = 0;
    QString m_voiceRecordingError;
    bool m_currentPlay = false;


};
