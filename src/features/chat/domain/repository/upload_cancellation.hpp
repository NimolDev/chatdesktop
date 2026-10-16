#ifndef FEATURES_CHAT_DOMAIN_REPOSITORY_UPLOAD_CANCELLATION_HPP
#define FEATURES_CHAT_DOMAIN_REPOSITORY_UPLOAD_CANCELLATION_HPP

#include <QFuture>
#include <QMutex>
#include <QMutexLocker>
#include <QPromise>
#include <functional>
#include <memory>
#include <vector>

namespace domain {
namespace repository {

// One attachment send can prepare images and start several XMPP transfers.
// Keep cancellation attached to the original futures across then()/unwrap().
class UploadCancellation
{
public:
    template<typename T>
    QFuture<T> track(QFuture<T> future)
    {
        bool canceled;
        {
            QMutexLocker lock(&m_mutex);
            canceled = m_canceled;
            if (!canceled && !m_committed) {
                m_cancelers.emplace_back([future]() mutable {
                    if (!future.isFinished()) {
                        future.cancel();
                    }
                });
            }
        }
        if (canceled) {
            future.cancel();
        }
        return future;
    }

    bool cancel()
    {
        std::vector<std::function<void()>> cancelers;
        {
            QMutexLocker lock(&m_mutex);
            if (m_committed || m_canceled) {
                return false;
            }
            m_canceled = true;
            cancelers.swap(m_cancelers);
        }
        for (const auto &cancel : cancelers) {
            cancel();
        }
        return true;
    }

    bool isCanceled() const
    {
        QMutexLocker lock(&m_mutex);
        return m_canceled;
    }

    // After handing the message to XMPP, cancellation cannot recall it.
    bool tryCommit()
    {
        QMutexLocker lock(&m_mutex);
        if (m_canceled) {
            return false;
        }
        m_committed = true;
        m_cancelers.clear();
        return true;
    }

    template<typename T>
    static QFuture<T> canceledFuture()
    {
        QPromise<T> promise;
        promise.start();
        auto future = promise.future();
        future.cancel();
        promise.finish();
        return future;
    }

private:
    mutable QMutex m_mutex;
    bool m_canceled = false;
    bool m_committed = false;
    std::vector<std::function<void()>> m_cancelers;
};

using UploadCancellationPtr = std::shared_ptr<UploadCancellation>;

} // namespace repository
} // namespace domain

#endif // FEATURES_CHAT_DOMAIN_REPOSITORY_UPLOAD_CANCELLATION_HPP
