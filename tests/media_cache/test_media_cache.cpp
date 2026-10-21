#include "storage/media_cache.hpp"
#include "media/image_compress.hpp"
#include "media/audio_player.hpp"
#include "xmpp/file_upload_operation.hpp"
#include "constants/app_constants.hpp"
#include <QDir>
#include <QFile>
#include <QImage>
#include <QDataStream>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

namespace core {
namespace constants {
QString AppConstants::appSecretKey()
{
    return QStringLiteral("media-cache-test-key");
}
} // namespace constants
} // namespace core

using core::storage::MediaCache;

class MediaCacheTest : public QObject
{
    Q_OBJECT
    QTemporaryDir directory;
private slots:
    void initTestCase()
    {
        QVERIFY(directory.isValid());
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName(QStringLiteral("MediaCacheTest-") + QUuid::createUuid().toString(QUuid::WithoutBraces));
    }

    void localFileReferences()
    {
        const QUrl url("https://example.test/attachment.pdf");
        const auto local = directory.filePath("attachment.pdf");
        QVERIFY(!MediaCache::saveLocalFilePath(url, local, "account-a"));
        QFile file(local);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("original attachment bytes");
        file.close();
        QVERIFY(MediaCache::saveLocalFilePath(url, local, "account-a"));
        QCOMPARE(MediaCache::localFilePath(url, "account-a"), local);
        QVERIFY(MediaCache::localFilePath(url, "account-b").isEmpty());
        QVERIFY(MediaCache::localFilePath(QUrl("https://example.test/other.pdf"), "account-a").isEmpty());
        QVERIFY(file.remove());
        QVERIFY(MediaCache::localFilePath(url, "account-a").isEmpty());
    }

    void binaryRoundTrip()
    {
        const QByteArray bytes = QByteArray("private-media\0bytes", 19) + QByteArray(16000, 'a');
        const auto path = MediaCache::filePath("message", "bin", "user-a");
        QVERIFY(QFileInfo(path).suffix().isEmpty());
        QVERIFY(path.contains("/user_data/media_cache/"));
        QVERIFY(!path.startsWith(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + '/'));
        QVERIFY(MediaCache::save(path, bytes));
        QCOMPARE(MediaCache::read(path), bytes);
        QVERIFY(MediaCache::exists(path));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto encrypted = file.readAll();
        QVERIFY(encrypted.startsWith("ESBS0001"));
        QVERIFY(!encrypted.contains("private-media"));
        QVERIFY(!encrypted.contains(QByteArray(128, 'a')));
        QVERIFY(MediaCache::read(MediaCache::filePath("message", "bin", "user-b")).isEmpty());
        QVERIFY(MediaCache::save(path, "replacement"));
        QCOMPARE(MediaCache::read(path), QByteArray("replacement"));
        QVERIFY(!MediaCache::save(path, {}));
        QCOMPARE(MediaCache::read(path), QByteArray("replacement"));
        QVERIFY(MediaCache::save(MediaCache::voicePath("voice", "wav", "user-a"), bytes));
    }

