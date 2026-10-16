 set(CHAT_QML_FILES
    presentation/view/ImageViewWindow.qml
    presentation/view/ChatPage.qml
    presentation/view/ListUser.qml
    presentation/view/MessagingEmpty.qml
    presentation/view/ListMessaging.qml
    presentation/view/components/ListUserItem.qml
    presentation/view/components/MessageBubble.qml
    presentation/view/components/TextMessageView.qml
    presentation/view/components/ImageMessageView.qml
    presentation/view/components/FileMessageView.qml
    presentation/view/components/StickerView.qml
    presentation/view/components/ToolbarButton.qml
    presentation/view/components/TextCounter.qml
    presentation/view/components/EmojiPopup.qml
    presentation/view/components/RigthSideContainer.qml
    presentation/view/components/Attachment.qml
    presentation/view/components/AttachmentUploadDialog.qml
    presentation/view/components/MapView.qml
    presentation/view/components/MessagingToolbar.qml
    presentation/view/components/MessagingBody.qml
    presentation/view/components/MessageComposer.qml
    presentation/view/components/MessageContextMenu.qml
    presentation/view/components/FileDropOverlay.qml
    presentation/view/components/UserProfilerView.qml
    presentation/view/components/VoiceMessageView.qml
)

protect_qml_files(${CHAT_QML_FILES})

qt_add_qml_module(chat
    STATIC
    URI Features.Chat
    VERSION 1.0
    QML_FILES
        ${CHAT_QML_FILES}
    SOURCES

        data/repository/conversations_repository_impl.hpp
        data/repository/conversations_repository_impl.cpp
        data/dto/message_dto.hpp
        data/repository/message_repository_impl.hpp
        data/repository/message_repository_impl.cpp
        data/dto/message_payload_dto.hpp
        data/mapping/message_payload_mapper.hpp
        data/dto/message_store.hpp
        data/dto/conversations_dto.hpp

        domain/messaging.hpp
        domain/repository/conversation_list_repository.hpp
        domain/repository/message_repository.hpp
        domain/repository/upload_cancellation.hpp
        domain/entity/messags.hpp
        domain/entity/conversation_list.hpp
        domain/entity/messag_payload.hpp
        domain/usecase/conversations_usecase.hpp
        domain/usecase/conversations_usecase.cpp
        domain/usecase/message_usecase.hpp
        domain/usecase/message_usecase.cpp
        domain/usecase/send_message_usecase.hpp
        domain/usecase/send_message_usecase.cpp

        presentation/view/bubble.hpp
        presentation/view/bubble.cpp
        presentation/viewmodel/messaging_vm.hpp
        presentation/viewmodel/messaging_vm.cpp
        presentation/viewmodel/conversations_vm.hpp
        presentation/viewmodel/conversations_vm.cpp
        presentation/viewmodel/home_chat_vm.hpp
        presentation/viewmodel/home_chat_vm.cpp
        SOURCES domain/repository/media_repository.hpp
        SOURCES data/repository/media_repository_impl.hpp data/repository/media_repository_impl.cpp
        SOURCES presentation/viewmodel/voice_message_controller.hpp presentation/viewmodel/voice_message_controller.cpp
        SOURCES presentation/viewmodel/attachment_model.hpp presentation/viewmodel/attachment_model.cpp
        SOURCES domain/entity/attachment_item.hpp
        SOURCES presentation/coordinator/image_view_coordinator.hpp presentation/coordinator/image_view_coordinator.cpp

)

target_include_directories(chat
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        "${CMAKE_CURRENT_SOURCE_DIR}/data"
        "${CMAKE_CURRENT_SOURCE_DIR}/domain"
        "${CMAKE_CURRENT_SOURCE_DIR}/presentation/coordinator"
        "${CMAKE_CURRENT_SOURCE_DIR}/presentation/viewmodel"
        "${CMAKE_CURRENT_SOURCE_DIR}/presentation/view"
)

target_link_libraries(chat
    PUBLIC
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
        network
        constants
        core::media
        core::storage
        core::platform
        # localization
        # theme
)

find_package(Qt6 REQUIRED COMPONENTS ShaderTools)
qt_add_shaders(chat "chat_message_effects"
    PREFIX "/chat/shaders"
    BASE "presentation/view/shaders"
    FILES "presentation/view/shaders/message_dissolve.frag"
)
