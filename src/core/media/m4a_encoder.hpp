#pragma once

#include <QAudioFormat>
#include <QIODevice>
#include <QTemporaryFile>
#include <memory>

namespace core {
namespace media {

// Collects PCM and asynchronously encodes snapshots using Qt Multimedia.
class M4aEncoder final : public QIODevice
{
    Q_OBJECT
public:
    explicit M4aEncoder(QObject *parent = nullptr);
    ~M4aEncoder() override;
    bool start(const QString &path, int sampleRate, int channels);
    bool finish();
    bool savePreview(const QString &path);
    qint64 pcmBytes() const { return m_pcmBytes; }
    QString fileName() const { return m_path; }
    bool isSequential() const override { return true; }

signals:
    void encodingFinished(const QString &path, bool success);

protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *data, qint64 size) override;

private:
    struct Job;
    bool encode(const QString &path);
    void complete(const QString &error);
    QTemporaryFile m_pcm;
    QAudioFormat m_format;
    QString m_path;
    qint64 m_pcmBytes = 0;
    bool m_failed = false;
    std::unique_ptr<Job> m_job;
};

} // namespace media
} // namespace core
