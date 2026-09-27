add_library(media STATIC
    media_device_manager.hpp
    media_device_manager.cpp
    media_device_model.hpp
    media_device_model.cpp
)
add_library(core::media ALIAS media)

target_include_directories(media PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(media PUBLIC
    Qt6::Core
    Qt6::Multimedia
)