    void corruptCacheIsMiss()
    {
        const auto path = MediaCache::imagePath("corrupt", "jpg", "user-a");
        QVERIFY(MediaCache::save(path, "some bytes"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("not encrypted data");
        file.close();
        QVERIFY(MediaCache::read(path).isEmpty());
        QVERIFY(!MediaCache::exists(path));
        QVERIFY(MediaCache::imageUrl(path).isEmpty());
    }

    void migratesExtension()
    {
        const auto path = MediaCache::imagePath("old-extension", "jpg", "sender");
        const auto legacy = path + QStringLiteral(".media");
        QVERIFY(MediaCache::save(legacy, "encrypted image bytes"));
        QCOMPARE(MediaCache::imagePath("old-extension", "jpg", "sender"), path);
        QVERIFY(!QFileInfo::exists(legacy));
        QCOMPARE(MediaCache::read(path), QByteArray("encrypted image bytes"));
        QVERIFY(MediaCache::isCachePath(path));
        QVERIFY(!MediaCache::isCachePath(QFileInfo(path).dir().filePath("unrelated")));
    }

    void jpegStaysEncrypted()
    {
        const auto input = directory.filePath("original.png");
        QImage original(300, 200, QImage::Format_ARGB32);
        original.fill(Qt::transparent);
        QVERIFY(original.save(input));
        const auto path = core::media::ImageCompress::compressImageToCache(input, "sent-0", "sender", 150, 150, 85).result();
        QCOMPARE(path, MediaCache::imagePath("sent-0", "jpg", "sender"));
        const auto bytes = MediaCache::read(path);
        QVERIFY(bytes.startsWith(QByteArray::fromHex("ffd8ff")));
        const auto decoded = QImage::fromData(bytes, "jpg");
        QCOMPARE(decoded.size(), QSize(150, 100));
        QCOMPARE(decoded.pixelColor(0, 0), QColor(Qt::white));
        const auto source = MediaCache::imageUrl(path).toString();
        QCOMPARE(QByteArray::fromBase64(source.mid(source.indexOf(',') + 1).toLatin1()), bytes);
        QFile disk(path);
        QVERIFY(disk.open(QIODevice::ReadOnly));
        QVERIFY(!disk.readAll().contains(bytes));
        const auto legacy = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath("media/images/sent-0.jpg");
        QVERIFY(!QFileInfo::exists(legacy));
    }

    void uploadDecryptedBytes()
    {
        const auto path = MediaCache::imagePath("sent-0", "jpg", "sender");
        const auto bytes = MediaCache::read(path);
        QVERIFY(!bytes.isEmpty());
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QByteArray received;
        connect(&server, &QTcpServer::newConnection, this, [&] {
            auto *socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QTcpSocket::readyRead, this, [&, socket] {
                received += socket->readAll();
                const auto header = received.indexOf("\r\n\r\n");
                if (header >= 0 && received.size() - header - 4 >= bytes.size()) {
                    socket->write("HTTP/1.1 201 Created\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                    socket->disconnectFromHost();
                }
            });
        });
        auto promise = std::make_shared<QPromise<core::xmpp::UploadedFile>>();
        promise->start();
        auto future = promise->future();
        auto *operation = new core::xmpp::FileUploadOperation(promise, this);
        QVERIFY(operation->openData(bytes));
        QCOMPARE(operation->size(), bytes.size());
        const QUrl url(QStringLiteral("http://127.0.0.1:%1/image.jpg").arg(server.serverPort()));
        operation->put(QNetworkRequest(url), url, [](qint64, qint64) {});
        QTRY_VERIFY(future.isFinished());
        QVERIFY(future.result().error.isEmpty());
        QCOMPARE(received.mid(received.indexOf("\r\n\r\n") + 4), bytes);
    }

    void encryptedVoicePlayback()
    {
        QByteArray wave;
        QDataStream stream(&wave, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream.writeRawData("RIFF", 4);
        stream << quint32(32036);
        stream.writeRawData("WAVEfmt ", 8);
        stream << quint32(16) << quint16(1) << quint16(1) << quint32(16000)
               << quint32(32000) << quint16(2) << quint16(16);
        stream.writeRawData("data", 4);
        stream << quint32(32000);
        wave.append(QByteArray(32000, '\0'));
        const auto path = MediaCache::voicePath("playback", "wav", "sender");
        QVERIFY(MediaCache::save(path, wave));
        core::media::AudioPlayer player;
        const auto source = QUrl::fromLocalFile(path);
        player.setSource(source);
        QTRY_COMPARE(player.duration(), 1000);
        QCOMPARE(player.source(), source);
        QCOMPARE(player.error(), QMediaPlayer::NoError);
        player.setSource(QUrl());
    }

    void migratesLegacyImage()
    {
        const auto legacy = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                                .filePath("media/images/legacy-0.jpg");
        QVERIFY(QDir().mkpath(QFileInfo(legacy).absolutePath()));
        QFile file(legacy);
        QVERIFY(file.open(QIODevice::WriteOnly));
        const auto bytes = MediaCache::read(MediaCache::imagePath("sent-0", "jpg", "sender"));
        QCOMPARE(file.write(bytes), bytes.size());
        file.close();
        QVERIFY(MediaCache::migrateLegacyImage("legacy-0", "sender"));
        QVERIFY(!QFileInfo::exists(legacy));
        QCOMPARE(MediaCache::read(MediaCache::imagePath("legacy-0", "jpg", "sender")), bytes);
        QVERIFY(!MediaCache::migrateLegacyImage("../outside", "sender"));
    }

    void cleanupTestCase()
    {
        QDir data(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        QVERIFY(data.removeRecursively());
        QDir cache(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        QVERIFY(cache.removeRecursively());
    }
};

QTEST_GUILESS_MAIN(MediaCacheTest)
#include "test_media_cache.moc"
