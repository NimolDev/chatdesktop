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
    virtual QFuture<domain::entity::MessageResponse>fetchMessageById(QString user_id, int page) = 0;
    virtual void sendMessage(domain::entity::Payload &payload) = 0;

public:
    virtual QString currentUserId() const  = 0;
signals:
    void messageReceived(const domain::entity::MessageItem &payload);
    void messageSent(const domain::entity::MessageItem &payload);
};

} // namespace repository
} // namespace domain



#endif // FEATURES_CHAT_DOMAIN_REPOSITORY_MESSAGE_REPOSITORY_HPP
