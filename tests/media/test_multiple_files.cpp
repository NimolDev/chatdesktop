#include "presentation/viewmodel/messaging_vm.hpp"
#include <QTemporaryFile>
#include <QtTest>

class FilesRepository final : public domain::repository::MessageRepository
{
public:
    domain::entity::MessageResponse response{};
    QFuture<domain::entity::MessageResponse> fetchMessageById(QString, int) override
    { return QtFuture::makeReadyValueFuture(response); }
    QString currentUserId() const override { return QStringLiteral("self"); }
    QFuture<QString> sendMessage(domain::entity::Payload &) override { return ready(); }
    QFuture<QString> sendVoice(domain::entity::Payload &, const QUrl &) override { return ready(); }
    QFuture<QString> sendFile(domain::entity::Payload &, const QUrl &) override { return ready(); }
    QFuture<QString> sendImage(domain::entity::Payload &, const QList<domain::AttachmentItem> &, domain::repository::UploadCancellationPtr) override { return ready(); }
    QList<domain::entity::Payload> sent;
    QFuture<QString> sendFiles(domain::entity::Payload &payload, const QList<domain::AttachmentItem> &items, domain::repository::UploadCancellationPtr) override
    {
        Q_ASSERT(items.size() == 1);
        sent.append(payload);
        return ready();
    }
    QStringList canceledUploads;
    QFuture<bool> cancelFileUpload(const QString &id) override
    {
        canceledUploads.append(id);
        return QtFuture::makeReadyValueFuture(true);
    }
    QFuture<QString> ready() { return QtFuture::makeReadyValueFuture(QString()); }
};

class FilesMedia final : public domain::repository::MediaRepository
{
public:
    QHash<QString, QUrl> requests;
    QString canceled;
    void downloadFile(const QString &id, const QUrl &url) override { requests.insert(id, url); }
    void cancelDownload(const QString &id) override { canceled = id; }
};

class MultipleFilesTest : public QObject
{
    Q_OBJECT
private slots:
    void separatePayloads()
    {
        QTemporaryFile first;
        QTemporaryFile second;
        QVERIFY(first.open());
        QVERIFY(second.open());
        first.write("one");
        second.write("two");
        first.flush();
        second.flush();
        auto repo = std::make_shared<FilesRepository>();
        domain::usecase::SendMessageUsecase usecase(repo);
        auto future = usecase.sendFiles("peer", {
            {.id = "a", .url = QUrl::fromLocalFile(first.fileName())},
            {.id = "b", .url = QUrl::fromLocalFile(second.fileName())}
        }, "Caption");
        QTRY_VERIFY(future.isFinished());
        QVERIFY(future.result().isEmpty());
        QCOMPARE(repo->sent.size(), 2);
        QVERIFY(repo->sent[0].message_id != repo->sent[1].message_id);
        QCOMPARE(repo->sent[0].content.media_url, QUrl::fromLocalFile(first.fileName()).toString());
        QCOMPARE(repo->sent[1].content.media_url, QUrl::fromLocalFile(second.fileName()).toString());
    }

    void independentTransfers()
    {
        auto repo = std::make_shared<FilesRepository>();
        auto media = std::make_shared<FilesMedia>();
        for (int i = 0; i < 2; ++i) {
            domain::entity::MessageItem message{};
            message.id = QStringLiteral("file-%1").arg(i);
            message.body.content = {.type = domain::entity::MessageType::File,
                                    .media_url = QStringLiteral("https://example.com/%1.pdf").arg(i)};
            repo->response.messages.append(message);
        }
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(repo),
                              std::make_shared<domain::usecase::SendMessageUsecase>(repo), nullptr, media);
        vm.fetchMessage("chat");
        QTRY_COMPARE(vm.rowCount(), 2);
        emit repo->fileUploadStated("upload-0", repo->response.messages[0].body.content.media_url);
        emit repo->fileUploadStated("upload-1", repo->response.messages[1].body.content.media_url);
        vm.cancelFileUpload("file-0");
        QTRY_COMPARE(vm.rowCount(), 1);
        QCOMPARE(vm.data(vm.index(0), MessagingViewModel::IdRole).toString(), QStringLiteral("file-1"));
        QVERIFY(vm.data(vm.index(0), MessagingViewModel::IsUploadingRole).toBool());
        emit repo->fileUploadProgress("upload-0", 0.9);
        QCOMPARE(vm.rowCount(), 1);
        vm.cancelFileUpload("file-1");
        QTRY_COMPARE(vm.rowCount(), 0);
        QCOMPARE(repo->canceledUploads, (QStringList{"upload-0", "upload-1"}));
    }
};
QTEST_GUILESS_MAIN(MultipleFilesTest)
#include "test_multiple_files.moc"
