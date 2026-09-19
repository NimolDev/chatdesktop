
if(APPLE)
set(WEBRTC_SOURCE
    "/Users/nimol/Documents/WEB_RTC/ios/webrtc/src"
)

    # Select the archive for the target, including cross-builds on Apple Silicon.
    if(CMAKE_OSX_ARCHITECTURES)
        set(WEBRTC_TARGET_ARCH "${CMAKE_OSX_ARCHITECTURES}")
    else()
        set(WEBRTC_TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    if(WEBRTC_TARGET_ARCH STREQUAL "arm64")
        set(WEBRTC_ARCH_DIR "arm64")
    elseif(WEBRTC_TARGET_ARCH STREQUAL "x86_64")
        set(WEBRTC_ARCH_DIR "x64")
    else()
        message(FATAL_ERROR
            "WebRTC requires a single macOS target architecture: arm64 or x86_64. "
            "Got '${WEBRTC_TARGET_ARCH}'.")
    endif()

    set(WEBRTC_BUILD "${CMAKE_CURRENT_SOURCE_DIR}/lib/mac/${WEBRTC_ARCH_DIR}")
    if(NOT EXISTS "${WEBRTC_BUILD}/libwebrtc.a")
        message(FATAL_ERROR "Missing WebRTC archive: ${WEBRTC_BUILD}/libwebrtc.a")
    endif()
    message(STATUS "WebRTC archive: ${WEBRTC_BUILD}/libwebrtc.a")

add_library(WebRTC STATIC IMPORTED GLOBAL)

set_target_properties(WebRTC PROPERTIES
    IMPORTED_LOCATION
        "${WEBRTC_BUILD}/libwebrtc.a"

    INTERFACE_INCLUDE_DIRECTORIES
        "${WEBRTC_SOURCE};${WEBRTC_SOURCE}/third_party/abseil-cpp;${WEBRTC_BUILD}"
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
