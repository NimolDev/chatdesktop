add_library(network STATIC
    http_method.hpp
    network_error.hpp
    network_interceptor.hpp
    retry_policy.hpp
    network_response.hpp
    network_request.hpp
    certificate/certificate_utils.hpp certificate/certificate_utils.cpp
    certificate/certificate_chain_validator.hpp certificate/certificate_chain_validator.cpp
    logging_interceptor.hpp logging_interceptor.cpp
    json_serializer.hpp
    auth_interceptor.hpp auth_interceptor.cpp
    network_client.hpp network_client.cpp

)
add_library(core::network ALIAS network)



target_include_directories(network
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        # "${OPENSSL_ROOT}/include"
         ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(network
    PUBLIC
        Qt6::Core
        Qt6::Network
        third_party::openssl
)