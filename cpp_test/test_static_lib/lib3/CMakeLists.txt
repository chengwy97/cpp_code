add_library(lib3 STATIC
    lib3.cpp
)

target_include_directories(lib3 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

