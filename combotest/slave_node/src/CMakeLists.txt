CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

file(GLOB SOURCE_FILES "*.cpp")
include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/slave_node/include)
add_executable(slave_node ${SOURCE_FILES})
target_link_libraries(slave_node common)
install(TARGETS slave_node RUNTIME DESTINATION bin/slave_node)
add_custom_command(
    TARGET slave_node POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/slave_node/configs.json
            ${CMAKE_SOURCE_DIR}/bin/slave_node/configs.json)
