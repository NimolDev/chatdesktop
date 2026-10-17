#include "messaging_vm.hpp"

#include "utils/date_time_utils.hpp"
#include "utils/voice_duration_formate.hpp"
#include <algorithm>
#include <QUuid>
#include <QFileInfo>


namespace {

QString  voiceRecordingDurationStr(quint32 duration) {
    const qint64 milliseconds = std::max<qint64>(0, duration);
    const qint64 minutes = milliseconds / 60000;
    const qint64 seconds = milliseconds / 1000 % 60;
    const qint64 tenths = milliseconds / 100 % 10;
    return QStringLiteral("%1:%2:%3")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'))
        .arg(tenths);
}

} // namespace

MessagingViewModel *MessagingViewModel::s_instance = nullptr;

MessagingViewModel::MessagingViewModel(
    std::shared_ptr<domain::usecase::MessageUsecase> usecase,
    std::shared_ptr<domain::usecase::SendMessageUsecase> msg_usecase,
    QObject *parent,
    std::shared_ptr<domain::repository::MediaRepository> mediaRepository
    )
    : m_usecase(std::move (usecase)),
    m_msgUsecase(std::move (msg_usecase)),
    m_attachmentModel(new AttachmentModel(this)),
    QAbstractListModel(parent),
    m_mediaRepository(mediaRepository),
    m_voiceController(mediaRepository,
                      [this](const QUrl &source) {
                          for (const auto &message : std::as_const (m_message.messages)) {
                              {
                                  const auto &content = message.body.content;
                                  if (QUrl(content.media_url) == source) {
                                      return message.id;
                                  }
                              }
                          }
                          return QString();
                      }, this)
{
    if (m_mediaRepository) {
        using domain::repository::MediaRepository;
        connect(m_mediaRepository.get(), &MediaRepository::downloadProgress, this,
                [this](const QString &request, qint64 received, qint64 total) {
                    if (!request.startsWith(QStringLiteral("file:"))) {
                        return;
                    }
                    const auto id = request.mid(5);
                    auto it = m_fileDownloads.find(id);
                    if (it == m_fileDownloads.end() || !it->downloading) {
                        return;
                    }
                    // Unknown totals and repeated network updates must not rewind
                    // an active transfer. A new attempt resets progress in downloadFile.
                    if (total <= 0 || received < 0) {
                        return;
                    }
                    const double progress = std::clamp(double(received) / total, 0.0, 1.0);
                    if (progress <= it->progress) {
                        return;
                    }
                    it->progress = progress;
                    notifyFileDownload(id);
                });
        connect(m_mediaRepository.get(), &MediaRepository::downloadCompleted, this,
                [this](const QString &request, const QString &path) {
                    if (!request.startsWith(QStringLiteral("file:"))) {
                        return;
                    }
                    const auto id = request.mid(5);
                    auto it = m_fileDownloads.find(id);
                    if (it == m_fileDownloads.end() || !it->downloading) {
                        return;
                    }
                    it->downloading = false;
                    if (QFileInfo(path).isFile()) {
                        it->localFile = QUrl::fromLocalFile(path);
                        it->progress = 1;
                    } else {
                        it->error = tr("Downloaded file is missing.");
                    }
                    notifyFileDownload(id);
                });
        connect(m_mediaRepository.get(), &MediaRepository::downloadFailed, this,
                [this](const QString &request, const QString &error) {
                    if (!request.startsWith(QStringLiteral("file:"))) {
                        return;
                    }
                    const auto id = request.mid(5);
                    auto it = m_fileDownloads.find(id);
                    if (it == m_fileDownloads.end() || !it->downloading) {
                        return;
                    }
                    it->downloading = false;
                    it->error = error;
                    notifyFileDownload(id);
                });
    }
    connect(&m_voiceController, &VoiceMessageController::voiceRecordingReady,
            this, [this](const QString &, const QUrl &, qint64 duration) {
                if (m_sendVoiceWhenReady) {
                    m_sendVoiceWhenReady = false;
                    sendVoice(duration);
                }
            });
    connect(&m_voiceController, &VoiceMessageController::voiceRecordingCanceled,
            this, [this] { m_sendVoiceWhenReady = false; });
    connect(&m_voiceController, &VoiceMessageController::voiceRecordingFailed,
            this, [this] { m_sendVoiceWhenReady = false; });
    connect(&m_voiceController, &VoiceMessageController::voiceRecordingFileChanged,
            this, [this] {
                if (!m_voiceSendError.isEmpty()) {
                    m_voiceSendError.clear();
                    emit voiceSendErrorChanged();
                }
            });
    connect (
        m_usecase.get (),
        &domain::usecase::MessageUsecase::onMessagedReceived,
        this,
        [this](const domain::entity::MessageItem &payload) {

            // auto const message = messageMapping();
            // messageMapping (payload);
            if (normalizedId(payload.sender_id) == m_activeConversationId)
                insertMessage (payload);
        }
        );
    connect(
        m_usecase.get (),
        &domain::usecase::MessageUsecase::messageSent,
        this,
        [this](const domain::entity::MessageItem &payload) {
            if (normalizedId(payload.recipient_id) == m_activeConversationId)
                insertMessage (payload);
        }
        );


    connect(m_msgUsecase.get (),
            &domain::usecase::SendMessageUsecase::fileUploadStated,
            this,
            [this](const QString &id, const QString &path) {
                for (auto it = m_message.messages.crbegin(); it != m_message.messages.crend(); ++it) {
                    if (it->id == id || QUrl(it->body.content.media_url) == QUrl(path)) {
                        m_fileUploads.insert(id, {it->id, 0, 0, true});
                        setUploadProgress(id, 0, true);
                        break;
                    }
                }
            });
    connect(m_msgUsecase.get (),
           &domain::usecase::SendMessageUsecase::fileUploadProgress,
           this,
           [this](const QString &id, double progress) {
                setUploadProgress(id, progress, true);
           });
    connect(m_msgUsecase.get (),
            &domain::usecase::SendMessageUsecase::fileUploadFinish,
            this,
            [this](const QString &id) {
                setUploadProgress(id, 1, false);
            });
    connect(m_msgUsecase.get (),
            &domain::usecase::SendMessageUsecase::fileUploadCanceled,
            this,
            [this](const QString &id) {
                removeCanceledUpload(id);
            });
}

