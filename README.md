# MotionSafe Vulkan Layer

A minimal Vulkan implicit layer for anti-motion-sickness overlay, designed for Steam Deck but testable on macOS using MoltenVK.

## Project Structure

```
deck-motion-sickness/
├── CMakeLists.txt
├── include/
│   ├── layer.h              # Layer entry points and exports
│   └── dispatch_table.h     # Instance/device dispatch tables
├── src/
│   ├── layer.cpp            # Core layer hooks (CreateInstance, CreateDevice, QueuePresent)
│   ├── dispatch_table.cpp   # Dispatch table management
│   ├── entry_points.cpp     # vkGetInstanceProcAddr/vkGetDeviceProcAddr
│   └── overlay.cpp          # Overlay rendering (placeholder)
├── shaders/
│   ├── placeholder.vert     # Placeholder vertex shader
│   └── placeholder.frag     # Placeholder fragment shader
├── layer_manifest.json.in   # Layer manifest template
└── README.md
```

## Requirements

### macOS (Development)
- macOS 10.15+
- Xcode Command Line Tools (for clang)
- CMake 3.20+
- Ninja build system
- Vulkan SDK with MoltenVK

### Linux/Steam Deck (Production)
- Linux kernel 5.x+
- Clang or GCC
- CMake 3.20+
- Ninja
- Vulkan SDK

## Installing Dependencies

### macOS

1. **Install Homebrew** (if not already installed):
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

2. **Install build tools**:
```bash
brew install cmake ninja
```

3. **Install Vulkan SDK**:
   - Download from: https://vulkan.lunarg.com/sdk/home
   - Or use Homebrew:
```bash
brew install --cask vulkan-sdk
```

4. **Set up environment** (add to `~/.zshrc`):
```bash
export VULKAN_SDK="$HOME/VulkanSDK/<version>/macOS"
export PATH="$VULKAN_SDK/bin:$PATH"
export DYLD_LIBRARY_PATH="$VULKAN_SDK/lib:$DYLD_LIBRARY_PATH"
export VK_ICD_FILENAMES="$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json"
export VK_LAYER_PATH="$VULKAN_SDK/share/vulkan/explicit_layer.d"
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
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..

# Build
ninja

# Install (copies to build directory for testing)
ninja install
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

1. **Build the layer** (see above)

2. **Set up layer environment**:
```bash
export VK_INSTANCE_LAYERS=VK_LAYER_MOTIONSAFE_overlay
export VK_LAYER_PATH=/Users/gabriel/ws/deck-motion-sickness/build
```

3. **Run vkcube** (included with Vulkan SDK):
```bash
vkcube
```

4. **Expected output**:
You should see console output like:
```
[MotionSafe] vkCreateInstance called
[MotionSafe] Instance created successfully
[MotionSafe] vkCreateDevice called
[MotionSafe] Device created successfully
[MotionSafe] vkQueuePresentKHR called
[MotionSafe] vkQueuePresentKHR called
...
```

### Verifying Layer Loading

Check if your layer is recognized:
```bash
vulkaninfo --layers | grep -i motionsafe
```

### Testing with Other Vulkan Applications

Try with any Vulkan application:
```bash
export VK_INSTANCE_LAYERS=VK_LAYER_MOTIONSAFE_overlay
export VK_LAYER_PATH=/path/to/build

# Run your Vulkan app
./your_vulkan_app
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
