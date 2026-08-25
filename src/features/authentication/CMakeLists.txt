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
        presentation/viewmodel/login_vm.hpp
        presentation/viewmodel/login_vm.cpp
        data/repository/login_repository_impl.hpp
        data/repository/login_repository_impl.cpp
        data/local/login_response_store.hpp
        data/local/login_response_store.cpp
        data/dto/login_response_dto.hpp
        domain/entity/login_request.hpp
        domain/repository/login_repository.hpp

        domain/usecase/login_use_case.hpp
        domain/usecase/login_use_case.cpp
        domain/entity/login_response.hpp
        SOURCES data/dto/login_request_dto.hpp
        SOURCES data/mapping/login_mapper.hpp
        SOURCES data/dto/local_user.hpp
        SOURCES domain/repository/session_repository.hpp
        SOURCES data/repository/session_repository_impl.hpp data/repository/session_repository_impl.cpp
        SOURCES domain/usecase/session_usecase.hpp domain/usecase/session_usecase.cpp
        SOURCES domain/usecase/logout_usecase.hpp domain/usecase/logout_usecase.cpp
        SOURCES data/dto/refresh_token_response_dto.hpp
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
        utils
        core::storage
        # localization
        # theme
)
