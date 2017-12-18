CMAKE_MINIMUM_REQUIRED(VERSION 3.6)
file(GLOB SOURCE_FILE "*.cpp")
add_executable(concurrency ${SOURCE_FILE})

target_link_libraries(concurrency pahottpp)
target_link_libraries(concurrency pahomqtt3a)
