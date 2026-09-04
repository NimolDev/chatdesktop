#ifndef FEATURES_CHAT_PRESENTATION_VIEWMODEL_HOME_CHAT_VM_HPP
#define FEATURES_CHAT_PRESENTATION_VIEWMODEL_HOME_CHAT_VM_HPP

#include <QObject>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

#include "domain/repository/message_repository.hpp"

class HomeChatVM : public QObject
{
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON
public:

    static HomeChatVM *create(QQmlEngine *engine, QJSEngine *scriptEngine);
    static void setInstance(HomeChatVM *instance);

    explicit HomeChatVM(
        std::shared_ptr<domain::repository::MessageRepository> repository,
        QObject *parent = nullptr
        );

    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged FINAL)

    bool visible() const { return m_visible; }
    void setVisible(bool value);


signals:

    void visibleChanged();
    void messageReceived(const domain::entity::MessageItem &item);


private:
   static HomeChatVM *s_instance;

    std::shared_ptr<domain::repository::MessageRepository> m_repository;
   bool m_visible = false;
};

#endif // FEATURES_CHAT_PRESENTATION_VIEWMODEL_HOME_CHAT_VM_HPP
