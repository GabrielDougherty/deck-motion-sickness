cmake_minimum_required(VERSION 3.28)
project(MotionSafeLayer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable experimental C++ module support
set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# On macOS, ensure SDK paths are available for module scanning
if(APPLE)
    execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE MACOS_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(MACOS_SDK_PATH)
        add_compile_options(-isysroot ${MACOS_SDK_PATH})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isysroot ${MACOS_SDK_PATH}")
    endif()
endif()

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
    src/entry_points.cpp
    src/overlay.cpp
)

# Build shared library
add_library(motionsafe_overlay SHARED)

target_sources(motionsafe_overlay
    PRIVATE
        ${LAYER_SOURCES}
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/dispatch_table.cppm
            src/layer.cppm
)

target_include_directories(motionsafe_overlay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(motionsafe_overlay
    ${Vulkan_LIBRARIES}
)

# Homebrew LLVM on macOS needs explicit C++ stdlib linking with library path
# Source - https://stackoverflow.com/a
if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_libraries(motionsafe_overlay "-L/opt/homebrew/opt/llvm/lib/c++" "-lc++" "-Wl,-no_warn_duplicate_libraries")
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
