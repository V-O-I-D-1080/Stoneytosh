cmake_minimum_required(VERSION 3.20)
project(LegacyRed)

# Compiler settings
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MacOS settings
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
set(CMAKE_SYSTEM_NAME Darwin)

# Include paths
include_directories(
    ${CMAKE_SOURCE_DIR}/MacKernelSDK/Headers
    ${CMAKE_SOURCE_DIR}/Lilu.kext/Contents/Resources
    ${CMAKE_SOURCE_DIR}/LegacyRed
)

# Source files
set(SOURCES
    LegacyRed/Framebuffer.cpp
    LegacyRed/LRed.cpp
    LegacyRed/DYLDPatches.cpp
)

# Build settings
add_library(LegacyRed MODULE ${SOURCES} "Learning/learning.cpp")
set_target_properties(LegacyRed PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "kext"
    MACOSX_BUNDLE_IDENTIFIER "com.stoneytosh.LegacyRed"
    PREFIX ""
    SUFFIX ""
)
