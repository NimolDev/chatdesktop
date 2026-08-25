#include "login_response_store.hpp"

#include "storage/database_manager.hpp"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

namespace data {
namespace local {


namespace {

bool logQueryError(const char *operation, const QSqlQuery &query)
{
    qWarning() << operation << "failed:" << query.lastError().text();
    return false;
}

} // namespace

bool LoginResponseStore::save(
    const dto::LoginResponseDTO &response,
    const QString password)
{
    QSqlDatabase database = QSqlDatabase::database(
        core::storage::DatabaseManager::connectionName(),
        false);

    if (!database.isValid() || !database.isOpen()) {
        qWarning() << "Cannot save login response: local database is not open";
        return false;
    }

    QSqlQuery createTable(database);
    if (!createTable.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS users (
            singleton_id INTEGER PRIMARY KEY CHECK (singleton_id = 1),
            access_token TEXT NOT NULL,
            refresh_token TEXT NOT NULL,
            user_id TEXT NOT NULL,
            username TEXT NOT NULL,
            password TEXT NOT NULL,
            display_name TEXT NOT NULL,
            xmpp_jid TEXT NOT NULL,
            xmpp_host TEXT NOT NULL,
            xmpp_port INTEGER NOT NULL
                CHECK (xmpp_port >= 0 AND xmpp_port <= 65535),
            logged INTEGER NOT NULL
        )
    )"))) {
        return logQueryError("Creating login_response table", createTable);
    }

    if (!database.transaction()) {
        qWarning() << "Starting login response transaction failed:"
                   << database.lastError().text();
        return false;
    }

    QSqlQuery query(database);
    query.prepare(QStringLiteral(R"(
        INSERT INTO users (
            singleton_id, access_token, refresh_token, user_id, username, password,
            display_name, xmpp_jid, xmpp_host, xmpp_port, logged
        ) VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(singleton_id) DO UPDATE SET
            access_token = excluded.access_token,
            refresh_token = excluded.refresh_token,
            user_id = excluded.user_id,
            username = excluded.username,
            password = excluded.password,
            display_name = excluded.display_name,
            xmpp_jid = excluded.xmpp_jid,
            xmpp_host = excluded.xmpp_host,
            xmpp_port = excluded.xmpp_port,
            logged = excluded.logged
    )"));
    query.addBindValue(response.access_token);
    query.addBindValue(response.refresh_token);
    query.addBindValue(response.user.id);
    query.addBindValue(response.user.username);
    query.addBindValue (password);
    query.addBindValue(response.user.display_name);
    query.addBindValue(response.xmpp.jid);
    query.addBindValue(response.xmpp.host);
    query.addBindValue(response.xmpp.port);
    query.addBindValue (1);

    if (!query.exec()) {
        logQueryError("Saving login response", query);
        database.rollback();
        return false;
    }

    if (!database.commit()) {
        qWarning() << "Committing login response failed:"
                   << database.lastError().text();
        database.rollback();
        return false;
    }
    return true;
}

std::optional<dto::LocalUser> LoginResponseStore::getLocalUser()
{
    QSqlDatabase database = QSqlDatabase::database(
        core::storage::DatabaseManager::connectionName(),
        false);

    if (!database.isValid() || !database.isOpen()) {
        qWarning() << "Cannot get local user: local database is not open";
        return std::nullopt;
    }

    QSqlQuery query(database);
    query.prepare(QStringLiteral(R"(
        SELECT access_token, refresh_token, user_id, username, password,
               display_name, xmpp_jid, xmpp_host, xmpp_port
        FROM users
        WHERE logged = 1
    )"));

    if (!query.exec()) {
        logQueryError("Getting local user", query);
        return std::nullopt;
    }

    if (!query.next()) {
        return std::nullopt;
    }

    return dto::LocalUser {
        .access_token = query.value(0).toString(),
        .refresh_token = query.value(1).toString(),
        .user_id = query.value(2).toString(),
        .username = query.value(3).toString(),
        .password = query.value(4).toString(),
        .display_name = query.value(5).toString(),
        .xmpp_jid = query.value(6).toString(),
        .xmpp_host = query.value(7).toString(),
        .xmpp_port = static_cast<quint16>(query.value(8).toUInt())
    };
}

bool LoginResponseStore::logout()
{
    QSqlDatabase database = QSqlDatabase::database(
        core::storage::DatabaseManager::connectionName(),
        false);

    if (!database.isValid() || !database.isOpen()) {
        qWarning() << "Cannot get local user: local database is not open";
        return false;
    }

    QSqlQuery query(database);
    query.prepare (QStringLiteral (R"(
        UPDATE users
        SET logged = 0
        WHERE logged = 1
     )"));
    if (!query.exec()) {
        logQueryError("Getting local user", query);
        return false;
    }

    return true;
}

bool LoginResponseStore::updateToken(dto::TokenResponseDto &token)
{
    QSqlDatabase database = QSqlDatabase::database(
        core::storage::DatabaseManager::connectionName(),
        false);

    if (!database.isValid() || !database.isOpen()) {
        qWarning() << "Cannot get local user: local database is not open";
        return false;
    }

    QSqlQuery query(database);
    query.prepare (QStringLiteral (R"(
        UPDATE users
        SET access_token = ?,
            refresh_token = ?
        WHERE logged = 1
     )"));
    query.addBindValue (token.access_token);
    query.addBindValue (token.refresh_token);

    if (!query.exec()) {
        logQueryError("Update user error:", query);
        return false;
    }

    return true;
}

} // namespace local
} // namespace data
