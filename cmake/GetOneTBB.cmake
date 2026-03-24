include(FetchContent)

message(STATUS "get OneTBB ...")

FetchContent_Declare(tbb SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/oneTBB")

set(TBB_TEST OFF)
FetchContent_MakeAvailable(tbb)


