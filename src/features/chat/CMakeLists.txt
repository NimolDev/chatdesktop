set(CHAT_QML_FILES
    presentation/view/ChatPage.qml
    presentation/view/ListUser.qml
    presentation/view/components/ListUserItem.qml
    presentation/view/MessagingEmpty.qml
)

protect_qml_files(${CHAT_QML_FILES})

qt_add_qml_module(chat
    URI Features.Chat
    VERSION 1.0
    QML_FILES
        ${CHAT_QML_FILES}
        QML_FILES presentation/view/ListMessaging.qml
)

target_include_directories(chat
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        # "${CMAKE_CURRENT_SOURCE_DIR}/presentation/view"
        # "${CMAKE_CURRENT_SOURCE_DIR}/presentation/viewmodel"
        # "${CMAKE_CURRENT_SOURCE_DIR}/domain"
        # "${CMAKE_CURRENT_SOURCE_DIR}/data"
)

target_link_libraries(chat
    PUBLIC
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
        # network
        # xmpp
        # localization
        # theme
)
