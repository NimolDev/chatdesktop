set(CHAT_QML_FILES
)

protect_qml_files(${CHAT_QML_FILES})

qt_add_qml_module(voip
   STATIC
   URI Features.Voip
   VERSION 1.0
   QML_FILES
       ${CHAT_QML_FILES}
   SOURCES
      presentation/viewmodel/call_vm.hpp
      presentation/viewmodel/call_vm.cpp

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
)