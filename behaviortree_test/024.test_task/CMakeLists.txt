cmake_minimum_required(VERSION 3.10)

# 获取当前目录名称
get_filename_component(CUR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
# 父命名空间由父CMakeLists.txt传递，这里假定为 PROJECT_NAMESPACE
set(NAMESPACE "${PROJECT_NAMESPACE}_${CUR_NAME}")
# message(STATUS "当前命名空间: ${NAMESPACE}")

# 生成可执行文件名
set(EXECUTABLE_NAME "${NAMESPACE}_${CUR_NAME}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 behaviortree.cpp
find_package(behaviortree_cpp REQUIRED)

file(GLOB SOURCE_FILES "*.cpp")

# 创建可执行文件
add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})

# 使用现代 CMake 方式链接到 behaviortree_cpp target
target_link_libraries(${EXECUTABLE_NAME} PRIVATE BT::behaviortree_cpp)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE utils::logging)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE behaviortree_test::sample_nodes
)# libbehaviortree_test_sample_nodes_sample_nodes
target_link_libraries(${EXECUTABLE_NAME} PRIVATE behaviortree_test::task)

target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall -Wextra -Wunused-function)
