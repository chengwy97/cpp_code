cmake_minimum_required(VERSION 3.10)

# 获取父命名空间和当前目录名称作为可执行名一部分
get_filename_component(CUR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(LIBRARY_NAME "${NAMESPACE}_${CUR_NAME}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 获取所有不是以 _test 结尾的 .cpp 源文件
file(GLOB_RECURSE ALL_SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
set(SOURCE_FILES ${ALL_SOURCE_FILES})
list(FILTER SOURCE_FILES EXCLUDE REGEX "(_test\\.cpp$)")
set(TEST_SOURCE_FILES ${ALL_SOURCE_FILES})
list(FILTER TEST_SOURCE_FILES INCLUDE REGEX "(_test\\.cpp$)")

find_package(Boost REQUIRED)

# 如果 SOURCE_FILES 为空，则创建 INTERFACE 库（header-only），否则创建普通库
if(SOURCE_FILES)
    add_library(${LIBRARY_NAME} ${SOURCE_FILES})
    # 启用 spdlog 的位置跟踪宏
    target_compile_definitions(${LIBRARY_NAME} PRIVATE SPDLOG_ACTIVE_LEVEL=0)
    target_compile_options(${LIBRARY_NAME} PRIVATE -Wall -Wextra -O2)
    target_include_directories(${LIBRARY_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
                                                  ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${LIBRARY_NAME} PUBLIC Boost::boost)
    target_link_libraries(${LIBRARY_NAME} PUBLIC third_party::gsl)
else()
    add_library(${LIBRARY_NAME} INTERFACE)
    target_include_directories(${LIBRARY_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..
                                                  ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${LIBRARY_NAME} INTERFACE Boost::boost)
    target_link_libraries(${LIBRARY_NAME} INTERFACE third_party::gsl)
endif()

# 起一个别名
add_library(${NAMESPACE}::${CUR_NAME} ALIAS ${LIBRARY_NAME})

if(TEST_SOURCE_FILES)
    find_package(GTest REQUIRED)
    add_executable(test_${LIBRARY_NAME} ${TEST_SOURCE_FILES})
    target_link_libraries(test_${LIBRARY_NAME} ${LIBRARY_NAME} GTest::GTest GTest::Main)
    add_test(NAME test_${LIBRARY_NAME} COMMAND test_${LIBRARY_NAME} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()
