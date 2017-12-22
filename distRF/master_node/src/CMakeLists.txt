CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

file(GLOB SOURCE_FILES "*.cpp")
include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/master_node/include)
add_executable(master_node ${SOURCE_FILES})
target_link_libraries(master_node common)
#add_library(${CMAKE_SOURCE_DIR}/lib/libcommon.a STATIC IMPORTED)
install(TARGETS master_node RUNTIME DESTINATION bin/master_node)
add_custom_command(
    TARGET master_node POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/master_node/configs.json
            ${CMAKE_SOURCE_DIR}/bin/master_node/configs.json)
