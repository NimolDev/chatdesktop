#include "token_validator.hpp"

namespace core {
namespace utils {

bool TokenValidator::isTokenExpired(QString &token)
{
    const QStringList parts = token.split (".");

    if (parts.size () != 3) {
        qWarning() << "Token invalid formate";
        return true;
    }

    const QByteArray payload = QByteArray::fromBase64 (
        parts[1].toUtf8 (),
        QByteArray::Base64UrlEncoding
        );

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson (payload, &error);

    if (error.error != QJsonParseError::NoError
        || !document.isObject ()) {
        return true;
    }
    const QJsonValue exp_value  = document.object ().value (QStringLiteral ("exp"));
    if (!exp_value.isDouble ()) {
        return true; // No valide expiration time
    }

    const qint64 expiration_time = static_cast<quint64>(exp_value.toDouble ());
    const quint64 current_time = QDateTime::currentSecsSinceEpoch ();

    qDebug() << "expiration_time:"<< expiration_time << "current time:"<<current_time;
    return current_time >= expiration_time - 30;

}

} // namespace utils
} // namespace core