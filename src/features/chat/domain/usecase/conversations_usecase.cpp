#include "conversations_usecase.hpp"

namespace domain {
namespace usecase {

ConversationsUsecase::ConversationsUsecase(
    std::shared_ptr<repository::ConversationListRepository> repository
    )
    : m_repository(std::move (repository))
{}

QFuture<QList<domain::entity::ConversationList>> ConversationsUsecase::execute()
{
    return m_repository->fetchConversations ();
}


} // namespace usecase
} // namespace domain