#include "home_chat_vm.hpp"

HomeChatVM *HomeChatVM::s_instance = nullptr;

HomeChatVM *HomeChatVM::create(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED (scriptEngine);
    Q_ASSERT (s_instance);
    Q_ASSERT (s_instance->thread() == engine->thread ());

    QQmlEngine::setObjectOwnership (s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

void HomeChatVM::setInstance(HomeChatVM *instance)
{
    s_instance = instance;
}

HomeChatVM::HomeChatVM(
    std::shared_ptr<domain::repository::MessageRepository> repository,
    QObject *parent)
    : m_repository(std::move (repository)),
    QObject(parent)
{

    connect(
        m_repository.get (),
        &domain::repository::MessageRepository::messageReceived,
        this,
        [this](const domain::entity::MessageItem &payload) {
            emit messageReceived (payload);
        }
        );

}

void HomeChatVM::setVisible(bool value)
{
    qDebug() << "Visible: " << value;
    if (m_visible == value) {
        return;
    }
    m_visible = value;
    emit visibleChanged ();
}

