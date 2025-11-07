# 获取当前CMake脚本父目录名作为命名空间
get_filename_component(NAMESPACE ${CMAKE_CURRENT_LIST_DIR} NAME)
# message(STATUS "检测到命名空间: ${NAMESPACE}")

# 设置命名空间变量
set(PROJECT_NAMESPACE ${NAMESPACE})
message(STATUS "设置命名空间: ${PROJECT_NAMESPACE}")

add_subdirectory(asio)

add_subdirectory(log)
