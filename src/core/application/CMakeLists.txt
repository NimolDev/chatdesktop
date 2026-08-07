add_library(application STATIC
    app_info.hpp app_info.cpp
    "${CMAKE_BINARY_DIR}/generated/app_version.hpp"
)
add_library(core::application ALIAS application)

target_compile_features(application PUBLIC cxx_std_20)

target_include_directories(application
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
    PRIVATE
        "${CMAKE_BINARY_DIR}/generated"
)

target_link_libraries(application
    PUBLIC
        Qt6::Core
        Qt6::Network
)
