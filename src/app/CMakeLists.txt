qt_add_executable(ChatApp
    MACOSX_BUNDLE
    main.cpp
    ${CMAKE_SOURCE_DIR}/resources/Resources.qrc
)
target_compile_definitions(ChatApp
    PRIVATE
        $<$<CONFIG:Debug>:QT_QML_DEBUG>
)
target_include_directories(ChatApp
    PRIVATE
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/third_party"
        "${CMAKE_SOURCE_DIR}/src/shared"
        "${CMAKE_SOURCE_DIR}/src/core"
        "${CMAKE_SOURCE_DIR}/src/features"
)

set(CHATAPP_QML_FILES
    Main.qml
    SplashScreen.qml
    component/Menu.qml
    AboutDialog.qml
)

protect_qml_files(${CHATAPP_QML_FILES})

qt_add_qml_module(ChatApp
    URI ChatApp
    VERSION 1.0
    QML_FILES
        ${CHATAPP_QML_FILES}

    SOURCES
        service_container.hpp
        app_container.hpp app_container.cpp
        app_controller.hpp app_controller.cpp
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
    # MACOSX_BUNDLE_COPYRIGHT "© 2026 Nimol. All rights reserved.";
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(ChatApp
    PUBLIC
        Qt6::Quick
        # Qt6::QuickControls2
        Qt6::Gui
        resources
        localization
        theme
        ui
        core::application
        core::storage
        authentication
        authenticationplugin
        chat
)


install(TARGETS ChatApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
