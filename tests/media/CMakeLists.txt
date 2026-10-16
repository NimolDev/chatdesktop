cmake_minimum_required(VERSION 3.16)
project(VoiceRecorderTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Multimedia Test)
enable_testing()
add_executable(test_voice_recorder
    test_voice_recorder.cpp
    ../../src/core/media/m4a_encoder.cpp
    ../../src/core/media/voice_recorder.cpp
    ../../src/core/media/voice_recorder.hpp
)
target_include_directories(test_voice_recorder PRIVATE ../../src/core)
target_link_libraries(test_voice_recorder PRIVATE Qt6::Core Qt6::Multimedia  Qt6::Test)
add_test(NAME VoiceRecorder COMMAND test_voice_recorder)

find_package(Qt6 REQUIRED COMPONENTS Qml)
add_executable(test_messaging_voice
    test_messaging_voice.cpp
    ../../src/core/media/audio_player.cpp
    ../../src/core/media/audio_player.hpp
    ../../src/features/chat/presentation/viewmodel/messaging_vm.cpp
    ../../src/features/chat/presentation/viewmodel/messaging_vm.hpp
    ../../src/features/chat/presentation/viewmodel/voice_message_controller.cpp
    ../../src/features/chat/presentation/viewmodel/voice_message_controller.hpp
    ../../src/features/chat/domain/usecase/message_usecase.cpp
    ../../src/features/chat/domain/usecase/send_message_usecase.cpp
    ../../src/features/chat/domain/repository/message_repository.hpp
    ../../src/features/chat/domain/repository/media_repository.hpp
    ../../src/core/media/m4a_encoder.cpp
    ../../src/core/media/voice_recorder.cpp
    ../../src/core/media/voice_recorder.hpp
    ../../src/core/utils/date_time_utils.cpp
)
target_include_directories(test_messaging_voice PRIVATE
    ../../src/core ../../src/features/chat ../../src/features/chat/domain)
target_link_libraries(test_messaging_voice PRIVATE Qt6::Core Qt6::Multimedia  Qt6::Qml Qt6::Test)
add_test(NAME MessagingVoice COMMAND test_messaging_voice)

# Keep duration regressions runnable independently of the legacy upload tests.
get_target_property(voice_duration_sources test_messaging_voice SOURCES)
list(REMOVE_ITEM voice_duration_sources test_messaging_voice.cpp)
add_executable(test_voice_duration test_voice_duration.cpp ${voice_duration_sources})
target_include_directories(test_voice_duration PRIVATE
    ../../src/core ../../src/features/chat ../../src/features/chat/domain)
target_link_libraries(test_voice_duration PRIVATE Qt6::Core Qt6::Multimedia  Qt6::Qml Qt6::Test)
add_test(NAME VoiceDuration COMMAND test_voice_duration)

add_library(audio_player_test_backend STATIC
    ../../src/core/media/audio_player.cpp
    ../../src/core/media/audio_player.hpp)
target_include_directories(audio_player_test_backend PUBLIC ../../src/core)
target_link_libraries(audio_player_test_backend PUBLIC Qt6::Core Qt6::Multimedia)
add_executable(test_audio_player test_audio_player.cpp)
target_link_libraries(test_audio_player PRIVATE audio_player_test_backend Qt6::Test)
add_test(NAME AudioPlayer COMMAND test_audio_player)

find_package(Qt6 REQUIRED COMPONENTS QuickTest)
add_executable(audio_qml_runner audio_qml_runner.cpp ${voice_duration_sources})
target_include_directories(audio_qml_runner PRIVATE
    ../../src/core ../../src/features/chat ../../src/features/chat/domain)
target_link_libraries(audio_qml_runner PRIVATE
    Qt6::Core Qt6::Multimedia  Qt6::QuickTest Qt6::Qml Qt6::Quick)

find_package(Qt6 REQUIRED COMPONENTS Network)
add_executable(test_media_download test_media_download.cpp ${voice_duration_sources}
    ../../src/features/chat/domain/repository/media_repository.hpp
    ../../src/features/chat/data/repository/media_repository_impl.cpp
    ../../src/core/network/http_download_service.cpp
    ../../src/core/network/http_download_service.hpp
    ../../src/core/storage/media_cache.cpp)
target_include_directories(test_media_download PRIVATE
    ../../src/core ../../src/features/chat ../../src/features/chat/domain)
target_link_libraries(test_media_download PRIVATE
    Qt6::Core Qt6::Multimedia Qt6::Qml Qt6::Network Qt6::Test )
add_test(NAME MediaDownload COMMAND test_media_download)

add_executable(test_attachment_upload test_attachment_upload.cpp ${voice_duration_sources})
target_include_directories(test_attachment_upload PRIVATE
    ../../src/core ../../src/features/chat ../../src/features/chat/domain)
target_link_libraries(test_attachment_upload PRIVATE Qt6::Core Qt6::Multimedia Qt6::Qml Qt6::Test)
add_test(NAME AttachmentUpload COMMAND test_attachment_upload)

add_executable(test_multiple_files test_multiple_files.cpp ${voice_duration_sources}
    ../../src/features/chat/presentation/viewmodel/attachment_model.cpp)
target_include_directories(test_multiple_files PRIVATE
    ../../src/core ../../src/features/chat ../../src/features/chat/domain
    ../../src/features/chat/presentation/viewmodel)
target_link_libraries(test_multiple_files PRIVATE Qt6::Core Qt6::Multimedia Qt6::Qml Qt6::Test)
add_test(NAME MultipleFiles COMMAND test_multiple_files)
