cmake_minimum_required(VERSION 3.20)
project(cpp_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(utils)
add_subdirectory(boost_test)
add_subdirectory(behaviortree_test)
