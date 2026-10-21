#include "storage/media_cache.hpp"
#include "audio_player.hpp"

#include <algorithm>

namespace core {
namespace media {

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent)
{
    m_player.setAudioOutput(&m_output);
    connect(&m_player, &QMediaPlayer::sourceChanged, this, &AudioPlayer::sourceChanged);
    connect(&m_player, &QMediaPlayer::positionChanged, this, &AudioPlayer::positionChanged);
    connect(&m_player, &QMediaPlayer::durationChanged, this, &AudioPlayer::durationChanged);
    connect(&m_player, &QMediaPlayer::seekableChanged, this, &AudioPlayer::seekableChanged);
    connect(&m_player, &QMediaPlayer::playbackStateChanged, this, &AudioPlayer::playbackStateChanged);
    connect(&m_player, &QMediaPlayer::errorChanged, this, &AudioPlayer::errorChanged);
    connect(&m_output, &QAudioOutput::volumeChanged, this, &AudioPlayer::volumeChanged);
    connect(&m_output, &QAudioOutput::mutedChanged, this, &AudioPlayer::mutedChanged);
    connect(&m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            m_player.setPosition(0);
            emit finished();
        }
        emit mediaStatusChanged();
    });
}

void AudioPlayer::setSource(const QUrl &source)
{
    if (source == m_player.source()) {
        return;
    }
    stop();
    m_player.setSource(QUrl());
    m_cachedAudio.close();
    m_cachedAudio.setData(QByteArray());
    if (source.isLocalFile() && core::storage::MediaCache::isCachePath(source.toLocalFile())) {
        m_cachedAudio.setData(core::storage::MediaCache::read(source.toLocalFile()));
        if (!m_cachedAudio.data().isEmpty() && m_cachedAudio.open(QIODevice::ReadOnly)) {
            m_player.setSourceDevice(&m_cachedAudio, source);
        }
    } else {
        m_player.setSource(source);
    }
}

void AudioPlayer::setPosition(qint64 position)
{
    if (m_player.isSeekable()) {
        m_player.setPosition(std::clamp<qint64>(position, 0, m_player.duration()));
    }
}

void AudioPlayer::setVolume(float volume)
{
    m_output.setVolume(std::clamp(volume, 0.0f, 1.0f));
}

void AudioPlayer::setMuted(bool muted)
{
    m_output.setMuted(muted);
}

void AudioPlayer::setAudioOutputDevice(const QAudioDevice &device)
{
    m_output.setDevice(device);
}

void AudioPlayer::play()
{
    if (!source().isEmpty()) {
        m_player.play();
    }
}

void AudioPlayer::pause()
{
    m_player.pause();
}

void AudioPlayer::stop()
{
    m_player.stop();
    m_player.setPosition(0);
}

void AudioPlayer::togglePlayback()
{
    if (playbackState() == QMediaPlayer::PlayingState) {
        pause();
    } else {
        play();
    }
}

void AudioPlayer::playbackRate(qreal rate)
{
    m_player.setPlaybackRate (rate);
}



} // namespace media
} // namespace core
