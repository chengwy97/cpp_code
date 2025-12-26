cmake_minimum_required(VERSION 3.20)
project(cpp_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/GetTlExpected.cmake)

find_package(Git QUIET)

if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(GIT_COMMIT_ID "unknown")
endif()

string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")

message(STATUS "Git commit ID: ${GIT_COMMIT_ID}")
message(STATUS "Build time: ${BUILD_TIME}")

configure_file(
    ${CMAKE_SOURCE_DIR}/version/version.hpp.in
    ${CMAKE_BINARY_DIR}/generated/version/version.hpp
    @ONLY
)

include_directories(${CMAKE_BINARY_DIR}/generated)

add_subdirectory(utils)
add_subdirectory(boost_test)
add_subdirectory(behaviortree_test)
add_subdirectory(cpp_test)
