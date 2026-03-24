include(FetchContent)

message(STATUS "get debug_assert ...")

add_library(debug_assert INTERFACE)
add_library(third_party::debug_assert ALIAS debug_assert)
add_library(debug_assert::debug_assert ALIAS debug_assert)

# Set include path of target
target_include_directories(
  debug_assert
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/debug_assert)
