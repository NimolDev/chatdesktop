#pragma once

#include <QJsonObject>
#include <QStringList>
#include <functional>
#include <optional>


namespace core {
namespace storage {


class MessageCache
{
public:
    explicit MessageCache();

    static bool save(const QString &objects, const QString &logical_name, const QString &secret_password);
    // Returns saved batches in insertion order. A missing cache is empty;
    // an unreadable or corrupt cache returns std::nullopt.
    static std::optional<QStringList> read(const QString &logical_name, const QString &secret_password);
    // Page roots index replaceable snapshots inside one conversation file.
    static bool savePage(const QString &objects, const QString &logical_name,
                         const QString &secret_password, int page);
    static std::optional<QString> readPage(const QString &logical_name,
                                          const QString &secret_password, int page);
    // Clears all snapshots and page roots, preserving records with other tags.
    static bool clear(const QString &logical_name, const QString &secret_password);
    // Release cached file handles and encryption keys (for example on logout).
    static void closeAll();
    static bool update(const std::function<bool(QJsonObject &)> &modify);


};


} // namespace storage
} // namespace core
