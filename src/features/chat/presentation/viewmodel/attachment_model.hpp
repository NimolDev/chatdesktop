#pragma once

#include <domain/entity/attachment_item.hpp>

#include <QObject>
#include <QAbstractListModel>

class AttachmentModel : public QAbstractListModel
{
    Q_OBJECT
public:

    enum Role {
        IdRole = Qt::UserRole + 1,
        UrlRole,
        FileNameRole,
        FileSizeRole,
        CompressRole,
        ProgressRole,
        StateRole,
        ErrorRole,
    };
    explicit AttachmentModel(QObject *parent = nullptr);

    void addFiles(const QList<QUrl> &urls, bool compress = false);
    void setProgress(const QString &id, double progress);
    void setState(const QString &id, domain::AttachmentItem::State state);
    void setError(const QString &id, const QString &error);
    void remove(const QString &id);
    void resetModel();
    const domain::AttachmentItem *find(const QString  &id) const;

    QList<domain::AttachmentItem> getItems() const;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
private:
    QList<domain::AttachmentItem> m_items;


};