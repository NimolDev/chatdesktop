#include "media/voice_recorder.hpp"
#include "media/pcm_wave.hpp"
#include "media/m4a_encoder.hpp"
#include <QAudioDecoder>
#include <QAudioBuffer>
#include <cmath>
#include <QBuffer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using core::media::VoiceRecorder;

class VoiceRecorderTest : public QObject
{
    Q_OBJECT
private slots:
    void repeatedPreviewsIncludeResumedAudio()
    {
        QTemporaryDir directory;
        core::media::M4aEncoder encoder;
        QSignalSpy encoded(&encoder, &core::media::M4aEncoder::encodingFinished);
        const QString finalPath = directory.filePath("voice.m4a");
        QVERIFY(encoder.start(finalPath, 48000, 1));
        const QByteArray second(48000 * 2, '\0');
        QStringList paths;
        for (int pause = 1; pause <= 2; ++pause) {
            QCOMPARE(encoder.write(second), second.size());
            const QString path = directory.filePath(QStringLiteral("preview-%1.m4a").arg(pause));
            QVERIFY2(encoder.savePreview(path), qPrintable(encoder.errorString()));
            QTRY_COMPARE_WITH_TIMEOUT(encoded.count(), pause, 10000);
            QVERIFY2(encoded.last().at(1).toBool(), qPrintable(encoder.errorString()));
            QVERIFY(encoder.isOpen());
            QCOMPARE(encoder.pcmBytes(), pause * second.size());
            paths.append(path);
        }
        QVERIFY(encoder.finish());
        QTRY_COMPARE_WITH_TIMEOUT(encoded.count(), 3, 10000);
        QVERIFY2(encoded.last().at(1).toBool(), qPrintable(encoder.errorString()));
        paths.append(finalPath);
        for (int i = 0; i < paths.size(); ++i) {
            QAudioDecoder decoder;
            QSignalSpy finished(&decoder, &QAudioDecoder::finished);
            qint64 frames = 0;
            connect(&decoder, &QAudioDecoder::bufferReady, &decoder, [&] {
                frames += decoder.read().frameCount();
            });
            decoder.setSource(QUrl::fromLocalFile(paths[i]));
            decoder.start();
            QTRY_VERIFY_WITH_TIMEOUT(finished.count() > 0 || decoder.error() != QAudioDecoder::NoError, 10000);
            QCOMPARE(decoder.error(), QAudioDecoder::NoError);
            QVERIFY(qAbs(frames - qint64(i == 0 ? 48000 : 96000)) <= 2048);
        }
    }

    void m4aRoundTrip_data()
    {
        QTest::addColumn<int>("channels");
        QTest::newRow("mono") << 1;
        QTest::newRow("stereo") << 2;
    }

    void m4aRoundTrip()
    {
        QFETCH(int, channels);
        QTemporaryDir directory;
        const QString path = directory.filePath("voice.m4a");
        core::media::M4aEncoder encoder;
        QSignalSpy encoded(&encoder, &core::media::M4aEncoder::encodingFinished);
        QVERIFY(encoder.start(path, 48000, channels));
        QByteArray pcm(48000 * 2 * channels, Qt::Uninitialized);
        for (int i = 0; i < 48000; ++i) {
            const qint16 sample = qint16(8000 * std::sin(2 * 3.141592653589793 * 440 * i / 48000));
            for (int channel = 0; channel < channels; ++channel) {
                const int offset = 2 * (i * channels + channel);
                pcm[offset] = char(sample & 0xff);
                pcm[offset + 1] = char((sample >> 8) & 0xff);
            }
        }
        // Exercise partial PCM frames and encoder buffering.
        for (qsizetype offset = 0; offset < pcm.size(); offset += 997) {
            const auto size = qMin<qsizetype>(997, pcm.size() - offset);
            QCOMPARE(encoder.write(pcm.constData() + offset, size), size);
        }
        QVERIFY2(encoder.finish(), qPrintable(encoder.errorString()));
        QTRY_COMPARE_WITH_TIMEOUT(encoded.count(), 1, 10000);
        QVERIFY2(encoded.last().at(1).toBool(), qPrintable(encoder.errorString()));
        QFile container(path);
        QVERIFY(container.open(QIODevice::ReadOnly));
        QCOMPARE(container.read(8).mid(4), QByteArray("ftyp"));
        QCOMPARE(encoder.pcmBytes(), pcm.size());
        QAudioDecoder decoder;
        QSignalSpy finished(&decoder, &QAudioDecoder::finished);
        qint64 samples = 0;
        double energy = 0;
        connect(&decoder, &QAudioDecoder::bufferReady, &decoder, [&] {
            const auto buffer = decoder.read();
            const auto format = buffer.format();
            const char *data = buffer.constData<char>();
            for (int i = 0; i < buffer.sampleCount(); ++i) {
                const float value = format.normalizedSampleValue(data + i * format.bytesPerSample());
                energy += value * value;
                ++samples;
            }
        });
        decoder.setSource(QUrl::fromLocalFile(path));
        decoder.start();
        QTRY_VERIFY_WITH_TIMEOUT(finished.count() > 0 || decoder.error() != QAudioDecoder::NoError, 10000);
        QCOMPARE(decoder.error(), QAudioDecoder::NoError);
        QVERIFY(samples >= 44000 * channels && samples <= 50000 * channels);
        const double rms = std::sqrt(energy / samples);
        QVERIFY(rms > 0.14 && rms < 0.20);
    }

