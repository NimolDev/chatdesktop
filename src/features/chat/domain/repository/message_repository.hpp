#ifndef FEATURES_CHAT_DOMAIN_REPOSITORY_MESSAGE_REPOSITORY_HPP
#define FEATURES_CHAT_DOMAIN_REPOSITORY_MESSAGE_REPOSITORY_HPP

#include <QObject>
#include <QFuture>


// #include "domain/entity/messags.hpp"
#include "domain/entity/messag_payload.hpp"


namespace domain {
namespace repository {

class MessageRepository : public QObject
{
    Q_OBJECT

public:
    explicit MessageRepository(QObject *parent = nullptr) : QObject(parent) {};
    virtual ~MessageRepository() = default;
    virtual QFuture<QList<domain::entity::Payload>>fetchMessageById(QString user_id) = 0;
    virtual void sendMessage(domain::entity::Payload &payload) = 0;

public:
    virtual QString currentUserId() const  = 0;
signals:
    void messageReceived(const domain::entity::Payload &payload);
    void messageSent(const domain::entity::Payload &payload);
};

} // namespace repository
} // namespace domain



#endif // FEATURES_CHAT_DOMAIN_REPOSITORY_MESSAGE_REPOSITORY_HPP
