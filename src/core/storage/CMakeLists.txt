add_library(storage STATIC
    database_manager.hpp database_manager.cpp
    database_helper.hpp database_helper.cpp
)

add_library(core::storage ALIAS storage)

target_link_libraries(storage
    PUBLIC
        Qt6::Core
        Qt6::Sql
)