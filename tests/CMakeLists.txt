find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(test_basic
    # unit/test_basic.cpp
)

target_link_libraries(test_basic
    PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(
    NAME BasicTest
    COMMAND test_basic
)
