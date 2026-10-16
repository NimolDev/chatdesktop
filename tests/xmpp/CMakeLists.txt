cmake_minimum_required(VERSION 3.16)
project(XmppUploadTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network Test)
enable_testing()
add_executable(test_file_upload
    test_file_upload.cpp
    ../../src/core/xmpp/file_upload_operation.cpp)
target_include_directories(test_file_upload PRIVATE ../../src/core/xmpp)
target_link_libraries(test_file_upload PRIVATE Qt6::Core Qt6::Network Qt6::Test)
add_test(NAME FileUpload COMMAND test_file_upload)
