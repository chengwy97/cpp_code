add_executable(test_static_lib_main
    main.cpp
)

# 链接所有库：三个静态库 + 一个动态库
# 使用 PUBLIC 或 INTERFACE 可以让包含目录自动传递
target_link_libraries(test_static_lib_main PRIVATE
    lib1
    lib2
    lib3
    shared_lib
)

