#ifndef FEATURES_CHAT_PRESENTATION_VIEWMODEL_CONVERSATIONS_VM_HPP
#define FEATURES_CHAT_PRESENTATION_VIEWMODEL_CONVERSATIONS_VM_HPP

#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

#include "domain/usecase/conversations_usecase.hpp"

class ConversationsVM : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit ConversationsVM(
        std::shared_ptr<domain::usecase::ConversationsUsecase> usecase,
        QObject *parent = nullptr);

    static ConversationsVM *create(QQmlEngine *engine, QJSEngine *scriptEngine);
    static void setInstance(ConversationsVM *instance);

    Q_INVOKABLE void fetchConversations();
signals:

private:
    static ConversationsVM *s_instance;
    std::shared_ptr<domain::usecase::ConversationsUsecase> m_usecase;
};

#endif // FEATURES_CHAT_PRESENTATION_VIEWMODEL_CONVERSATIONS_VM_HPP
