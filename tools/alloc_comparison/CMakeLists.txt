cmake_minimum_required(VERSION 3.5)
set(CMAKE_SUPPRESS_REGENERATION TRUE CACHE BOOL "Disable Zero Check Project")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

set(PROJ_NAME "alloc_comparison")
project(${PROJ_NAME})

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
   add_definitions(-D_DEBUG)
endif()

aux_source_directory(. PROJ_SRC)

add_executable(${PROJ_NAME} ${PROJ_SRC})

message(STATUS *******************************************************)
message(STATUS CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})
message(STATUS CMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION})
message(STATUS CMAKE_HOST_SYSTEM_NAME=${CMAKE_HOST_SYSTEM_NAME})
message(STATUS CMAKE_GENERATOR=${CMAKE_GENERATOR})
message(STATUS CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
message(STATUS CMAKE_C_COMPILER=${CMAKE_C_COMPILER})
message(STATUS CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER})
message(STATUS CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES})
message(STATUS CMAKE_CXX_STANDARD_LIBRARIES=${CMAKE_CXX_STANDARD_LIBRARIES})
message(STATUS CMAKE_C_FLAGS=${CMAKE_C_FLAGS})
message(STATUS CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS})
message(STATUS CMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS})
message(STATUS CMAKE_MODULE_LINKER_FLAGS=${CMAKE_MODULE_LINKER_FLAGS})
message(STATUS CMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS})
message(STATUS CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR})
message(STATUS *******************************************************)
