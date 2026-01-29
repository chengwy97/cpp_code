cmake_minimum_required(VERSION 3.20)
project(cpp_test VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(generate_version_info_cmake_file ${CMAKE_CURRENT_LIST_DIR}/cmake/GenerateVersionInfo.cmake)
set(generate_version_info_template_file ${CMAKE_CURRENT_LIST_DIR}/version/version.hpp.in)
set(generate_version_info_output_file ${CMAKE_BINARY_DIR}/generated/version/version.hpp)

include(cmake/GetTlExpected.cmake)
include(cmake/GetOneTBB.cmake)
include(cmake/GetGSL.cmake)
include(cmake/GetFrozen.cmake)
include(cmake/GetMagicEnum.cmake)

include_directories(${CMAKE_BINARY_DIR}/generated)

add_subdirectory(utils)
add_subdirectory(boost_test)
add_subdirectory(behaviortree_test)
add_subdirectory(cpp_test)

add_subdirectory(cmake_test)
