cmake_minimum_required(VERSION 3.10)

# 获取父命名空间和当前目录名称作为可执行名一部分
get_filename_component(CUR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(LIBRARY_NAME "${NAMESPACE}_${CUR_NAME}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 获取所有的源文件
file(GLOB_RECURSE SOURCE_FILES "*.cpp")

add_library(${LIBRARY_NAME} ${SOURCE_FILES})

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
                                                  ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${LIBRARY_NAME} PUBLIC utils::logging)
target_link_libraries(${LIBRARY_NAME} PUBLIC third_party::tl_expected)

# 启用 spdlog 的位置跟踪宏
target_compile_definitions(${LIBRARY_NAME} PRIVATE SPDLOG_ACTIVE_LEVEL=0)
target_compile_options(${LIBRARY_NAME} PRIVATE -Wall -Wextra -O2)
# 起一个别名
add_library(${NAMESPACE}::${CUR_NAME} ALIAS ${LIBRARY_NAME})
