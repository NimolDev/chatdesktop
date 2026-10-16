#include "file_name_helper.hpp"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

core::storage::FileNameHelper::FileNameHelper() {}

QString core::storage::FileNameHelper::hexName(
    const QString &logical_name,
    const QString &secret,
    size_t hex_chars)
{
    if (hex_chars < 8 || hex_chars > 64) {
        throw std::invalid_argument("hex_chars muse be 8..64");
    }
    const QString key = "storage/cache/" + secret;
    quint8 mac[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    if (!HMAC(EVP_sha256 (), key.data (), int(key.size ()),
              reinterpret_cast<const quint8 *>(logical_name.data ()),
              logical_name.size (), mac, &len)) {
        throw std::runtime_error("HMAC failed");
    }
    static const char digits[] = "0123456789abcdef";
    QString out;
    for(size_t i = 0; out.size () < hex_chars; i++) {
        out += digits[mac[i] >> 4];
        if (out.size () < hex_chars) {
            out += digits[mac[i] & 15];
        }
    }
    return out;
}

QString core::storage::FileNameHelper::hexNameV2(
    const QString &logical_name, const QString &secret, size_t hex_chars)
{
    if (hex_chars < 8 || hex_chars > 64) {
        throw std::invalid_argument("hex_chars must be 8..64");
    }
    const QByteArray key = (QStringLiteral("storage/cache/v2/") + secret).toUtf8();
    const QByteArray name = logical_name.toUtf8();
    unsigned char mac[EVP_MAX_MD_SIZE];
    unsigned int length = 0;
    if (!HMAC(EVP_sha256(), key.constData(), int(key.size()),
              reinterpret_cast<const unsigned char *>(name.constData()),
              size_t(name.size()), mac, &length)) {
        throw std::runtime_error("HMAC failed");
    }
    return QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(mac), int(length))
                                  .toHex().left(qsizetype(hex_chars)));
}

QString core::storage::FileNameHelper::randomHexName(size_t hex_chars)
{
    if (hex_chars < 8 || hex_chars > 64) {
        throw std::invalid_argument("hex_chars muse be 8...64");
    }
    quint8 r[32];
    randBytes (r, sizeof r);
    static const char digits[] = "0123456789abcdef";
    QString out;
    for (size_t i = 0; out.size() < hex_chars; i++) {
        out += digits[r[i] >> 4];
        if (out.size() < hex_chars) out += digits[r[i] & 15];
    }
    return out;
}

void core::storage::FileNameHelper::randBytes(quint8 *p, size_t n)
{
    if (RAND_bytes (p, int(n)) != 1) {
        throw std::runtime_error("RAND_bytes fialed");
    }
}
