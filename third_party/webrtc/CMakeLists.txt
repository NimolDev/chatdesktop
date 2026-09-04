
if(APPLE)
set(WEBRTC_SOURCE
    "/Users/nimol/Documents/WEB_RTC/ios/webrtc/src"
)

set(WEBRTC_BUILD
    "${WEBRTC_SOURCE}/out/mac_arm64"
)

add_library(WebRTC STATIC IMPORTED GLOBAL)

set_target_properties(WebRTC PROPERTIES
    IMPORTED_LOCATION
        "${WEBRTC_BUILD}/obj/libwebrtc.a"

    INTERFACE_INCLUDE_DIRECTORIES
        "${WEBRTC_SOURCE};${WEBRTC_SOURCE}/third_party/abseil-cpp;${WEBRTC_BUILD}/gen"
)

target_compile_definitions(WebRTC INTERFACE
    WEBRTC_MAC
    WEBRTC_POSIX
)


    target_link_libraries(WebRTC INTERFACE
        "-framework Foundation"
        "-framework AppKit"
        "-framework AVFoundation"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework VideoToolbox"
    )
endif()
