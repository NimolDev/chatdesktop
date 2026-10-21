#include "conversations_repository_impl.hpp"
#include "constants/app_constants.hpp"
#include "network/json_serializer.hpp"
#include "dto/conversations_dto.hpp"

#include "network/network_monitor.hpp"
#include "storage/cache_storage.hpp"
#include "storage/user_data.hpp"

#include <algorithm>
#include <exception>

#include <QList>
#include <QObject>

namespace data {
namespace repository {

ConversationsRepositoryImpl::ConversationsRepositoryImpl(
    std::shared_ptr<core::network::NetworkClient> network
    )
    : m_network(std::move (network))
{
    auto &network_monitor = core::network::NetworkMonitor::instance();
    QObject::connect(&network_monitor,
                     &core::network::NetworkMonitor::isRetryChanged,
                     this, [this]() {
                         if (core::network::NetworkMonitor::instance().isRetry()) {
                             retryPending();
                         }
                     });
    QObject::connect(&network_monitor,
                     &core::network::NetworkMonitor::networkConnected,
                     this, [this]() { retryPending(); });
}

QFuture<QList<domain::entity::ConversationList>> ConversationsRepositoryImpl::fetchConversations()
{
    auto promise = std::make_shared<QPromise<Conversations>>();
    promise->start();
    auto future = promise->future();
    const auto user = core::storage::UserData::getCurrentUser();
    const QString user_id = user ? user->user_id : QString{};
    if (!user_id.isEmpty()) {
        try {
            const auto cached = core::storage::CacheStorage::instance().read(user_id, user_id);
            const auto document = core::network::JsonSerializer::parse(cached.toUtf8());
            if (document && document->isObject()) {
                auto dto = core::network::JsonSerializer::fromObject<data::dto::ConversationDto>(document->object());
                if (dto) {
                    promise->addResult(dto->toDomain());
                }
            }
        } catch (const std::exception &error) {
            qWarning() << "Could not read cached conversations:" << error.what();
        }
    }
    fetchAttempt(promise, user_id);
    return future;
}

void ConversationsRepositoryImpl::fetchAttempt(const std::shared_ptr<QPromise<Conversations>> &promise,
                                               const QString &user_id)
{
    if (promise->isCanceled()) {
        promise->finish();
        return;
    }

    m_network->get(core::constants::AppConstants::conversations(), 1000)
        .then(this, [this, promise, user_id](const core::network::NetworkResponse &response) {
            if (promise->isCanceled()) {
                promise->finish();
                return;
            }
            if (!response.isSuccess()) {
                qWarning() << "Conversation request failed, status:" << response.status_code;
                if (response.error == core::network::NetworkError::UnknownNetworkError) {
                    // Retry the same operation so its original caller receives the result.
                    m_queue.enqueue([this, promise, user_id]() { fetchAttempt(promise, user_id); });
                    return;
                }
                if (promise->future().resultCount() == 0) {
                    promise->addResult(Conversations{});
                }
                promise->finish();
                return;
            }

            QString error;
            auto document = core::network::JsonSerializer::parse(response.body, &error);
            if (!document || !document->isObject()) {
                qWarning() << "Conversation parse error:" << error;
                if (promise->future().resultCount() == 0) {
                    promise->addResult(Conversations{});
                }
                promise->finish();
                return;
            }
            auto dto = core::network::JsonSerializer::fromObject<data::dto::ConversationDto>(document->object(), &error);
            if (!dto.has_value()) {
                qWarning() << "Conversation mapping error:" << error;
                if (promise->future().resultCount() == 0) {
                    promise->addResult(Conversations{});
                }
                promise->finish();
                return;
            }
            auto conversations = dto->toDomain();
            const auto future = promise->future();
            if (future.resultCount() > 0) {
                const auto cached = future.resultAt(0);
                const bool unchanged = std::equal(
                    cached.cbegin(), cached.cend(),
                    conversations.cbegin(), conversations.cend(),
                    [](const auto &left, const auto &right) {
                        return left.uuid == right.uuid && left.name == right.name;
                    });
                if (unchanged) {
                    promise->finish();
                    return;
                }
            }
            if (!user_id.isEmpty()) {
                try {
                    if (!core::storage::CacheStorage::instance().save(
                            QString::fromUtf8(response.body), user_id, user_id)) {
                        qWarning() << "Could not cache conversations";
                    }
                } catch (const std::exception &error) {
                    qWarning() << "Could not cache conversations:" << error.what();
                }
            }
            promise->addResult(std::move(conversations));
            promise->finish();
        });
}

void ConversationsRepositoryImpl::retryPending()
{
    // Failures queued by these attempts must wait for the next retry signal.
    QQueue<std::function<void()>> pending;
    pending.swap(m_queue);
    while (!pending.isEmpty()) {
        auto task = pending.dequeue();
        task();
    }
}

} // namespace repository
} // namespace data
