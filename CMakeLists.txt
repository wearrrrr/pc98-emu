cmake_minimum_required(VERSION 3.16)
project(PC98Emu)

set(BUILD_TARGET linux)

set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-extensions -fms-compatibility-version=19")

if (BUILD_TARGET STREQUAL "linux")
    include("cmake/linux.cmake")
elseif (BUILD_TARGET STREQUAL "win32")
    include("cmake/windows.cmake")
else()
    message (FATAL_ERROR "Unknown build kind: ${BUILD_TARGET}")
endif()

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)

find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h)

add_executable(PC98Emu ${SOURCES} ${HEADERS})

target_link_libraries(PC98Emu PRIVATE SDL2::SDL2main)

target_link_libraries(PC98Emu PRIVATE SDL2::SDL2)