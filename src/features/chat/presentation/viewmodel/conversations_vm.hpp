#ifndef FEATURES_CHAT_PRESENTATION_VIEWMODEL_CONVERSATIONS_VM_HPP
#define FEATURES_CHAT_PRESENTATION_VIEWMODEL_CONVERSATIONS_VM_HPP

#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>
#include <QAbstractListModel>
#include <QFutureWatcher>

#include "domain/usecase/conversations_usecase.hpp"

class ConversationsVM : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    enum Roles {
       UuidRole = Qt::UserRole +1,
     NameRole

    };

    explicit ConversationsVM(
        std::shared_ptr<domain::usecase::ConversationsUsecase> usecase,
        QObject *parent = nullptr);

    static ConversationsVM *create(QQmlEngine *engine, QJSEngine *scriptEngine);
    static void setInstance(ConversationsVM *instance);

    Q_INVOKABLE void fetchConversations();

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:

private:

    void onFinished();
    static ConversationsVM *s_instance;
    std::shared_ptr<domain::usecase::ConversationsUsecase> m_usecase;

    QFutureWatcher<QList<domain::entity::ConversationList>> m_watcher;
    QList<domain::entity::ConversationList> m_conversations;

};

#endif // FEATURES_CHAT_PRESENTATION_VIEWMODEL_CONVERSATIONS_VM_HPP
