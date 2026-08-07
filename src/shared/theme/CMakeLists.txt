
set_source_files_properties(Colors.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)
set_source_files_properties(Theme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)
set_source_files_properties(Typography.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

set_source_files_properties(AppAssets.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

set_source_files_properties(AppLayouts.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

set(THEME_QML_FILES
    Colors.qml
    Theme.qml
    Typography.qml
    AppAssets.qml
    AppLayouts.qml
)

protect_qml_files(${THEME_QML_FILES})

qt_add_qml_module(theme
    URI Theme
    VERSION 1.0
    STATIC

    QML_FILES
        ${THEME_QML_FILES}
    SOURCES
         font_manager.hpp font_manager.cpp
)
# add_library(shared::theme ALIAS theme)

target_include_directories(theme
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(theme
    PRIVATE
        Qt6::Core
        Qt6::Quick
)
