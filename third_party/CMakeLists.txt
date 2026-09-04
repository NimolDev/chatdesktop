# add_subdirectory(openssl)

set(BUILD_OMEMO OFF CACHE BOOL "Disable QXmpp OMEMO module" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(BUILD_DOCBOOK OFF CACHE BOOL "" FORCE)

add_subdirectory(openssl)
add_subdirectory(qxmpp)
add_subdirectory(webrtc)