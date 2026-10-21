#include "cache_storage.hpp"


#include "file_name_helper.hpp"

#include <QString>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
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
    const QString file = QDir(m_storagePath).filePath(FileNameHelper::hexName(logical_name, secret_key));
    m_storage->setPath(file.toStdString(), constants::AppConstants::appSecretKey().toStdString());
    const auto records = m_storage->ids(USER);
    if (records.empty()) {
        m_storage->insert(value.toStdString(), USER);
    } else {
        if (!m_storage->update(records.back(), value.toStdString())) {
            return false;
        }
        // Older versions appended snapshots. Retain only the latest one.
        for (const auto id : records) {
            if (id != records.back()) {
                m_storage->remove(id);
            }
        }
    }
    m_storage->sync();
    return true;
}

QString CacheStorage::getUser(const QString &logical_name, const QString &secret_key)
{
    const QString file = QDir(m_storagePath).filePath(FileNameHelper::hexName(logical_name, secret_key));
    if (!QFileInfo::exists(file)) {
        return {};
    }
    m_storage->setPath(file.toStdString(), constants::AppConstants::appSecretKey().toStdString());
    const auto records = m_storage->ids(USER);
    if (!records.empty()) {
        const auto value = m_storage->get(records.back());
        if (value) {
            return QString::fromStdString(*value);
        }
    }
    return {};
}

QString CacheStorage::read(const QString &logical_name, const QString &secret_key)
{
    const QString file = QDir(m_storagePath).filePath(FileNameHelper::hexName(logical_name, secret_key));
    if (!QFileInfo::exists(file)) {
        return {};
    }
    m_storage->setPath(file.toStdString(), constants::AppConstants::appSecretKey().toStdString());
    const auto records = m_storage->ids(USER);
    if (!records.empty()) {
        const auto value = m_storage->get(records.back());
        if (value) {
            return QString::fromStdString(*value);
        }
    }
    return {};
}


} // namespace storage
} // namespace core
