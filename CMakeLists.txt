cmake_minimum_required(VERSION 2.6)
project(cltracer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
file(STRINGS "cltracer.files" SOURCES REGEX .c$|.cpp$)
add_definitions(-DRAY_GL -DCL_GL_INTEROP -DCL_PROFILING)
include_directories(sources/cl)

add_executable(cltracer ${SOURCES})
target_link_libraries(${PROJECT_NAME} -lm -lOpenCL -lSDL2 -lGL -lGLEW)
