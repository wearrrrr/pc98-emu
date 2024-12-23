cmake_minimum_required(VERSION 3.16)
project(PC98Emu)

set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-extensions -fms-compatibility-version=19")

set(CMAKE_CXX_STANDARD 20)

option(VENDORED_SDL2 "Use vendored SDL2" OFF)

if (VENDORED_SDL2)
    add_subdirectory(vendor/SDL2)
    set(SDL2_INCLUDE_DIR vendor/SDL2/include)
    set(SDL2_LIBRARY SDL2)
else ()
    find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)

    find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
endif ()

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h)

add_executable(PC98Emu ${SOURCES} ${HEADERS})

if(TARGET SDL2::SDL2main)
    # It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static)
    target_link_libraries(PC98Emu PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(PC98Emu PRIVATE SDL2::SDL2)