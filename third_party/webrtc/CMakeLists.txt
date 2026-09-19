
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
elseif(WIN32)
    set(WEBRTC_SOURCE "C:/Users/Nimol/Documents/webrtc/webrtc/src"
        CACHE PATH "WebRTC source directory")

    if(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64$"
       OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$"
       OR CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
        set(_webrtc_arch "arm64")
    else()
        set(_webrtc_arch "x64")
    endif()
    set(WEBRTC_BUILD "${WEBRTC_SOURCE}/out/win_${_webrtc_arch}"
        CACHE PATH "WebRTC build directory")

    add_library(WebRTC STATIC IMPORTED GLOBAL)
    set_target_properties(WebRTC PROPERTIES
        IMPORTED_LOCATION "${WEBRTC_BUILD}/obj/webrtc.lib"
        INTERFACE_INCLUDE_DIRECTORIES
            "${WEBRTC_SOURCE};${WEBRTC_SOURCE}/third_party/abseil-cpp;${WEBRTC_BUILD}/gen"
    )
    target_compile_definitions(WebRTC INTERFACE WEBRTC_WIN NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(WebRTC INTERFACE
        ws2_32 winmm secur32 crypt32 iphlpapi dmoguids wmcodecdspuuid
        amstrmid msdmo ole32 oleaut32 uuid strmiids gdi32 user32 advapi32
    )
endif()
