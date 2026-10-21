cmake_minimum_required(VERSION 3.16)
project(MediaCacheTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Multimedia Test)
find_package(Threads REQUIRED)
add_subdirectory(../../third_party/openssl bundled_openssl)
enable_testing()
add_executable(test_media_cache test_media_cache.cpp
    ../../src/core/storage/media_cache.cpp
    ../../src/core/storage/local_storage.cpp
    ../../src/core/media/image_compress.cpp
    ../../src/core/media/audio_player.cpp
    ../../src/core/logging/logger.cpp
    ../../src/core/xmpp/file_upload_operation.cpp)
target_include_directories(test_media_cache PRIVATE ../../src/core ../../src/core/constants)
target_link_libraries(test_media_cache PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::Multimedia Qt6::Test OpenSSL::Crypto Threads::Threads)
add_test(NAME MediaCache COMMAND test_media_cache)
