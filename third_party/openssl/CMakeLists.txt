set(OPENSSL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT}/include")

if(APPLE)
    set(OPENSSL_ARCH "${CMAKE_OSX_ARCHITECTURES}")

    if(NOT OPENSSL_ARCH)
        set(OPENSSL_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    message(STATUS "OpenSSL architecture: ${OPENSSL_ARCH}")

    if(OPENSSL_ARCH STREQUAL "arm64")
        set(OPENSSL_LIB_DIR
            "${OPENSSL_ROOT}/lib/macos/arm64"
        )
    elseif(OPENSSL_ARCH MATCHES "^(x86_64|x64|amd64)$")
        set(OPENSSL_LIB_DIR
            "${OPENSSL_ROOT}/lib/macos/x64"
        )
    else()
        message(FATAL_ERROR
            "Unsupported macOS architecture: ${OPENSSL_ARCH}"
        )
    endif()

    set(OPENSSL_SSL_LIBRARY
        "${OPENSSL_LIB_DIR}/libssl.a"
    )

    set(OPENSSL_CRYPTO_LIBRARY
        "${OPENSSL_LIB_DIR}/libcrypto.a"
    )

elseif(WIN32)
    # CMAKE_SYSTEM_PROCESSOR can describe the Windows host rather than the
    # architecture selected by the Visual Studio/Qt kit. Prefer the compiler
    # target so an x64 build on an ARM64 Windows machine links x64 libraries.
    if(CMAKE_VS_PLATFORM_NAME)
        set(OPENSSL_ARCH "${CMAKE_VS_PLATFORM_NAME}")
    elseif(CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
        set(OPENSSL_ARCH "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
    else()
        set(OPENSSL_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    message(STATUS "OpenSSL Windows architecture: ${OPENSSL_ARCH}")

    if(OPENSSL_ARCH MATCHES "^(ARM64|arm64|aarch64)$")
        set(OPENSSL_LIB_DIR
            "${OPENSSL_ROOT}/lib/windows/arm64"
        )
        set(OPENSSL_CRYPTO_RUNTIME
            "${OPENSSL_LIB_DIR}/libcrypto-4-arm64.dll"
        )
        set(OPENSSL_SSL_RUNTIME
            "${OPENSSL_LIB_DIR}/libssl-4-arm64.dll"
        )
    elseif(OPENSSL_ARCH MATCHES "^(AMD64|amd64|x86_64|x64|X64)$")
        set(OPENSSL_LIB_DIR
            "${OPENSSL_ROOT}/lib/windows/x64"
        )
        set(OPENSSL_CRYPTO_RUNTIME
            "${OPENSSL_LIB_DIR}/libcrypto-4-x64.dll"
        )
        set(OPENSSL_SSL_RUNTIME
            "${OPENSSL_LIB_DIR}/libssl-4-x64.dll"
        )
    else()
        message(FATAL_ERROR
            "Unsupported Windows architecture: ${OPENSSL_ARCH}"
        )
    endif()

    set(OPENSSL_SSL_LIBRARY
        "${OPENSSL_LIB_DIR}/libssl.lib"
    )

    set(OPENSSL_CRYPTO_LIBRARY
        "${OPENSSL_LIB_DIR}/libcrypto.lib"
    )

else()
    message(FATAL_ERROR
        "Bundled OpenSSL is not configured for this platform"
    )
endif()

set(OPENSSL_REQUIRED_FILES
    "${OPENSSL_INCLUDE_DIR}/openssl/ssl.h"
    "${OPENSSL_INCLUDE_DIR}/openssl/crypto.h"
    "${OPENSSL_SSL_LIBRARY}"
    "${OPENSSL_CRYPTO_LIBRARY}"
)
if(WIN32)
    list(APPEND OPENSSL_REQUIRED_FILES
        "${OPENSSL_SSL_RUNTIME}"
        "${OPENSSL_CRYPTO_RUNTIME}"
    )
endif()

foreach(required_file IN LISTS OPENSSL_REQUIRED_FILES)
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR
            "Bundled OpenSSL file not found: ${required_file}"
        )
    endif()
endforeach()

# QXmpp calls find_package(OpenSSL) from a sibling directory. Publish the
# bundled paths in the cache so FindOpenSSL uses these artifacts instead of a
# system installation (and replaces any stale paths in an existing build).
set(OPENSSL_ROOT_DIR "${OPENSSL_ROOT}" CACHE PATH
    "Root directory of the bundled OpenSSL installation" FORCE)
set(OPENSSL_INCLUDE_DIR "${OPENSSL_INCLUDE_DIR}" CACHE PATH
    "Bundled OpenSSL include directory" FORCE)
set(OPENSSL_SSL_LIBRARY "${OPENSSL_SSL_LIBRARY}" CACHE FILEPATH
    "Bundled OpenSSL SSL library" FORCE)
set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_CRYPTO_LIBRARY}" CACHE FILEPATH
    "Bundled OpenSSL Crypto library" FORCE)


if(WIN32)
    add_library(bundled_openssl_crypto SHARED IMPORTED GLOBAL)
    set_target_properties(bundled_openssl_crypto PROPERTIES
        IMPORTED_IMPLIB "${OPENSSL_CRYPTO_LIBRARY}"
        IMPORTED_LOCATION "${OPENSSL_CRYPTO_RUNTIME}"
        INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
    )

    add_library(bundled_openssl_ssl SHARED IMPORTED GLOBAL)
    set_target_properties(bundled_openssl_ssl PROPERTIES
        IMPORTED_IMPLIB "${OPENSSL_SSL_LIBRARY}"
        IMPORTED_LOCATION "${OPENSSL_SSL_RUNTIME}"
        INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
    )
else()
    add_library(bundled_openssl_crypto STATIC IMPORTED GLOBAL)
    set_target_properties(bundled_openssl_crypto PROPERTIES
        IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
    )

    add_library(bundled_openssl_ssl STATIC IMPORTED GLOBAL)
    set_target_properties(bundled_openssl_ssl PROPERTIES
        IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}"
    )
endif()

target_link_libraries(bundled_openssl_ssl
    INTERFACE
        bundled_openssl_crypto
)

add_library(OpenSSL::Crypto ALIAS bundled_openssl_crypto)
add_library(OpenSSL::SSL ALIAS bundled_openssl_ssl)

add_library(bundled_openssl INTERFACE)

target_link_libraries(bundled_openssl
    INTERFACE
        OpenSSL::SSL
        OpenSSL::Crypto
)

add_library(third_party::openssl ALIAS bundled_openssl)

function(deploy_bundled_openssl_runtime target_name)
    if(WIN32)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_FILE:bundled_openssl_crypto>"
                "$<TARGET_FILE:bundled_openssl_ssl>"
                "$<TARGET_FILE_DIR:${target_name}>"
            VERBATIM
        )
    endif()
endfunction()

# StringEncryptor runs during the build, so OpenSSL must be beside it before
# the generated-header custom command invokes the tool.
if(TARGET StringEncryptor)
    deploy_bundled_openssl_runtime(StringEncryptor)
endif()

if(WIN32)
    install(FILES
        "${OPENSSL_CRYPTO_RUNTIME}"
        "${OPENSSL_SSL_RUNTIME}"
        TYPE BIN
    )
endif()
