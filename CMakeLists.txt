cmake_minimum_required(VERSION 3.5)
set(CMAKE_SUPPRESS_REGENERATION TRUE CACHE BOOL "Disable Zero Check Project")
project("tracing_malloc_all")

add_subdirectory(src)
add_subdirectory(tools/alloc_comparison)
add_subdirectory(tools/snapshot_parser)
