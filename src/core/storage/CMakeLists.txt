add_library(storage STATIC

    user_data.hpp user_data.cpp
    media_cache.hpp media_cache.cpp

    auth_storage.hpp auth_storage.cpp
    local_storage.hpp local_storage.cpp
    file_name_helper.hpp file_name_helper.cpp
    cache_storage.hpp cache_storage.cpp
    message_cache.hpp message_cache.cpp
)

add_library(core::storage ALIAS storage)

target_link_libraries(storage
    PUBLIC
        Qt6::Core
        third_party::openssl
        constants
)