cmake_minimum_required(VERSION 3.16)
project(MessageStorageTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(Qt6 REQUIRED COMPONENTS Core Sql)
enable_testing()
add_executable(test_message_store test_message_store.cpp
    ../../src/core/storage/message_store.cpp
    ../../src/core/storage/database_manager.cpp)
target_include_directories(test_message_store PRIVATE ../../src/core)
target_link_libraries(test_message_store PRIVATE Qt6::Core Qt6::Sql)
add_test(NAME MessageStore COMMAND test_message_store)
