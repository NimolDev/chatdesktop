#ifndef FEATURES_CHAT_PRESENTATION_VIEWMODEL_MESSAGING_VIEW_MODEL_HPP
#define FEATURES_CHAT_PRESENTATION_VIEWMODEL_MESSAGING_VIEW_MODEL_HPP

#include <QAbstractListModel>
#include <QObject>
#include <optional>
#include <QFutureWatcher>
#include <QtQml/qqmlregistration.h>

#include "domain/usecase/message_usecase.hpp"
#include "domain/usecase/send_message_usecase.hpp"
#include "voice_message_controller.hpp"
#include "domain/repository/media_repository.hpp"
#include "attachment_model.hpp"
// #include "domain/entity/attachment_item.hpp"
// #include "domain/entity/attachment_item.hpp"

class MessagingViewModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by the chat model factory")

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        IsMineRole,
        SenderIdRole,
        RecipientIdRole,
        BodyRole,
        SentAtRole,
        MsgType,
        SectionRole,
        SeenRole,
        SendingRole,
        SendingSuccessRole,
        FilePathRole,
        DurationRole,
        FileSizeRole,
        ContentsRole,
        UploadProgressRole,
        IsUploadingRole,
        DownloadProgressRole,
        IsDownloadingRole,
        DownloadedFileRole,
        DownloadErrorRole,
    };

    explicit MessagingViewModel(
        std::shared_ptr<domain::usecase::MessageUsecase> usecase,
        std::shared_ptr<domain::usecase::SendMessageUsecase> msg_usecase,
        QObject *parent = nullptr,
        std::shared_ptr<domain::repository::MediaRepository> mediaRepository = {}
        );

    ~MessagingViewModel() override;


public:
    Q_PROPERTY(bool isLoading READ isLoading WRITE setIsLoading NOTIFY isLoadingChanged FINAL)
    Q_PROPERTY(VoiceMessageController *voiceController READ voiceController CONSTANT)
    Q_PROPERTY(AttachmentModel *attachmentModel READ attachmentModel CONSTANT )
    Q_PROPERTY(bool isUploadingAttachments READ isUploadingAttachments NOTIFY attachmentUploadChanged)
    Q_PROPERTY(bool isCancelingAttachmentUpload READ isCancelingAttachmentUpload NOTIFY attachmentUploadChanged)
    Q_PROPERTY(QString attachmentUploadError READ attachmentUploadError NOTIFY attachmentUploadChanged)
    Q_PROPERTY(bool isSendingVoice READ isSendingVoice NOTIFY isSendingVoiceChanged)
    Q_PROPERTY(QString voiceSendError READ voiceSendError NOTIFY voiceSendErrorChanged)
    Q_PROPERTY(QStringList imageUrls READ imageUrls  NOTIFY imageUrlsChanged FINAL)
    Q_PROPERTY(bool messageSending READ messageSending WRITE setMessageSending NOTIFY messageSendingChanged FINAL)
    Q_PROPERTY(bool messageSendSuccess READ messageSendSuccess WRITE setMessageSendSuccess NOTIFY messageSendSuccessChanged FINAL)

public:
    Q_INVOKABLE void fetchMessage(QString user_id);
    Q_INVOKABLE void fetchNextMessage();
    Q_INVOKABLE void resetModel();
    Q_INVOKABLE void sendMessage(const QString &receiver_id, const QString &msg);
    Q_INVOKABLE void deleteMessage(QList<int> rows);
    Q_INVOKABLE void requestCall(const QString &receiverId, const QString &userName);
    Q_INVOKABLE void sendSelectedAttachments(const QString &caption);
    Q_INVOKABLE void sendImage(const QString &caption);
    Q_INVOKABLE void sendFile( const QString &caption);
    Q_INVOKABLE void sendVoice(qint64 duration);
    Q_INVOKABLE void sendSticker(const QString &sticker_path);
    Q_INVOKABLE void cancelAttachmentUpload();
    Q_INVOKABLE void downloadFile(const QString &messageId, int contentIndex = 0);
    Q_INVOKABLE void cancelFileDownload(const QString &messageId, int contentIndex = 0);
    Q_INVOKABLE void cancelFileUpload(const QString &messageId, int contentIndex = 0);
    Q_INVOKABLE QVariantMap createAttachment(const QUrl &url, bool compress);
    Q_INVOKABLE void addAttachments(const QList<QUrl> &urls, bool compress = false);
    Q_INVOKABLE void resetAttachments();
    Q_INVOKABLE void getImageList();

    bool isUploadingAttachments() const { return m_attachmentWatcher != nullptr; }
    bool isCancelingAttachmentUpload() const { return m_cancelingAttachmentUpload; }
    bool isSendingVoice() const { return m_isSendingVoice; }
    bool messageSending() const;
    bool messageSendSuccess() const;

    QString attachmentUploadError() const { return m_attachmentUploadError; }
    QString voiceSendError() const { return m_voiceSendError; }
    QStringList imageUrls();

    VoiceMessageController *voiceController() { return &m_voiceController; }
    AttachmentModel *attachmentModel() { return m_attachmentModel; }