MessagingViewModel::~MessagingViewModel()
{
    for (const auto &id : m_fileDownloads.keys()) {
        cancelFileDownload(id);
    }
    if (m_attachmentCancellation) {
        m_attachmentCancellation->cancel();
    }
}

MessagingViewModel *MessagingViewModel::create(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED (scriptEngine);
    Q_ASSERT (s_instance);
    Q_ASSERT (s_instance->thread() == engine->thread());

    QQmlEngine::setObjectOwnership (s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

void MessagingViewModel::setInstance(MessagingViewModel *instance)
{
    Q_ASSERT (instance);
    s_instance = instance;
}

void MessagingViewModel::requestCall(const QString &receiverId, const QString &userName)
{
    if (!receiverId.trimmed().isEmpty()) {
        emit callRequested(m_activeConversationId, userName);
    }
}
void MessagingViewModel::sendMessage(const QString &receiver_id, const QString &msg)
{
    const QString id = QString(receiver_id).remove(QLatin1Char('-'));
    m_msgUsecase->execute (id, msg).then(this,
                                        [this](QString error) {

                                        });
}

void MessagingViewModel::sendVoice(qint64 duration)
{
    if (m_isSendingVoice) {
        return;
    }
    if (m_voiceController.isVoiceRecordingBusy()) {
        m_sendVoiceWhenReady = true;
        m_voiceController.stopVoiceRecording();
        return;
    }
    const qint64 finalizedDuration = m_voiceController.finalizedRecordingDuration();
    if (finalizedDuration > 0) {
        duration = finalizedDuration;
    } else if (m_voiceController.voiceRecordingDuration() > 0) {
        duration = m_voiceController.voiceRecordingDuration();
    }
    const QUrl file = m_voiceController.voiceRecordingFile();
    const QString receiver = m_voiceController.recordingConversationId();
    if (duration <= 0 || file.isEmpty() || receiver.isEmpty()
        || receiver != m_activeConversationId) {
        return;
    }
    const QString uploadId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_voiceSendError.clear();
    emit voiceSendErrorChanged();
    m_isSendingVoice = true;
    emit isSendingVoiceChanged();
    auto sending = m_msgUsecase->sendVoice(receiver, file, duration, uploadId);

    // The queued send owns its file URL. Release the composer immediately;
    // completion of this upload must never reset a newer recording.
    m_voiceController.cancelVoiceRecording();
    const quint64 revision = m_voiceController.recordingRevision();
    m_isSendingVoice = false;
    emit isSendingVoiceChanged();
    emit voiceMessageQueued();

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this,
            [this, watcher, uploadId, revision] {
        const bool canceled = watcher->future().isCanceled();
        const QString error = canceled ? QString{} : watcher->result();
        watcher->deleteLater();
        if (canceled) {
            removeCanceledUpload(uploadId);
            return;
        }
        const auto upload = m_fileUploads.value(uploadId);
        setUploadProgress(uploadId, error.isEmpty() ? 1 : upload.progress, false);
        if (!error.isEmpty()) {
            if (revision == m_voiceController.recordingRevision()) {
                m_voiceSendError = error;
                emit voiceSendErrorChanged();
                emit voiceSendFailed(error);
            }
            return;
        }
        emit voiceMessageSent();
    });
    watcher->setFuture(sending);
}

void MessagingViewModel::sendImage(const QString &caption)
{
    auto attachments = m_attachmentModel->getItems ();
    startAttachmentUpload(attachments, caption, true);
}

void MessagingViewModel::sendFile(const QString &caption)
{
    auto attachments = m_attachmentModel->getItems ();
    startAttachmentUpload(attachments, caption, false);
}

void MessagingViewModel::startAttachmentUpload(
    const QList<domain::AttachmentItem> &attachments, const QString &caption, bool images)
{
    // if (isUploadingAttachments()) {
    //     return;
    // }
    m_attachmentCancellation = std::make_shared<domain::repository::UploadCancellation>();
    const auto cancellation = m_attachmentCancellation;
    auto *watcher = new QFutureWatcher<QString>(this);
    m_attachmentWatcher = watcher;
    m_cancelingAttachmentUpload = false;
    m_attachmentUploadError.clear();
    emit attachmentUploadChanged();
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, cancellation, attachments] {
        const bool canceled = cancellation->isCanceled() || watcher->future().isCanceled();
        m_attachmentUploadError = canceled ? tr("Upload canceled.") : watcher->result();
        const bool success = !canceled && m_attachmentUploadError.isEmpty();
        for (const auto &attachment : attachments) {
            const auto upload = m_fileUploads.value(attachment.id);
            if (upload.uploading) {
                setUploadProgress(attachment.id, success ? 1 : upload.progress, false);
            }
        }
        m_attachmentWatcher = nullptr;
        m_attachmentCancellation.reset();
        m_cancelingAttachmentUpload = false;
        watcher->deleteLater();
        emit attachmentUploadChanged();
        emit attachmentUploadFinished(success);
    });
    const QString receiver = normalizedId(m_activeConversationId);
    watcher->setFuture(images ? m_msgUsecase->sendImage(receiver, attachments, caption, cancellation)
                              : m_msgUsecase->sendFiles(receiver, attachments, caption, cancellation));
}

