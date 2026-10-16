#include "cache_storage.hpp"


#include "file_name_helper.hpp"

#include <QString>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>


namespace  {



}
namespace core {
namespace storage {

enum Tag: quint32 {
    USER = 1
};

CacheStorage &CacheStorage::instance()
{
    static CacheStorage instance;
    return instance;

}



CacheStorage::CacheStorage()
{
    const QDir app_data_dir(QStandardPaths::writableLocation (QStandardPaths::AppDataLocation));
    m_storagePath = app_data_dir.filePath ("user_data");
    QDir().mkpath (m_storagePath);
    m_storage = std::make_unique <LocalStorage>();

}

bool CacheStorage::save(const QString &value, const QString &logical_name, const QString &secret_key)
{
    // TODO: Serialize value and persist it using LocalStorage.

    const QString file = QDir(m_storagePath).filePath(FileNameHelper::hexName(logical_name, secret_key));
    qDebug() << "File cache location: " << file;
    // m_storage = std::make_unique<LocalStorage>(
    //     file.toStdString(), constants::AppConstants::appSecretKey().toStdString());
    m_storage->setPath (file.toStdString (), constants::AppConstants::appSecretKey ().toStdString ());
    m_storage->insert(value.toStdString (), USER);

    return true;
}
QString CacheStorage::getUser(const QString &logical_name, const QString &secret_key)
{
    const QString file = QDir(m_storagePath).filePath(FileNameHelper::hexName(logical_name, secret_key));
    qDebug() << "File cache location: " << file;
    m_storage->setPath (file.toStdString (), constants::AppConstants::appSecretKey ().toStdString ());

    auto user = m_storage->ids (USER);
    if (!user.empty ()) {
        qDebug() << "User:" << user;
        auto v = m_storage->get (user[0]);
        if (v) {
            qDebug() << "User: " << *v;
        }
    }
    return "";
}


} // namespace storage
} // namespace core
