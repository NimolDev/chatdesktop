set(AUTH_QML_FILES
    presentation/view/LoginView.qml
)

protect_qml_files(${AUTH_QML_FILES})

qt_add_qml_module(authentication
    STATIC
    URI Features.Auth
    VERSION 1.0
    QML_FILES
        ${AUTH_QML_FILES}
    SOURCES
        domain/login_request.hpp
        domain/login_repository.hpp
        domain/login_response.hpp
        data/login_repository_impl.hpp data/login_repository_impl.cpp
        domain/usecase/login_use_case.hpp domain/usecase/login_use_case.cpp
        presentation/viewmodel/login_vm.hpp presentation/viewmodel/login_vm.cpp
)

target_include_directories(authentication
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}

        # "${CMAKE_SOURCE_DIR}/src/shared/theme"
        # "${CMAKE_SOURCE_DIR}/src/shared/ui"
        "${CMAKE_CURRENT_SOURCE_DIR}/presentation/view"
        "${CMAKE_CURRENT_SOURCE_DIR}/presentation/viewmodel"
        "${CMAKE_CURRENT_SOURCE_DIR}/domain"
        "${CMAKE_CURRENT_SOURCE_DIR}/data"
        # ${CMAKE_CURRENT_SOURCE_DIR}/presentation/viewmodel
)

target_link_libraries(authentication
    PUBLIC
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
        network
        xmpp
        # localization
        # theme
)
