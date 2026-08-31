add_library(models STATIC
   page_response.hpp
)


target_link_libraries(models
    PUBLIC
        Qt6::Core
)

target_include_directories(models
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)