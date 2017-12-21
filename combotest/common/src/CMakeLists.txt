CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

add_library(common concurrency.cpp rts_tree.cpp rts_forest.cpp)

install(TARGETS common
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
