project(randomForests)
cmake_minimum_required(VERSION 2.8)

include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/common/include)
add_subdirectory(src)
