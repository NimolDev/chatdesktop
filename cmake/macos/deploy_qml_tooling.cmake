if(NOT BUILD_CONFIG STREQUAL "Debug")
    return()
endif()

file(COPY "${QML_TOOLING_DIR}/"
    DESTINATION "${APP_BUNDLE}/Contents/PlugIns/qmltooling"
    FILES_MATCHING PATTERN "*.dylib"
)
