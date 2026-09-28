add_library(platform STATIC
    application_menu.hpp
    platform_menu.hpp
    platform_notification.hpp)

target_sources(platform
    PRIVATE
        main_window.hpp
        tray_icon.hpp
        notification.hpp
        platform_tray.hpp
        platform_main_window.hpp
)

target_link_libraries(platform
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

target_include_directories(platform
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

if(APPLE)
    enable_language(OBJCXX)

    target_sources(platform
        PRIVATE
            macos/application_menu_mac.hpp
            macos/application_menu_mac.mm
            macos/main_window_mac.hpp
            macos/main_window_mac.mm

            macos/notification_mac.hpp
            macos/notification_mac.mm

            # macos/mac_window.hpp
            # macos/mac_window.mm

            macos/tray_mac.hpp
            macos/tray_mac.mm
    )

    target_link_libraries(platform
        PRIVATE
            "-framework AppKit"
            "-framework UserNotifications"
    )

elseif(WIN32)

    target_sources(platform
        PRIVATE
            win/main_window_win.hpp
            win/main_window_win.cpp

            win/notification_win.hpp
            win/notification_win.cpp

            win/win_tray.hpp
            win/win_tray.cpp
    )

    target_link_libraries(platform PRIVATE dwmapi user32 uxtheme)

endif()

add_library(core::platform ALIAS platform)
