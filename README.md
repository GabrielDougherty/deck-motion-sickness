# MotionSafe Vulkan Layer

A Vulkan layer that renders animated dots along the top and bottom edges of the screen to reduce motion sickness. Designed for Steam Deck.

## Dependencies

### macOS

```bash
brew install llvm cmake ninja vulkan-tools
```

Download Vulkan SDK from https://vulkan.lunarg.com/sdk/home

### Linux/Steam Deck

```bash
# Arch Linux / Steam Deck
sudo pacman -S cmake ninja clang vulkan-headers vulkan-icd-loader linux-api-headers glibc

# Ubuntu/Debian
sudo apt install cmake ninja-build clang vulkan-tools libvulkan-dev linux-libc-dev
```

## Building

### macOS

```bash
source setup-vulkan-env.sh

mkdir -p build && cd build

# Configure with CMake (using Homebrew LLVM)
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
  ..

ninja
ninja install
```

### Linux/Steam Deck

```bash
# Configure with Clang (required for C++23 modules)
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++

ninja -C build

# Install to both user and system directories (system install requires sudo)
sudo ninja -C build install
```

## Running

Test with vkcube:

```bash
just run-vkcube
```

Or manually:

```bash
source ./setup-vulkan-env.sh
VK_INSTANCE_LAYERS="VK_LAYER_MOTIONSAFE_overlay" vkcube
```

The layer will render animated red dots along the top and bottom screen edges.
