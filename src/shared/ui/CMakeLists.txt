set(UI_QML_FILES
    MyTextField.qml
    MyButton.qml
    Avatar.qml
    MenuButton.qml
    SearchField.qml
)

protect_qml_files(${UI_QML_FILES})

qt_add_qml_module(ui
    URI Shared.UI
    VERSION 1.0
    STATIC

    QML_FILES
        ${UI_QML_FILES}
    SOURCES circular_image.hpp circular_image.cpp
    QML_FILES CircleButton.qml
    QML_FILES GlassBackground.qml
    QML_FILES ContextMenuItem.qml

)


target_include_directories(ui
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ui
    PRIVATE
        Qt6::Quick
        Qt6::Qml
        Qt6::Core
        Qt6::Network
)
