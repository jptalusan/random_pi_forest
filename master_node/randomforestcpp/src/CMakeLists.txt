CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
#set(TARGET "../randomForests_exe")

set(SOURCE_FILES main_training.cpp rts_forest.cpp rts_tree.cpp)
add_executable(rf_exe ${SOURCE_FILES})
