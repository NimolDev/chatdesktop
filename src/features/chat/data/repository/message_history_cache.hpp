#pragma once

#include "data/dto/message_payload_dto.hpp"
#include "storage/message_cache.hpp"

#include <algorithm>
#include <mutex>

namespace data {
namespace repository {

// Keep live messages in a separate root in the same file until acknowledged. A request
// started before a message arrived must not erase that message from the cache.
class MessageHistoryCache
{
public:
    static QString conversationId(const QString &id)
    {
        return id.section(QLatin1Char('/'), 0, 0)
            .section(QLatin1Char('@'), 0, 0).remove(QLatin1Char('-'));
    }

    static dto::MessageDto read(const QString &account, const QString &conversation, int page)
    {
        std::lock_guard lock(mutex);
        auto result = readSnapshot(account, conversation, page, false);
        if (page == 1) {
            const auto live = readSnapshot(account, conversation, 1, true);
            for (const auto &message : live.messages) {
                const auto existing = std::find_if(result.messages.begin(), result.messages.end(),
                    [&message](const auto &item) { return sameMessage(item, message); });
                if (existing == result.messages.end()) {
                    result.messages.append(message);
                    ++result.total;
                }
            }
            if (!result.messages.isEmpty()) {
                result.total_pages = std::max(1, result.total_pages);
            }
        }
        return result;
    }

    static bool savePage(const QString &account, const QString &conversation,
                         const dto::MessageDto &response)
    {
        std::lock_guard lock(mutex);
        if (!writeSnapshot(account, conversation, response, false)) {
            return false;
        }
        auto live = readSnapshot(account, conversation, 1, true);
        const auto previousSize = live.messages.size();
        live.messages.removeIf([&response](const auto &message) {
            return std::any_of(response.messages.begin(), response.messages.end(),
                [&message](const auto &item) { return sameMessage(item, message); });
        });
        return live.messages.size() == previousSize || writeSnapshot(account, conversation, live, true);
    }

    static bool saveMessage(const QString &account, const QString &conversation,
                            const dto::MessageItemDto &message)
    {
        std::lock_guard lock(mutex);
        auto live = readSnapshot(account, conversation, 1, true);
        const auto existing = std::find_if(live.messages.begin(), live.messages.end(),
            [&message](const auto &item) { return sameMessage(item, message); });
        if (existing == live.messages.end()) {
            live.messages.append(message);
        } else {
            *existing = message;
        }
        return writeSnapshot(account, conversation, live, true);
    }

private:
    inline static std::mutex mutex;

    static bool sameMessage(const dto::MessageItemDto &left, const dto::MessageItemDto &right)
    {
        return (!left.body.message_id.isEmpty() && left.body.message_id == right.body.message_id)
            || (!left.id.isEmpty() && left.id == right.id);
    }

    static QString secret(const QString &account, const QString &conversation)
    {
        return QString::fromLatin1(QJsonDocument(QJsonArray{
            conversationId(account), conversationId(conversation)})
            .toJson(QJsonDocument::Compact).toBase64());
    }

    static dto::MessageDto readSnapshot(const QString &account, const QString &conversation,
                                        int page, bool live)
    {
        const auto json = live
            ? core::storage::MessageCache::readPending(conversationId(conversation), secret(account, conversation))
            : core::storage::MessageCache::readPage(conversationId(conversation), secret(account, conversation), page);
        if (json) {
            const auto document = QJsonDocument::fromJson(json->toUtf8());
            if (document.isObject()) {
                auto decoded = dto::MessageDto::fromJson(document.object());
                if (decoded && decoded->page == page) {
                    return *decoded;
                }
            }
        }
        return {{}, page, 200, 0, 0};
    }

    static bool writeSnapshot(const QString &account, const QString &conversation,
                              const dto::MessageDto &response, bool live)
    {
        if (conversationId(account).isEmpty() || conversationId(conversation).isEmpty()) {
            return false;
        }
        const auto json = QString::fromUtf8(QJsonDocument(response.toJson()).toJson(QJsonDocument::Compact));
        return live
            ? core::storage::MessageCache::savePending(json, conversationId(conversation), secret(account, conversation))
            : core::storage::MessageCache::savePage(json, conversationId(conversation), secret(account, conversation), response.page);
    }
};

} // namespace repository
} // namespace data
