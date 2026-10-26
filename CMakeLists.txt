cmake_minimum_required(VERSION 3.22)
project(LegendaryTooltipsNative VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(LEVILAMROID_ROOT "" CACHE PATH "Path to the LeviLamroid source tree")
if(NOT LEVILAMROID_ROOT)
    message(FATAL_ERROR "Set -DLEVILAMROID_ROOT=/path/to/LeviLamroid-main")
endif()
if(NOT EXISTS "${LEVILAMROID_ROOT}/include/levilamroid/LeviLamroid.h")
    message(FATAL_ERROR "LEVILAMROID_ROOT does not look like a LeviLamroid source tree")
endif()

add_library(legendary_tooltips_native SHARED
    src/LegendaryTooltipsNative.cpp
)

target_include_directories(legendary_tooltips_native PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${LEVILAMROID_ROOT}/include"
)

target_compile_options(legendary_tooltips_native PRIVATE
    -O2 -fPIC -fvisibility=hidden -Wall -Wextra -Wpedantic
)

# LeviLamroid exports these symbols at runtime; keep the mod compatible with its template's
# unresolved-symbol loading model rather than requiring a host-specific absolute library path.
target_link_libraries(legendary_tooltips_native PRIVATE dl log)
set_target_properties(legendary_tooltips_native PROPERTIES
    OUTPUT_NAME "legendary_tooltips_native"
    PREFIX "lib"
)
