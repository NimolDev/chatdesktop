#ifndef CORE_UTILS_TOKEN_VALIDATOR_HPP
#define CORE_UTILS_TOKEN_VALIDATOR_HPP

#include <QByteArray>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

namespace core {
namespace utils {

class TokenValidator
{
public:
    static bool isTokenExpired(QString &token);
};


} // namespace utils
} // namespace core

#endif // CORE_UTILS_TOKEN_VALIDATOR_HPP
