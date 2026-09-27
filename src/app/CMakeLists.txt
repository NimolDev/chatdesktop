qt_add_executable(ChatApp
    MACOSX_BUNDLE
    source/main.cpp
    coordinators/call_coordinator.hpp
    coordinators/call_coordinator.cpp
    ${CMAKE_SOURCE_DIR}/resources/Resources.qrc
)

target_compile_definitions(ChatApp
    PRIVATE
        $<$<CONFIG:Debug>:QT_QML_DEBUG>
)
target_include_directories(ChatApp
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/source"
        "${CMAKE_CURRENT_SOURCE_DIR}/coordinators"
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
    IMPORTS Features.Voip
    DEPENDENCIES Features.Chat
    QML_FILES
        ${CHATAPP_QML_FILES}

    SOURCES
        source/service_container.hpp
        source/app_container.hpp
        source/app_container.cpp
        source/app_controller.hpp
        source/app_controller.cpp
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
if(WIN32)
    target_sources(ChatApp PRIVATE
         ${CMAKE_SOURCE_DIR}/resources/window/app_icon.rc
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
        "${CMAKE_CURRENT_SOURCE_DIR}/../core/platform/macos/Info.plist"

    # MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Ninja/Makefile builds otherwise receive only a linker-generated ad-hoc
# signature. That signature does not seal Info.plist, so macOS cannot associate
# notification authorization with MACOSX_BUNDLE_GUI_IDENTIFIER. Sign the
# completed bundle for local development; distribution builds can disable this
# and apply their Developer ID signature during packaging.


if(APPLE)
    set_property(TARGET ChatApp APPEND PROPERTY BUILD_RPATH
        "@executable_path/../Frameworks")
    set_property(TARGET ChatApp APPEND PROPERTY INSTALL_RPATH
        "@executable_path/../Frameworks")
    # Copy before the bundle is signed, so the embedded library is sealed too.
    add_custom_command(TARGET ChatApp POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_BUNDLE_DIR:ChatApp>/Contents/Frameworks"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:webrtc>"
            "$<TARGET_BUNDLE_DIR:ChatApp>/Contents/Frameworks/$<TARGET_FILE_NAME:webrtc>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:QXmppQt6>"
            "$<TARGET_BUNDLE_DIR:ChatApp>/Contents/Frameworks/$<TARGET_SONAME_FILE_NAME:QXmppQt6>"
        VERBATIM
    )

# get_target_property(QXMPP_TARGET_TYPE QXmppQt6 TYPE)
    # if(QXMPP_TARGET_TYPE STREQUAL "SHARED_LIBRARY")
    #     add_custom_command(TARGET ChatApp POST_BUILD
    #         COMMAND ${CMAKE_COMMAND} -E make_directory
    #             "$<TARGET_BUNDLE_DIR:ChatApp>/Contents/Frameworks"

    #         COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #             "$<TARGET_FILE:QXmppQt6>"
    #             "$<TARGET_BUNDLE_DIR:ChatApp>/Contents/Frameworks/$<TARGET_SONAME_FILE_NAME:QXmppQt6>"
    #     )
    # endif()
endif()

# Populate QML imports for build-tree macOS bundles before signing them.
# Also generate the deployment script used by cmake --install.
qt_generate_deploy_qml_app_script(
    TARGET ChatApp
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
)

if(APPLE AND TARGET Qt6::QQmlPreviewServiceFactoryPlugin)
    # A bundled qt.conf redirects plugin lookup into the app. Include the
    # development plugins so Qt Creator can connect to Debug QML previews.
    add_custom_command(TARGET ChatApp POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            "-DBUILD_CONFIG=$<CONFIG>"
            "-DQML_TOOLING_DIR=$<TARGET_FILE_DIR:Qt6::QQmlPreviewServiceFactoryPlugin>"
            "-DAPP_BUNDLE=$<TARGET_BUNDLE_DIR:ChatApp>"
            -P "${CMAKE_SOURCE_DIR}/cmake/macos/deploy_qml_tooling.cmake"
        VERBATIM
    )
endif()


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
        Qt6::Core
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Gui
        Qt6::Widgets
        core::application
        core::storage
        core::utils
        core::logging
        core::platform
        constants
        resources
        localization
        theme
        ui
        authentication
        chat
        voip
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
install(SCRIPT ${deploy_script})