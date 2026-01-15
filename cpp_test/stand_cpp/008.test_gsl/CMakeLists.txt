cmake_minimum_required(VERSION 3.10)

# 获取父命名空间和当前目录名称作为可执行名一部分
get_filename_component(CUR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EXECUTABLE_NAME "${NAMESPACE}_${CUR_NAME}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 获取所有的源文件
file(GLOB_RECURSE SOURCE_FILES "*.cpp")

add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})
target_link_libraries(${EXECUTABLE_NAME} PRIVATE utils::logging utils::filesystem_wrapper)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE GSL)

target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall -Wextra -O2)
