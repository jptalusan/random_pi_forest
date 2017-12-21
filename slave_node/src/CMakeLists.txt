CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
#set(TARGET "../randomForests_exe")

file(GLOB SOURCE_FILES "*.cpp")
add_executable(rf_exe ${SOURCE_FILES})
