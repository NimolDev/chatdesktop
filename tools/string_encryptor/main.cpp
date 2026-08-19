#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using Byte = std::uint8_t;

constexpr std::size_t kKeySize = 32;
constexpr std::size_t kIvSize = 12;
constexpr std::size_t kTagSize = 16;

using AesKey = std::array<Byte, kKeySize>;
using Iv = std::array<Byte, kIvSize>;
using Tag = std::array<Byte, kTagSize>;

struct EncryptedValue
{
    std::vector<Byte> ciphertext;
    Iv iv{};
    Tag tag{};
};

[[nodiscard]]
std::string trim(const std::string& input)
{
    const auto begin = input.find_first_not_of(" \t\r\n");

    if (begin == std::string::npos) {
        return {};
    }

    const auto end = input.find_last_not_of(" \t\r\n");

    return input.substr(begin, end - begin + 1);
}

[[nodiscard]]
std::map<std::string, std::string> readConfig(
    const std::filesystem::path& configPath)
{
    std::ifstream input(configPath);

    if (!input.is_open()) {
        throw std::runtime_error(
            "Cannot open config file: " + configPath.string());
    }

    std::map<std::string, std::string> values;

    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;

        line = trim(line);

        if (line.empty() || line.starts_with('#')) {
            continue;
        }

        const auto separator = line.find('=');

        if (separator == std::string::npos) {
            throw std::runtime_error(
                "Invalid config line " + std::to_string(lineNumber));
        }

        const std::string key =
            trim(line.substr(0, separator));

        const std::string value =
            trim(line.substr(separator + 1));

        if (key.empty()) {
            throw std::runtime_error(
                "Empty key at line " + std::to_string(lineNumber));
        }

        values[key] = value;
    }

    return values;
}

[[nodiscard]]
std::string toCppName(std::string_view input)
{
    std::string output;
    bool uppercaseNext = true;

    for (const char character : input) {
        if (character == '_' ||
            character == '-' ||
            character == ' ') {
            uppercaseNext = true;
            continue;
        }

        if (uppercaseNext) {
            output.push_back(
                static_cast<char>(
                    std::toupper(
                        static_cast<unsigned char>(character))));
            uppercaseNext = false;
        } else {
            output.push_back(
                static_cast<char>(
                    std::tolower(
                        static_cast<unsigned char>(character))));
        }
    }

    if (output.empty()) {
        throw std::runtime_error("Invalid empty C++ name");
    }

    return output;
}

template<std::size_t Size>
[[nodiscard]]
std::array<Byte, Size> generateRandomBytes()
{
    std::array<Byte, Size> data{};

    if (RAND_bytes(
            data.data(),
            static_cast<int>(data.size())) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }

    return data;
}

[[nodiscard]]
EncryptedValue encryptAes256Gcm(
    const std::string& plaintext,
    const AesKey& key)
{
    EncryptedValue result;

    result.iv = generateRandomBytes<kIvSize>();
    result.ciphertext.resize(plaintext.size());

    EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();

    if (context == nullptr) {
        throw std::runtime_error(
            "EVP_CIPHER_CTX_new failed");
    }

    const auto freeContext = [&context]() {
        EVP_CIPHER_CTX_free(context);
        context = nullptr;
    };

    try {
        if (EVP_EncryptInit_ex(
                context,
                EVP_aes_256_gcm(),
                nullptr,
                nullptr,
                nullptr) != 1) {
            throw std::runtime_error(
                "EVP_EncryptInit_ex failed");
        }

        if (EVP_CIPHER_CTX_ctrl(
                context,
                EVP_CTRL_GCM_SET_IVLEN,
                static_cast<int>(result.iv.size()),
                nullptr) != 1) {
            throw std::runtime_error(
                "Cannot set GCM IV length");
        }

        if (EVP_EncryptInit_ex(
                context,
                nullptr,
                nullptr,
                key.data(),
                result.iv.data()) != 1) {
            throw std::runtime_error(
                "Cannot set AES key and IV");
        }

        int encryptedLength = 0;

        if (!plaintext.empty()) {
            if (EVP_EncryptUpdate(
                    context,
                    result.ciphertext.data(),
                    &encryptedLength,
                    reinterpret_cast<const Byte*>(plaintext.data()),
                    static_cast<int>(plaintext.size())) != 1) {
                throw std::runtime_error(
                    "EVP_EncryptUpdate failed");
            }
        }

        int finalLength = 0;

        if (EVP_EncryptFinal_ex(
                context,
                result.ciphertext.data() + encryptedLength,
                &finalLength) != 1) {
            throw std::runtime_error(
                "EVP_EncryptFinal_ex failed");
        }

        result.ciphertext.resize(
            static_cast<std::size_t>(
                encryptedLength + finalLength));

        if (EVP_CIPHER_CTX_ctrl(
                context,
                EVP_CTRL_GCM_GET_TAG,
                static_cast<int>(result.tag.size()),
                result.tag.data()) != 1) {
            throw std::runtime_error(
                "Cannot get GCM tag");
        }

        freeContext();
        return result;
    } catch (...) {
        freeContext();
        throw;
    }
}

