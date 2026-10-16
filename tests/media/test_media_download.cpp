#include "data/repository/media_repository_impl.hpp"
#include "presentation/viewmodel/messaging_vm.hpp"
#include "media/pcm_wave.hpp"
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

class DownloadMessageRepository final : public domain::repository::MessageRepository
{
public:
    QFuture<domain::entity::MessageResponse> fetchMessageById(QString, int) override
    {
        return QtFuture::makeReadyValueFuture(domain::entity::MessageResponse{});
    }
    void sendMessage(domain::entity::Payload &) override {}
    QString currentUserId() const override { return QStringLiteral("test"); }
};

class MediaDownloadTest : public QObject
{
    Q_OBJECT
    QTcpServer server;
    QByteArray audio;
    int requests = 0;
    QStringList cachedFiles;

    QUrl url(const QString &path = QStringLiteral("/audio.mp3")) const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1%2").arg(server.serverPort()).arg(path));
    }
private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName(QStringLiteral("ChatMediaDownloadTests"));
        QBuffer buffer(&audio);
        QVERIFY(buffer.open(QIODevice::ReadWrite));
        buffer.write(QByteArray(44 + 16000 * 2 * 2, '\0'));
        QAudioFormat format;
        format.setSampleRate(16000);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Int16);
        QVERIFY(core::media::writePcmWaveHeader(buffer, format));
        QVERIFY(server.listen(QHostAddress::LocalHost));
        connect(&server, &QTcpServer::newConnection, this, [this] {
            while (server.hasPendingConnections()) {
                auto *socket = server.nextPendingConnection();
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                connect(socket, &QTcpSocket::readyRead, socket, [this, socket] {
                    auto request = socket->property("request").toByteArray() + socket->readAll();
                    socket->setProperty("request", request);
                    if (!request.contains("\r\n\r\n") || socket->property("handled").toBool()) {
                        return;
                    }
                    socket->setProperty("handled", true);
                    ++requests;
                    if (request.startsWith("GET /slow")) {
                        return;
                    }
                    const bool missing = request.startsWith("GET /missing");
                    const auto body = missing ? QByteArray("missing") : audio;
                    socket->write(missing ? "HTTP/1.1 404 Not Found\r\n" : "HTTP/1.1 200 OK\r\n");
                    socket->write("Content-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
            }
        });
    }

    void cacheAndPlayback()
    {
        auto media = std::make_shared<data::repository::MediaRepositoryImpl>();
        QSignalSpy completed(media.get(), &domain::repository::MediaRepository::downloadCompleted);
        QSignalSpy failed(media.get(), &domain::repository::MediaRepository::downloadFailed);
        const auto id = QUuid::createUuid().toString();
        const int before = requests;
        media->downloadFile(id, url());
        media->downloadFile(id, url());
        QTRY_COMPARE(completed.count(), 1);
        QCOMPARE(requests, before + 1);
        QCOMPARE(failed.count(), 0);
        const auto path = completed.first().at(1).toString();
        cachedFiles.append(path);
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), audio);
        media->downloadFile(id, url());
        QCOMPARE(completed.count(), 2);
        QCOMPARE(requests, before + 1);

        auto messages = std::make_shared<DownloadMessageRepository>();
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(messages),
                              std::make_shared<domain::usecase::SendMessageUsecase>(messages), nullptr, media);
        vm.voiceController()->playVoice(url());
        QVERIFY(vm.voiceController()->voiceDownloading());
        QTRY_VERIFY(!vm.voiceController()->voiceDownloading());
        QTRY_VERIFY(vm.voiceController()->voicePlaybackPosition() > 0);
        QCOMPARE(vm.voiceController()->voicePlaybackSource(), url());
        bool playingLocalFile = false;
        for (auto *player : vm.findChildren<core::media::AudioPlayer *>()) {
            if (player->playbackState() == QMediaPlayer::PlayingState) {
                QVERIFY(player->source().isLocalFile());
                playingLocalFile = true;
            }
        }
        QVERIFY(playingLocalFile);
        QVERIFY(vm.voiceController()->voicePlaybackError().isEmpty());
        QVERIFY(vm.voiceController()->isInPlay());
        QSignalSpy playbackStatusChanged(vm.voiceController(), &VoiceMessageController::isInPlayChanged);
        // Resetting the composer on a conversation switch stops only its preview.
        vm.voiceController()->stopVoicePreview();
        vm.fetchMessage(QStringLiteral("another-chat"));
        QVERIFY(vm.voiceController()->isInPlay());
        QCOMPARE(playbackStatusChanged.count(), 0);
        QCOMPARE(vm.voiceController()->voicePlaybackSource(), url());
        QVERIFY(vm.voiceController()->voicePlaybackPlaying());
        const auto position = vm.voiceController()->voicePlaybackPosition();
        QTRY_VERIFY(vm.voiceController()->voicePlaybackPosition() > position);
        cachedFiles.append(completed.last().at(1).toString());
        vm.voiceController()->stopVoicePlayback();
        QVERIFY(!vm.voiceController()->isInPlay());
    }

    void downloadExtension_data()
    {
        QTest::addColumn<QString>("sourcePath");
        QTest::addColumn<QString>("extension");
        QTest::newRow("m4a") << "/httpfileupload/slot/voice.m4a" << "m4a";
        QTest::newRow("mp3") << "/voice.mp3" << "mp3";
        QTest::newRow("query") << "/voice.M4A?token=test.mp3" << "m4a";
        QTest::newRow("no-suffix") << "/voice" << "bin";
    }

    void downloadExtension()
    {
        QFETCH(QString, sourcePath);
        QFETCH(QString, extension);
        data::repository::MediaRepositoryImpl media;
        QSignalSpy completed(&media, &domain::repository::MediaRepository::downloadCompleted);
        const auto id = QUuid::createUuid().toString();
        const int before = requests;
        media.downloadFile(id, url(sourcePath));
        QTRY_COMPARE(completed.count(), 1);
        const auto path = completed.first().at(1).toString();
        cachedFiles.append(path);
        QCOMPARE(QFileInfo(path).suffix(), extension);
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), audio);
        media.downloadFile(id, url(sourcePath));
        QCOMPARE(completed.count(), 2);
        QCOMPARE(requests, before + 1);
    }

    void failureAndCancellation()
    {
        data::repository::MediaRepositoryImpl media;
        QSignalSpy failed(&media, &domain::repository::MediaRepository::downloadFailed);
        QSignalSpy completed(&media, &domain::repository::MediaRepository::downloadCompleted);
        const auto id = QUuid::createUuid().toString();
        media.downloadFile(id, url(QStringLiteral("/missing")));
        QTRY_COMPARE(failed.count(), 1);
        QCOMPARE(completed.count(), 0);
        media.downloadFile(id, url(QStringLiteral("/slow")));
        media.cancelDownload(id);
        media.downloadFile(id, url());
        QTRY_COMPARE(completed.count(), 1);
        cachedFiles.append(completed.first().at(1).toString());
        QCOMPARE(failed.count(), 1);
    }

    void switchingConversationPreservesPendingPlayback()
    {
        auto media = std::make_shared<data::repository::MediaRepositoryImpl>();
        auto messages = std::make_shared<DownloadMessageRepository>();
        MessagingViewModel vm(std::make_shared<domain::usecase::MessageUsecase>(messages),
                              std::make_shared<domain::usecase::SendMessageUsecase>(messages), nullptr, media);
        vm.voiceController()->playVoice(url(QStringLiteral("/slow")));
        QVERIFY(vm.voiceController()->voiceDownloading());
        vm.fetchMessage(QStringLiteral("another-chat"));
        QVERIFY(vm.voiceController()->voiceDownloading());
        QCOMPARE(vm.voiceController()->voicePlaybackSource(), url(QStringLiteral("/slow")));
        vm.resetModel();
        QVERIFY(!vm.voiceController()->voiceDownloading());
        QVERIFY(vm.voiceController()->voicePlaybackSource().isEmpty());
        QVERIFY(!vm.voiceController()->voicePlaybackPlaying());
    }

    void filesystemFailureAndShutdown()
    {
        QTemporaryDir directory;
        QFile blocker(directory.filePath("file"));
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        core::network::HttpDownloadService service;
        QSignalSpy failed(&service, &core::network::HttpDownloadService::downloadFailedChanged);
        const auto id = service.download(url(), blocker.fileName() + "/audio.mp3");
        QTRY_COMPARE(failed.count(), 1);
        QCOMPARE(failed.first().first().toString(), id);
        {
            core::network::HttpDownloadService pending;
            pending.download(url(QStringLiteral("/slow")), directory.filePath("pending.mp3"));
        }
        QCOMPARE(QDir(directory.path()).entryList({"*.part"}, QDir::Files).size(), 0);
    }

    void cleanupTestCase()
    {
        for (const auto &path : cachedFiles) {
            QFile::remove(path);
        }
    }
};
QTEST_GUILESS_MAIN(MediaDownloadTest)
#include "test_media_download.moc"
