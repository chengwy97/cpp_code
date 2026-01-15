include(FetchContent)

message(STATUS "get OneTBB ...")

FetchContent_Declare(tbb SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/oneTBB")

FetchContent_MakeAvailable(tbb)


