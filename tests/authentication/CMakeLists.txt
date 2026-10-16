cmake_minimum_required(VERSION 3.16)
project(AuthStorageTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Threads REQUIRED)
add_subdirectory(../../third_party/openssl bundled_openssl)
enable_testing()
add_executable(test_auth_storage test_auth_storage.cpp
    ../../src/features/authentication/data/local/login_response_store.cpp
    ../../src/core/storage/auth_storage.cpp
    ../../src/core/storage/database_helper.cpp
    ../../src/core/storage/file_name_helper.cpp
    ../../src/core/storage/local_storage.cpp)
target_include_directories(test_auth_storage PRIVATE
    ../../src/core ../../src/core/constants
    ../../src/features/authentication ../../src/features/authentication/data)
target_link_libraries(test_auth_storage PRIVATE Qt6::Core OpenSSL::Crypto Threads::Threads)
add_test(NAME AuthStorage COMMAND test_auth_storage)
set_tests_properties(AuthStorage PROPERTIES TIMEOUT 60)
