add_library(utils STATIC
   date_time_utils.hpp date_time_utils.cpp
   token_validator.hpp token_validator.cpp
)

add_library(core::utils ALIAS utils)

target_link_libraries(utils
    PUBLIC
        Qt6::Core
)