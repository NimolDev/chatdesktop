#include "presentation/viewmodel/messaging_vm.hpp"
#include <QTemporaryFile>
#include <QPromise>
#include <QtTest>
#include <thread>

class AttachmentRepository final : public domain::repository::MessageRepository
{
public:
    std::shared_ptr<QPromise<QString>> pending;
    int starts = 0;
    int sent = 0;
    int images = 0;

    QFuture<domain::entity::MessageResponse> fetchMessageById(QString, int) override
    {
        return QtFuture::makeReadyValueFuture(domain::entity::MessageResponse{});
    }
    QString currentUserId() const override { return QStringLiteral("self"); }
    QFuture<QString> sendMessage(domain::entity::Payload &) override
    {
        return QtFuture::makeReadyValueFuture(QString());
    }
    QFuture<QString> sendVoice(domain::entity::Payload &, const QUrl &) override
    {
        return QtFuture::makeReadyValueFuture(QString());
    }
    QFuture<QString> sendFile(domain::entity::Payload &, const QUrl &) override
    {
        return QtFuture::makeReadyValueFuture(QString());
    }
    QFuture<QString> sendImage(domain::entity::Payload &payload, const QStringList &files,
                              domain::repository::UploadCancellationPtr cancellation) override
    {
        ++images;
        return sendFiles(payload, files, cancellation);
    }
    QFuture<QString> sendFiles(domain::entity::Payload &, const QStringList &,
                              domain::repository::UploadCancellationPtr cancellation) override
    {
        ++starts;
        pending = std::make_shared<QPromise<QString>>();
        pending->start();
        auto *watcher = new QFutureWatcher<QString>(this);
        connect(watcher, &QFutureWatcher<QString>::canceled, this,
                [promise = pending] { promise->finish(); });
        connect(watcher, &QFutureWatcher<QString>::finished, watcher, &QObject::deleteLater);
        watcher->setFuture(pending->future());
        return cancellation->track(pending->future()).then(this,
            [this, cancellation](const QString &error) {
                if (error.isEmpty() && cancellation->tryCommit()) {
                    ++sent;
                }
                return error;
            });
    }
    void complete(const QString &error = {})
    {
        pending->addResult(error);
        pending->finish();
    }
};

class AttachmentUploadTest : public QObject
{
    Q_OBJECT
    QTemporaryFile file;
private slots:
    void initTestCase()
    {
        QVERIFY(file.open());
        file.write("attachment");
        file.flush();
    }
    void cancelAndRetry_data()
    {
        QTest::addColumn<bool>("image");
        QTest::newRow("file") << false;
        QTest::newRow("image") << true;
    }
    void cancelAndRetry()
    {
        QFETCH(bool, image);
        auto repo = std::make_shared<AttachmentRepository>();
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(repo),
                              std::make_shared<domain::usecase::SendMessageUsecase>(repo));
        vm.fetchMessage("recipient");
        QTRY_VERIFY(!vm.isLoading());
        const QStringList files{QUrl::fromLocalFile(file.fileName()).toString()};
        const auto send = [&] {
            if (image) {
                vm.sendImage(files, "caption");
            } else {
                vm.sendFile(files, "caption");
            }
        };
        QSignalSpy completed(&vm, &MessagingViewModel::attachmentUploadFinished);
        send();
        QVERIFY(vm.isUploadingAttachments());
        send();
        QCOMPARE(repo->starts, 1);
        vm.cancelAttachmentUpload();
        QTRY_VERIFY(!vm.isUploadingAttachments());
        QVERIFY(repo->pending->isCanceled());
        QCOMPARE(repo->sent, 0);
        QVERIFY(!vm.attachmentUploadError().isEmpty());
        QCOMPARE(completed.size(), 1);
        QCOMPARE(completed.first().first().toBool(), false);
        send();
        QVERIFY(vm.attachmentUploadError().isEmpty());
        repo->complete();
        QTRY_VERIFY(!vm.isUploadingAttachments());
        QCOMPARE(repo->sent, 1);
        QCOMPARE(completed.last().first().toBool(), true);
        QCOMPARE(repo->images, image ? 2 : 0);
    }
    void completedUploadCannotSendAfterCancel()
    {
        auto repo = std::make_shared<AttachmentRepository>();
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(repo),
                              std::make_shared<domain::usecase::SendMessageUsecase>(repo));
        vm.fetchMessage("recipient");
        QTRY_VERIFY(!vm.isLoading());
        vm.sendFile({QUrl::fromLocalFile(file.fileName()).toString()}, "caption");
        // Finish the transfer, then cancel before its queued send continuation.
        std::thread worker([repo] { repo->complete(); });
        worker.join();
        vm.cancelAttachmentUpload();
        QTRY_VERIFY(!vm.isUploadingAttachments());
        QCOMPARE(repo->sent, 0);
    }
    void failureAndConversationSwitch()
    {
        auto repo = std::make_shared<AttachmentRepository>();
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(repo),
                              std::make_shared<domain::usecase::SendMessageUsecase>(repo));
        vm.fetchMessage("recipient");
        QTRY_VERIFY(!vm.isLoading());
        const QStringList files{QUrl::fromLocalFile(file.fileName()).toString()};
        vm.sendFile(files, "caption");
        repo->complete("Upload failed");
        QTRY_VERIFY(!vm.isUploadingAttachments());
        QCOMPARE(vm.attachmentUploadError(), QStringLiteral("Upload failed"));
        vm.sendFile(files, "caption");
        vm.fetchMessage("different-recipient");
        QTRY_VERIFY(!vm.isUploadingAttachments());
        QVERIFY(repo->pending->isCanceled());
        QCOMPARE(repo->sent, 0);
    }
};
QTEST_GUILESS_MAIN(AttachmentUploadTest)
#include "test_attachment_upload.moc"
