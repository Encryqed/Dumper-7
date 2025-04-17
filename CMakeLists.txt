cmake_minimum_required(VERSION 3.15...4.0)

# Ninja generator does not support platform specification
if(CMAKE_GENERATOR MATCHES "Ninja")
    # Remove all variables related to platform
    unset(CMAKE_GENERATOR_PLATFORM CACHE)
    unset(CMAKE_GENERATOR_PLATFORM)
    unset(CMAKE_VS_PLATFORM_NAME CACHE)
    unset(CMAKE_VS_PLATFORM_NAME)
    message(STATUS "Using Ninja generator - platform specification disabled")
endif()

project(Dumper-7 LANGUAGES CXX C)

include(${CMAKE_SOURCE_DIR}/cmake/common.cmake)

# Source files - automatic search for all source files
file(GLOB_RECURSE CPP_SOURCES 
    "Dumper/*.cpp"
    "Dumper/*.cxx"
    "Dumper/*.cc"
    "Dumper/*.c"
)

add_library(${PROJECT_NAME} SHARED ${CPP_SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
# Dumper
    ${CMAKE_SOURCE_DIR}/Dumper

    # Engine
    ${CMAKE_SOURCE_DIR}/Dumper/Engine
    ${CMAKE_SOURCE_DIR}/Dumper/Engine/Private
    ${CMAKE_SOURCE_DIR}/Dumper/Engine/Private/OffsetFinder
    ${CMAKE_SOURCE_DIR}/Dumper/Engine/Private/Unreal

    # Engine Public
    ${CMAKE_SOURCE_DIR}/Dumper/Engine/Public
    ${CMAKE_SOURCE_DIR}/Dumper/Engine/Public/OffsetFinder
    ${CMAKE_SOURCE_DIR}/Dumper/Engine/Public/Unreal

    # Generator
    ${CMAKE_SOURCE_DIR}/Dumper/Generator
    ${CMAKE_SOURCE_DIR}/Dumper/Generator/Private
    ${CMAKE_SOURCE_DIR}/Dumper/Generator/Private/Generators
    ${CMAKE_SOURCE_DIR}/Dumper/Generator/Private/Managers
    ${CMAKE_SOURCE_DIR}/Dumper/Generator/Private/Wrappers

    # Generator Public
    ${CMAKE_SOURCE_DIR}/Dumper/Generator/Public
    ${CMAKE_SOURCE_DIR}/Dumper/Generator/Public/Generators
    ${CMAKE_SOURCE_DIR}/Dumper/Generator/Public/Managers
    ${CMAKE_SOURCE_DIR}/Dumper/Generator/Public/Wrappers

    # Utils
    ${CMAKE_SOURCE_DIR}/Dumper/Utils
    ${CMAKE_SOURCE_DIR}/Dumper/Utils/Compression
    ${CMAKE_SOURCE_DIR}/Dumper/Utils/Dumpspace
    ${CMAKE_SOURCE_DIR}/Dumper/Utils/Encoding
    ${CMAKE_SOURCE_DIR}/Dumper/Utils/Json
)

# Compiler definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    _CONSOLE
    WIN32
)

# Set Windows subsystem
set_target_properties(${PROJECT_NAME} PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    VS_GLOBAL_KEYWORD "Win32Proj"
) 