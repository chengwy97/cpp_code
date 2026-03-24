cmake_minimum_required(VERSION 3.20)
project(cpp_test VERSION 0.1.0)

set(CMAKE_BUILD_TYPE Debug)

# 如果 buile_type 没有设置，则设置为 debug
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose default build type: Debug" FORCE)
endif()

# 如果编译类型为 Release，则将编译选项替换为 relwithdebinfo
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 将绝对路径转换为相对路径
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")

enable_testing()

set(generate_version_info_cmake_file ${CMAKE_CURRENT_LIST_DIR}/cmake/GenerateVersionInfo.cmake)
set(generate_version_info_template_file ${CMAKE_CURRENT_LIST_DIR}/version/version.hpp.in)
set(generate_version_info_output_file ${CMAKE_BINARY_DIR}/generated/version/version.hpp)

# 获取第三方库
# tl::expected
include(cmake/GetTlExpected.cmake)
# oneTBB
# include(cmake/GetOneTBB.cmake)
# GSL
include(cmake/GetGSL.cmake)
# Frozen
include(cmake/GetFrozen.cmake)
# MagicEnum
include(cmake/GetMagicEnum.cmake)
# debug_assert
include(cmake/GetDebugAssert.cmake)

# GTest (with GMock)
include(cmake/GetGTest.cmake)

# Iceoryx
include(cmake/GetIceoryx.cmake)

# 设置 CPM 缓存目录，用于缓存下载的依赖包
# 优先使用环境变量，否则使用项目源目录下的 .cpm-cache（可在多个 build 目录间共享）
if(NOT DEFINED CPM_SOURCE_CACHE AND NOT DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cpm-cache" CACHE PATH "CPM source cache directory")
    message(STATUS "CPM cache directory: ${CPM_SOURCE_CACHE}")
endif()

include(cmake/tools.cmake)
CPMAddPackage(
    NAME random
    GITHUB_REPOSITORY ilqvya/random
    VERSION 1.5.0
    OPTIONS
        "Random_BuildTests OFF"
)

# CPMAddPackage 的 VERSION 字段会在 tag 前自动加上 v（如 VERSION 1.5.0 -> 拉取 v1.5.0）。
# 对于分支名（如 main），需要使用 GIT_TAG 而不是 VERSION，因为 VERSION 会自动加 v 前缀。
CPMAddPackage(
    NAME stdexec
    GITHUB_REPOSITORY NVIDIA/stdexec
    GIT_TAG main # 使用 GIT_TAG 指定分支名，不会加 v 前缀
)
# 如果需要具体 tag（如 v1.0.0），VERSION 应写成 1.0.0，让 CPM 自动加 v；若 tag 不带 v，需用 GIT_TAG 手动指定完整 tag。


include_directories(${CMAKE_BINARY_DIR}/generated)

add_subdirectory(utils)
add_subdirectory(boost_test)
add_subdirectory(behaviortree_test)
add_subdirectory(cpp_test)

add_subdirectory(cmake_test)
