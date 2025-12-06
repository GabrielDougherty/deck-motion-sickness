#pragma once

#include <cstdint>
#include <vector>

namespace motionsafe {

// Steam Deck HID frame structure
// Based on SteamDeckGyroDSU by kmicki
struct SdHidFrame {
    uint32_t Header;
    uint32_t Increment;
    uint32_t Buttons1; 
    uint32_t Buttons2;
    
    int16_t LeftTrackpadX;
    int16_t LeftTrackpadY;
    int16_t RightTrackpadX;
    int16_t RightTrackpadY;
    
    int16_t AccelAxisRightToLeft;  // Accelerometer X
    int16_t AccelAxisTopToBottom;  // Accelerometer Y
    int16_t AccelAxisFrontToBack;  // Accelerometer Z
    
    // Gyroscope - rotation rates
    int16_t GyroAxisRightToLeft;  // Pitch (rotation around X)
    int16_t GyroAxisTopToBottom;  // Yaw (rotation around Y)
    int16_t GyroAxisFrontToBack;  // Roll (rotation around Z)
    
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

// Simple HID device reader for Steam Deck controls
class SdHidReader {
public:
    SdHidReader();
    ~SdHidReader();
    
    // Open the Steam Deck HID device
    bool Open();
    
    // Close the device
    void Close();
    
    // Check if device is open
    bool IsOpen() const;
    
    // Read one frame (blocking)
    // Returns true if successful, false on error
    bool ReadFrame(SdHidFrame& frame);
    
private:
    int fd_;
    std::vector<char> buffer_;
    static constexpr int FRAME_SIZE = 64;
    static constexpr const char* DEVICE_PATHS[] = {
        "/dev/hidraw0",
        "/dev/hidraw1", 
        "/dev/hidraw2",
        "/dev/hidraw3",
        nullptr
    };
    
    // Send the gyro enable command
    bool EnableGyro();
};

} // namespace motionsafe
