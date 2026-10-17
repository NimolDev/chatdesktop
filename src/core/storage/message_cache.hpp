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
    // Pending messages share the conversation file with fetched pages.
    static bool savePending(const QString &objects, const QString &logical_name,
                            const QString &secret_password);
    static std::optional<QString> readPending(const QString &logical_name,
                                              const QString &secret_password);
    // Clears all snapshots and page roots, preserving records with other tags.
    static bool clear(const QString &logical_name, const QString &secret_password);
    // Release cached file handles and encryption keys (for example on logout).
    static void closeAll();
    static bool update(const std::function<bool(QJsonObject &)> &modify);

private:
    static bool saveSnapshot(const QString &objects, const QString &logical_name,
                             const QString &secret_password, const QString &root);
    static std::optional<QString> readSnapshot(const QString &logical_name,
                                               const QString &secret_password, const QString &root);


};


} // namespace storage
} // namespace core
