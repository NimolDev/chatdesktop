#include "app_constants.hpp"


#include "encrypted_app_strings.hpp"


#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <QByteArray>
#include <QUrl>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>


namespace core {
namespace constants {

namespace {
using Byte = std::uint8_t;

constexpr std::size_t kKeySize = 32;

using AesKey = std::array<Byte, kKeySize>;

#if defined(APP_STRINGS_ENCRYPTED)

[[nodiscard]]
AesKey createKey()
{
    AesKey key{};
    for (std::size_t index = 0;
         index < key.size();
         ++index) {
        key[index] =
            generated::app_constants::kMaskedKey[index] ^
            generated::app_constants::kKeyMask[index];
    }
    return key;
}

[[nodiscard]]
QString decrypt(
    const generated::app_constants::EncryptedString& value)
{

    AesKey key = createKey();
    EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();

    if (context == nullptr) {
        OPENSSL_cleanse(
            key.data(),
            key.size());

        throw std::runtime_error("Cannot create decryption context");

    }

    QByteArray plaintext;

    plaintext.resize(static_cast<qsizetype>(value.ciphertextSize));

    const auto cleanup = [&]() {
        EVP_CIPHER_CTX_free(context);

        OPENSSL_cleanse(
            key.data(),
            key.size());

    };

    try {
        if (EVP_DecryptInit_ex(
                context,

                EVP_aes_256_gcm(),

                nullptr,

                nullptr,

                nullptr) != 1) {

            throw std::runtime_error(

                "Cannot initialize AES-256-GCM");

        }

        if (EVP_CIPHER_CTX_ctrl(

                context,

                EVP_CTRL_GCM_SET_IVLEN,

                static_cast<int>(value.ivSize),

                nullptr) != 1) {

            throw std::runtime_error(

                "Cannot set AES-GCM IV length");

        }

        if (EVP_DecryptInit_ex(

                context,

                nullptr,

                nullptr,

                key.data(),

                value.iv) != 1) {

            throw std::runtime_error(

                "Cannot set AES key and IV");

        }

        int outputLength = 0;

        if (value.ciphertextSize > 0) {

            if (EVP_DecryptUpdate(

                    context,

                    reinterpret_cast<unsigned char*>(

                        plaintext.data()),

                    &outputLength,

                    value.ciphertext,

                    static_cast<int>(

                        value.ciphertextSize)) != 1) {

                throw std::runtime_error(

                    "AES decryption failed");

            }

        }

        std::array<Byte, 16> tag{};

        if (value.tagSize != tag.size()) {

            throw std::runtime_error(

                "Invalid AES-GCM tag size");

        }

        for (std::size_t index = 0;

             index < tag.size();

             ++index) {

            tag[index] = value.tag[index];

        }

        if (EVP_CIPHER_CTX_ctrl(

                context,

                EVP_CTRL_GCM_SET_TAG,

                static_cast<int>(tag.size()),

                tag.data()) != 1) {

            throw std::runtime_error(

                "Cannot set AES-GCM tag");

        }

        int finalLength = 0;

        const int verified =

            EVP_DecryptFinal_ex(

                context,

                reinterpret_cast<unsigned char*>(

                    plaintext.data()) + outputLength,

                &finalLength);

        OPENSSL_cleanse(

            tag.data(),

            tag.size());

        if (verified != 1) {

            throw std::runtime_error(

                "Encrypted string verification failed");

        }

        plaintext.resize(

            outputLength + finalLength);

        const QString result =

            QString::fromUtf8(plaintext);

        OPENSSL_cleanse(

            plaintext.data(),

            static_cast<std::size_t>(

                plaintext.size()));

        cleanup();

        return result;

    } catch (...) {

        if (!plaintext.isEmpty()) {

            OPENSSL_cleanse(

                plaintext.data(),

                static_cast<std::size_t>(

                    plaintext.size()));

        }

        cleanup();

        throw;

    }

}

#else

[[nodiscard]]
QString fromPlainString(
    const std::string_view value)
{
    return QString::fromUtf8(
        value.data(),
        static_cast<qsizetype>(
            value.size()));
}

#endif

#if defined(APP_STRINGS_ENCRYPTED)
[[nodiscard]]
QString getKey(
    const generated::app_constants::EncryptedString& value)
{
    return decrypt(value);
}
#else
[[nodiscard]]
QString getKey(const std::string_view value)
{
    return fromPlainString(value);
}
#endif

} // namepsace

QString AppConstants::baseUrl()
{
    return getKey(generated::app_constants::kApiBaseUrl);
}

QString AppConstants::login()
{
    // return getKey(generated::app_constants::kChat);
    return getKey(generated::app_constants::kRegister);
}

QString AppConstants::refreshToken()
{
    return getKey(generated::app_constants::kRefrestToken);
}

QString AppConstants::conversations()
{
    return getKey(generated::app_constants::kUsers);
}

QString AppConstants::messages(const QString &user_id)
{
    QString path = getKey(generated::app_constants::kMessages );
    const QString encoded_user_id = QString::fromUtf8(
        QUrl::toPercentEncoding(user_id.trimmed()));
    return path.replace(QStringLiteral("{userId}"), encoded_user_id);
}

QString AppConstants::group()
{
    return getKey(generated::app_constants::kGroups);
}

QString AppConstants::groupMessage(const QString group_id)
{
    QString path = getKey(generated::app_constants::kGroupsMessage);
    const QString encoded_group_id = QString::fromUtf8 (QUrl::toPercentEncoding (group_id.trimmed ()));
    return path.replace (QStringLiteral ("{groupId}"), encoded_group_id);
}

QString AppConstants::saveMessages()
{
    return getKey (generated::app_constants::kSaveMessages);
}






} // namespace constants
} // cores
