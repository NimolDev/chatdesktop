add_library(xmpp STATIC
    file_upload_operation.hpp
    file_upload_operation.cpp
    xmpp_manager.hpp
    xmpp_manager.cpp
    xmpp_service_discovery.cpp
    xmpp_service_discovery.hpp
)
add_library(core::xmpp ALIAS xmpp)

target_link_libraries(xmpp
    PUBLIC
        Qt6::Core
        Qt6::Network
        core::storage
        # Qt6::Concurrent
        QXmppQt6
)

target_include_directories(xmpp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(xmpp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
