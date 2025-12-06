#include "sd_hid_reader.h"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

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
}

void SdHidReader::Close() {
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

bool SdHidReader::IsOpen() const {
    return fd_ >= 0;
}

bool SdHidReader::ReadFrame(SdHidFrame& frame) {
    if (fd_ < 0) {
        return false;
    }
    
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
