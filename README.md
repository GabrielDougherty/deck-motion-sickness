# MotionSafe Vulkan Layer

A minimal Vulkan implicit layer for anti-motion-sickness overlay, designed for Steam Deck but testable on macOS using MoltenVK.

## Project Structure

```
deck-motion-sickness/
├── CMakeLists.txt               # Build configuration with C++23 modules support
├── setup-vulkan-env.sh          # Script to set up Vulkan environment for testing
├── layer_manifest.json.in       # Layer manifest template
├── .gitignore                   # Git ignore patterns
├── include/
│   └── layer.h                  # C header for layer entry points
├── src/
│   ├── dispatch_table.cppm      # C++23 module: dispatch table management
│   ├── layer.cppm               # C++23 module: layer implementation
│   ├── layer.cpp                # Thin C wrapper calling module functions
│   ├── entry_points.cpp         # vkGetInstanceProcAddr/vkGetDeviceProcAddr
│   └── overlay.cpp              # Overlay rendering (placeholder)
├── scripts/
│   └── update_loader_settings.py # Python script to update Vulkan loader settings
├── shaders/
│   ├── placeholder.vert         # Placeholder vertex shader
│   └── placeholder.frag         # Placeholder fragment shader
├── build/
│   ├── libmotionsafe_overlay.dylib          # Compiled layer library
│   └── VK_LAYER_MOTIONSAFE_overlay.json     # Generated layer manifest
└── README.md
```

### Key Features
- **C++23 Modules**: Modern modular architecture with `dispatch_table.cppm` and `layer.cppm`
- **Clean Separation**: C++ implementation in modules, thin C wrapper for Vulkan loader
- **Homebrew LLVM**: Uses Clang 21+ with full C++23 support
- **clangd Integration**: IntelliSense support for C++ modules

## Requirements

### macOS (Development)
- macOS 14+ (for C++23 support)
- Homebrew LLVM Clang 21+ (for C++ modules)
- CMake 3.28+ (for C++ module support)
- Ninja build system
- Vulkan SDK 1.4+ with MoltenVK

### Linux/Steam Deck (Production)
- Linux kernel 5.x+
- Clang 17+ or GCC 14+ (for C++23 modules)
- CMake 3.28+
- Ninja
- Vulkan SDK

## Installing Dependencies

### macOS

1. **Install Homebrew** (if not already installed):
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

2. **Install Homebrew LLVM and build tools**:
```bash
brew install llvm cmake ninja
```

3. **Install Vulkan SDK**:
   - Download from: https://vulkan.lunarg.com/sdk/home
   - Install to `~/VulkanSDK/<version>/`

4. **Install testing tools** (optional):
```bash
brew install vulkan-tools  # Provides vkcube for testing
```

5. **Set up environment**:
```bash
# Source the setup script in your shell
source setup-vulkan-env.sh
```

### Linux/Steam Deck

```bash
# Arch Linux / Steam Deck
sudo pacman -S cmake ninja clang vulkan-headers vulkan-icd-loader

# Ubuntu/Debian
sudo apt install cmake ninja-build clang vulkan-tools libvulkan-dev
```

## Building

### macOS

```bash
# Set up Vulkan environment
source setup-vulkan-env.sh

# Create build directory
mkdir -p build && cd build

# Configure with CMake (using Homebrew LLVM)
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
  ..

# Build
ninja
```

The build produces:
- `libmotionsafe_overlay.dylib` - The layer library
- `VK_LAYER_MOTIONSAFE_overlay.json` - The layer manifest

### Installing the Layer (macOS)

The layer can be installed to your user's local Vulkan directory:

```bash
cd build
ninja install
```

This will:
- Install `libmotionsafe_overlay.dylib` to `~/.local/lib/`
- Install `VK_LAYER_MOTIONSAFE_overlay.json` to `~/.local/share/vulkan/explicit_layer.d/`
- Automatically update `~/.local/share/vulkan/loader_settings.d/vk_loader_settings.json` (if it exists)

The manifest will be configured with the absolute path to the library, so it will work from anywhere.

**Verifying Installation:**
```bash
# Check if files were installed
ls ~/.local/lib/libmotionsafe_overlay.dylib
ls ~/.local/share/vulkan/explicit_layer.d/VK_LAYER_MOTIONSAFE_overlay.json
```

**Uninstalling:**
```bash
rm ~/.local/lib/libmotionsafe_overlay.dylib
rm ~/.local/share/vulkan/explicit_layer.d/VK_LAYER_MOTIONSAFE_overlay.json
# Manually remove from loader settings if needed
```

### Linux

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..

# Build
ninja

