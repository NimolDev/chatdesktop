 set(CHAT_QML_FILES
    presentation/view/ChatPage.qml
    presentation/view/ListUser.qml
    presentation/view/components/ListUserItem.qml
    presentation/view/MessagingEmpty.qml
    presentation/view/ListMessaging.qml
    presentation/view/components/MessageBubble.qml
    presentation/view/components/ToolbarButton.qml
    presentation/view/components/TextCounter.qml
    presentation/view/components/EmojiPopup.qml
    presentation/view/components/RigthSideContainer.qml
    presentation/view/components/Attachment.qml
    presentation/view/components/MapView.qml
    presentation/view/components/MessagingToolbar.qml
    presentation/view/components/MessagingBody.qml
    presentation/view/components/MessageComposer.qml
    presentation/view/components/MessageContextMenu.qml
    presentation/view/components/FileDropOverlay.qml
    presentation/view/components/UserProfilerView.qml
)

protect_qml_files(${CHAT_QML_FILES})

qt_add_qml_module(chat
    STATIC
    URI Features.Chat
    VERSION 1.0
    QML_FILES
        ${CHAT_QML_FILES}
    SOURCES
        presentation/view/bubble.hpp
        presentation/view/bubble.cpp
        presentation/viewmodel/messaging_view_model.hpp
        presentation/viewmodel/messaging_view_model.cpp
        domain/messaging.hpp

        domain/repository/conversation_list_repository.hpp
        domain/entity/conversation_list.hpp
        domain/usecase/conversations_usecase.hpp
        domain/usecase/conversations_usecase.cpp
        data/repository/conversations_repository_impl.hpp
        data/repository/conversations_repository_impl.cpp
        presentation/viewmodel/conversations_vm.hpp
        presentation/viewmodel/conversations_vm.cpp
        SOURCES data/dto/conversations_dto.hpp
)

target_include_directories(chat
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        "${CMAKE_CURRENT_SOURCE_DIR}/presentation/view"
        "${CMAKE_CURRENT_SOURCE_DIR}/presentation/viewmodel"
        "${CMAKE_CURRENT_SOURCE_DIR}/domain"
        "${CMAKE_CURRENT_SOURCE_DIR}/data"
)

target_link_libraries(chat
    PUBLIC
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
        network
        constants
        # xmpp
        # localization
        # theme
)
