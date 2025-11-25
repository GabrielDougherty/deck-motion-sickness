cmake_minimum_required(VERSION 3.20)
project(MotionSafeLayer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Vulkan SDK
find_package(Vulkan REQUIRED)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

# Source files
set(LAYER_SOURCES
    src/layer.cpp
    src/dispatch_table.cpp
    src/entry_points.cpp
    src/overlay.cpp
)

set(LAYER_HEADERS
    include/layer.h
    include/dispatch_table.h
)

# Build shared library
add_library(motionsafe_overlay SHARED
    ${LAYER_SOURCES}
    ${LAYER_HEADERS}
)

target_link_libraries(motionsafe_overlay
    ${Vulkan_LIBRARIES}
)

# Homebrew LLVM on macOS needs explicit C++ stdlib linking with library path
if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_libraries(motionsafe_overlay "-L/opt/homebrew/opt/llvm/lib/c++" "-lc++")
endif()

# Set output name based on platform
if(APPLE)
    set_target_properties(motionsafe_overlay PROPERTIES
        OUTPUT_NAME "motionsafe_overlay"
        PREFIX "lib"
        SUFFIX ".dylib"
    )
elseif(UNIX)
    set_target_properties(motionsafe_overlay PROPERTIES
        OUTPUT_NAME "motionsafe_overlay"
        PREFIX "lib"
        SUFFIX ".so"
    )
endif()

# Configure layer manifest
if(APPLE)
    set(LIBRARY_PATH "./libmotionsafe_overlay.dylib")
elseif(UNIX)
    set(LIBRARY_PATH "./libmotionsafe_overlay.so")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/layer_manifest.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/VK_LAYER_MOTIONSAFE_overlay.json
    @ONLY
)

# Installation rules
if(UNIX AND NOT APPLE)
    install(TARGETS motionsafe_overlay
        LIBRARY DESTINATION lib
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VK_LAYER_MOTIONSAFE_overlay.json
        DESTINATION share/vulkan/implicit_layer.d
    )
else()
    # For macOS, just copy to build directory for local testing
    install(TARGETS motionsafe_overlay
        LIBRARY DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VK_LAYER_MOTIONSAFE_overlay.json
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Vulkan Include: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan Library: ${Vulkan_LIBRARIES}")
