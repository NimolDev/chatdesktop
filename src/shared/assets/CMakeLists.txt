set(UI_QML_FILES
)

protect_qml_files(${UI_QML_FILES})

qt_add_qml_module(assets
    URI Shared.assets
    VERSION 1.0
    STATIC

    QML_FILES
        ${UI_QML_FILES}
        SOURCES sticker_mananger.hpp sticker_mananger.cpp

)


target_include_directories(assets
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(assets
    PRIVATE
        Qt6::Qml
)