    void m4aRejectsEmptyRecording()
    {
        QTemporaryDir directory;
        core::media::M4aEncoder encoder;
        QSignalSpy encoded(&encoder, &core::media::M4aEncoder::encodingFinished);
        QVERIFY(encoder.start(directory.filePath("empty.m4a"), 48000, 1));
        QVERIFY(!encoder.finish());
    }

    void finalEncodingSupersedesPendingPreview()
    {
        QTemporaryDir directory;
        core::media::M4aEncoder encoder;
        QSignalSpy encoded(&encoder, &core::media::M4aEncoder::encodingFinished);
        const QString path = directory.filePath("voice.m4a");
        QVERIFY(encoder.start(path, 48000, 1));
        const QByteArray pcm(96000, '\0');
        QCOMPARE(encoder.write(pcm), pcm.size());
        QVERIFY(encoder.savePreview(directory.filePath("preview.m4a")));
        QVERIFY(encoder.finish());
        QTRY_COMPARE_WITH_TIMEOUT(encoded.count(), 1, 10000);
        QCOMPARE(encoded.first().at(0).toString(), path);
        QVERIFY2(encoded.first().at(1).toBool(), qPrintable(encoder.errorString()));
    }

    void destructionCancelsEncoding()
    {
        QTemporaryDir directory;
        auto encoder = std::make_unique<core::media::M4aEncoder>();
        QVERIFY(encoder->start(directory.filePath("voice.m4a"), 48000, 1));
        const QByteArray pcm(96000, '\0');
        QCOMPARE(encoder->write(pcm), pcm.size());
        QVERIFY(encoder->finish());
        encoder.reset();
        QCoreApplication::processEvents();
        QVERIFY(QDir(directory.path()).entryList({"*.pcm-*"}, QDir::Files).isEmpty());
    }

    void pcmWavePreservesSamples()
    {
        QAudioFormat format;
        format.setSampleRate(48000);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Int16);
        QBuffer output;
        QVERIFY(output.open(QIODevice::ReadWrite));
        QVERIFY(core::media::writePcmWaveHeader(output, format));
        const QByteArray samples = QByteArray::fromHex("0000ff7f00800040");
        QCOMPARE(output.write(samples), samples.size());
        QVERIFY(core::media::writePcmWaveHeader(output, format));
        // Independent expected RIFF header: mono, 48 kHz, 16-bit, eight PCM bytes.
        const QByteArray expectedHeader = QByteArray::fromHex(
            "524946462c00000057415645666d7420100000000100010080bb000000770100"
            "020010006461746108000000");
        QCOMPARE(output.data(), expectedHeader + samples);
    }

    void pcmWaveRejectsWrongSampleFormat()
    {
        QAudioFormat format;
        format.setSampleRate(48000);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Float);
        QBuffer output;
        QVERIFY(output.open(QIODevice::ReadWrite));
        QVERIFY(!core::media::writePcmWaveHeader(output, format));
        QVERIFY(output.data().isEmpty());
    }

    void idleControlsAreHarmless()
    {
        VoiceRecorder recorder;
        QSignalSpy states(&recorder, &VoiceRecorder::stateChanged);
        QSignalSpy ready(&recorder, &VoiceRecorder::recordingReady);
        recorder.pause();
        recorder.resume();
        recorder.stop();
        recorder.cancel();
        QCOMPARE(recorder.state(), VoiceRecorder::Idle);
        QCOMPARE(recorder.duration(), 0);
        QVERIFY(recorder.outputLocation().isEmpty());
        QCOMPARE(states.count(), 0);
        QCOMPARE(ready.count(), 0);
    }

    void rejectsRemoteOutput()
    {
        VoiceRecorder recorder;
        QSignalSpy errors(&recorder, &VoiceRecorder::errorOccurred);
        recorder.start(QUrl("https://example.com/recordings"));
        QCOMPARE(errors.count(), 1);
        QVERIFY(!recorder.errorString().isEmpty());
        QCOMPARE(recorder.state(), VoiceRecorder::Idle);
    }

    void preservesExistingFile()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("existing");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("keep this");
        file.close();
        VoiceRecorder recorder;
        QSignalSpy errors(&recorder, &VoiceRecorder::errorOccurred);
        recorder.start(QUrl::fromLocalFile(path));
        QCOMPARE(errors.count(), 1);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("keep this"));
    }

    void cancelsBeforePermissionOrCapture()
    {
        QTemporaryDir directory;
        VoiceRecorder recorder;
        QSignalSpy canceled(&recorder, &VoiceRecorder::canceled);
        QSignalSpy ready(&recorder, &VoiceRecorder::recordingReady);
        connect(&recorder, &VoiceRecorder::stateChanged, &recorder, [&] {
            if (recorder.state() == VoiceRecorder::Starting) {
                // Duplicate start must not create a second request.
                recorder.start(QUrl::fromLocalFile(directory.path()));
                recorder.cancel();
            }
        });
        for (int attempt = 0; attempt < 2; ++attempt) {
            recorder.start(QUrl::fromLocalFile(directory.path()));
            QCoreApplication::processEvents();
            QCOMPARE(recorder.state(), VoiceRecorder::Idle);
        }
        QCOMPARE(canceled.count(), 2);
        QCOMPARE(ready.count(), 0);
        QVERIFY(QDir(directory.path()).entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot).isEmpty());
    }
};

QTEST_GUILESS_MAIN(VoiceRecorderTest)
#include "test_voice_recorder.moc"
