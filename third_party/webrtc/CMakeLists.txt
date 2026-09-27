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

    INTERFACE_INCLUDE_DIRECTORIES "${WEBRTC_SOURCE}; ${WEBRTC_SOURCE}/third_party/abseil-cpp; ${WEBRTC_SOURCE}/third_party/libyuv/include;${WEBRTC_BUILD}"
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

    # Prefer the compiler target over the host processor for cross-builds.
    if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
        set(_webrtc_target "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
    elseif(CMAKE_GENERATOR_PLATFORM)
        set(_webrtc_target "${CMAKE_GENERATOR_PLATFORM}")
    else()
        set(_webrtc_target "${CMAKE_SYSTEM_PROCESSOR}")
    endif()
    string(TOLOWER "${_webrtc_target}" _webrtc_target)
    if(_webrtc_target MATCHES "^(arm64|aarch64)$")
        set(_webrtc_arch "arm64")
    elseif(_webrtc_target MATCHES "^(x64|amd64|x86_64)$")
        set(_webrtc_arch "x64")
    else()
        message(FATAL_ERROR "Unsupported Windows WebRTC target: ${_webrtc_target}")
    endif()
    set(WEBRTC_BUILD "${WEBRTC_SOURCE}/out/win_${_webrtc_arch}"
        CACHE PATH "WebRTC build directory")

    if((_webrtc_arch STREQUAL "x64" AND WEBRTC_BUILD MATCHES "win_arm64[/\\\\]?$")
       OR (_webrtc_arch STREQUAL "arm64" AND WEBRTC_BUILD MATCHES "win_x64[/\\\\]?$"))
        message(FATAL_ERROR
            "WebRTC architecture mismatch: the compiler targets ${_webrtc_arch}, "
            "but WEBRTC_BUILD is '${WEBRTC_BUILD}'. "
            "Select the ARM64 Qt kit to use lib/win/arm64/webrtc.lib, "
            "or set WEBRTC_BUILD to a build matching the compiler target.")
    endif()

    if(_webrtc_arch STREQUAL "arm64")
        set(WEBRTC_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/lib/win/arm64/webrtc.lib")
    else()
        set(WEBRTC_LIBRARY "${WEBRTC_BUILD}/obj/webrtc.lib")
    endif()

    if(NOT EXISTS "${WEBRTC_LIBRARY}")
        message(FATAL_ERROR
            "Missing Windows WebRTC archive: ${WEBRTC_LIBRARY}. "
            "Provide the WebRTC archive for ${_webrtc_arch} at this path.")
    endif()
    message(STATUS "WebRTC archive: ${WEBRTC_LIBRARY}")

    add_library(WebRTC STATIC IMPORTED GLOBAL)
    set_target_properties(WebRTC PROPERTIES
        IMPORTED_LOCATION "${WEBRTC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${WEBRTC_SOURCE};${WEBRTC_SOURCE}/third_party/abseil-cpp;${WEBRTC_SOURCE}/third_party/libyuv/include;${WEBRTC_BUILD}/gen"
    )
    target_compile_definitions(WebRTC INTERFACE WEBRTC_WIN NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(WebRTC INTERFACE
        ws2_32 winmm secur32 crypt32 iphlpapi dmoguids wmcodecdspuuid
        amstrmid msdmo ole32 oleaut32 uuid strmiids gdi32 user32 advapi32
    )
endif()
