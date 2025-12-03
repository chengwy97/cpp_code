# 添加三个静态库
add_subdirectory(lib1)
add_subdirectory(lib2)
add_subdirectory(lib3)

# 添加一个动态库
add_subdirectory(shared_lib)

# 添加主程序
add_subdirectory(main)

