
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (NOT EXISTS ${PROJECT_BINARY_DIR}/external/zlib/INSTALL/include/zlib.h)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/external)
    file(DOWNLOAD
        https://github.com/madler/zlib/archive/refs/tags/v1.2.11.tar.gz
        ${PROJECT_BINARY_DIR}/external/zlib.1.2.11.tar.gz
        EXPECTED_HASH MD5=0095d2d2d1f3442ce1318336637b695f
    )
    file(ARCHIVE_EXTRACT
        INPUT ${PROJECT_BINARY_DIR}/external/zlib.1.2.11.tar.gz
        DESTINATION ${PROJECT_BINARY_DIR}/external/zlib
    )
    if(is_multi_config)
        set(cmake_cmd_config "")
    else()
        set(cmake_cmd_config -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND}
        -S ${PROJECT_BINARY_DIR}/external/zlib/zlib-1.2.11
        -B ${PROJECT_BINARY_DIR}/external/zlib/build
        -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/external/zlib/INSTALL
        ${cmake_cmd_config}
        )
    if(is_multi_config)
        set(build_configs ${CMAKE_CONFIGURATION_TYPES})
    else()
        set(build_configs ${CMAKE_BUILD_TYPE})
    endif()
    foreach(cfg ${build_configs})
        execute_process(COMMAND ${CMAKE_COMMAND} --build ${PROJECT_BINARY_DIR}/external/zlib/build --config ${cfg})
        execute_process(COMMAND ${CMAKE_COMMAND} --install ${PROJECT_BINARY_DIR}/external/zlib/build --config ${cfg})
    endforeach()
endif()
