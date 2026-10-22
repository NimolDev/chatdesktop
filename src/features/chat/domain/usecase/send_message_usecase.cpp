#include "send_message_usecase.hpp"

#include <QUuid>
#include <optional>
#include <QFutureWatcher>
#include <QFileInfo>
#include <QtCore/qdatetime.h>

namespace domain {
namespace usecase {

SendMessageUsecase::SendMessageUsecase(
    std::shared_ptr<repository::MessageRepository> repository,
    QObject *parent)
    : m_repository(std::move (repository)),
    QObject(parent)
{
    connect(m_repository.get (),
            &repository::MessageRepository::fileUploadStated,
            this,
            [this](const QString &id, const QString &file_path) {

                emit fileUploadStated (id, file_path);
            });
    connect(m_repository.get (),
            &repository::MessageRepository::fileUploadProgress,
            this,
            [this](const QString &id, double progress) {

                emit fileUploadProgress (id, progress);
            }
            );

    connect(m_repository.get (),
            &repository::MessageRepository::fileUploadFinish,
            this,
            [this](const QString &id) {

                emit fileUploadFinish (id);
            });
    connect(m_repository.get (),
            &repository::MessageRepository::fileUploadCanceled,
            this,
            [this](const QString &id) {

                emit fileUploadCanceled (id);
            });
}

QFuture<QString> SendMessageUsecase::sendVoice(
    const QString &receiver,
    const QUrl &file,
    qint64 duration,
    const QString &messageId)
{
    const QFileInfo info(file.toLocalFile());
    if (receiver.isEmpty() || !file.isLocalFile() || !info.isFile()
        || info.size() <= 0 || duration <= 0) {
        return QtFuture::makeReadyValueFuture(tr("Record audio before sending a voice message."));
    }
    domain::entity::Payload payload;
    payload.is_mine = true;
    payload.sender_id = m_repository->currentUserId();
    payload.receiver_id = receiver;
    payload.message_id = messageId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : messageId;
    payload.timestamp = QString::number(QDateTime::currentSecsSinceEpoch());
    payload.content.type = domain::entity::MessageType::Voice;
    payload.content.text = "Voice message";
    payload.content.file_size = info.size();
    payload.content.file_name = info.fileName();
    payload.content.duration = duration;
    payload.content.media_url = file.toString(QUrl::FullyEncoded);
    payload.content.localpath = file.toLocalFile();
    return m_repository->sendVoice(payload, file);

}

namespace {

QFuture<QString> sendAttachments(
    repository::MessageRepository *repository, SendMessageUsecase *context,
    const QString &receiver, const QList<domain::AttachmentItem> &attachments,
    const QString &caption, repository::UploadCancellationPtr cancellation, std::optional<bool> imageOverride)
{
    if (receiver.isEmpty() || attachments.isEmpty()) {
        return QtFuture::makeReadyValueFuture(QStringLiteral("Select attachments and a recipient before sending."));
    }
    if (cancellation && cancellation->isCanceled()) {
        return repository::UploadCancellation::canceledFuture<QString>();
    }
    for (const auto &attachment : attachments) {
        const QFileInfo info(attachment.url.toLocalFile());
        if (!attachment.url.isLocalFile() || !info.isFile() || !info.isReadable() || info.size() <= 0) {
            return QtFuture::makeReadyValueFuture(QStringLiteral("Attachment is missing, empty, or unreadable."));
        }
    }
    auto promise = std::make_shared<QPromise<QString>>();
    promise->start();
    QObject::connect(context, &QObject::destroyed, [promise] {
        if (!promise->future().isFinished()) {
            promise->future().cancel();
            promise->finish();
        }
    });
    const QString batchId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    int attachmentIndex = 0;
    auto remaining = std::make_shared<int>(attachments.size());
    auto error = std::make_shared<QString>();
    for (const auto &attachment : attachments) {
        const bool images = imageOverride.value_or(attachment.compress);
        domain::entity::Payload payload;
        payload.is_mine = true;
        payload.attachment_batch_id = batchId;
        payload.attachment_index = attachmentIndex++;
        payload.sender_id = repository->currentUserId();
        payload.receiver_id = receiver;
        payload.message_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        payload.timestamp = QString::number(QDateTime::currentSecsSinceEpoch());
        const QFileInfo info(attachment.url.toLocalFile());
        payload.content = {
            .type = images ? domain::entity::MessageType::Image : domain::entity::MessageType::File,
            .text = caption,
            .media_url = attachment.url.toString(),
            .file_name = info.fileName(),
            .file_size = info.size()
        };
        auto childCancellation = std::make_shared<repository::UploadCancellation>();
        auto future = images ? repository->sendImage(payload, {attachment}, childCancellation)
                             : repository->sendFiles(payload, {attachment}, childCancellation);
        if (cancellation) {
            cancellation->track(future);
        }
        auto *watcher = new QFutureWatcher<QString>(context);
        QObject::connect(watcher, &QFutureWatcher<QString>::canceled, context, [childCancellation] {
            childCancellation->cancel();
        });
        QObject::connect(watcher, &QFutureWatcher<QString>::finished, context,
                         [watcher, promise, remaining, error, context, id = attachment.id] {
            bool success = false;
            if (watcher->isCanceled()) {
                *error = QStringLiteral("Upload canceled.");
            } else {
                try {
                    const auto result = watcher->result();
                    success = result.isEmpty();
                    if (!success) {
                        *error = result;
                    }
                } catch (...) {
                    *error = QStringLiteral("File upload failed. Please retry.");
                }
            }
            emit context->fileUploadSettled(id, success);
            if (--*remaining == 0) {
                promise->addResult(*error);
                promise->finish();
            }
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }
    return promise->future();
}

} // namespace

QFuture<QString> SendMessageUsecase::sendImage(
    const QString &receiver, const QList<domain::AttachmentItem> &attachments,
    const QString &caption, repository::UploadCancellationPtr cancellation)
{
    return sendAttachments(m_repository.get(), this, receiver, attachments, caption, cancellation, true);
}

QFuture<QString> SendMessageUsecase::sendFiles(
    const QString &receiver, const QList<domain::AttachmentItem> &attachments,
    const QString &caption, repository::UploadCancellationPtr cancellation)
{
    return sendAttachments(m_repository.get(), this, receiver, attachments, caption, cancellation, false);
}

QFuture<QString> SendMessageUsecase::sendSelectedAttachments(
    const QString &receiver, const QList<domain::AttachmentItem> &attachments,
    const QString &caption, repository::UploadCancellationPtr cancellation)
{
    return sendAttachments(m_repository.get(), this, receiver, attachments, caption, cancellation, std::nullopt);
}

QFuture<bool> SendMessageUsecase::cancelUpload(const QString &upload_id)
{
    return m_repository->cancelFileUpload (upload_id);
}

 QFuture<QString> domain::usecase::SendMessageUsecase::execute(
    const QString &receiver_id,
    const QString &msg)
{
    domain::entity::Payload payload;
    payload.sender_id = m_repository->currentUserId ();
    payload.receiver_id = receiver_id;
    payload.message_id = QUuid::createUuid ().toString (QUuid::WithoutBraces);
    payload.reply_message_id = NULL;
    payload.timestamp = QString::number(QDateTime::currentSecsSinceEpoch());
    payload.content = {
        .type = domain::entity::MessageType::Text,
        .text = msg,
        .media_url = 0,
        .file_size = 0
    };
    return m_repository->sendMessage (payload);
}

QFuture<QString> SendMessageUsecase::sendSticker(const QString &receiver_id,
                                                 const QString &sticker_path)
{
    domain::entity::Payload payload;
    payload.sender_id = m_repository->currentUserId ();
    payload.receiver_id = receiver_id;
    payload.message_id = QUuid::createUuid ().toString (QUuid::WithoutBraces);
    payload.reply_message_id = NULL;
    payload.timestamp = QString::number (QDateTime::currentSecsSinceEpoch ());
    payload.content.type = domain::entity::MessageType::Sticker;
    payload.content.text = NULL;
    payload.content.media_url = sticker_path;
    payload.content.file_size = 0;
    payload.content.file_name = 0;
    return m_repository->sendSticker (payload);
}


} // namespace usecase
} // namespace domain
