# Steam Deck Gyro Implementation Notes

## Overview

The Steam Deck's gyroscope and accelerometer are accessible through the HID device at `/dev/usb/hiddev0`.

## Device Information

- **Vendor ID (VID)**: `0x28de` (Valve Software)
- **Product ID (PID)**: `0x1205` (Steam Controller/Deck)
- **Device Path**: `/dev/usb/hiddev0`
- **Interface Number**: 2
- **Report Size**: 64 bytes
- **Update Rate**: ~4000 microseconds between reports (~250 Hz)

## HID Frame Structure

The Steam Deck sends 64-byte HID reports with the following structure:

```cpp
struct SdHidFrame {
    uint32_t Header;
    uint32_t Increment;
    uint32_t Buttons1; 
    uint32_t Buttons2;
    
    int16_t LeftTrackpadX;
    int16_t LeftTrackpadY;
    int16_t RightTrackpadX;
    int16_t RightTrackpadY;
    
    // Accelerometer (int16_t, units unknown - likely raw ADC values)
    int16_t AccelAxisRightToLeft;  // X axis (left/right)
    int16_t AccelAxisTopToBottom;  // Y axis (up/down)
    int16_t AccelAxisFrontToBack;  // Z axis (forward/back)
    
    // Gyroscope (int16_t, units unknown - likely deg/s * some scale factor)
    // Positive rotation = counterclockwise when looking along axis
    int16_t GyroAxisRightToLeft;  // Pitch (rotation around X axis)
    int16_t GyroAxisTopToBottom;  // Yaw (rotation around Y axis)
    int16_t GyroAxisFrontToBack;  // Roll (rotation around Z axis)
    
    int16_t Unknown1;
    int16_t Unknown2;
    int16_t Unknown3;
    int16_t Unknown4;
    
    int16_t L2Analog;
    int16_t R2Analog;
    int16_t LeftStickX;
    int16_t LeftStickY;
    int16_t RightStickX;
    int16_t RightStickY;
    
    int16_t LeftTrackpadPushForce;
    int16_t RightTrackpadPushForce;
    int16_t LeftStickTouchCoverage;
    int16_t RightStickTouchCoverage;
};
```

## Coordinate System

The Steam Deck uses the following coordinate system when held in landscape (normal gaming position):

- **X axis (RightToLeft)**: Points from right edge to left edge
  - Positive rotation (pitch up): Top edge moving away from you
- **Y axis (TopToBottom)**: Points from top edge to bottom edge  
  - Positive rotation (yaw left): Left edge moving away from you
- **Z axis (FrontToBack)**: Points from screen through back of device
  - Positive rotation (roll right): Right edge moving down

## Testing Gyro Access

### Compile and Run Test Program

```bash
# On Steam Deck:
cd ~
clang++ -std=c++20 -o test_gyro test_gyro.cpp
sudo ./test_gyro
```

The test program reads 100 frames from the gyroscope and displays values every 10 frames.

### Expected Behavior

When you move the Steam Deck:
- **Pitch (X)**: Tilting top/bottom edges forward/backward
- **Yaw (Y)**: Rotating left/right (like shaking your head "no")
- **Roll (Z)**: Tilting left/right edges (like steering a wheel)

Values at rest should be close to zero with minor noise/drift.
Moving the device should produce significant value changes (hundreds to thousands).

## Implementation Approach

### Reading the Device

```cpp
// Open device (requires root or appropriate permissions)
int fd = open("/dev/usb/hiddev0", O_RDONLY | O_NONBLOCK);

// Read 64-byte frames
char buffer[64];
int bytes_read = read(fd, buffer, 64);

// Parse as SdHidFrame
SdHidFrame* frame = reinterpret_cast<SdHidFrame*>(buffer);
```

### Permissions

The device requires either:
1. Running as root (not recommended for games)
2. Adding user to `input` group: `sudo usermod -a -G input $USER`
3. Setting udev rules for the device

### Converting to Velocities

The raw gyro values need to be converted to usable velocities:

```cpp
// Scale factors (to be determined through testing)
const float GYRO_SCALE = 0.0001f;  // Adjust based on actual units

float velocityX = frame->GyroAxisRightToLeft * GYRO_SCALE;
float velocityY = frame->GyroAxisTopToBottom * GYRO_SCALE;
float velocityZ = frame->GyroAxisFrontToBack * GYRO_SCALE;

// Apply smoothing to reduce jitter
const float SMOOTHING = 0.1f;
smoothedVelX += (velocityX - smoothedVelX) * SMOOTHING;
smoothedVelY += (velocityY - smoothedVelY) * SMOOTHING;
```

## References

- [SteamDeckGyroDSU](https://github.com/kmicki/SteamDeckGyroDSU) - Reference implementation
- Steam Deck controls use Valve's proprietary HID protocol
- Data structure reverse-engineered by the community

## Notes

- The `/dev/iio:device*` devices do NOT exist on Steam Deck
- The gyro is accessed through USB HID, not IIO subsystem
- Gyro data is part of the same report as controller buttons/sticks
- Reading is blocking by default, use O_NONBLOCK flag for non-blocking reads
- Consider using a separate thread for reading to avoid blocking rendering
