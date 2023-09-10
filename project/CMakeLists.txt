cmake_minimum_required(VERSION 3.25)
project(RayVis VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options("/MP")

set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(RAYVIS_SOURCE_DIR ${RayVis_SOURCE_DIR} CACHE INTERNAL "RAYVIS_SOURCE_DIR")
set(RAYVIS_INCLUDE_DIR ${RayVis_SOURCE_DIR}/include CACHE INTERNAL "RAYVIS_INCLUDE_DIR")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)


add_subdirectory(imported)

add_subdirectory(rayloader)
add_subdirectory(rayvis-utils)
add_subdirectory(d3d12ex)

set_property(DIRECTORY "." PROPERTY VS_STARTUP_PROJECT d3d12ex)