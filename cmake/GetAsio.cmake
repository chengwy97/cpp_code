include(FetchContent)

message(STATUS "get json ...")

add_library(asio INTERFACE)
add_library(third_party::asio ALIAS asio)

# Set include path of target
target_include_directories(
    asio
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/include)
