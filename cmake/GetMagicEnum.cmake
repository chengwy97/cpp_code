include(FetchContent)

message(STATUS "get magic_enum ...")

add_library(magic_enum INTERFACE)
add_library(third_party::magic_enum ALIAS magic_enum)

# Set include path of target
target_include_directories(
  magic_enum
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/magic_enum/include)
