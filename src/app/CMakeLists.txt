qt_add_executable(ChatApp
    MACOSX_BUNDLE
    source/main.cpp
    ${CMAKE_SOURCE_DIR}/resources/Resources.qrc
)

if(APPLE)
    target_sources(ChatApp
        PRIVATE
            platform/macos/macos_menu_bar.hpp
            platform/macos/macos_menu_bar.mm
            platform/macos/macos_tray_icon.hpp
            platform/macos/macos_tray_icon.mm
            platform/macos/notification.hpp
            platform/macos/notification.mm
            platform/macos/pet_window.hpp
            platform/macos/pet_window.mm

    )
    target_link_libraries(ChatApp PRIVATE "-framework AppKit" "-framework UserNotifications")
endif()

target_compile_definitions(ChatApp
    PRIVATE
        $<$<CONFIG:Debug>:QT_QML_DEBUG>
)
target_include_directories(ChatApp
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/source"
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/third_party"
        "${CMAKE_SOURCE_DIR}/src/shared"
        "${CMAKE_SOURCE_DIR}/src/core"
        "${CMAKE_SOURCE_DIR}/src/features"
)

set(CHATAPP_QML_FILES
    qml/Main.qml
    qml/SplashScreen.qml
    qml/AboutDialog.qml
    qml/component/Menu.qml
)

protect_qml_files(${CHATAPP_QML_FILES})

qt_add_qml_module(ChatApp
    URI ChatApp
    VERSION 1.0
    QML_FILES
        ${CHATAPP_QML_FILES}

    SOURCES
        source/service_container.hpp
        source/app_container.hpp
        source/app_container.cpp
        source/app_controller.hpp
        source/app_controller.cpp
        SOURCES platform/notification.hpp
        QML_FILES qml/PetWindow.qml


    # RESOURCES Info.plist
)


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.

if(APPLE)
    set(APP_ICON ${CMAKE_SOURCE_DIR}/resources/icons/app.icns)
    target_sources(ChatApp PRIVATE ${APP_ICON})
    set_source_files_properties(${APP_ICON}
        PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
    )
set_target_properties(ChatApp PROPERTIES
    MACOSX_BUNDLE_ICON_FILE app.icns
)
endif()


if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Mininum macOS version" FORCE)
endif()

set_target_properties(ChatApp PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "${APP_NAME}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${APP_BUNDLE_IDENTIFIER}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${APP_BUILD_NUMBER}"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/macos/Info.plist"

    # MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Ninja/Makefile builds otherwise receive only a linker-generated ad-hoc
# signature. That signature does not seal Info.plist, so macOS cannot associate
# notification authorization with MACOSX_BUNDLE_GUI_IDENTIFIER. Sign the
# completed bundle for local development; distribution builds can disable this
# and apply their Developer ID signature during packaging.

# if(APPLE)
#     option(CHATAPP_ADHOC_CODE_SIGN
#         "Ad-hoc sign the completed macOS app bundle"
#         ON
#     )
#     if(CHATAPP_ADHOC_CODE_SIGN)
#         find_program(CHATAPP_CODESIGN_EXECUTABLE codesign REQUIRED)
#         add_custom_command(TARGET ChatApp POST_BUILD
#             COMMAND "${CHATAPP_CODESIGN_EXECUTABLE}"
#                 --force
#                 --deep
#                 --sign -
#                 "$<TARGET_BUNDLE_DIR:ChatApp>"
#             COMMENT "Ad-hoc signing ChatApp.app for macOS services"
#             VERBATIM
#         )
#     endif()
# endif()

if(APPLE AND DEFINED APPLE_SIGNING_IDENTITY)

    message(STATUS "Apple signing enabled")

    add_custom_command(
        TARGET ChatApp
        POST_BUILD

        COMMAND /usr/bin/codesign
            --force
            --deep
            --sign "${APPLE_SIGNING_IDENTITY}"
            "$<TARGET_BUNDLE_DIR:ChatApp>"

        COMMENT "Signing ChatApp with Apple Development certificate"
        VERBATIM
    )

endif()

target_link_libraries(ChatApp
    PUBLIC
        Qt6::Quick
        Qt6::Gui
        Qt6::Widgets
        core::application
        core::storage
        core::utils
        constants
        resources
        localization
        theme
        ui
        authentication
        chat
)

if(WIN32)
    # windeployqt handles Qt's own DLLs, but not third-party runtime DLLs.
    deploy_bundled_openssl_runtime(ChatApp)
    add_custom_command(TARGET ChatApp POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:QXmppQt6>"
            "$<TARGET_FILE_DIR:ChatApp>"
        VERBATIM
    )
endif()


install(TARGETS ChatApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
qt_generate_deploy_qml_app_script(

    TARGET ChatApp
    OUTPUT_SCRIPT deploy_script

)

install(SCRIPT ${deploy_script})