void MessagingViewModel::notifyFileDownload(const QString &transferId)
{
    const QString messageId = m_fileDownloads.value(transferId).messageId;
    for (int row = 0; row < m_message.messages.size(); ++row) {
        if (m_message.messages[row].id == messageId) {
            emit dataChanged(index(row), index(row),
                             {ContentsRole, DownloadProgressRole, IsDownloadingRole, DownloadedFileRole, DownloadErrorRole});
            return;
        }
    }
}

void MessagingViewModel::downloadFile(const QString &messageId, int contentIndex)
{
    const QString transferId = contentIndex == 0 ? messageId : messageId + QStringLiteral(":content:") + QString::number(contentIndex);
    if (m_fileDownloads.value(transferId).downloading) {
        return;
    }
    for (const auto &message : std::as_const(m_message.messages)) {
        if (message.id != messageId || contentIndex != 0 || message.body.content.type != domain::entity::MessageType::File) {
            continue;
        }
        const QUrl source(message.body.content.media_url);
        auto &download = m_fileDownloads[transferId];
        download = {};
        download.messageId = messageId;
        if (source.isLocalFile() && QFileInfo(source.toLocalFile()).isFile()) {
            download.localFile = source;
            download.progress = 1;
        } else if (!m_mediaRepository) {
            download.error = tr("File download service is unavailable.");
        } else {
            download.downloading = true;
            notifyFileDownload(transferId);
            m_mediaRepository->downloadFile(QStringLiteral("file:") + transferId, source);
            return;
        }
        notifyFileDownload(transferId);
        return;
    }
}

