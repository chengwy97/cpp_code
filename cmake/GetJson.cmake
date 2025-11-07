include(FetchContent)

message(STATUS "get json ...")

add_library(json INTERFACE)
add_library(third_party::json ALIAS json)

# Set include path of target
target_include_directories(
  json
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json)
