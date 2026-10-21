#include "conversations_vm.hpp"



ConversationsVM *ConversationsVM::s_instance = nullptr;

ConversationsVM::ConversationsVM(
     std::shared_ptr<domain::usecase::ConversationsUsecase> usecase,
    QObject *parent
    )
    : m_usecase(std::move (usecase)),
    QAbstractListModel(parent)
{
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

ConversationsVM *ConversationsVM::create(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED (scriptEngine);
    Q_ASSERT (s_instance);
    Q_ASSERT (s_instance->thread () == engine->thread ());

    QQmlEngine::setObjectOwnership (s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

void ConversationsVM::setInstance(ConversationsVM *instance)
{
    Q_ASSERT (instance);
    s_instance = instance;
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
