#ifndef FEATURES_CHAT_DOMAIN_REPOSITORY_MESSAGE_REPOSITORY_HPP
#define FEATURES_CHAT_DOMAIN_REPOSITORY_MESSAGE_REPOSITORY_HPP

#include <QFuture>
#include "domain/entity/messags.hpp"

namespace domain {
namespace repository {

class MessageRepository
{
public:
    virtual ~MessageRepository() = default;
    virtual QFuture<domain::entity::Message> fetchMessageById(QString user_id) = 0;
};

} // namespace repository
} // namespace domain



#endif // FEATURES_CHAT_DOMAIN_REPOSITORY_MESSAGE_REPOSITORY_HPP
