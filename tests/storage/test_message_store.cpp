#include "storage/message_store.hpp"
#include "storage/database_manager.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>

using core::storage::DatabaseManager;
using core::storage::MessageStore;
using core::storage::StoredMessage;

#define CHECK(condition) do { if (!(condition)) { qCritical() << "Failed:" << #condition << "at" << __LINE__; return 1; } } while (false)

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    CHECK(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("messages.sqlite"));
    {
        auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), DatabaseManager::connectionName());
        db.setDatabaseName(path);
        CHECK(db.open());
    }
    CHECK(MessageStore::initialize());
    CHECK(MessageStore::saveMediaPath("me", "https://example.com/my%20file.pdf", "/Downloads/my file.pdf"));
    CHECK(MessageStore::saveMediaPath("me", "https://example.com/my file.pdf"));
    CHECK(MessageStore::localMediaPath("me", "https://example.com/my%20file.pdf") == "/Downloads/my file.pdf");
    CHECK(MessageStore::localMediaPath("other-account", "https://example.com/my%20file.pdf").isEmpty());
    QList<StoredMessage> records;
    for (int i = 1; i <= 55; ++i) {
        records.append({QString::number(i), "me", "peer", QStringLiteral("{\"text\":\"hello ' 世界\"}"), "", i});
    }
    CHECK(MessageStore::save("me", "peer", records));
    CHECK(MessageStore::save("other-account", "peer", {records.first()}));
    CHECK(MessageStore::save("me", "other-peer", {records.first()}));
    auto page = MessageStore::load("me", "peer", 1);
    CHECK(page.total == 55 && page.messages.size() == 50);
    CHECK(page.messages.first().id == "55");
    CHECK(MessageStore::load("me", "peer", 2).messages.size() == 5);
    CHECK(MessageStore::load("unknown", "peer", 1).total == 0);
    CHECK(MessageStore::load("other-account", "peer", 1).total == 1);
    records.last().body = QStringLiteral("{\"content\":[{\"type\":\"file\",\"media_url\":\"https://example.com/a.pdf\"}]}");
    CHECK(MessageStore::save("me", "peer", {records.last()}));
    CHECK(MessageStore::load("me", "peer", 1).total == 55);
    CHECK(MessageStore::load("me", "peer", 1).messages.first().body == records.last().body);
    auto invalid = records.first();
    invalid.id.clear();
    auto rolledBack = records.last();
    rolledBack.body = "must roll back";
    CHECK(!MessageStore::save("me", "peer", {rolledBack, invalid}));
    CHECK(MessageStore::load("me", "peer", 1).messages.first().body == records.last().body);
    DatabaseManager::close();
    {
        auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), DatabaseManager::connectionName());
        db.setDatabaseName(path);
        CHECK(db.open());
    }
    CHECK(MessageStore::initialize());
    CHECK(MessageStore::load("me", "peer", 1).messages.first().body == records.last().body);
    CHECK(MessageStore::load("me", "peer", 1).total == 55);
    CHECK(MessageStore::localMediaPath("me", "https://example.com/my%20file.pdf") == "/Downloads/my file.pdf");
    CHECK(MessageStore::saveMediaPath("me", "https://example.com/my%20file.pdf", "/Downloads/my file (1).pdf"));
    CHECK(MessageStore::localMediaPath("me", "https://example.com/my file.pdf") == "/Downloads/my file (1).pdf");
    DatabaseManager::close();
    qInfo() << "Passed: persistence, pagination, account isolation, upsert, and transaction rollback";
    return 0;
}
