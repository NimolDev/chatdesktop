#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace core {
namespace media {


struct MediaDeviceItem
{
    QString id;
    QString name;
    bool isDefault = false;
};


class MediaDeviceModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit MediaDeviceModel(QObject *parent = nullptr);

    enum Role { IdRole = Qt::UserRole + 1, NameRole, IsDefaultRole };
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setDevices(QVector<MediaDeviceItem> devices);

    [[nodiscard]]
    int indexOf(const QString &device_id) const;

    [[nodiscard]]
    QString idAt(int index) const;

    [[nodiscard]]
    bool isEmpty() const;
private:
    QVector<MediaDeviceItem> m_devices;
signals:
};
} // namespace media
} // namespace core