template<typename Container>
void writeBytes(
    std::ostream& output,
    const Container& bytes)
{
    output << "{\n        ";

    for (std::size_t index = 0;
         index < bytes.size();
         ++index) {

        output
            << "0x"
            << std::uppercase
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<unsigned int>(bytes[index])
            << std::dec;

        if (index + 1 < bytes.size()) {
            output << ", ";
        }

        if ((index + 1) % 12 == 0 &&
            index + 1 < bytes.size()) {
            output << "\n        ";
        }
    }

    output << "\n    }";
}

[[nodiscard]]
std::string escapeCppString(std::string_view input)
{
    std::ostringstream output;

    for (const char character : input) {
        switch (character) {
        case '\\':
            output << "\\\\";
            break;

        case '"':
            output << "\\\"";
            break;

        case '\n':
            output << "\\n";
            break;

        case '\r':
            output << "\\r";
            break;

        case '\t':
            output << "\\t";
            break;

        default:
            output << character;
            break;
        }
    }

    return output.str();
}

void generatePlainHeader(
    const std::filesystem::path& outputPath,
    const std::map<std::string, std::string>& values)
{
    std::ofstream output(outputPath);

    if (!output.is_open()) {
        throw std::runtime_error(
            "Cannot create: " + outputPath.string());
    }

    output << R"(#pragma once

#include <string_view>

namespace generated::app_constants {

inline constexpr bool kEncrypted = false;

)";

    for (const auto& [key, value] : values) {
        const std::string cppName = toCppName(key);

        output
            << "inline constexpr std::string_view k"
            << cppName
            << " = \""
            << escapeCppString(value)
            << "\";\n";
    }

    output << R"(
} // namespace generated::app_constants
)";
}

void generateEncryptedHeader(
    const std::filesystem::path& outputPath,
    const std::map<std::string, std::string>& values)
{
    const AesKey realKey =
        generateRandomBytes<kKeySize>();

    const AesKey keyMask =
        generateRandomBytes<kKeySize>();

    AesKey maskedKey{};

    for (std::size_t index = 0;
         index < realKey.size();
         ++index) {
        maskedKey[index] =
            realKey[index] ^ keyMask[index];
    }

    std::ofstream output(outputPath);

    if (!output.is_open()) {
        throw std::runtime_error(
            "Cannot create: " + outputPath.string());
    }

    output << R"(#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace generated::app_constants {

inline constexpr bool kEncrypted = true;

struct EncryptedString
{
    const std::uint8_t* ciphertext;
    std::size_t ciphertextSize;
    const std::uint8_t* iv;
    std::size_t ivSize;
    const std::uint8_t* tag;
    std::size_t tagSize;
};

)";

    output
        << "inline constexpr std::array<std::uint8_t, 32> "
        << "kKeyMask = ";

    writeBytes(output, keyMask);
    output << ";\n\n";

    output
        << "inline constexpr std::array<std::uint8_t, 32> "
        << "kMaskedKey = ";

    writeBytes(output, maskedKey);
    output << ";\n\n";

    for (const auto& [key, value] : values) {
        const std::string cppName = toCppName(key);

        const EncryptedValue encrypted =
            encryptAes256Gcm(value, realKey);

        output
            << "inline constexpr std::array<std::uint8_t, "
            << encrypted.ciphertext.size()
            << "> k"
            << cppName
            << "Ciphertext = ";

        writeBytes(output, encrypted.ciphertext);
        output << ";\n\n";

        output
            << "inline constexpr std::array<std::uint8_t, "
            << encrypted.iv.size()
            << "> k"
            << cppName
            << "Iv = ";

        writeBytes(output, encrypted.iv);
        output << ";\n\n";

        output
            << "inline constexpr std::array<std::uint8_t, "
            << encrypted.tag.size()
            << "> k"
            << cppName
            << "Tag = ";

        writeBytes(output, encrypted.tag);
        output << ";\n\n";

        output
            << "inline constexpr EncryptedString k"
            << cppName
            << "{\n"
            << "    k"
            << cppName
            << "Ciphertext.data(),\n"
            << "    k"
            << cppName
            << "Ciphertext.size(),\n"
            << "    k"
            << cppName
            << "Iv.data(),\n"
            << "    k"
            << cppName
            << "Iv.size(),\n"
            << "    k"
            << cppName
            << "Tag.data(),\n"
            << "    k"
            << cppName
            << "Tag.size()\n"
            << "};\n\n";
    }

    output << R"(} // namespace generated::app_constants
)";
}

} // namespace

int main(int argc, char* argv[])
{
    try {
        if (argc != 4) {
            std::cerr
                << "Usage:\n"
                << "StringEncryptor "
                << "<plain|encrypted> "
                << "<input.conf> "
                << "<output.hpp>\n";

            return 1;
        }

        const std::string mode = argv[1];

        const std::filesystem::path inputPath =
            argv[2];

        const std::filesystem::path outputPath =
            argv[3];

        std::filesystem::create_directories(
            outputPath.parent_path());

        const auto values =
            readConfig(inputPath);

        if (mode == "plain") {
            generatePlainHeader(
                outputPath,
                values);
        } else if (mode == "encrypted") {
            generateEncryptedHeader(
                outputPath,
                values);
        } else {
            throw std::runtime_error(
                "Mode must be plain or encrypted");
        }

        std::cout
            << "Generated: "
            << outputPath
            << '\n';

        return 0;
    } catch (const std::exception& exception) {
        std::cerr
            << "StringEncryptor error: "
            << exception.what()
            << '\n';

        return 1;
    }
}