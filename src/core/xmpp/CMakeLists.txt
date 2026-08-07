add_library(xmpp STATIC
    xmpp_manager.hpp xmpp_manager.cpp
    xmpp_service_discovery.cpp xmpp_service_discovery.hpp
)
add_library(core::xmpp ALIAS xmpp)

target_link_libraries(xmpp
    PUBLIC
        Qt6::Core
        Qt6::Network
        QXmppQt6
)

target_include_directories(xmpp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)