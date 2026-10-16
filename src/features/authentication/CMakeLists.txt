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

        data/repository/session_repository_impl.hpp
        data/repository/session_repository_impl.cpp
        data/repository/login_repository_impl.hpp
        data/repository/login_repository_impl.cpp
        data/local/login_response_store.hpp
        data/local/login_response_store.cpp
        data/dto/login_response_dto.hpp
        data/dto/login_request_dto.hpp
        data/dto/local_user.hpp
        data/dto/refresh_token_response_dto.hpp
        data/mapping/login_mapper.hpp

        domain/entity/login_request.hpp
        domain/entity/login_response.hpp

        domain/repository/login_repository.hpp
        domain/repository/session_repository.hpp
        domain/usecase/login_use_case.hpp
        domain/usecase/login_use_case.cpp
        domain/usecase/session_usecase.hpp
        domain/usecase/session_usecase.cpp
        domain/usecase/logout_usecase.hpp
        domain/usecase/logout_usecase.cpp

        presentation/viewmodel/login_vm.hpp
        presentation/viewmodel/login_vm.cpp

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