public:
    int rowCount(const QModelIndex &parent = QModelIndex ()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool isLoading() const;

signals:
    void attachmentUploadChanged();
    void attachmentUploadFinished(bool success);
    void isSendingVoiceChanged();
    void voiceSendErrorChanged();
    void voiceMessageQueued();
    void voiceMessageSent();
    void voiceSendFailed(const QString &message);
    void callRequested(const QString &receiverId, const QString &userName);
    void isLoadingChanged();
    void messageChanged();
    void initialMessagesLoaded();
    void olderMessagesLoaded(int insertedCount);
    void imageUrlsChanged();
    void messageSendingChanged();
    void messageSendSuccessChanged();

private:
    struct FileDownloadState {
        QString messageId;
        double progress = 0;
        bool downloading = false;
        QUrl localFile;
        QString error;
    };
    QUrl cachedFileUrl(const QString &localPath, const QString &remoteUrl) const;
    mutable QHash<QString, QUrl> m_cachedFileUrls;
    void notifyFileDownload(const QString &transferId);
    std::shared_ptr<domain::repository::MediaRepository> m_mediaRepository;
    QHash<QString, FileDownloadState> m_fileDownloads;

    void setUploadProgress(const QString &id, double progress, bool uploading);
    void removeCanceledUpload(const QString &id);
    struct FileUploadState {
        QString messageId;
        int contentIndex = 0;
        double progress = 0;
        bool uploading = false;
        bool canceled = false;
    };
    QHash<QString, FileUploadState> m_fileUploads;

    void startAttachmentUpload(
        const QList<domain::AttachmentItem> &attachments,
        const QString &caption, std::optional<bool> images
        );
    QFutureWatcher<QString> *m_attachmentWatcher = nullptr;
    domain::repository::UploadCancellationPtr m_attachmentCancellation;
    bool m_cancelingAttachmentUpload = false;
    QString m_attachmentUploadError;

    void applyFetchedMessages(domain::entity::MessageResponse messages,
                              quint64 requestId);
    QString sectionForDate(const QString &sentAt) const;
    static QString normalizedId(const QString &id);
    void setIsLoading(bool loading);
    void insertMessage(const domain::entity::MessageItem &payload);

    void setMessageSending(bool value);
    void setMessageSendSuccess(bool value);

    void updateMessageSeningStatus(const QString &message_id, bool sending);

private:

    void connectMessageUsecases();

    std::shared_ptr<domain::usecase::MessageUsecase> m_usecase;
    std::shared_ptr<domain::usecase::SendMessageUsecase> m_msgUsecase;
    domain::entity::MessageResponse m_message;
    QList<QString> m_displayDates;
    QList<QString> m_sections;


    QString m_activeConversationId;
    QString m_displayedConversationId;
    quint64 m_fetchRequestId = 0;
    VoiceMessageController m_voiceController;
    AttachmentModel *m_attachmentModel;

private:
    bool m_loading = false;
    bool m_isSendingVoice = false;
    bool m_sendVoiceWhenReady = false;
    bool m_messageSending = false;
    bool m_messageSendSuccess = false;

    QString m_voiceSendError;
    QStringList m_imageUrls;

};

#endif // FEATURES_CHAT_PRESENTATION_VIEWMODEL_MESSAGING_VIEW_MODEL_HPP
