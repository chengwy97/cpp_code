cmake_minimum_required(VERSION 3.10)

# 获取父命名空间和当前目录名称作为可执行名一部分
get_filename_component(CUR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EXECUTABLE_NAME "${NAMESPACE}_${CUR_NAME}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 获取所有的源文件
file(GLOB_RECURSE SOURCE_FILES "*.cpp")

# 这里的目的是让每次 build 目标时，都去运行一次这个 cmake 脚本
add_custom_target(GenerateGitInfo ALL
    COMMAND ${CMAKE_COMMAND}
            -DFROM_TEMPLATE="${generate_version_info_template_file}"
            -DTO_FILE="${generate_version_info_output_file}"
            -DPROJECT_VERSION="${PROJECT_VERSION}"
            -P "${generate_version_info_cmake_file}"
    BYPRODUCTS "${generate_version_info_output_file}"
)

add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})
# 只为当前 target 添加 -g 标志（仅在 Release 模式下）
target_compile_options(${EXECUTABLE_NAME} PRIVATE $<$<CONFIG:Release>:-g>)
# target_compile_options(${EXECUTABLE_NAME} PRIVATE -fsanitize=address)
target_compile_options(${EXECUTABLE_NAME} PRIVATE -fsanitize=undefined)
# -fsanitize=memory 仅 Clang 支持（MemorySanitizer），GCC 不支持，需使用 Clang 编译时才可启用
# target_compile_options(${EXECUTABLE_NAME} PRIVATE -fsanitize=memory -fsanitize-recover=memory)
# -fsanitize-recover=memory 配合 export MSAN_OPTIONS="halt_on_error=0" 使用，可以避免程序在内存错误时立即退出
# target_compile_options(${EXECUTABLE_NAME} PRIVATE -fsanitize=leak)
target_compile_options(${EXECUTABLE_NAME} PRIVATE -fno-omit-frame-pointer)
add_dependencies(${EXECUTABLE_NAME} GenerateGitInfo)
target_include_directories(${EXECUTABLE_NAME} PRIVATE "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(${EXECUTABLE_NAME} PRIVATE utils::logging utils::filesystem_wrapper)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE GSL)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE third_party::frozen)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE third_party::magic_enum)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE effolkronium_random)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE STDEXEC::stdexec)
# target_link_options(${EXECUTABLE_NAME} PRIVATE -fsanitize=address)
target_link_options(${EXECUTABLE_NAME} PRIVATE -fsanitize=undefined)
# target_link_options(${EXECUTABLE_NAME} PRIVATE -fsanitize=memory)
# target_link_options(${EXECUTABLE_NAME} PRIVATE -fsanitize=leak)
target_link_options(${EXECUTABLE_NAME} PRIVATE -fno-omit-frame-pointer)
