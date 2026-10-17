#include "message_repository_impl.hpp"
#include "constants/app_constants.hpp"
#include "network/json_serializer.hpp"
#include "data/mapping/message_payload_mapper.hpp"
#include "data/dto/message_store.hpp"
#include "media/image_compress.hpp"

#include "message_history_cache.hpp"
#include "logging/logger.hpp"

#include <QFileInfo>
#include <QFutureWatcher>
#include <exception>

#include "storage/user_data.hpp"
namespace data {
namespace repository {

namespace {


bool cacheMessage(const QString &account, const QString &userId,
                  const data::dto::MessageDto &message)
{
    if (!MessageHistoryCache::savePage(account, userId, message)) {
        LOG_WARNING("Could not cache fetched messages");
        return false;
    }
    return true;
}

domain::entity::MessageResponse localMessages(const QString &account, const QString &userId, int page)
{
    return MessageHistoryCache::read(account, userId, page).toDomain();
}

void persistMessage(const QString &account, const QString &conversation,
                    const data::dto::MessageItemDto &message)
{
    if (!MessageHistoryCache::saveMessage(account, conversation, message)) {
        LOG_WARNING("Could not cache message");
    }
}

} // namespace


MessageRepositoryImpl::MessageRepositoryImpl(
    std::shared_ptr<core::xmpp::XmppManager> xmpp,
    std::shared_ptr<core::network::NetworkClient> network,
    QObject *parent)
    : domain::repository::MessageRepository(parent),
    m_xmpp(std::move (xmpp)),
    m_network(std::move (network))
{

    const QString current_userid = m_xmpp.get ()->currentJid ().split ("@").first ();
    m_currentUserId = current_userid;
    connect(
        m_xmpp.get (),
        &core::xmpp::XmppManager::messageReceived,
        this,
        &MessageRepositoryImpl::onMessageReceived
        );
    connect(m_xmpp.get (),
            &core::xmpp::XmppManager::uploadProgressChanged,
            this,
            [this](const QString &id, double progress) {
                // qDebug() << "Upload ID: " << id << "Uploading :" << progress;
                if (progress < 1) {
                    emit fileUploadProgress (id, progress);
                } else {
                    emit fileUploadProgress (id, 1);
                    emit fileUploadFinish (id);
                }
            }
            );
}

QFuture<domain::entity::MessageResponse>
MessageRepositoryImpl::fetchMessageById(QString user_id, int page)
{
    using MessageResponse = domain::entity::MessageResponse;

    auto promise = std::make_shared<QPromise<MessageResponse>>();
    promise->start();

    auto future = promise->future();

    if (page < 1) {
        promise->finish();
        return future;
    }

    const QString account = currentUserId();

    MessageResponse cache{};

    // 1. Load cache only for page 1
    if (page == 1) {
        cache = localMessages(account, user_id, page);

        if (!cache.messages.isEmpty()) {
            qDebug() << "Loaded cached messages";
            promise->addResult(cache);
        }
    }

    // 2. Prepare request
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("limit"), QStringLiteral("200"));
    query.addQueryItem(QStringLiteral("page"), QString::number(page));

    // 3. Request server
    m_network->get(
                 core::constants::AppConstants::messages(user_id),
                 query
                 )
        .then(
            QtFuture::Launch::Async,
            [promise, page, cache, account, user_id]
            (const core::network::NetworkResponse& response) {

                if (!response.isSuccess()) {
                    qWarning() << "Message request failed:"
                               << response.status_code;

                    promise->finish();
                    return;
                }

                QString error;

                const auto document =
                    core::network::JsonSerializer::parse(
                        response.body,
                        &error
                        );

                if (!document || !document->isObject()) {
                    qWarning() << "Message parse error:" << error;

                    promise->finish();
                    return;
                }

                auto dto =
                    core::network::JsonSerializer::fromObject<
                        data::dto::MessageDto
                        >(
                        document->object(),
                        &error
                        );

                if (!dto) {
                    qWarning() << "Message mapping error:" << error;

                    promise->finish();
                    return;
                }

                if (dto->page != page) {
                    qWarning() << "Message response page mismatch";

                    promise->finish();
                    return;
                }

                // 4. Handle empty server result
                if (dto->messages.isEmpty()) {
                    if (cache.messages.isEmpty()) {
                        promise->addResult(MessageResponse{});
                    }

                    promise->finish();
                    return;
                }

                // 5. Convert server response
                MessageResponse server = dto->toDomain();

                if (page == 1) {
                    const bool shouldUpdate =
                        cache.messages.isEmpty() ||
                        server.messages.last().sent_at >
                            cache.messages.last().sent_at;

                    if (shouldUpdate) {
                        qDebug() << " =====> Update message: <======";
                        promise->addResult(server);
                        cacheMessage (account, user_id, dto.value ());
                    }

                }
                else {
                    promise->addResult(server);
                }
                promise->finish();
            }
            );