# Install (requires sudo for system-wide installation)
sudo ninja install
```

## Testing

### macOS Testing with vkcube

1. **Build and install the layer** (see Building section above)

2. **Set up Vulkan environment**:
```bash
source setup-vulkan-env.sh
```

3. **Run vkcube with the layer**:
```bash
# Using Homebrew vulkan-tools vkcube
VK_INSTANCE_LAYERS="VK_LAYER_MOTIONSAFE_overlay" \
  /opt/homebrew/Cellar/vulkan-tools/1.4.328.1/cube/vkcube.app/Contents/MacOS/vkcube

# Or use VulkanSDK vkcube if available
VK_INSTANCE_LAYERS="VK_LAYER_MOTIONSAFE_overlay" \
  ~/VulkanSDK/1.4.328.1/Applications/vkcube.app/Contents/MacOS/vkcube
```

4. **Expected output**:
You should see console output like:
```
Selected WSI platform: metal
[MotionSafe] vkCreateInstance called
[MotionSafe] Instance created successfully
Selected GPU 0: Apple M4 Pro, type: IntegratedGpu
[MotionSafe] vkCreateDevice called
[MotionSafe] Device created successfully
[MotionSafe] vkQueuePresentKHR called
[MotionSafe] vkQueuePresentKHR called
...
```

### Verifying Layer Installation

Check if your layer is recognized by the Vulkan loader:
```bash
# List all available layers
vulkaninfo --summary | grep -A5 "Layers"

# Or check with loader debug output
VK_LOADER_DEBUG=all vulkaninfo 2>&1 | grep MOTIONSAFE
```

### Testing with Other Vulkan Applications

Try with any Vulkan application:
```bash
source setup-vulkan-env.sh
VK_INSTANCE_LAYERS="VK_LAYER_MOTIONSAFE_overlay" your_vulkan_app
```

## Steam Deck Deployment

### Installation on Steam Deck

1. **Build on Steam Deck** (or cross-compile):
```bash
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/.local ..
ninja
ninja install
```

2. **Verify installation**:
```bash
ls $HOME/.local/share/vulkan/implicit_layer.d/
ls $HOME/.local/lib/
```

3. **Set environment for Steam games**:

   In Steam, right-click game → Properties → Launch Options:
```bash
VK_LAYER_PATH=$HOME/.local/share/vulkan/implicit_layer.d %command%
```

### Making Layer Implicit

The layer manifest is configured as implicit (`type: GLOBAL`), which means it will automatically load for all Vulkan applications once installed.

To **enable** the layer:
```bash
export ENABLE_MOTIONSAFE_LAYER=1
```

To **disable** the layer:
```bash
export DISABLE_MOTIONSAFE_LAYER=1
```

Or unset `ENABLE_MOTIONSAFE_LAYER`.

## Development Notes

### Layer Architecture

This layer implements the standard Vulkan layer chain pattern:

1. **Instance Creation**: Intercepts `vkCreateInstance`, chains to next layer
2. **Device Creation**: Intercepts `vkCreateDevice`, chains to next layer
3. **Present Hook**: Intercepts `vkQueuePresentKHR` (main injection point)
4. **Dispatch Tables**: Maintains function pointer tables for efficient forwarding

### Key Intercept Points

- **vkCreateInstance**: Initialize instance-level dispatch table
- **vkCreateDevice**: Initialize device-level dispatch table
- **vkQueuePresentKHR**: Primary hook for overlay rendering (currently just logs)

### Next Steps for Implementation

1. **Overlay Rendering**: Implement actual rendering in `src/overlay.cpp`
2. **Shader Compilation**: Add shader compilation pipeline
3. **Graphics Pipeline**: Create Vulkan pipeline for overlay
4. **Anti-Motion-Sickness**: Implement your motion reduction algorithm
5. **Configuration**: Add runtime configuration system
6. **Performance**: Profile and optimize hot paths

## Troubleshooting

### macOS: "Library not loaded" error

Ensure `DYLD_LIBRARY_PATH` includes Vulkan SDK:
```bash
export DYLD_LIBRARY_PATH="$VULKAN_SDK/lib:$DYLD_LIBRARY_PATH"
```

### Layer not loading

1. Check `VK_LAYER_PATH` points to directory containing `.json` file
2. Verify `.dylib`/`.so` is in same directory as `.json`
3. Check layer name matches in manifest and environment variable

### No output in console

Some applications redirect stdout. Try:
```bash
your_app 2>&1 | grep MotionSafe
```

## Resources

- [Vulkan Layer Specification](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderLayerInterface.md)
- [Vulkan SDK Documentation](https://vulkan.lunarg.com/doc/sdk)
- [MoltenVK Documentation](https://github.com/KhronosGroup/MoltenVK)
- [Steam Deck Development](https://partner.steamgames.com/doc/steamdeck/devkit)

## License

[Add your license here]

## Author

[Your name/contact]
