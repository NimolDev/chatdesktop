#include "media/audio_player.hpp"
#include "media/pcm_wave.hpp"
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class AudioPlayerTest : public QObject
{
    Q_OBJECT
private slots:
    void playbackLifecycle()
    {
        QTemporaryDir directory;
        QFile file(directory.filePath("audio.wav"));
        QVERIFY(file.open(QIODevice::ReadWrite));
        file.write(QByteArray(44 + 16000 * 2 * 2, '\0'));
        QAudioFormat format;
        format.setSampleRate(16000);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Int16);
        QVERIFY(core::media::writePcmWaveHeader(file, format));
        file.close();

        core::media::AudioPlayer player;
        player.setMuted(true);
        QSignalSpy finished(&player, &core::media::AudioPlayer::finished);
        player.setSource(QUrl::fromLocalFile(file.fileName()));
        QTRY_VERIFY_WITH_TIMEOUT(player.duration() > 0 && player.isSeekable(), 10000);
        player.play();
        QTRY_VERIFY(player.position() > 100);
        player.pause();
        QCOMPARE(player.playbackState(), QMediaPlayer::PausedState);
        player.setPosition(1500);
        QTRY_COMPARE(player.position(), 1500);
        player.togglePlayback();
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 5000);
        QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
        QCOMPARE(player.position(), 0);
        player.play();
        QTRY_VERIFY(player.position() > 100);
        player.stop();
        QCOMPARE(player.position(), 0);
        QCOMPARE(player.playbackState(), QMediaPlayer::StoppedState);
        player.setSource(QUrl());
        QCOMPARE(player.source(), QUrl());
        QCOMPARE(player.position(), 0);
    }

    void volumeAndErrors()
    {
        core::media::AudioPlayer player;
        player.setVolume(2);
        QCOMPARE(player.volume(), 1.0f);
        player.setVolume(-1);
        QCOMPARE(player.volume(), 0.0f);
        player.setMuted(true);
        QVERIFY(player.isMuted());
        QTemporaryDir directory;
        player.setSource(QUrl::fromLocalFile(directory.filePath("missing.mp3")));
        player.play();
        QTRY_VERIFY(player.error() != QMediaPlayer::NoError);
        QVERIFY(!player.errorString().isEmpty());
    }
};
QTEST_GUILESS_MAIN(AudioPlayerTest)
#include "test_audio_player.moc"
