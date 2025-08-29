cmake_minimum_required(VERSION 3.30)
project(RCP-Target)

set(CMAKE_CXX_STANDARD 23)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/VERSION.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION
        COMMAND ${CMAKE_COMMAND} -DSOURCE:STRING=${CMAKE_CURRENT_SOURCE_DIR} -DBIN:STRING=${CMAKE_CURRENT_BINARY_DIR}
        -DBTYPE:STRING=${CMAKE_BUILD_TYPE} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gen_version.cmake
)

add_library(RCP-Target src/RCPTarget.cpp src/procedures.cpp ${CMAKE_CURRENT_BINARY_DIR}/VERSION.cpp)
target_include_directories(RCP-Target PUBLIC src/ PRIVATE src/RCP_Target)

target_compile_options(RCP-Target PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W3 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)