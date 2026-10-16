cmake_minimum_required(VERSION 3.16)
project(LocalStorageTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(../../third_party/openssl bundled_openssl)
find_package(Threads REQUIRED)
enable_testing()
add_executable(test_local_storage test_local_storage.cpp
    ../../src/core/storage/local_storage.cpp)
target_include_directories(test_local_storage PRIVATE ../../src/core)
target_link_libraries(test_local_storage PRIVATE OpenSSL::Crypto Threads::Threads)
add_test(NAME LocalStorage COMMAND test_local_storage)
set_tests_properties(LocalStorage PROPERTIES TIMEOUT 30)
