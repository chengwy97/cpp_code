cmake_minimum_required(VERSION 3.10)

get_filename_component(CUR_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(BASE_NAME "${NAMESPACE}_${CUR_NAME}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMPILE_FLAGS -Wall -Wextra -O2)

# ── roudi ──────────────────────────────────────────────────────────────────
add_executable(${BASE_NAME}_roudi 003.roudi.cpp)
target_compile_options(${BASE_NAME}_roudi PRIVATE ${COMPILE_FLAGS})
target_link_libraries(${BASE_NAME}_roudi PRIVATE rt)

# ── sender ─────────────────────────────────────────────────────────────────
add_executable(${BASE_NAME}_sender 003.sender.cpp)
target_compile_options(${BASE_NAME}_sender PRIVATE ${COMPILE_FLAGS})
target_link_libraries(${BASE_NAME}_sender PRIVATE rt)

# ── receiver ───────────────────────────────────────────────────────────────
add_executable(${BASE_NAME}_receiver 003.receiver.cpp)
target_compile_options(${BASE_NAME}_receiver PRIVATE ${COMPILE_FLAGS})
target_link_libraries(${BASE_NAME}_receiver PRIVATE rt)
