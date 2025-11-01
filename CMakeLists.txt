cmake_minimum_required(VERSION 3.30)
project(RCP-Target)

set(CMAKE_CXX_STANDARD 17)

option(RCPT_BUILD_TESTS "Build GTest RCPT tests" OFF)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/VERSION.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION
        COMMAND ${CMAKE_COMMAND} -DSOURCE:STRING=${CMAKE_CURRENT_SOURCE_DIR} -DBIN:STRING=${CMAKE_CURRENT_BINARY_DIR}
        -DBTYPE:STRING=${CMAKE_BUILD_TYPE} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gen_version.cmake
)

add_library(RCP-Target src/RCPTarget.cpp src/procedures.cpp ${CMAKE_CURRENT_BINARY_DIR}/VERSION.cpp)
target_include_directories(RCP-Target PUBLIC src/)

target_compile_options(RCP-Target PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W3 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

if(${RCPT_BUILD_TESTS})
    add_subdirectory(test/googletest)
    enable_testing()
    add_executable(RCPT_Tests test/test.cpp)
    target_link_libraries(RCPT_Tests PRIVATE GTest::gtest_main RCP-Target)
    include(GoogleTest)
    gtest_discover_tests(RCPT_Tests)
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_custom_target(RCP-Target-GithubRelease COMMAND ${CMAKE_CURRENT_SOURCE_DIR}\\GithubRelease.sh ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    add_dependencies(RCP-Target-GithubRelease RCP-Target)

    if(${RCPT_BUILD_TESTS})
        add_dependencies(RCP-Target-GithubRelease RCPT_Tests)
    endif()
endif()