    return future;
}

QFuture<QString> MessageRepositoryImpl::sendMessage(domain::entity::Payload &payload)
{
    auto dto = data::mapping::toDto (payload);
    dto::MessageItemDto msgDto;
    msgDto.id = dto.message_id;
    msgDto.sender_id = dto.sender_id;
    msgDto.recipient_id = dto.receiver_id;
    msgDto.body = std::move (dto);

    emit messageSent(msgDto.toDomain ());
    return enqueue([this, payload] {
        return sendMessageNow(payload);
    });
}

QFuture<QString> MessageRepositoryImpl::sendMessageNow(domain::entity::Payload payload)
{
    const QString jsonString = data::mapping::toDto(payload).toJsonString();
    return m_xmpp->sendMessage (payload.receiver_id, jsonString,payload.message_id)
       .then(this,
             [this, payload, jsonString](QString error) mutable {
                  if (error.isEmpty ()) {
                      saveMessage (payload.receiver_id, jsonString);
                  }
                  return error;
             });
}

QFuture<QString> MessageRepositoryImpl::sendVoice(
    domain::entity::Payload &payload,
    const QUrl &file)
{
    domain::AttachmentItem attachment;
    attachment.id = payload.message_id;
    attachment.url = file;
    return sendFiles(payload, attachment);
}

QFuture<QString> MessageRepositoryImpl::sendImage(
    domain::entity::Payload &payload,
    const QList<domain::AttachmentItem> &attachments,
    domain::repository::UploadCancellationPtr cancellation)
{
    if (attachments.size() != 1) {
        return QtFuture::makeReadyValueFuture(QStringLiteral("Image files and message content do not match."));
    }
    return sendFiles(payload, attachments.first(), cancellation);
}

QFuture<QString> MessageRepositoryImpl::sendFile(
    domain::entity::Payload &payload,
    const QUrl &file)
{

    return QtFuture::makeReadyValueFuture (QStringLiteral (""));
}

QFuture<QString> MessageRepositoryImpl::enqueue(
    std::function<QFuture<QString>()> start,
    domain::repository::UploadCancellationPtr cancellation,
    std::shared_ptr<FileUploadTask> task)
{
    auto promise = std::make_shared<QPromise<QString>>();
    promise->start();
    auto future = promise->future();
    if (cancellation) {
        cancellation->track(future);
    }
    m_queue.enqueue({std::move(start), cancellation, task, promise});
    nextProcess();
    return future;
}

void MessageRepositoryImpl::nextProcess()
{
    if (m_processing || m_queue.isEmpty()) {
        return;
    }
    // Skip canceled entries without starting a network upload.
    while (!m_queue.isEmpty() && m_queue.head().cancellation && m_queue.head().cancellation->isCanceled()) {
        const auto upload = m_queue.dequeue();
        if (upload.task) {
            m_uploadTasks.remove(upload.task->id);
        }
        upload.promise->finish();
    }
    if (m_queue.isEmpty()) {
        return;
    }

    m_processing = true;
    auto upload = m_queue.dequeue();
    QFuture<QString> sending;
    try {
        sending = upload.start();
    } catch (...) {
        upload.promise->setException(std::current_exception());
        upload.promise->finish();
        if (upload.task) {
            m_uploadTasks.remove(upload.task->id);
        }
        m_processing = false;
        QMetaObject::invokeMethod(this, [this] { nextProcess(); }, Qt::QueuedConnection);
        return;
    }

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this,
            [this, watcher, upload] {
        const auto future = watcher->future();
        if (future.isCanceled()) {
            upload.promise->future().cancel();
        } else {
            try {
                if (future.resultCount() > 0) {
                    upload.promise->addResult(future.result());
                }
            } catch (...) {
                upload.promise->setException(std::current_exception());
            }
        }
        upload.promise->finish();
        if (upload.task) {
            m_uploadTasks.remove(upload.task->id);
        }
        watcher->deleteLater();
        m_processing = false;
        nextProcess();
    });
    watcher->setFuture(sending);

}

