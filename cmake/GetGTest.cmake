include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)
message(STATUS "get GTest (with GMock) ...")
CPMAddPackage(
NAME googletest
GITHUB_REPOSITORY google/googletest
VERSION 1.15.2
OPTIONS
  "INSTALL_GTEST OFF"
  "gtest_force_shared_crt ON"
  "BUILD_GMOCK ON"
)
