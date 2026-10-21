cmake_minimum_required(VERSION 3.16)
project(MessageFetchTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Qml Test)
enable_testing()
add_executable(test_message_fetch test_message_fetch.cpp
    ../../src/features/chat/domain/usecase/message_usecase.cpp
    ../../src/features/chat/domain/usecase/message_usecase.hpp
    ../../src/features/chat/domain/repository/message_repository.hpp)
target_include_directories(test_message_fetch PRIVATE ../../src/features/chat ../../src/features/chat/domain ../../src/features/chat/presentation/viewmodel)
target_link_libraries(test_message_fetch PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
add_test(NAME MessageFetch COMMAND test_message_fetch)
