#ifndef FEATURES_CHAT_PRESENTATION_VIEWMODEL_MESSAGING_VIEW_MODEL_HPP
#define FEATURES_CHAT_PRESENTATION_VIEWMODEL_MESSAGING_VIEW_MODEL_HPP

#include "domain/messaging.hpp"

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

class MessagingViewModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        SenderIdRole,
        MsgRole,
        DateRole,
        MsgTypeRole,
        SectionRole

    };
    explicit MessagingViewModel(QObject *parent = nullptr);

    static MessagingViewModel *create(QQmlEngine *engine, QJSEngine *scriptEngine);
    static void setInstance(MessagingViewModel *instance);

signals:

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent = QModelIndex ()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int columnCount(const QModelIndex &parent) const override;


private:

    QVector<domain::Messaging> m_messaging;
    void dummy();

    static MessagingViewModel *s_instance;
};

#endif // FEATURES_CHAT_PRESENTATION_VIEWMODEL_MESSAGING_VIEW_MODEL_HPP
