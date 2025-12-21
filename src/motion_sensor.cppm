module;

export module motionsafe.motion_sensor;

import std;

import motionsafe.sd_hid_reader;

export namespace motionsafe {
namespace motion_sensor {

/**
 * @brief Motion sensor state and data
 */
struct MotionData {
    float gyroX = 0.0f;         // Angular velocity around X axis (pitch)
    float gyroY = 0.0f;         // Angular velocity around Y axis (yaw)
    float gyroZ = 0.0f;         // Angular velocity around Z axis (roll)
    float smoothedVelX = 0.0f;  // Smoothed horizontal velocity for rendering
    float smoothedVelY = 0.0f;  // Smoothed vertical velocity for rendering
};

/**
 * @brief Initialize the motion sensor
 * 
 * Opens the Steam Deck's IMU device for reading gyroscope data.
 * On the Steam Deck, this is typically /dev/iio:device0.
 * 
 * @return true if sensor was successfully opened, false otherwise
 */
bool Initialize();

/**
 * @brief Shutdown the motion sensor
 * 
 * Closes the sensor device and cleans up resources.
 */
void Shutdown();

/**
 * @brief Update motion sensor data
 * 
 * Reads the latest gyroscope data and calculates smoothed velocity.
 * This should be called each frame before rendering.
 */
void Update();

/**
 * @brief Get the current motion data
 * 
 * @return Current motion data with smoothed velocities
 */
const MotionData& GetMotionData();

} // namespace motion_sensor
} // namespace motionsafe

module :private;

namespace motionsafe {
namespace motion_sensor {

// Internal sensor state
struct SensorState {
    SdHidReader reader;
    MotionData data;
    std::chrono::steady_clock::time_point lastUpdate;
};

static SensorState g_sensor;

bool Initialize() {
    std::cout << "[MotionSafe][SENSOR] Initializing motion sensor..." << std::endl;
    
    if (!g_sensor.reader.Open()) {
        std::cerr << "[MotionSafe][SENSOR] ERROR: Failed to open Steam Deck HID device" << std::endl;
        return false;
    }
    
    g_sensor.lastUpdate = std::chrono::steady_clock::now();
    std::cout << "[MotionSafe][SENSOR] Motion sensor initialized successfully" << std::endl;
    return true;
}

void Shutdown() {
    std::cout << "[MotionSafe][SENSOR] Shutting down motion sensor..." << std::endl;
    g_sensor.reader.Close();
    std::cout << "[MotionSafe][SENSOR] Motion sensor closed" << std::endl;
}

void Update() {
    if (!g_sensor.reader.IsOpen()) {
        return;
    }
    
    SdHidFrame frame;
    if (!g_sensor.reader.ReadFrame(frame)) {
        // No data available or read error
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(now - g_sensor.lastUpdate).count();
    g_sensor.lastUpdate = now;
    
    // Convert gyro values to velocities
    // The gyro values are int16_t, need to scale them appropriately
    // Typical Steam Deck gyro values for moderate motion: ~2000-4000
    const float GYRO_SCALE = 0.0003f;  // Scale to reasonable velocity range
    const float MAX_VELOCITY = 0.3f;   // Cap maximum velocity for smooth animation
    
    // Apply exponential smoothing to velocity
    const float smoothing = 0.15f;  // Higher = more responsive, lower = smoother
    
    // Map gyro axes to screen movement
    // Roll (Z axis) affects horizontal movement
    // Pitch (X axis) affects vertical movement
    float targetVelX = -frame.GyroAxisFrontToBack * GYRO_SCALE;  // Roll -> horizontal
    float targetVelY = frame.GyroAxisRightToLeft * GYRO_SCALE;   // Pitch -> vertical
    
    // Clamp velocities to maximum
    targetVelX = std::clamp(targetVelX, -MAX_VELOCITY, MAX_VELOCITY);
    targetVelY = std::clamp(targetVelY, -MAX_VELOCITY, MAX_VELOCITY);
    
    g_sensor.data.smoothedVelX += (targetVelX - g_sensor.data.smoothedVelX) * smoothing;
    g_sensor.data.smoothedVelY += (targetVelY - g_sensor.data.smoothedVelY) * smoothing;
    
    // Store raw gyro data
    g_sensor.data.gyroX = frame.GyroAxisRightToLeft;
    g_sensor.data.gyroY = frame.GyroAxisTopToBottom;
    g_sensor.data.gyroZ = frame.GyroAxisFrontToBack;
    
    // Debug: Log velocities occasionally
    static int update_count = 0;
    if (update_count++ % 60 == 0) {  // More frequent logging
        std::cout << "[MotionSafe][SENSOR] VelX=" << g_sensor.data.smoothedVelX 
                  << " VelY=" << g_sensor.data.smoothedVelY 
                  << " (target: " << targetVelX << ", " << targetVelY << ")"
                  << " (raw gyro: X=" << frame.GyroAxisRightToLeft
                  << " Y=" << frame.GyroAxisTopToBottom
                  << " Z=" << frame.GyroAxisFrontToBack << ")" << std::endl;
    }
    
    // Suppress unused variable warning
    (void)deltaTime;
}

const MotionData& GetMotionData() {
    return g_sensor.data;
}

} // namespace motion_sensor
} // namespace motionsafe
