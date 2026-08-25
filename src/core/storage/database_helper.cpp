#include "database_helper.hpp"


QSqlQuery core::storage::DatabaseHelper::getCurrentUser()
{
    QSqlDatabase database = QSqlDatabase::database(
        core::storage::DatabaseManager::connectionName(),
        false);

    if (!database.isValid() || !database.isOpen()) {
        qWarning() << "Cannot get local user: local database is not open";
        return QSqlQuery{};
    }

    QSqlQuery query(database);
    query.prepare(QStringLiteral(R"(
        SELECT username, user_id
        FROM users
        WHERE logged = 1
    )"));

    if (!query.exec()) {
        qWarning() <<  "Getting local user" << query.lastError ();

         return QSqlQuery{};;
    }

    if (!query.next()) {
         return QSqlQuery{};;
    }
    return query;
}
