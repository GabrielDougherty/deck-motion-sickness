module;

#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

export module motionsafe.sd_hid_reader;

import std;

namespace motionsafe {

// Steam Deck HID frame structure
// Based on SteamDeckGyroDSU by kmicki
export struct SdHidFrame {
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
export class SdHidReader {
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

// Private implementation
module :private;

namespace motionsafe {

SdHidReader::SdHidReader() 
    : fd_(-1)
    , buffer_(FRAME_SIZE) 
{
}

SdHidReader::~SdHidReader() {
    Close();
}

bool SdHidReader::Open() {
    if (fd_ >= 0) {
        std::cout << "[MotionSafe][HID] Already open, fd=" << fd_ << std::endl;
        return true;  // Already open
    }
    
#ifdef __APPLE__
    // macOS: Use fake mode for testing
    std::cout << "[MotionSafe][HID] macOS detected - using FAKE gyro data for testing" << std::endl;
    fd_ = -2;  // Special value to indicate fake mode
    return true;
#else
    std::cout << "[MotionSafe][HID] Attempting to open hidraw devices..." << std::endl;
    
    // Try all hidraw devices (Steam Deck controller is usually hidraw2 or hidraw3)
    for (int i = 0; DEVICE_PATHS[i] != nullptr; ++i) {
        std::cout << "[MotionSafe][HID] Trying " << DEVICE_PATHS[i] << "..." << std::endl;
        fd_ = open(DEVICE_PATHS[i], O_RDWR | O_NONBLOCK);
        if (fd_ >= 0) {
            std::cout << "[MotionSafe][HID] SUCCESS: Opened " << DEVICE_PATHS[i] << " (fd=" << fd_ << ")" << std::endl;
            
            // Send gyro enable command
            std::cout << "[MotionSafe][HID] Sending gyro enable command..." << std::endl;
            if (!EnableGyro()) {
                std::cerr << "[MotionSafe][HID] ERROR: Failed to enable gyro on " << DEVICE_PATHS[i] << std::endl;
                close(fd_);
                fd_ = -1;
                continue;  // Try next device
            } else {
                std::cout << "[MotionSafe][HID] SUCCESS: Gyro enabled on " << DEVICE_PATHS[i] << std::endl;
                return true;
            }
        } else {
            std::cout << "[MotionSafe][HID] Failed to open " << DEVICE_PATHS[i] << ": " << strerror(errno) << std::endl;
        }
    }
    
    std::cerr << "[MotionSafe][HID] ERROR: Failed to open any hidraw device" << std::endl;
    return false;
#endif
}

void SdHidReader::Close() {
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

bool SdHidReader::IsOpen() const {
    return fd_ >= 0 || fd_ == -2;  // -2 is fake mode
}

bool SdHidReader::ReadFrame(SdHidFrame& frame) {
    if (fd_ < 0 && fd_ != -2) {
        return false;
    }
    
#ifdef __APPLE__
    if (fd_ == -2) {
        // macOS fake mode: Generate smooth sine wave motion
        static auto start_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - start_time).count();
        
        // Generate smooth circular motion at different frequencies
        // Scale values to match typical Steam Deck gyro range (~1000-2000 for moderate motion)
        float scale = 1500.0f;  // Moderate motion amplitude
        frame.GyroAxisRightToLeft = static_cast<int16_t>(std::sin(elapsed * 0.3f) * scale);  // Pitch (slow)
        frame.GyroAxisTopToBottom = static_cast<int16_t>(std::cos(elapsed * 0.4f) * scale);  // Yaw (medium)
        frame.GyroAxisFrontToBack = static_cast<int16_t>(std::sin(elapsed * 0.2f) * scale);  // Roll (slowest)
        
        // Debug: Log fake gyro values occasionally
        static int frame_count = 0;
        if (frame_count++ % 60 == 0) {  // Every second at 60fps
            std::cout << "[MotionSafe][HID][FAKE] Gyro: X=" << frame.GyroAxisRightToLeft
                      << " Y=" << frame.GyroAxisTopToBottom
                      << " Z=" << frame.GyroAxisFrontToBack << std::endl;
        }
        
        return true;
    }
#endif
    
    // Real device mode
    // Read 64 bytes
    ssize_t bytes_read = read(fd_, buffer_.data(), FRAME_SIZE);
    
    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available yet (non-blocking mode) - this is normal
            return false;
        }
        std::cerr << "[MotionSafe][HID] Read error: " << strerror(errno) << std::endl;
        return false;
    }
    
    if (bytes_read != FRAME_SIZE) {
        std::cerr << "[MotionSafe][HID] Partial read: " << bytes_read << " bytes" << std::endl;
        return false;
    }
    
    // Copy data to frame structure
    std::memcpy(&frame, buffer_.data(), sizeof(SdHidFrame));
    
    // Debug: Log gyro values occasionally
    static int frame_count = 0;
    if (frame_count++ % 250 == 0) {  // Every ~1 second at 250Hz
        std::cout << "[MotionSafe][HID][DEBUG] Gyro: X=" << frame.GyroAxisRightToLeft
                  << " Y=" << frame.GyroAxisTopToBottom
                  << " Z=" << frame.GyroAxisFrontToBack << std::endl;
    }
    
    return true;
}

bool SdHidReader::EnableGyro() {
    if (fd_ < 0) {
        std::cerr << "[MotionSafe][HID] Cannot enable gyro: device not open" << std::endl;
        return false;
    }
    
    std::cout << "[MotionSafe][HID] Sending 64-byte gyro enable command..." << std::endl;
    
    // Magic command to enable gyro on Steam Deck
    // From SteamDeckGyroDSU
    unsigned char cmd[] = {
        0x00,
        0x87, 0x0f, 0x30, 0x18, 0x00, 0x07, 0x07, 0x00, 0x08, 0x07, 0x00, 0x31, 0x02, 0x00, 0x18, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    ssize_t bytes_written = write(fd_, cmd, sizeof(cmd));
    std::cout << "[MotionSafe][HID] Write result: " << bytes_written << " bytes (expected " << sizeof(cmd) << ")" << std::endl;
    
    if (bytes_written != sizeof(cmd)) {
        std::cerr << "[MotionSafe][HID] ERROR: Failed to write gyro enable command: " 
                  << strerror(errno) << " (wrote " << bytes_written << " bytes)" << std::endl;
        return false;
    }
    
    std::cout << "[MotionSafe][HID] Gyro enable command sent successfully" << std::endl;
    return true;
}

} // namespace motionsafe
