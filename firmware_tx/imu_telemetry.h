#ifndef IMU_TELEMETRY_H
#define IMU_TELEMETRY_H

#include <stdint.h>
#include <math.h>

// ==============================================================================
// IMU Telemetry Packet Specification (Compact Binary Format)
// ==============================================================================
// Total Payload Length: Exactly 10 bytes
// At 250 bps Manchester: (10 payload + 5 overhead) * 8 * 4 ms = 480 ms per frame
#define PKT_TYPE_IMU 0x01

#pragma pack(push, 1)
struct ImuTelemetryPacket {
    uint8_t  type;         // Packet type identifier (0x01)
    uint8_t  seq;          // Sequence counter (0..255) for packet error rate tracking
    int16_t  accel_x_mg;   // Acceleration X in milli-g (±2000 mg = ±2.0g)
    int16_t  accel_y_mg;   // Acceleration Y in milli-g
    int16_t  accel_z_mg;   // Acceleration Z in milli-g
    int16_t  temp_cdeg;    // Temperature in centi-degrees Celsius (e.g. 2540 = 25.40 °C)
};
#pragma pack(pop)

// Helper functions for attitude estimation (Pitch & Roll in degrees)
static inline float imuCalculatePitchDeg(int16_t ax, int16_t ay, int16_t az) {
    // Pitch: Rotation around Y axis
    float fx = (float)ax;
    float fy = (float)ay;
    float fz = (float)az;
    return atan2f(fx, sqrtf(fy * fy + fz * fz)) * (180.0f / 3.141592653589793f);
}

static inline float imuCalculateRollDeg(int16_t ay, int16_t az) {
    // Roll: Rotation around X axis
    float fy = (float)ay;
    float fz = (float)az;
    return atan2f(fy, fz) * (180.0f / 3.141592653589793f);
}

#endif // IMU_TELEMETRY_H
