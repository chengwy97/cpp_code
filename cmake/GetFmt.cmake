include(FetchContent)

message(STATUS "get fmt ...")

add_library(fmt INTERFACE)
add_library(third_party::fmt ALIAS fmt)
add_library(fmt::fmt ALIAS fmt)

# Set include path of target
target_include_directories(
  fmt
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmt/include)