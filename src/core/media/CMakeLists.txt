
add_library(media STATIC
    audio_player.hpp
    audio_player.cpp
    media_device_manager.hpp
    media_device_manager.cpp
    media_device_model.hpp
    media_device_model.cpp
    voice_recorder.hpp
    voice_recorder.cpp
    m4a_encoder.hpp
    m4a_encoder.cpp
    image_compress.hpp image_compress.cpp
)
add_library(core::media ALIAS media)

target_include_directories(media PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_link_libraries(media PUBLIC
    Qt6::Core
    Qt6::Multimedia
    core::logging
    core::storage
)
