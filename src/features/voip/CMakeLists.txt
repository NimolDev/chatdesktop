add_library(voip_signaling STATIC
    signaling/jingle_action.hpp
    signaling/jingle_action.cpp
    signaling/jingle_message.hpp
    signaling/jingle_message.cpp
    signaling/jingle_extension.hpp
    signaling/jingle_extension.cpp
    signaling/ice_server.hpp
    signaling/jingle_service.hpp
    signaling/jingle_service.cpp
)
add_library(voip::signaling ALIAS voip_signaling)
target_include_directories(voip_signaling PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/src/core"
)
target_link_libraries(voip_signaling PUBLIC core::xmpp core::webrtc)

set(CHAT_QML_FILES
  presentation/view/CallPage.qml
)

protect_qml_files(${CHAT_QML_FILES})

qt_add_qml_module(voip
   STATIC
   URI Features.Voip
   VERSION 1.0
   QML_FILES
       ${CHAT_QML_FILES}
   SOURCES
       presentation/view/remote_video_item.hpp
       presentation/view/remote_video_item.cpp
       presentation/viewmodel/call_vm.hpp
       presentation/viewmodel/call_vm.cpp
       domain/repository/call_repository.hpp
       domain/usecase/call_use_case.hpp
       domain/usecase/call_use_case.cpp
       data/call_repository_impl.hpp
       data/call_repository_impl.cpp
       domain/model/jingle_state.hpp
)

target_include_directories(voip
   PUBLIC
       ${CMAKE_CURRENT_SOURCE_DIR}
       "${CMAKE_CURRENT_SOURCE_DIR}/data"
       "${CMAKE_CURRENT_SOURCE_DIR}/domain"
       "${CMAKE_CURRENT_SOURCE_DIR}/presentation/viewmodel"
       "${CMAKE_CURRENT_SOURCE_DIR}/presentation/view"

)

target_link_libraries(voip
   PUBLIC
      Qt6::Core
      Qt6::Qml
      Qt6::Quick
      network
      constants
      webrtc
      voip::signaling
)
