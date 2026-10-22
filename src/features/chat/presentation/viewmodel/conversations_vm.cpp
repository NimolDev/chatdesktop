#include "conversations_vm.hpp"

ConversationsVM::ConversationsVM(
     std::shared_ptr<domain::usecase::ConversationsUsecase> usecase,
    QObject *parent
    )
    : m_usecase(std::move (usecase)),
    QAbstractListModel(parent)
{
    qDebug() << "ConverationVM instantiate";
    connect(&m_watcher,
            &QFutureWatcher<QList<domain::entity::ConversationList>>::resultReadyAt,
            this, [this](int index) {
                beginResetModel();
                m_conversations = m_watcher.resultAt(index);
                endResetModel();
                m_isLoading = false;
                emit isLoadingChanged();
            });
    connect(&m_watcher,
            &QFutureWatcher<QList<domain::entity::ConversationList>>::finished,
            this, &ConversationsVM::onFinished);
}

void ConversationsVM::fetchConversations()
{
    if (m_watcher.isRunning()) {
        m_watcher.cancel();
    }
    m_isLoading = true;
    emit isLoadingChanged();
    m_watcher.setFuture(m_usecase->execute());
}

bool ConversationsVM::isLoading() const
{
    return m_isLoading;
}

int ConversationsVM::rowCount(const QModelIndex &parent) const
{

    if (parent.isValid ()) {
        qDebug() << "Parent invalide";
        return 0;
    }
    return m_conversations.size();
}

QVariant ConversationsVM::data(const QModelIndex &index, int role) const
{
    if (!index.isValid ()
        || index.row () < 0
        || index.row () >= m_conversations.size ()
        ) {
        return {};
    }

    const auto &conversation = m_conversations[index.row ()];
    switch (role) {
    case UuidRole:
        return conversation.uuid;
    case NameRole:
        return conversation.name;
    default:
        return {};
    }
}


QHash<int, QByteArray> ConversationsVM::roleNames() const
{
    return {
        { UuidRole, "uuid" },
        { NameRole, "name" }
    };
}

void ConversationsVM::onFinished()
{
    m_isLoading = false;
    emit isLoadingChanged();
}
