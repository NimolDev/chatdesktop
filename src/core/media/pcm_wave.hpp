#pragma once

#include <QAudioFormat>
#include <QDataStream>
#include <QIODevice>

namespace core {
namespace media {

// Finalize or initialize a WAV containing signed 16-bit little-endian PCM.
inline bool writePcmWaveHeader(QIODevice &output, const QAudioFormat &format)
{
    const qint64 dataSize = qMax<qint64>(0, output.size() - 44);
    if (!format.isValid() || format.sampleFormat() != QAudioFormat::Int16
        || dataSize > 0xffffffffLL - 36 || dataSize % format.bytesPerFrame() != 0
        || !output.seek(0)) {
        return false;
    }
    QDataStream stream(&output);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.writeRawData("RIFF", 4);
    stream << quint32(dataSize + 36);
    stream.writeRawData("WAVEfmt ", 8);
    stream << quint32(16) << quint16(1) << quint16(format.channelCount())
           << quint32(format.sampleRate()) << quint32(format.bytesPerFrame() * format.sampleRate())
           << quint16(format.bytesPerFrame()) << quint16(16);
    stream.writeRawData("data", 4);
    stream << quint32(dataSize);
    return stream.status() == QDataStream::Ok;
}

} // namespace media
} // namespace core
