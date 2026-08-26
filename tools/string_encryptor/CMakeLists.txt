add_executable(StringEncryptor
    main.cpp
)

target_compile_features(StringEncryptor
    PRIVATE
        cxx_std_20
)

target_link_libraries(StringEncryptor
    PRIVATE
        third_party::openssl
)

if(WIN32)
    # This executable is invoked by generated-header build steps. Keep the
    # bundled OpenSSL runtime beside it so a clean build can run the tool.
    deploy_bundled_openssl_runtime(StringEncryptor)
endif()
