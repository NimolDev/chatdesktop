#include "m4a_encoder.hpp"

#include <QAudioBuffer>
#include <QAudioBufferInput>
#include <QFileInfo>
#include <QMediaCaptureSession>
#include <QMediaFormat>
#include <QMediaRecorder>
#include <QTimer>
#include <QUrl>

namespace core {
namespace media {

struct M4aEncoder::Job
{
    QMediaCaptureSession session;
    QMediaRecorder recorder;
    QAudioBufferInput input;
    QFile pcm;
    QTimer timeout;
    QString path;
    qint64 limit = 0;
    bool ended = false;
    bool completed = false;
};

M4aEncoder::M4aEncoder(QObject *parent) : QIODevice(parent) {}
M4aEncoder::~M4aEncoder() = default;

bool M4aEncoder::start(const QString &path, int sampleRate, int channels)
{
    if (isOpen() || !m_path.isEmpty() || sampleRate <= 0 || (channels != 1 && channels != 2)) {
        setErrorString(tr("Invalid M4A capture format."));
        return false;
    }
    QMediaFormat mediaFormat(QMediaFormat::Mpeg4Audio);
    mediaFormat.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    if (!mediaFormat.isSupported(QMediaFormat::Encode)) {
        setErrorString(tr("AAC/M4A recording is unavailable in this Qt Multimedia backend."));
        return false;
    }
    m_path = path;
    m_format.setSampleRate(sampleRate);
    m_format.setChannelCount(channels);
    m_format.setSampleFormat(QAudioFormat::Int16);
    m_pcm.setFileTemplate(path + QStringLiteral(".pcm-XXXXXX"));
    if (!m_pcm.open()) {
        setErrorString(m_pcm.errorString());
        return false;
    }
    return open(QIODevice::WriteOnly);
}

qint64 M4aEncoder::writeData(const char *data, qint64 size)
{
    if (m_failed || m_pcm.write(data, size) != size) {
        m_failed = true;
        setErrorString(tr("Cannot store voice recording audio."));
        return -1;
    }
    m_pcmBytes += size;
    return size;
}

bool M4aEncoder::savePreview(const QString &path)
{
    return encode(path);
}

bool M4aEncoder::finish()
{
    if (!encode(m_path)) {
        return false;
    }
    close();
    return true;
}

bool M4aEncoder::encode(const QString &path)
{
    const int bytes_per_frame = m_format.bytesPerFrame ();
    if (!isOpen() || m_failed || m_pcmBytes == 0
        || !m_pcm.flush()
        || m_pcmBytes % bytes_per_frame != 0) {
        setErrorString(tr("The recording contains no complete audio or could not be encoded."));
        return false;
    }
    // A newer snapshot supersedes an in-flight preview; PCM capture is retained.
    m_job.reset();
    m_job = std::make_unique<Job>();
    auto &job = *m_job;
    job.path = path;
    job.limit = m_pcmBytes;
    job.pcm.setFileName(m_pcm.fileName());
    if (!job.pcm.open(QIODevice::ReadOnly)) {
        setErrorString(job.pcm.errorString());
        m_job.reset();
        return false;
    }
    QMediaFormat format(QMediaFormat::Mpeg4Audio);
    format.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    job.recorder.setMediaFormat(format);
    job.recorder.setAudioSampleRate(m_format.sampleRate());
    job.recorder.setAudioChannelCount(m_format.channelCount());
    job.recorder.setAudioBitRate(128000);
    job.recorder.setOutputLocation(QUrl::fromLocalFile(path));
    job.recorder.setAutoStop(true);
    job.session.setAudioBufferInput(&job.input);
    job.session.setRecorder(&job.recorder);
    connect(&job.input, &QAudioBufferInput::readyToSendAudioBuffer,
            &job.input, [this] {
        auto &job = *m_job;
        while (!job.ended && !job.completed) {
            const qint64 position = job.pcm.pos();
            if (position == job.limit) {
                job.ended = job.input.sendAudioBuffer(QAudioBuffer());
                return;
            }
            const qint64 bytes = qMin(job.limit - position,
                                     qint64(4096 * m_format.bytesPerFrame()));
            const QByteArray pcm = job.pcm.read(bytes);
            if (pcm.size() != bytes) {
                complete(tr("Cannot read captured audio."));
                return;
            }
            if (!job.input.sendAudioBuffer(QAudioBuffer(pcm, m_format,
                                            m_format.durationForBytes(position)))) {
                job.pcm.seek(position);
                return;
            }
        }
    });
    connect(&job.recorder, &QMediaRecorder::errorOccurred, &job.input,
            [this](QMediaRecorder::Error, const QString &error) { complete(error); });
    connect(&job.recorder, &QMediaRecorder::recorderStateChanged, &job.input,
            [this](QMediaRecorder::RecorderState state) {
        if (state == QMediaRecorder::StoppedState && m_job->ended) {
            const QFileInfo output(m_job->path);
            complete(output.isFile() && output.size() > 0
                         ? QString() : tr("The M4A encoder produced no audio."));
        }
    });
    job.timeout.setSingleShot(true);
    connect(&job.timeout, &QTimer::timeout, &job.input,
            [this] { complete(tr("M4A encoding timed out.")); });
    job.timeout.start(60000);
    job.recorder.record();
    return true;
}

void M4aEncoder::complete(const QString &error)
{
    if (!m_job || m_job->completed) {
        return;
    }
    m_job->completed = true;
    m_job->timeout.stop();
    // Deliver after the recorder callback returns; the receiver may destroy us.
    QTimer::singleShot(0, &m_job->input, [this, error] {
        const QString path = m_job->path;
        if (!error.isEmpty()) {
            setErrorString(error);
        }
        emit encodingFinished(path, error.isEmpty());
    });
}

} // namespace media
} // namespace core
