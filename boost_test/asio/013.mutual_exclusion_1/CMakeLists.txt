cmake_minimum_required(VERSION 3.10)

# 获取父命名空间和当前目录名称作为可执行名一部分
get_filename_component(CUR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EXECUTABLE_NAME "${NAMESPACE}_${CUR_NAME}")

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

# 获取所有的源文件
file(GLOB_RECURSE SOURCE_FILES "*.cpp")

add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})

target_include_directories(${EXECUTABLE_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${Boost_LIBRARIES})

target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall -Wextra -O2)
