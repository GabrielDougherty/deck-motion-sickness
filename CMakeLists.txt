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

# Find glslc shader compiler
find_program(GLSLC glslc HINTS ${Vulkan_GLSLC_EXECUTABLE} PATHS ENV VULKAN_SDK PATH_SUFFIXES bin)
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found! Please install the Vulkan SDK.")
endif()

message(STATUS "Found glslc: ${GLSLC}")

# Shader compilation function
function(add_shader TARGET SHADER)
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER_NAME}.spv)
    set(SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER})
    
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${GLSLC} ${SHADER_SOURCE} -o ${SHADER_OUTPUT}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling shader: ${SHADER_NAME}"
        VERBATIM
    )
    
    set_source_files_properties(${SHADER_OUTPUT} PROPERTIES GENERATED TRUE)
    target_sources(${TARGET} PRIVATE ${SHADER_OUTPUT})
endfunction()

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
            src/overlay.cppm
)

target_include_directories(motionsafe_overlay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(motionsafe_overlay
    ${Vulkan_LIBRARIES}
)

# Compile shaders
add_shader(motionsafe_overlay motion_overlay.vert)
add_shader(motionsafe_overlay motion_overlay.frag)

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
    set(LIBRARY_EXTENSION ".dylib")
elseif(UNIX)
    set(LIBRARY_EXTENSION ".so")
endif()

# Determine installation paths
if(APPLE)
    # macOS: Install to user's local directory by default
    set(LAYER_INSTALL_DIR "$ENV{HOME}/.local/share/vulkan/explicit_layer.d" CACHE PATH "Layer manifest installation directory")
    set(LIBRARY_INSTALL_DIR "$ENV{HOME}/.local/lib" CACHE PATH "Library installation directory")
elseif(UNIX)
    # Linux: Install to user's local directory (works with Proton/Steam)
    set(LAYER_INSTALL_DIR "$ENV{HOME}/.local/share/vulkan/implicit_layer.d" CACHE PATH "Layer manifest installation directory")
    set(LIBRARY_INSTALL_DIR "$ENV{HOME}/.local/lib" CACHE PATH "Library installation directory")
endif()

# Set the library path in the manifest
set(LIBRARY_PATH "${LIBRARY_INSTALL_DIR}/libmotionsafe_overlay${LIBRARY_EXTENSION}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/layer_manifest.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/VK_LAYER_MOTIONSAFE_overlay.json
    @ONLY
)

# Installation rules
install(TARGETS motionsafe_overlay
    LIBRARY DESTINATION ${LIBRARY_INSTALL_DIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VK_LAYER_MOTIONSAFE_overlay.json
    DESTINATION ${LAYER_INSTALL_DIR}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

# Install shaders
install(FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/motion_overlay.vert.spv
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/motion_overlay.frag.spv
    DESTINATION ${LIBRARY_INSTALL_DIR}/shaders
)

# On Linux, also install to system directories (requires sudo)
if(UNIX AND NOT APPLE)
    # Create system manifest with system library path
    set(OLD_LIBRARY_PATH "${LIBRARY_PATH}")
    set(LIBRARY_PATH "/usr/lib/libmotionsafe_overlay${LIBRARY_EXTENSION}")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/layer_manifest.json.in
        ${CMAKE_CURRENT_BINARY_DIR}/VK_LAYER_MOTIONSAFE_overlay_system.json
        @ONLY
    )
    set(LIBRARY_PATH "${OLD_LIBRARY_PATH}")
    
    install(TARGETS motionsafe_overlay
        LIBRARY DESTINATION /usr/lib
    )
    
    # Install shaders to system directory too
    install(FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/motion_overlay.vert.spv
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/motion_overlay.frag.spv
        DESTINATION /usr/lib/shaders
    )
    
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VK_LAYER_MOTIONSAFE_overlay_system.json
        DESTINATION /usr/share/vulkan/implicit_layer.d
        RENAME VK_LAYER_MOTIONSAFE_overlay.json
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endif()

# Post-install script to update loader settings (macOS only)
if(APPLE)
    install(CODE "
        message(STATUS \"Updating Vulkan loader settings...\")
        execute_process(
            COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/update_loader_settings.py
            RESULT_VARIABLE UPDATE_RESULT
            OUTPUT_VARIABLE UPDATE_OUTPUT
            ERROR_VARIABLE UPDATE_ERROR
        )
        if(UPDATE_RESULT EQUAL 0)
            message(STATUS \"\${UPDATE_OUTPUT}\")
        else()
            message(WARNING \"Failed to update loader settings: \${UPDATE_ERROR}\")
            message(STATUS \"You may need to manually add the layer to:\")
            message(STATUS \"  $ENV{HOME}/.local/share/vulkan/loader_settings.d/vk_loader_settings.json\")
        endif()
    ")
endif()

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Vulkan Include: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan Library: ${Vulkan_LIBRARIES}")
