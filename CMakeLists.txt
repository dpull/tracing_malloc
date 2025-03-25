cmake_minimum_required(VERSION 3.12)
set(CMAKE_SUPPRESS_REGENERATION TRUE CACHE BOOL "Disable Zero Check Project")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(PROJ_NAME "tracing_malloc")
project(${PROJ_NAME})

add_subdirectory(src)
add_subdirectory(tools/alloc_comparison)
add_subdirectory(tools/stacktrace_comparison)
