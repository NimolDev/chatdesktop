cmake_minimum_required(VERSION 3.16)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake")

project(Chat_Desktop VERSION ${APP_VERSION} LANGUAGES CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

option(
    PROTECT_QML_SOURCE
    "Remove raw QML source from application resources"
    OFF
)

function(protect_qml_files)
    if(PROTECT_QML_SOURCE)
        set_source_files_properties(
            ${ARGN}
            PROPERTIES
                QT_DISCARD_FILE_CONTENTS TRUE
        )
    endif()
endfunction()

message(STATUS "Application version: ${APP_VERSION}")
message(STATUS "Application build number: ${APP_BUILD_NUMBER}")

set(APP_GENERATED_INCLUDE_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${APP_GENERATED_INCLUDE_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/application/app_version.hpp.in"
    "${APP_GENERATED_INCLUDE_DIR}/app_version.hpp"
    @ONLY
)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Quick
    Core
    Widgets
    Network
    LinguistTools
    Xml
    Sql
    Positioning
    Location
)

qt_standard_project_setup(REQUIRES 6.5)

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml")

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

# qt_standard_project_setup(
#     REQUIRES 6.10
#     I18N_SOURCE_LANGUAGE en
#     I18N_TRANSLATED_LANGUAGES km
# )
add_subdirectory(tools/string_encryptor)
add_subdirectory(resources)
add_subdirectory(third_party)
add_subdirectory(src)
