#include "storage/message_cache.hpp"
#include "data/repository/message_history_cache.hpp"
#include "data/dto/message_payload_dto.hpp"
#include "storage/local_storage.hpp"
#include "storage/file_name_helper.hpp"
#include "constants/app_constants.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUuid>
#include <stdexcept>

namespace core {
namespace constants {
QString AppConstants::appSecretKey()
{
    return QStringLiteral("message-cache-test-key");
}
} // namespace constants
} // namespace core

void check(bool condition)
{
    if (!condition) {
        throw std::runtime_error("Message cache check failed");
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setApplicationName(QStringLiteral("ChatDesktopMessageCacheTest-")
        + QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QDir appData(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    const QString directory = appData.filePath(QStringLiteral("user_data"));
    const QString name = QStringLiteral("conversation");
    const QString secret = QStringLiteral("test-secret");
    const QString payload = QString::fromUtf8("[{\"body\":\"សួស្តី 🌏\"}]");
    using core::storage::MessageCache;
    check(!QDir(directory).exists());
    const auto missing = MessageCache::read(name, secret);
    check(missing.has_value() && missing->isEmpty());
    check(MessageCache::clear(name, secret));
    check(!QDir(directory).exists());
    check(MessageCache::save(payload, name, secret));
    check(MessageCache::save(QStringLiteral("[]"), name, secret));
    const auto batches = MessageCache::read(name, secret);
    check(batches.has_value() && *batches == QStringList{payload, QStringLiteral("[]")});
    const QString path = QDir(directory).filePath(core::storage::FileNameHelper::hexName(name, secret));
    MessageCache::closeAll();
    {
        core::storage::LocalStorage storage(path.toStdString(),
            core::constants::AppConstants::appSecretKey().toStdString());
        const auto ids = storage.ids(2);
        check(ids.size() == 2);
        check(storage.get(ids.front()) == payload.toStdString());
        check(storage.get(ids.back()) == "[]");
        check(!MessageCache::save(payload, name, secret));
        check(!MessageCache::read(name, secret).has_value());
        check(!MessageCache::clear(name, secret));
        storage.insert("other data", 1);
        storage.sync();
    }
    const QString otherName = QStringLiteral("other-conversation");
    check(MessageCache::save(payload, otherName, secret));
    check(MessageCache::clear(name, secret));
    check(MessageCache::clear(name, secret));
    const auto cleared = MessageCache::read(name, secret);
    check(cleared.has_value() && cleared->isEmpty());
    const auto other = MessageCache::read(otherName, secret);
    check(other.has_value() && *other == QStringList{payload});
    MessageCache::closeAll();
    {
        core::storage::LocalStorage storage(path.toStdString(),
            core::constants::AppConstants::appSecretKey().toStdString());
        const auto ids = storage.ids(1);
        check(ids.size() == 1 && storage.get(ids.front()) == "other data");
        check(storage.ids(2).empty());
    }
    check(MessageCache::save(payload, name, secret));
    const auto resaved = MessageCache::read(name, secret);
    check(resaved.has_value() && *resaved == QStringList{payload});
    check(core::storage::FileNameHelper::hexNameV2("conversation-1", secret)
          != core::storage::FileNameHelper::hexNameV2("conversation-2", secret));
    check(core::storage::FileNameHelper::hexNameV2(name, "account-1")
          != core::storage::FileNameHelper::hexNameV2(name, "account-2"));
    check(core::storage::FileNameHelper::hexNameV2(QString::fromUtf8("សួស្តី១"), secret)
          != core::storage::FileNameHelper::hexNameV2(QString::fromUtf8("សួស្តី២"), secret));
    check(!MessageCache::readPage(name, secret, 1));
    check(!MessageCache::savePage(payload, name, secret, 0));
    check(MessageCache::savePage(payload, name, secret, 1));
    check(MessageCache::savePage(payload, name, secret, 2));
    for (int index = 0; index < 10; ++index) {
        check(MessageCache::savePage(QStringLiteral("[]"), name, secret, 1));
    }
    check(MessageCache::readPage(name, secret, 1) == QStringLiteral("[]"));
    check(MessageCache::readPage(name, secret, 2) == payload);
    check(!MessageCache::readPage(name, "different-account", 1));
    MessageCache::closeAll();
    {
        core::storage::LocalStorage storage(path.toStdString(),
            core::constants::AppConstants::appSecretKey().toStdString());
        check(storage.root("1").has_value());
        check(storage.root("2").has_value());
    }
    check(MessageCache::readPage(name, secret, 1) == QStringLiteral("[]"));
    check(MessageCache::readPage(name, secret, 2) == payload);
    check(QDir(directory).entryList({QFileInfo(path).fileName() + "*"}, QDir::Files).size() == 1);
    for (int index = 0; index < 9; ++index) {
        check(MessageCache::savePage(payload, QStringLiteral("eviction-%1").arg(index), secret, 1));
    }
    check(MessageCache::readPage(name, secret, 2) == payload);
    check(MessageCache::clear(name, secret));
    MessageCache::closeAll();
    check(!MessageCache::readPage(name, secret, 1));
    check(!MessageCache::readPage(name, secret, 2));
    data::dto::MessageItemDto item{};
    item.id = QStringLiteral("parse-test");
    item.body.content.type = domain::entity::MessageType::Text;
    item.body.content.text = QString::fromUtf8("សួស្តី 🌏");
    data::dto::MessageDto response{{item}, 1, 200, 1, 1};
    const QString responseJson = QString::fromUtf8(QJsonDocument(response.toJson()).toJson(QJsonDocument::Compact));
    check(MessageCache::savePage(responseJson, name, secret, 1));
    MessageCache::closeAll();
    const auto cachedResponse = MessageCache::readPage(name, secret, 1);
    check(cachedResponse.has_value());
    QString parseError;
    const auto document = core::network::JsonSerializer::parse(cachedResponse->toUtf8(), &parseError);
    check(document && document->isObject());
    auto decoded = data::dto::MessageDto::fromJson(document->object(), &parseError);
    check(decoded && decoded->page == 1 && decoded->messages.size() == 1);
    check(decoded->toDomain().messages.first().body.content.text == item.body.content.text);
    MessageCache::closeAll();
    // Repository cache: live messages survive stale fetches and process restarts.
    using data::repository::MessageHistoryCache;
    const QString account = QStringLiteral("account-1@example.test/desktop");
    const QString conversation = QStringLiteral("peer-1@example.test/mobile");
    auto live = item;
    live.id = QStringLiteral("live-id");
    live.body.message_id = QStringLiteral("wire-id");
    const auto filesBeforeConversation = QDir(directory).entryList(QDir::Files).size();
    check(MessageHistoryCache::saveMessage(account, conversation, live));
    check(MessageHistoryCache::saveMessage(account, conversation, live));
    MessageCache::closeAll();
    auto history = MessageHistoryCache::read("account1", "peer1", 1);
    check(history.messages.size() == 1 && history.total == 1 && history.total_pages == 1);
    check(MessageHistoryCache::read("other-account", conversation, 1).messages.isEmpty());
    check(MessageHistoryCache::savePage(account, conversation, response));
    check(QDir(directory).entryList(QDir::Files).size() == filesBeforeConversation + 1);
    history = MessageHistoryCache::read(account, conversation, 1);
    check(history.messages.size() == 2 && history.total == 2);
    // Server IDs can differ from the stable ID carried in the message body.
    live.id = QStringLiteral("server-id");
    data::dto::MessageDto acknowledged{{item, live}, 1, 200, 2, 1};
    check(MessageHistoryCache::savePage(account, conversation, acknowledged));
    check(QDir(directory).entryList(QDir::Files).size() == filesBeforeConversation + 1);
    history = MessageHistoryCache::read(account, conversation, 1);
    check(history.messages.size() == 2 && history.total == 2);
    check(history.messages.last().id == QStringLiteral("server-id"));
    auto olderPage = response;
    olderPage.page = 2;
    check(MessageHistoryCache::savePage(account, conversation, olderPage));
    check(MessageHistoryCache::read(account, conversation, 2).messages.size() == 1);
    check(QDir(directory).entryList(QDir::Files).size() == filesBeforeConversation + 1);
    // An authoritative empty page replaces old messages, including acknowledged live ones.
    check(MessageHistoryCache::savePage(account, conversation, {{}, 1, 200, 0, 0}));
    check(MessageHistoryCache::read(account, conversation, 1).messages.isEmpty());
    check(MessageHistoryCache::read(account, conversation, 2).messages.size() == 1);
    check(!MessageHistoryCache::savePage(account, conversation, {{}, 0, 200, 0, 0}));
    MessageCache::closeAll();
    QFile corrupt(path);
    check(corrupt.open(QIODevice::WriteOnly | QIODevice::Truncate));
    check(corrupt.write("invalid cache") == 13);
    corrupt.close();
    check(!MessageCache::read(name, secret).has_value());
    check(!MessageCache::clear(name, secret));
    MessageCache::closeAll();
    check(QDir(directory).removeRecursively());
    QFile blocker(directory);
    check(blocker.open(QIODevice::WriteOnly));
    blocker.close();
    check(!MessageCache::save(payload, name, secret));
    check(QDir(appData).removeRecursively());
}