void MessagingViewModel::cancelFileDownload(const QString &messageId, int contentIndex)
{
    const QString transferId = contentIndex == 0 ? messageId : messageId + QStringLiteral(":content:") + QString::number(contentIndex);
    auto it = m_fileDownloads.find(transferId);
    if (it == m_fileDownloads.end() || !it->downloading) {
        return;
    }
    it->downloading = false;
    it->progress = 0;
    if (m_mediaRepository) {
        m_mediaRepository->cancelDownload(QStringLiteral("file:") + transferId);
    }
    notifyFileDownload(transferId);
}

void MessagingViewModel::cancelFileUpload(const QString &messageId, int contentIndex)
{
    for (auto it = m_fileUploads.cbegin(); it != m_fileUploads.cend(); ++it) {
        if (it->messageId == messageId && it->contentIndex == contentIndex && it->uploading) {
            const QString uploadId = it.key();
            m_msgUsecase->cancelUpload(uploadId).then(this, [this, uploadId](bool success) {
                if (success) {
                    removeCanceledUpload(uploadId);
                }
            });
            return;
        }
    }
}

void MessagingViewModel::cancelAttachmentUpload()
{
    if (m_attachmentCancellation && m_attachmentCancellation->cancel()) {
        m_cancelingAttachmentUpload = true;
        emit attachmentUploadChanged();
    }
}

QVariantMap MessagingViewModel::createAttachment(const QUrl &url, bool compress)
{

    return {
        {
            QStringLiteral ("id"), QUuid::createUuid ().toString (QUuid::WithoutBraces)
        },
        {
            QStringLiteral ("url"), url
        },
        { QStringLiteral ("compress"), compress}
    };
}

void MessagingViewModel::addAttachments(const QList<QUrl> &urls)
{
    m_attachmentModel->addFiles (urls);
}

void MessagingViewModel::resetAttachments()
{
    m_attachmentModel->resetModel();
}

void MessagingViewModel::getImageList()
{
    m_imageUrls.clear();
    for (const auto &message : std::as_const (m_message.messages)) {
        if ( message.body.content.type == domain::entity::MessageType::Image )  {
            m_imageUrls.append (message.body.content.media_url);
        }
    }
    emit imageUrlsChanged();
}

QStringList MessagingViewModel::imageUrls()
{
    return m_imageUrls;
}

void MessagingViewModel::fetchMessage(QString user_id)
{
    const quint64 requestId = ++m_fetchRequestId;
    const QString conversationId = normalizedId(user_id);
    if (conversationId != m_activeConversationId) {
        cancelAttachmentUpload();
        m_sendVoiceWhenReady = false;
    }
    m_voiceController.setConversation(conversationId);
    m_activeConversationId = conversationId;
    setIsLoading (true);

    m_usecase->execute(user_id, 1).then(
        this,
        [this, requestId](domain::entity::MessageResponse messages) {
            applyFetchedMessages(std::move(messages), requestId);
        });
}

