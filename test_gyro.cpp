// Simple test program to read gyro data from Steam Deck
// Compile: clang++ -std=c++20 -o test_gyro test_gyro.cpp
// Run: sudo ./test_gyro

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <vector>
#include <cstring>

struct SdHidFrame {
    uint32_t Header;
    uint32_t Increment;
    uint32_t Buttons1; 
    uint32_t Buttons2;
    
    int16_t LeftTrackpadX;
    int16_t LeftTrackpadY;
    int16_t RightTrackpadX;
    int16_t RightTrackpadY;
    
    int16_t AccelAxisRightToLeft;
    int16_t AccelAxisTopToBottom;
    int16_t AccelAxisFrontToBack;
    
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

int main() {
    const char* device_path = "/dev/usb/hiddev0";
    
    std::cout << "Opening " << device_path << "..." << std::endl;
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        std::cerr << "Failed to open device: " << strerror(errno) << std::endl;
        std::cerr << "Try running with sudo" << std::endl;
        return 1;
    }
    
    std::cout << "Device opened successfully! Reading gyro data..." << std::endl;
    std::cout << "Move the Steam Deck around to see gyro values change" << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;
    std::cout << std::endl;
    
    std::vector<char> buffer(64);  // Steam Deck HID report is 64 bytes
    
    int count = 0;
    while (true) {  // Run indefinitely until Ctrl+C
        int bytes_read = read(fd, buffer.data(), buffer.size());
        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000);  // Sleep 1ms if no data
                continue;
            }
            std::cerr << "Read error: " << strerror(errno) << std::endl;
            break;
        }
        
        if (bytes_read == 64) {
            SdHidFrame* frame = reinterpret_cast<SdHidFrame*>(buffer.data());
            
            // Print gyro values every 50 frames (~0.2 seconds at 250Hz)
            if (count % 50 == 0) {
                std::cout << "Gyro: X=" << frame->GyroAxisRightToLeft
                          << " Y=" << frame->GyroAxisTopToBottom  
                          << " Z=" << frame->GyroAxisFrontToBack
                          << " | Accel: X=" << frame->AccelAxisRightToLeft
                          << " Y=" << frame->AccelAxisTopToBottom
                          << " Z=" << frame->AccelAxisFrontToBack
                          << std::endl;
            }
            count++;
        }
    }
    
    close(fd);
    std::cout << "\nTest complete!" << std::endl;
    return 0;
}
