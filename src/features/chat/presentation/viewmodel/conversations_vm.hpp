#ifndef FEATURES_CHAT_PRESENTATION_VIEWMODEL_CONVERSATIONS_VM_HPP
#define FEATURES_CHAT_PRESENTATION_VIEWMODEL_CONVERSATIONS_VM_HPP

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QAbstractListModel>
#include <QFutureWatcher>

#include "domain/usecase/conversations_usecase.hpp"

class ConversationsVM : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ConversationsVM is provided by the application")
public:
    enum Roles {
       UuidRole = Qt::UserRole +1,
     NameRole

    };

    explicit ConversationsVM(
        std::shared_ptr<domain::usecase::ConversationsUsecase> usecase,
        QObject *parent = nullptr);

    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged FINAL)

    Q_INVOKABLE void fetchConversations();

    bool isLoading() const;

public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void isLoadingChanged();
private:
    void onFinished();
    bool m_isLoading = false;
private:


    std::shared_ptr<domain::usecase::ConversationsUsecase> m_usecase;

    QFutureWatcher<QList<domain::entity::ConversationList>> m_watcher;
    QList<domain::entity::ConversationList> m_conversations;

};

#endif // FEATURES_CHAT_PRESENTATION_VIEWMODEL_CONVERSATIONS_VM_HPP