void MessagingViewModel::fetchNextMessage()
{
    if (m_loading)
        return;

    if (m_activeConversationId.isEmpty()
        || m_message.page < 1
        || m_message.page >= m_message.total_pages) {
        emit olderMessagesLoaded(0);
        return;
    }

    const quint64 requestId = m_fetchRequestId;
    const int nextPage = m_message.page + 1;
    setIsLoading (true);
    m_usecase->execute(m_activeConversationId, nextPage)
        .then(this,
              [this, requestId, nextPage](domain::entity::MessageResponse response) {
                  if (requestId != m_fetchRequestId)
                      return;

                  if (response.page < 1) {
                      setIsLoading(false);
                      emit olderMessagesLoaded(0);
                      return;
                  }

                  QList<QString> displayDates;
                  QList<QString> sections;
                  displayDates.reserve(response.messages.size());
                  sections.reserve(response.messages.size());
                  for (const auto &message : std::as_const(response.messages)) {
                      displayDates.append(
                          core::utils::formateConversationDate(message.body.timestamp));
                      sections.append(sectionForDate(message.body.timestamp));
                  }

                  const int insertedCount = response.messages.size();
                  if (insertedCount > 0) {
                      const int last = insertedCount - 1;
                      beginInsertRows(QModelIndex(), 0, last);
                      m_message.messages = response.messages + m_message.messages;
                      m_displayDates = displayDates + m_displayDates;
                      m_sections = sections + m_sections;
                      endInsertRows();
                  }

                  m_message.page = response.page > 0 ? response.page : nextPage;
                  m_message.limit = response.limit;
                  m_message.total = response.total;
                  m_message.total_pages = response.total_pages;
                  setIsLoading (false);
                  emit olderMessagesLoaded(insertedCount);
                  // emit messageChanged ();
              });
}

void MessagingViewModel::resetModel()
{
    cancelAttachmentUpload();
    m_sendVoiceWhenReady = false;
    m_voiceController.reset();
    ++m_fetchRequestId;
    m_activeConversationId.clear();
    setIsLoading (true);
    beginResetModel ();
    m_message = {};
    m_displayDates.clear();
    m_sections.clear();
    endResetModel ();
    setIsLoading (false);
    m_imageUrls.clear ();
    emit imageUrlsChanged();
    qDebug() << "Reset model";
    emit olderMessagesLoaded(0);
}

bool MessagingViewModel::isLoading() const
{
    return m_loading;
}

void MessagingViewModel::setIsLoading(bool loading)
{
    if (m_loading == loading) {
        return;
    }
    m_loading = loading;
    emit isLoadingChanged ();
}

void MessagingViewModel::insertMessage(const domain::entity::MessageItem &payload)
{
    int row = m_message.messages.count ();

    beginInsertRows (QModelIndex(), row, row);
    m_message.messages.append (payload);
    m_displayDates.append(core::utils::formateConversationDate(payload.body.timestamp));
    m_sections.append(sectionForDate(payload.body.timestamp));
    endInsertRows ();
    emit messageChanged ();
}


void MessagingViewModel::setUploadProgress(const QString &id, double progress, bool uploading)
{
    auto upload = m_fileUploads.find(id);
    if (upload == m_fileUploads.end() || upload->canceled) {
        return;
    }
    upload->progress = std::clamp(progress, 0.0, 1.0);
    upload->uploading = uploading;
    for (int row = 0; row < m_message.messages.size(); ++row) {
        if (m_message.messages[row].id == upload->messageId) {
            emit dataChanged(index(row), index(row), {ContentsRole, UploadProgressRole, IsUploadingRole});
            return;
        }
    }
}

void MessagingViewModel::removeCanceledUpload(const QString &id)
{
    auto upload = m_fileUploads.find(id);
    if (upload == m_fileUploads.end() || upload->canceled) {
        return;
    }
    upload->uploading = false;
    upload->canceled = true;
    const QString messageId = upload->messageId;
    for (int row = 0; row < m_message.messages.size(); ++row) {
        if (m_message.messages[row].id == messageId) {
            if (data(index(row), ContentsRole).toList().isEmpty()) {
                deleteMessage({row});
            } else {
                emit dataChanged(index(row), index(row), {ContentsRole, UploadProgressRole, IsUploadingRole});
            }
            return;
        }
    }
}

