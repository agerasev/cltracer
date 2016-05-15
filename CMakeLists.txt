cmake_minimum_required(VERSION 2.6)
project(cltracer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")

set(SOURCES
	sources/main.c
	sources/ray.cpp
	sources/gl.c
)

include_directories(.)
include_directories(sources)
include_directories(submodules/libla)
add_definitions(-DRAY_GL -DCL_GL_INTEROP -DCL_PROFILING)

add_executable(${PROJECT_NAME} ${SOURCES})

add_subdirectory(submodules/libcl4u)
include_directories(submodules/libcl4u/include)
include_directories(submodules/libcl4u/includer)

target_link_libraries(${PROJECT_NAME} OpenCL SDL2 GL GLEW cl4u)
