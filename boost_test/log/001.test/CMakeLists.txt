cmake_minimum_required(VERSION 3.10)

# 获取父命名空间和当前目录名称作为可执行名一部分
get_filename_component(CUR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EXECUTABLE_NAME "${NAMESPACE}_${CUR_NAME}")

set(CMAKE_BUILD_TYPE Debug)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 获取所有的源文件
file(GLOB_RECURSE SOURCE_FILES "*.cpp")

add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})
target_link_libraries(${EXECUTABLE_NAME} PRIVATE utils::logging)
# -ldl 提供 dladdr 用于符号解析；addr2line 后端需要调试符号 (-g) 才能解析函数名和行号
target_link_libraries(${EXECUTABLE_NAME} PRIVATE dl)
# -rdynamic 将可执行文件的符号表导出到动态链接器，使 stacktrace 能解析自身函数名
target_link_options(${EXECUTABLE_NAME} PRIVATE -rdynamic)

# -O0 禁止内联，保留完整调用链；-fno-omit-frame-pointer 保留帧指针供 stacktrace 回溯
target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall -Wextra -O0 -g -fno-omit-frame-pointer)
# 使用 addr2line 后端（需要系统安装 binutils；会解析文件名、行号、函数名）
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE BOOST_STACKTRACE_USE_ADDR2LINE)
