include(FetchContent)

message(STATUS "get GSL ...")

FetchContent_Declare(gsl SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/GSL")

FetchContent_MakeAvailable(GSL)



