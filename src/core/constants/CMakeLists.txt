set(
    APP_STRINGS_CONFIG_FILE
    "${CMAKE_SOURCE_DIR}/config/app_constants.conf"
)

set(
    APP_STRINGS_GENERATED_DIRECTORY
    "${CMAKE_BINARY_DIR}/generated"
)

set(
    APP_STRINGS_GENERATED_HEADER
    "${APP_STRINGS_GENERATED_DIRECTORY}/encrypted_app_strings.hpp"
)

# This implementation is intended for separate Debug and Release
# build directories using Ninja or Unix Makefiles.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(APP_STRINGS_GENERATION_MODE plain)
else()
    set(APP_STRINGS_GENERATION_MODE encrypted)
endif()

add_custom_command(
    OUTPUT
        "${APP_STRINGS_GENERATED_HEADER}"

    COMMAND
        "${CMAKE_COMMAND}"
        -E
        make_directory
        "${APP_STRINGS_GENERATED_DIRECTORY}"

    COMMAND
        $<TARGET_FILE:StringEncryptor>
        "${APP_STRINGS_GENERATION_MODE}"
        "${APP_STRINGS_CONFIG_FILE}"
        "${APP_STRINGS_GENERATED_HEADER}"

    DEPENDS
        StringEncryptor
        "${APP_STRINGS_CONFIG_FILE}"

    COMMENT
        "Generating application string constants"

    VERBATIM
)

add_custom_target(
    GenerateAppStrings
    DEPENDS
        "${APP_STRINGS_GENERATED_HEADER}"
)

add_library(constants STATIC
    app_constants.hpp
    app_constants.cpp
    "${APP_STRINGS_GENERATED_HEADER}"
)

add_dependencies(
    constants
    GenerateAppStrings
)

target_compile_features(constants
    PUBLIC
        cxx_std_20
)

target_include_directories(constants
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"

    PRIVATE
        "${APP_STRINGS_GENERATED_DIRECTORY}"
)

target_link_libraries(constants
    PUBLIC
        Qt6::Core

    PRIVATE
        third_party::openssl
)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(constants
        PRIVATE
            APP_STRINGS_ENCRYPTED
    )
endif()
