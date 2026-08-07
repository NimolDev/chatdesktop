#ifndef CORE_STORAGE_DATABASE_HELPER_HPP
#define CORE_STORAGE_DATABASE_HELPER_HPP

#include "database_manager.hpp"


#include <QString>
#include <QSqlDatabase>
#include <QtSql/qsqlquery.h>

namespace core {
namespace storage {
class DatabaseHelper
{
public:

    static QSqlQuery getUserQuery();


};


} // namespace storage
} // namespace name

#endif // CORE_STORAGE_DATABASE_HELPER_HPP
