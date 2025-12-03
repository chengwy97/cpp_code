add_library(shared_lib SHARED
    shared_lib.cpp
)

target_include_directories(shared_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 在 Windows 上定义导出符号
if(WIN32)
    target_compile_definitions(shared_lib PRIVATE SHARED_LIB_EXPORTS)
endif()

