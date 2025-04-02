cmake_minimum_required(VERSION 3.5)
set(CMAKE_SUPPRESS_REGENERATION TRUE CACHE BOOL "Disable Zero Check Project")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

set(PROJ_NAME "tracing_malloc")
project(${PROJ_NAME})

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
   add_definitions(-D_DEBUG)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fvisibility=hidden -Wl,--exclude-libs=ALL -ffunction-sections")

set(USE_LIBUNWIND true)
if(USE_LIBUNWIND)
    add_definitions(-DUSE_LIBUNWIND) 
endif(USE_LIBUNWIND)

file(GLOB_RECURSE PROJ_SRC 
    *.h
    *.cpp
    *.cc
    *.c
)

add_library(${PROJ_NAME} SHARED ${PROJ_SRC})
target_link_libraries(${PROJ_NAME} dl)
target_link_libraries(${PROJ_NAME} pthread)

if(USE_LIBUNWIND) 
    target_link_libraries(${PROJ_NAME} unwind.a)
endif(USE_LIBUNWIND)

add_custom_command(
    TARGET ${PROJ_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/src/libtracing_malloc.so /tmp/libtracing_malloc.so
    # COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/tools/addr2line.py /tmp/addr2line.py
)

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
message(STATUS CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR})
message(STATUS PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR})
message(STATUS PROJECT_BINARY_DIR=${PROJECT_BINARY_DIR})
message(STATUS *******************************************************)
