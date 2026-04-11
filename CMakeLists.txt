cmake_minimum_required(VERSION 3.25)

project(cest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header-only library target.
add_library(cest INTERFACE)
target_include_directories(cest INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(cest INTERFACE cxx_std_17)

# Examples
option(CEST_BUILD_EXAMPLES "Build examples/self-test" ON)

if (CEST_BUILD_EXAMPLES)
    add_executable(cest_example examples/example.cpp)
    target_link_libraries(cest_example PRIVATE cest)
    # Hot-patching requires the target function to NOT be inlined.
    target_compile_options(cest_example PRIVATE -O0 -fno-inline)
endif ()