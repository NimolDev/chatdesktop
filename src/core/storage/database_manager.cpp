#include "database_manager.hpp"


namespace core {
namespace storage {


namespace {
constexpr auto kConnectionName = "local_storage";
constexpr auto kDatabaseFileName = "local.sqlite";

QString databaseDirectory()
{
    return QDir(QStandardPaths::writableLocation (QStandardPaths::AppDataLocation)).filePath ("data");
}

QString databasePath()
{
    return QDir(databaseDirectory ()).filePath (kDatabaseFileName);
}

bool executeQuery(QSqlQuery &query, const QString &sql) {
    if (!query.exec (sql)) {
        qCritical()
            << "Datebase query failed:"
            << query.lastError ().text ()
            << "\nSQL:"
            << sql;
        return false;
    }
    return true;
}


}// namspace


QString DatabaseManager::connectionName()
{
    return QString::fromLatin1 (kConnectionName);
}



bool DatabaseManager::initialize()
{
    if (!createDirectory ()) {
        return false;
    }

    QSqlDatabase database;

    if (QSqlDatabase::contains (connectionName ())) {
        database = QSqlDatabase::database (connectionName ());
    } else {
        database = QSqlDatabase::addDatabase (
            QStringLiteral ("QSQLITE"),
            connectionName ()
            );

        database.setDatabaseName (databasePath ());
    }

    if (!database.open ()) {
        qCritical()
            << "Failed to open database:"
            << database.lastError ().text ();
        return false;
    }

    QSqlQuery pragmaQuery(database);

    if (!executeQuery (pragmaQuery, QStringLiteral ("PRAGMA foreign_keys = ON;"))) {
        return false;
    }
    if (!executeQuery (pragmaQuery, QStringLiteral ("PRAGMA journal_mode = WAL;"))) {
        return false;
    }

    // return createTables();
    return true;
}

void DatabaseManager::close()
{
    const QString name = connectionName ();

    if (!QSqlDatabase::contains (name)) {
        return;
    }
    {
        QSqlDatabase database = QSqlDatabase::database (name);
        database.close ();
    }
    QSqlDatabase::removeDatabase (name);
}

bool DatabaseManager::createDirectory()
{
    QDir directory;

    if (directory.mkpath (databaseDirectory ())) {
        return true;
    }
    qCritical()
        << "Failed to create database directory:"
        << databaseDirectory ();
    return false;
}

bool DatabaseManager::createTables()
{
    const QSqlDatabase database = QSqlDatabase::database (connectionName ());

    if (!database.isOpen ()) {
        qCritical() << "Database is not open";
        return false;
    }

    QSqlQuery query(database);

    const QString createUserTable = QStringLiteral (R"(
        CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL UNIQUE,
        password TEXT NOT NULL,
        jid TEXT NOT NULL,
        logged_in INTEGER NOT NULL DEFAULT 0
        );
    )");

    if (!executeQuery (query, createUserTable)) {
        return false;
    }
    return true;
}


} // namespace storage
} // namespace core
