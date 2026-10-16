#pragma once

#include <QString>

namespace core {
namespace storage {

/*
 * File Name Helper
 *
 * Turn a logical name into a hex file name with no extension, e.g. "users" -> "3fa9324kdfjaoe090".
 * Deterministic: the same (logical_name, secret) alway gives the same name, so you can find the file again.
 * Without storing the name anywhere. `secret` is optional; with one, the name can't be quessed form "users".
 * Don't use the store password as the secret (a wrond password would point at a new, empty file).
 *
 */

class FileNameHelper
{
public:
    explicit FileNameHelper();

    static QString hexName(const QString &logical_name, const QString &secret = "", size_t hex_chars = 32);
    // UTF-8 byte lengths; legacy hexName is retained for existing auth files.
    static QString hexNameV2(const QString &logical_name, const QString &secret = "", size_t hex_chars = 32);
    static QString randomHexName(size_t hext_chars = 32);
    static void randBytes(quint8 *p, size_t n);


};


} // namespace storage
} // namespace core
