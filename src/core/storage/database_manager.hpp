#ifndef CORE_STORAGE_DATABASE_MANAGER_HPP
#define CORE_STORAGE_DATABASE_MANAGER_HPP

#include <QString>
#include <QDebug>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

namespace core{
namespace storage {

class DatabaseManager
{
public:
    static bool initialize();
    static void close();

    [[nodiscard]]
    static QString connectionName();

private:
    static bool createDirectory();
    static bool createTables();
};
} // namespace storage
} // namespace core


#endif // CORE_STORAGE_DATABASE_MANAGER_HPP
