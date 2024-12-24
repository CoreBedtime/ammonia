# Minimum required CMake version
cmake_minimum_required(VERSION 3.15)

# Project name
project(ammonia LANGUAGES C CXX OBJC)

# Set the CMake build type to Release by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Set the architectures to build for
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64e")

# Add the main executable
set(AMMONIA_SOURCES
    ammonia/main.m
    ammonia/main.h
)

add_executable(ammonia ${AMMONIA_SOURCES})

target_include_directories(ammonia PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link necessary frameworks (e.g., CoreFoundation, Foundation, etc.)
find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)

target_link_libraries(ammonia
    ${COCOA_FRAMEWORK}
    ${COREFOUNDATION_FRAMEWORK}
)

# Add custom library for libinfect
add_library(libinfect SHARED
    libinfect/libinfect.c
    libinfect/frida-gum.h
)
target_link_libraries(libinfect
    ${COCOA_FRAMEWORK}
    ${COREFOUNDATION_FRAMEWORK}
    ${CMAKE_CURRENT_SOURCE_DIR}/libfrida-gum-x86_64-arm64e-arm64.a
)

target_include_directories(libinfect PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


# Add any compiler flags
target_compile_options(ammonia PRIVATE
    -fobjc-arc  # Enable Automatic Reference Counting for Objective-C
)


# Add the opener library
file(GLOB OPENER_SOURCES opener/*.c opener/*.h)
add_library(opener SHARED ${OPENER_SOURCES})
target_include_directories(opener PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)