void MessagingViewModel::deleteMessage(QList<int> rows)
{
    std::sort (rows.begin (), rows.end (), std::greater<int>());
    rows.erase (std::unique(rows.begin (), rows.end ()), rows.end ());
    for (int row : rows) {
        if (row < 0 || row >= m_message.messages.count ()) {
            continue;
        }
        beginRemoveRows (QModelIndex(), row, row);
        m_message.messages.remove (row);
        m_displayDates.remove(row);
        m_sections.remove(row);
        endRemoveRows ();
    }
}

void MessagingViewModel::applyFetchedMessages(
   domain::entity::MessageResponse messages,
    quint64 requestId)
{
    // A slower request for the previously selected conversation must never
    // replace the messages of the conversation selected after it.
    if (requestId != m_fetchRequestId)
        return;

    QList<QString> displayDates;
    QList<QString> sections;
    displayDates.reserve(messages.messages.size());
    sections.reserve(messages.messages.size());
    for (const auto &message : std::as_const(messages.messages)) {
        displayDates.append(
            core::utils::formateConversationDate(message.body.timestamp));
        sections.append(sectionForDate(message.body.timestamp));
    }

    beginResetModel ();
    m_message = std::move (messages);
    m_displayDates = std::move(displayDates);
    m_sections = std::move(sections);
    endResetModel ();
    setIsLoading (false);
    emit initialMessagesLoaded();
    emit messageChanged ();
}

QString MessagingViewModel::normalizedId(const QString &id)
{
    QString result = id;
    return result.remove(QLatin1Char('-'));
}

QString MessagingViewModel::sectionForDate(const QString &sentAt) const
{
    bool ok = false;
    const qint64 epochSeconds = sentAt.toLongLong(&ok);
    if (!ok) {
        return {};
    }

    const QDateTime dateTime = QDateTime::fromSecsSinceEpoch(epochSeconds)
                                   .toLocalTime();
    if (!dateTime.isValid()) {
        return {};
    }
    const QDate date = dateTime.date();
    const QDate today = QDate::currentDate();

    if (date == today) {
        return QStringLiteral("Today");
    }

    if (date == today.addDays(-1)) {
        return QStringLiteral("Yesterday");
    }

    return date.toString(QStringLiteral("dd MMM yyyy"));
}

// QAbstractListModel
int MessagingViewModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid ()) {
        qDebug() << "Parent invalide";
        return 0;
    }
    return m_message.messages.size ();
}

