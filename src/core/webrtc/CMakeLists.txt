add_library(webrtc STATIC
    webrtc_client.hpp webrtc_client.cpp
    peer_connection_observer.hpp peer_connection_observer.cpp
    sdp_observer.hpp sdp_observer.cpp

)

add_library(core::webrtc ALIAS webrtc)

# The imported WebRTC archive is built with is_debug=false. Keep code that
# instantiates WebRTC headers on the same side of its debug ABI boundary.
target_compile_definitions(webrtc PRIVATE NDEBUG)

target_link_libraries(webrtc
 PUBLIC
    Qt6::Core
    xmpp
    WebRTC
    logging
)

target_include_directories(webrtc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
