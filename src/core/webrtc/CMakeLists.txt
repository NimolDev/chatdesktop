
if(APPLE)
    # BoringSSL in the WebRTC archive must not share the app's OpenSSL symbols.
    set(WEBRTC_WRAPPER_TYPE SHARED)
else()
    set(WEBRTC_WRAPPER_TYPE STATIC)
endif()

add_library(webrtc ${WEBRTC_WRAPPER_TYPE}
    webrtc_client.hpp webrtc_client.cpp
    peer_connection_observer.hpp peer_connection_observer.cpp
    sdp_observer.hpp sdp_observer.cpp
    set_remote_description_observer.hpp set_remote_description_observer.cpp
    ice_server.hpp
    camera_video_source.hpp camera_video_source.cpp

)
add_library(core::webrtc ALIAS webrtc)

# The imported WebRTC archive is built with is_debug=false. Keep code that
# instantiates WebRTC headers on the same side of its debug ABI boundary.
target_compile_definitions(webrtc PRIVATE NDEBUG)

target_link_libraries(webrtc
 PUBLIC
    Qt6::Core
    Qt6::Gui
    WebRTC
 PRIVATE
    logging
)

if(APPLE)
    target_sources(webrtc PRIVATE mac_camera_capture.hpp mac_camera_capture.mm)
    set_source_files_properties(mac_camera_capture.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    set_target_properties(webrtc PROPERTIES
        OUTPUT_NAME chat_webrtc
        MACOSX_RPATH TRUE
        INSTALL_NAME_DIR "@rpath"
        BUILD_WITH_INSTALL_NAME_DIR TRUE
    )
    target_link_options(webrtc PRIVATE
        "LINKER:-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/macos_exports.txt"
    )
    set_property(TARGET webrtc APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/macos_exports.txt")
endif()

target_include_directories(webrtc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
