cmake_minimum_required(VERSION 3.10)

# 获取当前目录名称
get_filename_component(CUR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
# 父命名空间由父CMakeLists.txt传递，这里假定为 PROJECT_NAMESPACE
set(NAMESPACE "${PROJECT_NAMESPACE}")
# message(STATUS "当前命名空间: ${NAMESPACE}")

# 生成可执行文件名
set(LIBRARY_NAME "${NAMESPACE}_${CUR_NAME}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 指定 Boost 安装路径
set(BOOST_ROOT "/usr/local/boost/1.89.0")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib")

# 使用旧的 FindBoost 模块
find_package(Boost 1.89.0 REQUIRED COMPONENTS filesystem thread)

if(Boost_FOUND)

else()
    message(FATAL_ERROR "Boost not found!")
endif()

# 查找 behaviortree.cpp
find_package(behaviortree_cpp REQUIRED)

file(GLOB SOURCE_FILES "*.cpp")

# 创建可执行文件
add_library(${LIBRARY_NAME} ${SOURCE_FILES})

# 使用现代 CMake 方式链接到 behaviortree_cpp target
target_link_libraries(${LIBRARY_NAME} PUBLIC BT::behaviortree_cpp)
target_link_libraries(${LIBRARY_NAME} PUBLIC utils::logging)
target_link_libraries(${LIBRARY_NAME} PUBLIC ${Boost_LIBRARIES})
target_link_libraries(${LIBRARY_NAME} PUBLIC third_party::tl_expected)

# 使用 PUBLIC 或 INTERFACE，因为 thread_pool.hpp 是头文件库，依赖它的目标也需要访问 Boost 头文件
target_include_directories(${LIBRARY_NAME} PUBLIC ${Boost_INCLUDE_DIRS})

target_compile_options(${LIBRARY_NAME} PRIVATE -Wall -Wextra -Wunused-function)

add_library(${NAMESPACE}::${CUR_NAME} ALIAS ${LIBRARY_NAME})

# 别的库引用时，导入当前目录
target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
