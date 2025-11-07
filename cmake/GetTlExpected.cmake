# 移除 FetchContent，因为文件已经存在于本地
message(STATUS "configuring tl_expected ...")

add_library(tl_expected INTERFACE)
add_library(third_party::tl_expected ALIAS tl_expected)

# 设置包含路径，使用绝对路径
target_include_directories(tl_expected INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tl)
