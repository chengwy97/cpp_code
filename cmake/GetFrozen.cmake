include(FetchContent)

message(STATUS "get frozen ...")

add_library(frozen INTERFACE)
add_library(third_party::frozen ALIAS frozen)

# Set include path of target
target_include_directories(
  frozen
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/frozen/include)
