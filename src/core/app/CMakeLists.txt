qt_add_library(core_app STATIC
    app_state.hpp
    app_state.cpp
)

target_link_libraries(core_app
    PUBLIC
        Qt6::Core
)
target_include_directories(core_app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)