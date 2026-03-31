cmake_minimum_required(VERSION 3.10)

get_filename_component(CUR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EXECUTABLE_NAME "${NAMESPACE}_${CUR_NAME}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCE_FILES "*.cpp")

add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})
target_link_libraries(${EXECUTABLE_NAME} PRIVATE
    iceoryx_hoofs::iceoryx_hoofs
    iceoryx_platform::iceoryx_platform
    CLI11::CLI11)

target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall -Wextra -O2)
