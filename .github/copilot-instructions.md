# Copilot Instructions

## Project: MotionSafe Vulkan Layer

A Vulkan layer that renders animated dots along screen edges to reduce motion sickness. The dots move in the opposite direction of the Steam Deck's motion to provide a stable visual reference.

## Build & Run Workflows

### Development on macOS (Local Testing)

```bash
# Build locally for testing
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

### Deploy to Steam Deck (Required for Motion Sensor Testing)

**IMPORTANT**: When developing on macOS, always use these scripts to deploy to the Steam Deck:

```bash
# 1. Copy files to Deck
./copytodeck.sh

# 2. Build and install on Deck
./buildondeck.sh
```

The motion sensor features only work on the actual Steam Deck hardware, as they read from `/dev/iio:device0` (the IMU sensor).

## Key Files

- `src/overlay.cppm` - Main overlay rendering with animated dots
- `src/motion_sensor.cppm` - Motion sensor reading and data processing
- `shaders/motion_overlay.frag` - Fragment shader for dot rendering
- `src/dispatch_table.cppm` - Vulkan dispatch table management
- `src/layer.cppm` - Layer interception logic
- `docs/GYRO_IMPLEMENTATION.md` - Detailed gyroscope implementation notes
