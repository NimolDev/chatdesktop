add_library(logging STATIC
    logger.hpp
    logger.cpp
)
add_library(core::logging ALIAS logging)
target_link_libraries(logging
    PRIVATE
        Qt6::Core
)
target_include_directories(logging
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)