QFuture<QString> MessageRepositoryImpl::sendFiles(
    domain::entity::Payload &payload,
    const domain::AttachmentItem &attachment,
    domain::repository::UploadCancellationPtr cancellation)
{
    using domain::repository::UploadCancellation;

    if (cancellation && cancellation->isCanceled()) {
        return UploadCancellation::canceledFuture<QString>();
    }

    if (m_uploadTasks.contains(attachment.id)) {
        return QtFuture::makeReadyValueFuture(QStringLiteral("File upload is already queued or in progress."));
    }

    const QFileInfo info(attachment.url.toLocalFile());
    if (!attachment.url.isLocalFile() || !info.isFile() || !info.isReadable() || info.size() <= 0) {
        return QtFuture::makeReadyValueFuture(QStringLiteral("File is missing, empty, or unreadable."));
    }

    auto message = payload;
    message.content.file_name = info.fileName();
    message.content.file_size = info.size();
    dto::MessageItemDto pending;
    pending.id = message.message_id;
    pending.sender_id = message.sender_id;
    pending.recipient_id = message.receiver_id;
    pending.body = data::mapping::toDto(message);
    emit messageSent(pending.toDomain());

    auto task = std::make_shared<FileUploadTask>();
    task->id = attachment.id;
    task->file_path = attachment.url.toString();
    task->cancellation = cancellation ? cancellation : std::make_shared<UploadCancellation>();
    m_uploadTasks.insert(task->id, task);
    emit fileUploadStated(task->id, task->file_path);
    return enqueue([this, message, task]() mutable {
        auto prepared = QtFuture::makeReadyValueFuture(task->file_path);
        if (message.content.type == domain::entity::MessageType::Image) {
            prepared = task->cancellation->track(
                core::media::ImageCompress::compressImageToCache(
                    task->file_path, message.message_id + QStringLiteral("-0")))
                .then([](const QString &path) {
                    return path.isEmpty() ? QString{} : QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded);
                });
        }
        return prepared.then(this, [this, message, task](const QString &path) mutable {
            if (task->cancellation->isCanceled()) {
                return UploadCancellation::canceledFuture<QString>();
            }
            if (path.isEmpty()) {
                return QtFuture::makeReadyValueFuture(
                    QStringLiteral("Could not compress image. No message was sent."));
            }
            task->file_path = path;
            message.content.file_size = QFileInfo(QUrl(path).toLocalFile()).size();
            task->future = task->cancellation->track(m_xmpp->fileUpload(task->id, task->file_path));


            return task->future.then(this, [this, message, task](const core::xmpp::UploadedFile &upload) mutable {
                if (!upload.error.isEmpty()) {
                    return QtFuture::makeReadyValueFuture(upload.error);
                }
                if (!upload.url.isValid() || upload.url.isEmpty()) {
                    return QtFuture::makeReadyValueFuture(QStringLiteral("File upload returned an invalid URL."));
                }
                if (!task->cancellation->tryCommit()) {
                    return UploadCancellation::canceledFuture<QString>();
                }
                message.content.media_url = upload.url.toString(QUrl::FullyEncoded);
                return sendMessageNow(message);
            }).unwrap();
        }).unwrap();
    }, task->cancellation, task);
}

QFuture<bool> MessageRepositoryImpl::cancelFileUpload(const QString &upload_id)
{
    const auto it = m_uploadTasks.find (upload_id);
    if (it == m_uploadTasks.end ()) {
        return QtFuture::makeReadyValueFuture (false);
    }
    const auto &task = it.value ();
    if (!task || !task->cancellation) {
        return QtFuture::makeReadyValueFuture (false);
    }
    const bool result = task->cancellation->cancel ();
    if (result) {
        emit fileUploadCanceled (upload_id);
    }
    return QtFuture::makeReadyValueFuture (result);

}


QString MessageRepositoryImpl::currentUserId() const
{
    const QString id = m_xmpp->currentJid().split(QLatin1Char('@')).first();
    if (!id.isEmpty()) {
        return id;
    }
    // The saved account is available before an offline XMPP connection succeeds.
    const auto user = core::storage::UserData::getCurrentUser();
    return user ? user->user_id : QString{};
}

void MessageRepositoryImpl::saveMessage(QString receiver_id, QString body)
{
    const auto payload = dto::PayloadDto::fromJsonString(body);
    if (payload) {
        dto::MessageItemDto message{};
        message.id = payload->message_id;
        message.sender_id = payload->sender_id;
        message.recipient_id = payload->receiver_id;
        message.body = *payload;
        persistMessage(currentUserId(), receiver_id, message);
    }
    dto::MesssageStore message;

    message.recipient_id = receiver_id;
    message.body = body;

    QJsonObject request = message.toJson ();
    m_network->post (core::constants::AppConstants::saveMessages (), request)
        .then ([](const core::network::NetworkResponse &response) {
            if (!response.isSuccess ()) {
                qWarning() << "Save message request failed, status:" << response.status_code;
                return;
            }
            QString error;
            auto document = core::network::JsonSerializer::parse (response.body, &error);

            if (!document.has_value ()) {
                qWarning() << "Save message response parse error:"<<error;
                return;
            }
            return;
        });
}

void MessageRepositoryImpl::onMessageReceived(const core::xmpp::Message &message)
{
    // qDebug() << "Message receive:"<<message.from;

    const auto parsed = data::dto::PayloadDto::fromJsonString(message.body);
    if (!parsed) {
        qWarning() << "Ignoring invalid message payload";
        return;
    }
    data::dto::PayloadDto dto = *parsed;
    dto::MessageItemDto msgDto;
    msgDto.id = dto.message_id;
    // msgDto.sent_at =
    msgDto.sender_id = dto.sender_id;
    msgDto.recipient_id = dto.receiver_id;
    msgDto.body = std::move (dto);


    domain::entity::MessageItem payload;
    payload = msgDto.toDomain ();
    const QString account = currentUserId();
    const QString conversation = MessageHistoryCache::conversationId(payload.sender_id) == MessageHistoryCache::conversationId(account)
        ? payload.recipient_id : payload.sender_id;
    persistMessage(account, conversation, msgDto);
    emit messageReceived (payload);

}



} // namespace repository
} // namespace data
