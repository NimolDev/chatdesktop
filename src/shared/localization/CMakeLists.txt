set_source_files_properties(AppStrings.qml
 PROPERTIES
  QT_QML_SINGLETON_TYPE TRUE
)

set(LOCALIZATION_QML_FILES
 AppStrings.qml
)

protect_qml_files(${LOCALIZATION_QML_FILES})

qt_add_qml_module(localization
 STATIC
 URI Localization
 VERSION 1.0
 QML_FILES
  ${LOCALIZATION_QML_FILES}
  SOURCES
    language_manager.hpp language_manager.cpp
)

qt_standard_project_setup(
I18N_TRANSLATED_LANGUAGES en km
)
qt_add_translations(ChatApp
  TS_FILE_BASE app
  TS_FILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/translations
      RESOURCE_PREFIX
          /i18n
)
# add_library(shared::localization ALIAS localization)
target_include_directories(localization
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(localization
 PRIVATE
  Qt6::Core
)
