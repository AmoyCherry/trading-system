cmake_minimum_required(VERSION 3.24)
project(trading_system VERSION 0.1.0 LANGUAGES CXX)

option(TS_ENABLE_TESTS "Build tests" ON)
option(TS_ENABLE_BENCHMARKS "Build benchmarks" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

# Useful includes (optional)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ProjectOptions OPTIONAL)

add_subdirectory(src)

if(TS_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(TS_ENABLE_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
