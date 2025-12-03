# Copilot Instructions

## Project: MotionSafe Vulkan Layer

A Vulkan layer that renders animated dots along screen edges to reduce motion sickness.

## Build & Run (macOS)

```bash
# Build
source setup-vulkan-env.sh
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
  -B build
ninja -C build
ninja -C build install

# Run
just run-vkcube
```

## Key Files

- `src/overlay.cppm` - Main overlay rendering with animated dots
- `shaders/motion_overlay.frag` - Fragment shader for dot rendering
- `src/dispatch_table.cppm` - Vulkan dispatch table management
- `src/layer.cppm` - Layer interception logic
