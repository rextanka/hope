cmake_minimum_required(VERSION 3.20)
project(hope CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Path to Hope standard library modules (../lib relative to this file)
set(HOPELIB "${CMAKE_CURRENT_SOURCE_DIR}/../lib"
    CACHE PATH "Path to Hope standard library")

# ---------------------------------------------------------------------------
# Google Test — fetched at configure time, never installed alongside hope
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(INSTALL_GTEST   OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK     ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# ---------------------------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------------------------
add_subdirectory(src)

enable_testing()
add_subdirectory(test)
