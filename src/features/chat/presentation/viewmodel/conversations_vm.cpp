#include "conversations_vm.hpp"


ConversationsVM *ConversationsVM::s_instance = nullptr;

ConversationsVM::ConversationsVM(
     std::shared_ptr<domain::usecase::ConversationsUsecase> usecase,
    QObject *parent
    )
    : m_usecase(std::move (usecase)),
    QObject(parent)
{}

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
    m_usecase->execute ();
}

