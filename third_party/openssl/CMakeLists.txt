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
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
        set(OPENSSL_LIB_DIR
            "${OPENSSL_ROOT}/lib/windows/arm64"
        )
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|amd64|x86_64|x64)$")
        set(OPENSSL_LIB_DIR
            "${OPENSSL_ROOT}/lib/windows/x64"
        )
    else()
        message(FATAL_ERROR
            "Unsupported Windows architecture: ${CMAKE_SYSTEM_PROCESSOR}"
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

foreach(required_file
    "${OPENSSL_INCLUDE_DIR}/openssl/ssl.h"
    "${OPENSSL_INCLUDE_DIR}/openssl/crypto.h"
    "${OPENSSL_SSL_LIBRARY}"
    "${OPENSSL_CRYPTO_LIBRARY}"
)
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR
            "Bundled OpenSSL file not found: ${required_file}"
        )
    endif()
endforeach()


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