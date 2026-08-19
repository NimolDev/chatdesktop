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

# add_dependencies(StringEncryptor openssl_external)
# target_link_bundled_openssl(StringEncryptor)