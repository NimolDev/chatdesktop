#include "message_repository_impl.hpp"
#include "constants/app_constants.hpp"
#include "network/json_serializer.hpp"
#include "data/mapping/message_payload_mapper.hpp"
#include "data/dto/message_store.hpp"
#include "media/image_compress.hpp"

#include "storage/message_cache.hpp"
#include "logging/logger.hpp"

#include <QFileInfo>
#include <QFutureWatcher>

#include "storage/user_data.hpp"
namespace data {
namespace repository {

namespace {


QString messageCacheSecret(const QString &account, const QString &userId)
{
    const QString secret = QString::fromUtf8(QJsonDocument(QJsonArray{account, userId})
                                                .toJson(QJsonDocument::Compact));
    return QString::fromLatin1(secret.toUtf8().toBase64());
}

void cacheMessage(const QString &account, const QString &user_id,
                  const data::dto::MessageDto &message)
{
    if (message.messages.empty ()) {
        return;
    }
    const QString json = QString::fromUtf8(QJsonDocument(message.toJson()).toJson(QJsonDocument::Compact));
    if (!core::storage::MessageCache::savePage(json, user_id,
                                          messageCacheSecret(account, user_id), message.page)) {
        LOG_WARNING("Could not cache fetched messages");
    }
}

domain::entity::MessageResponse localMessages(const QString &account, const QString &user_id, int page)
{
    domain::entity::MessageResponse result{};
    result.page = page;
    const auto cache = core::storage::MessageCache::readPage(
        user_id, messageCacheSecret(account, user_id), page);
    if (!cache) {
        return result;
    }
    QString error;
    const auto document = core::network::JsonSerializer::parse(cache->toUtf8(), &error);
    if (!document) {
        LOG_WARNING(QStringLiteral("Invalid message cache JSON for page %1: %2").arg(page).arg(error));
        return result;
    }
    if (!document->isObject()) {
        LOG_WARNING("Invalid message cache response: expected a MessageDto JSON object");
        return result;
    }
    auto decoded = data::dto::MessageDto::fromJson(document->object(), &error);
    if (!decoded) {
        LOG_WARNING(QStringLiteral("Could not decode cached response: %1").arg(error));
        return result;
    }
    if (decoded->page != page) {
        LOG_WARNING(QStringLiteral("Cached response page mismatch: requested %1, stored %2")
                        .arg(page).arg(decoded->page));
        return result;
    }
    return decoded->toDomain();
}

QString storageId(QString id)
{
    return id.split(QLatin1Char('/')).first().split(QLatin1Char('@')).first().remove(QLatin1Char('-'));
}

void persistMessages(const QString &account, const QString &conversation,
                     const QList<domain::entity::MessageItem> &messages)
{
    // QList<core::storage::StoredMessage> records;
    // for (const auto &message : messages) {
    //     {
    //         const auto &content = message.body.content;
    //         if (!content.media_url.isEmpty()) {
    //             core::storage::MessageStore::saveMediaPath(storageId(account), content.media_url, content.localpath);
    //         }
    //     }
    //     records.append({message.body.message_id.isEmpty() ? message.id : message.body.message_id,
    //                     message.sender_id, message.recipient_id,
    //                     data::mapping::toDto(message.body).toJsonString(), message.sent_at,
    //                     message.body.timestamp.toLongLong()});
    // }
    // if (!core::storage::MessageStore::save(storageId(account), storageId(conversation), records)) {
    //     qWarning() << "Could not persist messages locally";
    // }
}

void restoreMediaPaths(const QString &account, domain::entity::MessageItem &message)
{
    {
        auto &content = message.body.content;
        // const auto path = core::storage::MessageStore::localMediaPath(storageId(account), content.media_url);
        // content.localpath = QFileInfo(path).isFile() ? path : QString();
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

QFuture<domain::entity::MessageResponse> MessageRepositoryImpl::fetchMessageById(QString user_id, int page)
{
    QUrlQuery query;
    query.addQueryItem (QStringLiteral ("limit"), "200");
    query.addQueryItem (QStringLiteral ("page"), QString::number (page));

    const QString account = currentUserId();
    auto cache = localMessages (account, user_id, 0);
    qDebug() << "Cache: " << cache.messages.count ();
    return m_network ->get (core::constants::AppConstants::messages (user_id), query)
        .then(QtFuture::Launch::Async,
              [account, user_id, page](const core::network::NetworkResponse &response)
                  -> domain::entity::MessageResponse {
            if (!response.isSuccess ()) {
                qWarning() << "Message request failed, status:"<<response.status_code;
                return localMessages(account, user_id, page);
            }

            QString error;
            auto document = core::network::JsonSerializer::parse (response.body, &error);
            if (!document.has_value ()) {
                qWarning() << "Message pares error: "<<error;
                 return localMessages(account, user_id, page);
            }

            auto d = core::network::JsonSerializer::fromObject<data::dto::MessageDto> (document->object (), &error);
            if (!d.has_value ()) {
                qWarning() << "Message mapping error: "<< error;
                  return localMessages(account, user_id, page);
            }

            auto domain = d->toDomain ();


            // persistMessages(account, user_id, domain.messages);
            // for (auto &message : domain.messages) {
            //     restoreMediaPaths(account, message);
            // }

            cacheMessage(account, user_id, d.value());
            return domain;
        });
}

 QFuture<QString> MessageRepositoryImpl::sendMessage(domain::entity::Payload &payload)
{
    const QString current_userid = currentUserId ();
    auto dto = data::mapping::toDto (payload);
    QString jsonString = dto.toJsonString ();
    dto::MessageItemDto msgDto;
    msgDto.id = dto.message_id;
    msgDto.sender_id = dto.sender_id;
    msgDto.recipient_id = dto.receiver_id;
    msgDto.body = std::move (dto);

    emit messageSent(msgDto.toDomain ());
    // const QString message_id = QUuid::createUuid ().toString (QUuid::WithoutBraces);
    return m_xmpp->sendMessage (payload.receiver_id, jsonString,payload.message_id)
       .then(this,
             [this, payload,&dto, jsonString](QString error) mutable {
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

    return m_xmpp->fileUpload(payload.message_id,file.toString ())
        .then(
            this,
            [this, payload](const core::xmpp::UploadedFile &upload) mutable {
                if (!upload.error.isEmpty()) {
                    return QtFuture::makeReadyValueFuture(upload.error);
                }
                payload.content.media_url = upload.url.toString(QUrl::FullyEncoded);
                 const QString message_id = QUuid::createUuid ().toString (QUuid::WithoutBraces);
                return sendMessage (payload);
            }).unwrap();
}

QFuture<QString> MessageRepositoryImpl::sendImage(
    domain::entity::Payload &payload,
    const QList<domain::AttachmentItem> &attachments,
    domain::repository::UploadCancellationPtr cancellation)
{
    if (attachments.size() != 1) {
        return QtFuture::makeReadyValueFuture(QStringLiteral("Image files and message content do not match."));
    }

    using domain::repository::UploadCancellation;
    if (cancellation && cancellation->isCanceled()) {
        return UploadCancellation::canceledFuture<QString>();
    }
    auto cachedFiles = QtFuture::makeReadyValueFuture(QStringList{});
    for (qsizetype index = 0; index < attachments.size(); ++index) {
        const QString cacheId = payload.message_id + QStringLiteral("-%1").arg(index);
        cachedFiles = cachedFiles.then(this,
            [input = attachments[index], cacheId, cancellation](QStringList paths) {
                if (cancellation && cancellation->isCanceled()) {
                    return UploadCancellation::canceledFuture<QStringList>();
                }
                auto compressed = core::media::ImageCompress::compressImageToCache(input.url.toString (), cacheId);
                if (cancellation) {
                    cancellation->track(compressed);
                }
                return compressed.then([paths](const QString &path) mutable {
                        paths.append(path);
                        return paths;
                    });
            }).unwrap();
    }

    return cachedFiles.then(this, [this, payload, cancellation](const QStringList &paths) mutable {
        if (cancellation && cancellation->isCanceled()) {
            return UploadCancellation::canceledFuture<QString>();
        }
        QStringList uploadUrls;
        for (qsizetype index = 0; index < paths.size(); ++index) {
            if (paths[index].isEmpty()) {
                return QtFuture::makeReadyValueFuture(
                    QStringLiteral("Could not compress image %1. No message was sent.").arg(index + 1));
            }
            payload.content.file_size = QFileInfo(paths[index]).size();
            uploadUrls.append(QUrl::fromLocalFile(paths[index]).toString(QUrl::FullyEncoded));
        }
        auto uploading = m_xmpp->fileUploads(payload.message_id,uploadUrls);
        if (cancellation) {
            cancellation->track(uploading);
        }
        return uploading.then(this, [this, payload, cancellation](const QList<core::xmpp::UploadedFile> &uploads) mutable {
                if (uploads.size() != 1) {
                    return QtFuture::makeReadyValueFuture(QStringLiteral("Incomplete image upload results."));
                }
                for (qsizetype index = 0; index < uploads.size(); ++index) {
                    if (!uploads[index].error.isEmpty()) {
                        return QtFuture::makeReadyValueFuture(uploads[index].error);
                    }
                    payload.content.media_url = uploads[index].url.toString(QUrl::FullyEncoded);
                }
                if (cancellation && !cancellation->tryCommit()) {
                    return UploadCancellation::canceledFuture<QString>();
                }
                return sendMessage(payload);
            }).unwrap();
    }).unwrap();
}

QFuture<QString> MessageRepositoryImpl::sendFile(
    domain::entity::Payload &payload,
    const QUrl &file)
{
    // if (payload.content.isEmpty()) {
    //     return QtFuture::makeReadyValueFuture(QStringLiteral("Message content is empty."));
    // }

    // return m_xmpp->fileUpload (file.toString ())
    // .then (this,
    //       [this, payload](const core::xmpp::UploadedFile &upload) mutable {
    //           if (!upload.error.isEmpty ()) {
    //               return QtFuture::makeReadyValueFuture (upload.error);
    //           }
    //           payload.content.media_url = upload.url.toString (QUrl::FullyEncoded);
    //           return sendMessage(payload);
    //       }).unwrap ();

    return QtFuture::makeReadyValueFuture (QStringLiteral (""));
}

QFuture<QString> MessageRepositoryImpl::sendFiles(
    domain::entity::Payload &payload,
    const QList<domain::AttachmentItem> &attachments,
    domain::repository::UploadCancellationPtr cancellation)
{
    using domain::repository::UploadCancellation;
    if (attachments.size() != 1) {
        return QtFuture::makeReadyValueFuture(QStringLiteral("Each message must contain one file."));
    }
    if (cancellation && cancellation->isCanceled()) {
        return UploadCancellation::canceledFuture<QString>();
    }
    const auto &attachment = attachments.first();
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
    task->future = task->cancellation->track(m_xmpp->fileUpload(task->id, task->file_path));
    m_uploadTasks.insert(task->id, task);
    emit fileUploadStated(task->id, task->file_path);

    auto *watcher = new QFutureWatcher<core::xmpp::UploadedFile>(this);
    connect(watcher, &QFutureWatcher<core::xmpp::UploadedFile>::finished, this, [this, watcher, task] {
        m_uploadTasks.remove(task->id);
        watcher->deleteLater();
    });
    watcher->setFuture(task->future);
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
        const QString json = data::mapping::toDto(message).toJsonString();
        return m_xmpp->sendMessage(message.receiver_id, json, message.message_id)
            .then(this, [this, message, json](QString error) {
                if (error.isEmpty()) {
                    saveMessage(message.receiver_id, json);
                }
                return error;
            });
    }).unwrap();
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
        persistMessages(currentUserId(), receiver_id,
                        {{payload->message_id, payload->sender_id, payload->receiver_id,
                          payload->toDomain(), QString(), true}});
    }
    dto::MesssageStore message;

    message.recipient_id = receiver_id;
    message.body = body;

    QJsonObject request = message.toJson ();
    m_network->post (core::constants::AppConstants::saveMessages (), request)
        .then ([this](const core::network::NetworkResponse &response) {
            if (!response.isSuccess ()) {
                qWarning() << "Login request failed, status:" << response.status_code;
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
    const QString conversation = storageId(payload.sender_id) == storageId(account)
        ? payload.recipient_id : payload.sender_id;
    persistMessages(account, conversation, {payload});
    restoreMediaPaths(account, payload);
    emit messageReceived (payload);

}



} // namespace repository
} // namespace data
