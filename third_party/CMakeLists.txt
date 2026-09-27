# add_subdirectory(openssl)

set(BUILD_OMEMO OFF CACHE BOOL "Disable QXmpp OMEMO module" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(BUILD_DOCBOOK OFF CACHE BOOL "" FORCE)

# Disable tests only while configuring third-party dependencies
# set(_BUILD_TESTING ${BUILD_TESTING})

# set(BUILD_TESTING OFF)

# add_subdirectory(qxmpp)
# # add_subdirectory(other_library)

# set(BUILD_TESTING ${_BUILD_TESTING})
# unset(_BUILD_TESTING)

add_subdirectory(openssl)
add_subdirectory(qxmpp)
add_subdirectory(webrtc)