QVariant MessagingViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid () ||
        index.row() < 0 ||
        index.row () >= m_message.messages.size ()) {
        return {};
    }

    const auto &messaging = m_message.messages[index.row ()];

    const auto content = messaging.body.content;

    switch (role) {
    case DownloadProgressRole:
        return m_fileDownloads.value(messaging.id).progress;
    case IsDownloadingRole:
        return m_fileDownloads.value(messaging.id).downloading;
    case DownloadedFileRole: {
        const auto downloaded = m_fileDownloads.value(messaging.id).localFile;
        if (QFileInfo(downloaded.toLocalFile()).isFile()) {
            return downloaded;
        }
        return QFileInfo(content.localpath).isFile() ? QUrl::fromLocalFile(content.localpath) : QUrl();
    }
    case DownloadErrorRole:
        return m_fileDownloads.value(messaging.id).error;
    case UploadProgressRole:
        return 0.0;
    case IsUploadingRole:
        for (const auto &upload : m_fileUploads) {
            if (upload.messageId == messaging.id && upload.contentIndex == 0) {
                return role == UploadProgressRole ? QVariant(upload.progress) : QVariant(upload.uploading);
            }
        }
        return role == UploadProgressRole ? QVariant(0.0) : QVariant(false);
    case IdRole:
        return messaging.id;
    case IsMineRole:
        return messaging.is_mine;
    case SenderIdRole:
        return messaging.sender_id;
    case RecipientIdRole:
        return messaging.recipient_id;
    case ContentsRole: {
        QVariantList items;
        {
            const int contentIndex = 0;
            const auto &item = messaging.body.content;
            const QString transferId = contentIndex == 0 ? messaging.id : messaging.id + QStringLiteral(":content:") + QString::number(contentIndex);
            const auto download = m_fileDownloads.value(transferId);
            QUrl localFile = download.localFile;
            if (!QFileInfo(localFile.toLocalFile()).isFile()) {
                localFile = QFileInfo(item.localpath).isFile() ? QUrl::fromLocalFile(item.localpath) : QUrl();
            }
            FileUploadState upload;
            for (const auto &candidate : m_fileUploads) {
                if (candidate.messageId == messaging.id && candidate.contentIndex == contentIndex) {
                    upload = candidate;
                    break;
                }
            }
            if (upload.canceled) {
                return items;
            }
            items.append(QVariantMap{
                {QStringLiteral("msgType"), static_cast<int>(item.type)},
                {QStringLiteral("body"), item.text},
                {QStringLiteral("filePath"), QUrl(item.media_url)},
                {QStringLiteral("fileName"), item.file_name},
                {QStringLiteral("fileSize"), item.file_size},
                {QStringLiteral("duration"), item.duration},
                {QStringLiteral("contentIndex"), contentIndex},
                {QStringLiteral("downloadProgress"), download.progress},
                {QStringLiteral("isDownloading"), download.downloading},
                {QStringLiteral("downloadedFile"), localFile},
                {QStringLiteral("downloadError"), download.error},
                {QStringLiteral("uploadProgress"), upload.progress},
                {QStringLiteral("isUploading"), upload.uploading}
            });
        }
        return items;
    }
    case BodyRole:
        return content.text;
    case SentAtRole:
        return m_displayDates.value(index.row());
    case SectionRole:
        return m_sections.value(index.row());
    case MsgType:
        switch(content.type) {
        case domain::entity::MessageType::Text:
            return 0;
        case domain::entity::MessageType::Voice:
            return 1;
        case domain::entity::MessageType::Video:
            return 2;
        case domain::entity::MessageType::Location:
            return 3;
        case domain::entity::MessageType::Image:
            return 4;
        case domain::entity::MessageType::File:
            return 5;
        case domain::entity::MessageType::Sticker:
            return 6;
        default:  return -1;
        }
    case SeenRole:
        return false;
    case FilePathRole:
        if (content.type == domain::entity::MessageType::Voice
            || content.type == domain::entity::MessageType::Image
            || content.type == domain::entity::MessageType::File
            || content.type == domain::entity::MessageType::Sticker) {
            return QUrl(content.media_url);
        }
        return "";
    case FileSizeRole:
        return content.file_size;
    case DurationRole:
        if (content.type == domain::entity::MessageType::Voice) {
            // return voiceRecordingDurationStr (content.duration);
            return core::utils::voiceRecordingDurationStr (content.duration);
        }
    default:
        return {};
    }
}

QHash<int, QByteArray> MessagingViewModel::roleNames() const
{
    return {
        { DownloadProgressRole, "downloadProgress" },
        { IsDownloadingRole, "isDownloading" },
        { DownloadedFileRole, "downloadedFile" },
        { DownloadErrorRole, "downloadError" },
        { UploadProgressRole, "uploadProgress" },
        { IsUploadingRole, "isUploading" },
        { IdRole, "id" },
        { IsMineRole, "isMine" },
        { SenderIdRole, "senderId" },
        { RecipientIdRole, "recipientId" },
        { BodyRole, "body" },
        { ContentsRole, "contents" },
        { SentAtRole, "sentAt" },
        { SectionRole, "section" },
        { MsgType, "msgType"},
        { SeenRole, "seen"},
        { FilePathRole, "filePath"},
        { DurationRole, "duration" },
        { FileSizeRole, "fileSize" },
    };
}
