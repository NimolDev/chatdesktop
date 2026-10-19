#include "attachment_model.hpp"

#include <QFileInfo>
#include <QUuid>

AttachmentModel::AttachmentModel(QObject *parent)
    : QAbstractListModel{parent}
{}

void AttachmentModel::addFiles(const QList<QUrl> &urls, bool compress)
{
    beginResetModel ();
    for (const auto &url : std::as_const (urls)) {
        QFileInfo info(url.toLocalFile ());

        domain::AttachmentItem item;

        item.id = QUuid::createUuid ().toString (QUuid::WithoutBraces);
        item.url = url;
        item.compress = compress;
        item.file_name = info.fileName ();
        item.file_size = info.size ();
        item.state = domain::AttachmentItem::State::Pending;

        m_items.append (std::move (item));
    }
    endResetModel ();
}

void AttachmentModel::setProgress(const QString &id, double progress)
{
    for (const auto item : m_items) {
        if (id == item.id) {
            // m_items.removeOne (item);
        }
    }
}

void AttachmentModel::setState(const QString &id, domain::AttachmentItem::State state)
{

}

void AttachmentModel::setError(const QString &id, const QString &error)
{

}

void AttachmentModel::remove(const QString &id)
{
    for (qsizetype i = 0; i < m_items.size(); ++i) {
        if (m_items[i].id != id) {
            continue;
        }
        beginRemoveRows(QModelIndex(), i, i);
        m_items.removeAt(i);
        endRemoveRows();
        return ;
    }
    return;
}

void AttachmentModel::resetModel()
{
    beginResetModel ();
    m_items = {};
    endResetModel ();
}

const domain::AttachmentItem *AttachmentModel::find(const QString &id) const
{
    return nullptr;
}

QList<domain::AttachmentItem> AttachmentModel::getItems() const
{
    return m_items;
}


int AttachmentModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid ()) {
        return 0;
    }
    return m_items.count ();
}

QVariant AttachmentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid () || index.row () < 0 || index.row () >= m_items.count ()) {
        return {};
    }
    const auto &item = m_items[index.row ()];
    switch (role) {
    case IdRole:
        return item.id;
    case UrlRole:
        return item.url;
    case FileNameRole:
        return item.file_name;
    case FileSizeRole:
        return item.file_size;
    case ProgressRole:
        return item.progress;
    case StateRole:
        switch (item.state) {
        case domain::AttachmentItem::State::Pending:
            return "pending";
        case domain::AttachmentItem::State::Uploading:
            return "uploading";
        case domain::AttachmentItem::State::Uploaded:
            return "uploaded";
        case domain::AttachmentItem::State::Failed:
            return "failed";
        case domain::AttachmentItem::State::Canceled:
            return "canceld";
        default:
            return "unkwon";
        }
    case CompressRole:
        return item.compress;
    case ErrorRole:
        return item.error;
    default: return {};
    }
}

QHash<int, QByteArray> AttachmentModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {UrlRole, "url"},
        {FileNameRole, "fileName"},
        {FileSizeRole, "fileSize"},
        {CompressRole, "compress"},
        {ProgressRole, "progress"},
        {StateRole, "state"},
        {ErrorRole, "error"}
    };
}