# 获取父命名空间和当前目录名称，拼接成当前的命名空间
get_filename_component(CUR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
# 父命名空间由父CMakeLists.txt传递，这里假定为 PROJECT_NAMESPACE
set(NAMESPACE "${PROJECT_NAMESPACE}_${CUR_NAME}")
message(STATUS "当前命名空间: ${NAMESPACE}")

add_subdirectory(001.time.sync)
add_subdirectory(002.time.async)
add_subdirectory(003.time.bind_param)





