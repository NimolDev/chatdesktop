#pragma once

#include <QAudioOutput>
#include <QBuffer>
#include <QMediaPlayer>
#include <QObject>
#include <QUrl>

namespace core {
namespace media {

// Create on the GUI thread. Each instance owns its playback and audio output.
class AudioPlayer final : public QObject
{
    Q_OBJECT

public:
    explicit AudioPlayer(QObject *parent = nullptr);

    QUrl source() const { return m_player.source(); }
    qint64 position() const { return m_player.position(); }
    qint64 duration() const { return m_player.duration(); }
    bool isSeekable() const { return m_player.isSeekable(); }
    QMediaPlayer::PlaybackState playbackState() const { return m_player.playbackState(); }
    QMediaPlayer::MediaStatus mediaStatus() const { return m_player.mediaStatus(); }
    QMediaPlayer::Error error() const { return m_player.error(); }
    QString errorString() const { return m_player.errorString(); }
    float volume() const { return m_output.volume(); }
    bool isMuted() const { return m_output.isMuted(); }

    void setSource(const QUrl &source);
    void setPosition(qint64 position);
    void setVolume(float volume);
    void setMuted(bool muted);

    void play();
    void pause();
    void stop();
    void togglePlayback();
    void playbackRate(qreal rate);

public slots:
    void setAudioOutputDevice(const QAudioDevice &device);

signals:
    void sourceChanged();
    void positionChanged();
    void durationChanged();
    void seekableChanged();
    void playbackStateChanged();
    void mediaStatusChanged();
    void errorChanged();
    void volumeChanged();
    void mutedChanged();
    void finished();

private:
    // The player is destroyed first, while its output is still alive.
    QBuffer m_cachedAudio;
    QAudioOutput m_output;
    QMediaPlayer m_player;
};

} // namespace media
} // namespace core
