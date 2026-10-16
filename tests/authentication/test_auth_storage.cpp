#include "local/login_response_store.hpp"
#include "constants/app_constants.hpp"
#include "storage/database_helper.hpp"
#include "storage/file_name_helper.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QUuid>

#include <iostream>
#include <stdexcept>

// Use a test-only key; no application secrets or SQL libraries are needed.
namespace core {
namespace constants {
QString AppConstants::appSecretKey()
{
    return QStringLiteral("auth-storage-test-key");
}
} // namespace constants
} // namespace core

using data::local::LoginResponseStore;

namespace {

void check(bool condition, const char *message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

QString authPath(const QString &id = QStringLiteral("42"), const QString &username = QStringLiteral("nimol"))
{
    const QString secret = QString::fromLatin1((id + username).toUtf8().toBase64());
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(QStringLiteral("user_data/") +
                  core::storage::FileNameHelper::hexName(username, secret));
}

QString indexPath()
{
    return QFileInfo(authPath()).dir().filePath(
        core::storage::FileNameHelper::hexName(QStringLiteral("active_auth")));
}

data::dto::LoginResponseDTO response()
{
    return {
        .access_token = QStringLiteral("private-access-token"),
        .refresh_token = QStringLiteral("private-refresh-token"),
        .user = {QStringLiteral("42"), QStringLiteral("nimol"), QStringLiteral("និមល 世界")},
        .xmpp = {QStringLiteral("nimol@example.test"), QStringLiteral("xmpp.example.test"), 5222}
    };
}

void checkUser(bool refreshed)
{
    const auto user = LoginResponseStore::getLocalUser();
    const auto expected = response();
    check(user.has_value(), "Session not restored");
    const auto identity = core::storage::DatabaseHelper::getCurrentUser();
    check(identity && identity->username == expected.user.username && identity->user_id == expected.user.id,
          "App and chat identity not restored from local storage");
    check(user->access_token == (refreshed ? "new-access" : expected.access_token), "Access token mismatch");
    check(user->refresh_token == (refreshed ? "new-refresh" : expected.refresh_token), "Refresh token mismatch");
    check(user->user_id == expected.user.id && user->username == expected.user.username &&
          user->display_name == expected.user.display_name, "User profile mismatch");
    check(user->password == QStringLiteral("private-password-សម្ងាត់"), "XMPP password missing");
    check(user->xmpp_jid == expected.xmpp.jid && user->xmpp_host == expected.xmpp.host &&
          user->xmpp_port == expected.xmpp.port, "XMPP connection settings mismatch");
}

void stage(const QString &name)
{
    if (name == "save") {
        check(!LoginResponseStore::getLocalUser(), "Unexpected initial session");
        data::dto::TokenResponseDto token{"new-access", "new-refresh"};
        check(!LoginResponseStore::updateToken(token), "Updated missing session");
        check(LoginResponseStore::save(response(), QStringLiteral("private-password-សម្ងាត់")), "Save failed");
        checkUser(false);
        QFile index(indexPath());
        check(index.open(QIODevice::ReadOnly), "Startup index missing");
        check(index.readAll() == QFileInfo(authPath()).fileName().toLatin1(), "Index must contain only the derived hex filename");
        QFile file(authPath());
        check(file.open(QIODevice::ReadOnly), "Cannot inspect encrypted store");
        const auto raw = file.readAll();
        check(!raw.contains("private-access-token") && !raw.contains("private-refresh-token") &&
              !raw.contains("private-password"), "Credentials saved as plaintext");
    } else if (name == "failed-save") {
        checkUser(false);
        const QString directory = QFileInfo(authPath()).absolutePath();
        const auto permissions = QFile::permissions(directory);
        check(QFile::setPermissions(directory, QFileDevice::ReadOwner | QFileDevice::ExeOwner),
              "Cannot make directory read-only");
        const bool saved = LoginResponseStore::save(response(), "must-not-replace-password");
        check(QFile::setPermissions(directory, permissions), "Cannot restore directory permissions");
        check(!saved, "Save unexpectedly succeeded in a read-only directory");
        checkUser(false);
    } else if (name == "refresh") {
        checkUser(false);
        const QString legacy = QFileInfo(authPath()).dir().filePath(QStringLiteral("auth.store"));
        check(QFile::rename(authPath(), legacy), "Cannot prepare legacy filename");
        check(QFile::remove(indexPath()), "Cannot prepare legacy lookup");
        checkUser(false);
        check(QFile::exists(authPath()) && !QFile::exists(legacy), "Legacy filename was not migrated");
        data::dto::TokenResponseDto token{"new-access", "new-refresh"};
        check(LoginResponseStore::updateToken(token), "Token refresh save failed");
    } else if (name == "replace") {
        checkUser(true);
        auto next = response();
        next.user.id = "99";
        next.user.username = "another-user";
        check(LoginResponseStore::save(next, "another-password"), "Replacement save failed");
        check(QFile::exists(authPath("99", "another-user")), "New account filename is incorrect");
        check(!QFile::exists(authPath()), "Previous account snapshot was not removed");
    } else if (name == "logout") {
        const auto user = LoginResponseStore::getLocalUser();
        check(user && user->user_id == "99" && user->password == "another-password", "Old account restored");
        check(LoginResponseStore::logout(), "Logout failed");
        check(!QFile::exists(authPath("99", "another-user")), "Logout did not remove the active snapshot");
    } else if (name == "empty") {
        check(!LoginResponseStore::getLocalUser(), "Logged-out session restored");
        check(!core::storage::DatabaseHelper::getCurrentUser(), "Logged-out chat identity restored");
        check(LoginResponseStore::logout(), "Repeated logout failed");
        data::dto::TokenResponseDto token{"new-access", "new-refresh"};
        check(!LoginResponseStore::updateToken(token), "Refresh recreated logged-out session");
        QFile corrupt(authPath());
        check(corrupt.open(QIODevice::WriteOnly), "Cannot create corruption fixture");
        corrupt.write("corrupt auth file");
        QFile index(indexPath());
        check(index.open(QIODevice::WriteOnly | QIODevice::Truncate), "Cannot prepare corrupt snapshot lookup");
        index.write(QFileInfo(authPath()).fileName().toLatin1());
    } else if (name == "corrupt") {
        check(!LoginResponseStore::getLocalUser(), "Corrupted session accepted");
        data::dto::TokenResponseDto token{"new-access", "new-refresh"};
        check(!LoginResponseStore::updateToken(token), "Corrupted session updated");
        check(LoginResponseStore::save(response(), QStringLiteral("private-password-សម្ងាត់")), "Cannot replace corrupt session");
        checkUser(false);
    } else {
        throw std::runtime_error("Unknown stage");
    }
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    const auto args = app.arguments();
    const bool child = args.size() == 3;
    app.setApplicationName(child ? args[1] : "ChatDesktopAuthTest-" + QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    int result = 0;
    try {
        if (child) {
            stage(args[2]);
        } else {
            for (const auto *name : {"save", "failed-save", "refresh", "replace", "logout", "empty", "corrupt"}) {
                QProcess process;
                process.setProcessChannelMode(QProcess::ForwardedChannels);
                process.start(app.applicationFilePath(), {app.applicationName(), QString::fromLatin1(name)});
                check(process.waitForFinished(15000) && process.exitStatus() == QProcess::NormalExit &&
                      process.exitCode() == 0, "Auth storage subprocess failed");
            }
            std::cout << "Auth storage restart tests passed\n";
        }
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    if (!child) {
        QDir(directory).removeRecursively();
    }
    return result;
}
