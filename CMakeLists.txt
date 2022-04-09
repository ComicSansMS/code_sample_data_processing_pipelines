cmake_minimum_required(VERSION 3.14)

project(coroutines_playground)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Do build tests" ON)

add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/permissive->)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/await:strict>)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/W4>)
add_compile_options($<$<CXX_COMPILER_ID:GNU,Clang>:-Wall>)
add_compile_options($<$<CXX_COMPILER_ID:GNU,Clang>:-pedantic>)
add_compile_options($<$<CXX_COMPILER_ID:Clang>:-stdlib=libc++>)

include(external/InstallZlib.cmake)
set(ZLIB_ROOT ${PROJECT_BINARY_DIR}/external/zlib/INSTALL)
find_package(ZLIB REQUIRED)
find_package(fmt REQUIRED)

add_library(aux_helper
    ${PROJECT_SOURCE_DIR}/execute_step.hpp
    ${PROJECT_SOURCE_DIR}/execute_step_co.hpp
    ${PROJECT_SOURCE_DIR}/file_source.cpp
    ${PROJECT_SOURCE_DIR}/file_source.hpp
    ${PROJECT_SOURCE_DIR}/file_sink.cpp
    ${PROJECT_SOURCE_DIR}/file_sink.hpp
    ${PROJECT_SOURCE_DIR}/filter_id.hpp
    ${PROJECT_SOURCE_DIR}/filter_deflate.cpp
    ${PROJECT_SOURCE_DIR}/filter_deflate.hpp
    ${PROJECT_SOURCE_DIR}/filter_inflate.cpp
    ${PROJECT_SOURCE_DIR}/filter_inflate.hpp
    ${PROJECT_SOURCE_DIR}/pipe_syntax_operator.hpp
    ${PROJECT_SOURCE_DIR}/pipeline_concepts.hpp
    ${PROJECT_SOURCE_DIR}/process_return.hpp
    ${PROJECT_SOURCE_DIR}/ring_buffer.cpp
    ${PROJECT_SOURCE_DIR}/ring_buffer.hpp
    ${PROJECT_SOURCE_DIR}/mt_ring_buffer.cpp
    ${PROJECT_SOURCE_DIR}/mt_ring_buffer.hpp
    ${PROJECT_SOURCE_DIR}/op_state.hpp
)
target_include_directories(aux_helper PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(aux_helper PUBLIC ZLIB::ZLIB)

add_executable(copy_deflate copy_deflate.cpp)
target_link_libraries(copy_deflate PUBLIC aux_helper fmt::fmt)

add_executable(copy_deflate_co copy_deflate_co.cpp)
target_link_libraries(copy_deflate_co PUBLIC aux_helper fmt::fmt)

add_executable(copy_inflate copy_inflate.cpp)
target_link_libraries(copy_inflate PUBLIC aux_helper fmt::fmt)

add_executable(sample_symmetric_transfer ${PROJECT_SOURCE_DIR}/samples/symmetric_transfer.cpp)
target_link_libraries(sample_symmetric_transfer PUBLIC aux_helper fmt::fmt)

add_executable(sample_producer_consumer ${PROJECT_SOURCE_DIR}/samples/producer_consumer.cpp)
target_link_libraries(sample_producer_consumer PUBLIC aux_helper fmt::fmt)

if(BUILD_TESTS)
    enable_testing()

    if(NOT TARGET Catch)
        if(NOT EXISTS ${PROJECT_BINARY_DIR}/external/catch2/catch.hpp)
            file(
                DOWNLOAD https://github.com/catchorg/Catch2/releases/download/v2.13.7/catch.hpp
                ${PROJECT_BINARY_DIR}/external/catch2/catch.hpp
                EXPECTED_MD5 afc1a3a08d753ec1bef9a449ff418e07
            )
        endif()
        add_library(Catch2 INTERFACE)
        target_include_directories(Catch2 INTERFACE ${CMAKE_BINARY_DIR}/external/catch2)
    endif()

    add_executable(aux_helper_test
        ${PROJECT_SOURCE_DIR}/catch.cpp
        ${PROJECT_SOURCE_DIR}/ring_buffer.t.cpp
        ${PROJECT_SOURCE_DIR}/mt_ring_buffer.t.cpp
    )
    target_link_libraries(aux_helper_test PUBLIC aux_helper Catch2)
    add_test(NAME TestAuxHelper COMMAND aux_helper_test)
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT copy_deflate_co)

if (WIN32)
    file(COPY
        ${PROJECT_BINARY_DIR}/external/zlib/INSTALL/bin/zlib.dll
        ${PROJECT_BINARY_DIR}/external/zlib/INSTALL/bin/zlibd.dll
        DESTINATION ${PROJECT_BINARY_DIR})
endif()
