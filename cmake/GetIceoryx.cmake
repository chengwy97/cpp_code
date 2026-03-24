include(FetchContent)

message(STATUS "get Iceoryx ...")

FetchContent_Declare(iceoryx SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/iceoryx/iceoryx_meta")

FetchContent_MakeAvailable(iceoryx)

# add_library(third_party::iceoryx ALIAS iceoryx)

# sudo apt-get install libacl1-dev



