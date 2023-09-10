project(CLI11 VERSION 1.9.1)
add_library(cli11 INTERFACE)
target_include_directories(cli11 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})