if(APPLE)
    add_library(platform STATIC
        notification.hpp
        macos/mac_window.hpp
        macos/mac_window.mm
        macos/macos_menu_bar.hpp
        macos/macos_menu_bar.mm
        macos/macos_tray_icon.hpp
        macos/macos_tray_icon.mm
        macos/notification.hpp
        macos/notification.mm
        macos/pet_window.hpp
        macos/pet_window.mm
    )
    target_link_libraries(platform
        PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets
        PRIVATE "-framework AppKit" "-framework UserNotifications"
    )
else()
    # No platform implementation has been added for other systems yet.
    add_library(platform INTERFACE)
    target_link_libraries(platform INTERFACE Qt6::Core Qt6::Gui)
endif()

target_include_directories(platform INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_library(core::platform ALIAS platform)
