cmake_minimum_required(VERSION 3.16)
project(NetworkRetryTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Network Test Qml)
find_package(OpenSSL REQUIRED)
enable_testing()
add_executable(test_network_retry test_network_retry.cpp
    ../../src/core/network/network_client.cpp
    ../../src/core/network/certificate/certificate_utils.cpp
    ../../src/core/network/certificate/certificate_chain_validator.cpp)
target_include_directories(test_network_retry PRIVATE ../../src/core)
target_link_libraries(test_network_retry PRIVATE Qt6::Core Qt6::Network Qt6::Test OpenSSL::Crypto OpenSSL::SSL)
add_test(NAME NetworkRetry COMMAND test_network_retry)

add_executable(test_conversations_retry test_conversations_retry.cpp
    ../../src/features/chat/presentation/viewmodel/conversations_vm.cpp
    ../../src/features/chat/domain/usecase/conversations_usecase.cpp
    ../../src/core/storage/local_storage.cpp
    ../../src/features/chat/data/repository/conversations_repository_impl.cpp
    ../../src/core/network/network_monitor.cpp
    ../../src/core/network/network_client.cpp
    ../../src/core/network/certificate/certificate_utils.cpp
    ../../src/core/network/certificate/certificate_chain_validator.cpp)
target_include_directories(test_conversations_retry PRIVATE
    ../../src/core ../../src/core/constants ../../src/features/chat
    ../../src/features/chat/data ../../src/features/chat/domain)
target_link_libraries(test_conversations_retry PRIVATE Qt6::Qml Qt6::Core Qt6::Network Qt6::Test OpenSSL::Crypto OpenSSL::SSL)
add_test(NAME ConversationsRetry COMMAND test_conversations_retry)
