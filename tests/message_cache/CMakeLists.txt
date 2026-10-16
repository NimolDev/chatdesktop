cmake_minimum_required(VERSION 3.16)
project(MessageCacheTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Threads REQUIRED)
add_subdirectory(../../third_party/openssl bundled_openssl)
enable_testing()
add_executable(test_message_cache test_message_cache.cpp
    ../../src/core/storage/message_cache.cpp
    ../../src/core/storage/file_name_helper.cpp
    ../../src/core/storage/local_storage.cpp)
target_include_directories(test_message_cache PRIVATE ../../src/core ../../src/core/constants ../../src/features/chat)
target_link_libraries(test_message_cache PRIVATE Qt6::Core OpenSSL::Crypto Threads::Threads)
add_test(NAME MessageCache COMMAND test_message_cache)

add_executable(benchmark_message_cache benchmark_message_cache.cpp
    ../../src/core/storage/message_cache.cpp
    ../../src/core/storage/file_name_helper.cpp
    ../../src/core/storage/local_storage.cpp)
target_include_directories(benchmark_message_cache PRIVATE
    ../../src/core ../../src/core/constants ../../src/features/chat)
target_link_libraries(benchmark_message_cache PRIVATE Qt6::Core OpenSSL::Crypto Threads::Threads)
