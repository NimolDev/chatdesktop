set(WEBRTC_SOURCE
    "/Users/nimol/Documents/WEB_RTC/libwebrtc_build/src"
)

set(WEBRTC_DEPOT_SOURCE
    "/Users/nimol/Documents/WEB_RTC/ios/webrtc/src"
)

set(WEBRTC_BUILD
    "${WEBRTC_DEPOT_SOURCE}/out/Release-mac-arm64"
)

set(WEBRTC_LIBRARY
    "${CMAKE_CURRENT_LIST_DIR}/lib/mac/arm64/libwebrtc.a"
)

add_library(WebRTC STATIC IMPORTED GLOBAL)

set_target_properties(WebRTC PROPERTIES
    IMPORTED_LOCATION
        "${WEBRTC_LIBRARY}"

    INTERFACE_INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_LIST_DIR}/original_headers;${WEBRTC_SOURCE};${WEBRTC_DEPOT_SOURCE};${WEBRTC_DEPOT_SOURCE}/third_party/abseil-cpp;${WEBRTC_BUILD}/gen"
)

target_compile_definitions(WebRTC INTERFACE
    WEBRTC_MAC
    WEBRTC_POSIX
)

if(APPLE)
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
