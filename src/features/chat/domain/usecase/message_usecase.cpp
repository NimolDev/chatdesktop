#include "message_usecase.hpp"

namespace domain {
namespace usecase {

MessageUsecase::MessageUsecase(std::shared_ptr<repository::MessageRepository> repository)
    : m_repository(std::move (repository))
{}

QFuture<entity::Message> MessageUsecase::execute(QString user_id)
{
    return m_repository->fetchMessageById (user_id);
}


} // namespace usecase
} // namespace domain

