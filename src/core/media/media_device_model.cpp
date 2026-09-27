#include "media_device_model.hpp"


core::media::MediaDeviceModel::MediaDeviceModel(QObject *parent)
    : QAbstractListModel{parent}
{}


void core::media::MediaDeviceModel::setDevices(QVector<MediaDeviceItem> devices)
{
    beginResetModel();
    m_devices = std::move(devices);
    endResetModel();
}

int core::media::MediaDeviceModel::indexOf(const QString &device_id) const
{
    for (int i = 0; i < m_devices.size (); i++) {
        if (m_devices.at (i).id == device_id) {
            return i;
        }
    }
    return -1;
}

QString core::media::MediaDeviceModel::idAt(int index) const
{
    if (index < 0 || index >= m_devices.size ()) {
        return {};
    }
    return m_devices.at (index).id;
}


bool core::media::MediaDeviceModel::isEmpty() const
{
    return m_devices.isEmpty ();
}




int core::media::MediaDeviceModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_devices.size();
}

QVariant core::media::MediaDeviceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_devices.size()) {
        return {};
    }
    const auto &device = m_devices.at(index.row());
    switch (role) {
    case IdRole: return device.id;
    case Qt::DisplayRole:
    case NameRole: return device.name;
    case IsDefaultRole: return device.isDefault;
    default: return {};
    }
}

QHash<int, QByteArray> core::media::MediaDeviceModel::roleNames() const
{
    return {{IdRole, "deviceId"}, {NameRole, "name"}, {IsDefaultRole, "isDefault"}};
}
