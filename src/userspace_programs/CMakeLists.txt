cmake_minimum_required(VERSION 3.10)

project(UserspacePrograms)

OPTION(ENABLE_SANI "Compile tests with sanitizers" OFF)

IF(NOT ENABLE_SANI)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  set(BUILD_SHARED_LIBS OFF)
  set(CMAKE_EXE_LINKER_FLAGS "-static")
ENDIF(ENABLE_SANI)

set(ARCH "" CACHE STRING "Target CPU architecture, currently Intel CPU available")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    # set(CMAKE_BUILD_TYPE Release)
    # set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif ()

if (ARCH STREQUAL "")
else()
    message("Target CPU microarch set to: ${ARCH}")
    set(CMAKE_C_FLAGS "-march=${ARCH}")
    set(CMAKE_CXX_FLAGS "-march=${ARCH}")
endif()

include(CheckCSourceCompiles)

CHECK_C_SOURCE_COMPILES("
#include <stdlib.h>
#include <immintrin.h>
int main() {
    #ifndef __MOVDIR64B__
    deliberate error message
    #endif
    return 0;
}" MOVDIR64B_ENABLED)

# Print the result
if (MOVDIR64B_ENABLED)
    add_definitions(-D__MOVDIR64B_ENABLED__=1)
    message(STATUS "MOVDIR64B enabled")
else()
    message(STATUS "MOVDIR64B disabled")
endif()

CHECK_C_SOURCE_COMPILES("
#include <stdlib.h>
#include <immintrin.h>
int main() {
    #ifndef __AVX512F__
    deliberate error message
    #endif
    return 0;
}" AVX512F_ENABLED)

# Print the result
if (AVX512F_ENABLED)
    add_definitions(-D__AVX512F_ENABLED__=1)
    message(STATUS "AVX512F enabled")
else()
    message(STATUS "AVX512F disabled")
endif()


CHECK_C_SOURCE_COMPILES("
#include <stdlib.h>
#include <immintrin.h>
int main() {
    #ifndef __AVX__
    deliberate error message
    #endif
    return 0;
}" AVX_ENABLED)

# Print the result
if (AVX_ENABLED)
    add_definitions(-D__AVX_ENABLED__=1)
    message(STATUS "AVX enabled")
else()
    message(STATUS "AVX disabled")
endif()

CHECK_C_SOURCE_COMPILES("
#include <stdlib.h>
#include <immintrin.h>
int main() {
    #ifndef __SSE2__
    deliberate error message
    #endif
    return 0;
}" SSE2_ENABLED)

# Print the result
if (SSE2_ENABLED)
    add_definitions(-D__SSE2_ENABLED__=1)
    message(STATUS "SSE2 enabled")
else()
    message(STATUS "SSE2 disabled")
endif()

CHECK_C_SOURCE_COMPILES("
#include <stdlib.h>
#include <x86intrin.h>
int main() {
    int a = 0;
    _mm_clflush(&a);
    return 0;
}" CLFLUSH_ENABLED)

# Print the result
if (CLFLUSH_ENABLED)
    add_definitions(-D__CLFLUSH_ENABLED__=1)
    message(STATUS "CLFLUSH enabled")
else()
    message(STATUS "CLFLUSH disabled")
endif()

CHECK_C_SOURCE_COMPILES("
#include <stdlib.h>
#include <x86intrin.h>
int main() {
    int a = 0;
    _mm_clflushopt(&a);
    return 0;
}" CLFLUSHOPT_ENABLED)

# Print the result
if (CLFLUSHOPT_ENABLED)
    add_definitions(-D__CLFLUSHOPT_ENABLED__=1)
    message(STATUS "CLFLUSHOPT enabled")
else()
    message(STATUS "CLFLUSHOPT disabled")
endif()

CHECK_C_SOURCE_COMPILES("
#include <stdlib.h>
#include <x86intrin.h>
int main() {
    int a = 0;
    _mm_clwb(&a);
    return 0;
}" CLWB_ENABLED)

# Print the result
if (CLWB_ENABLED)
    add_definitions(-D__CLWB_ENABLED__=1)
    message(STATUS "CLWB ensabled")
else()
    message(STATUS "CLWB disabled")
endif()

include_directories(${PROJECT_SOURCE_DIR})

find_package (gflags REQUIRED)

add_subdirectory(third_party)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_FLAGS -Wpedantic)
set(CMAKE_C_FLAGS -Wpedantic)
enable_testing()

add_subdirectory(tpp)
