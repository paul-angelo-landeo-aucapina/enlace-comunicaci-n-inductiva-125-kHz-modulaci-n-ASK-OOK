#include "mpu6050_driver.h"
#include <math.h>

#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#endif

Mpu6050Driver::Mpu6050Driver(uint8_t address)
    : _address(address),
      _connected(false),
      _simulate_if_missing(true),
      _sim_step(0) {
}

bool Mpu6050Driver::writeRegister(uint8_t reg, uint8_t value) {
#ifdef ARDUINO
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
#else
    (void)reg; (void)value;
    return false;
#endif
}

bool Mpu6050Driver::readRegisters(uint8_t start_reg, uint8_t *buffer, uint8_t length) {
#ifdef ARDUINO
    Wire.beginTransmission(_address);
    Wire.write(start_reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    uint8_t bytes_read = Wire.requestFrom(_address, length);
    if (bytes_read != length) {
        return false;
    }

    for (uint8_t i = 0; i < length; i++) {
        buffer[i] = Wire.read();
    }
    return true;
#else
    (void)start_reg; (void)buffer; (void)length;
    return false;
#endif
}

bool Mpu6050Driver::begin(uint8_t sda_pin, uint8_t scl_pin, uint32_t frequency) {
#ifdef ARDUINO
    Wire.begin(sda_pin, scl_pin, frequency);

    // 1. Probe for device presence
    Wire.beginTransmission(_address);
    if (Wire.endTransmission() != 0) {
        // Try alternate address (0x69)
        _address = (_address == MPU6050_I2C_ADDR_DEFAULT) ? MPU6050_I2C_ADDR_ALT : MPU6050_I2C_ADDR_DEFAULT;
        Wire.beginTransmission(_address);
        if (Wire.endTransmission() != 0) {
            _connected = false;
            return false;
        }
    }

    // 2. Wake up MPU6050 (PWR_MGMT_1 = 0x00, clears sleep bit)
    if (!writeRegister(MPU6050_REG_PWR_MGMT_1, 0x00)) {
        _connected = false;
        return false;
    }
    delay(20);

    // 3. Configure Digital Low Pass Filter (DLPF = 44 Hz, CONFIG = 0x03)
    // Helps suppress inductive 125 kHz switching noise from entering the analog front-end
    writeRegister(MPU6050_REG_CONFIG, 0x03);

    // 4. Configure Accel full-scale range to +/- 2g (ACCEL_CONFIG = 0x00 -> 16384 LSB/g)
    writeRegister(MPU6050_REG_ACCEL_CONFIG, 0x00);

    _connected = true;
    return true;
#else
    (void)sda_pin; (void)scl_pin; (void)frequency;
    _connected = false;
    return false;
#endif
}

bool Mpu6050Driver::isConnected() const {
    return _connected;
}

void Mpu6050Driver::setSimulateIfMissing(bool enable) {
    _simulate_if_missing = enable;
}

bool Mpu6050Driver::isSimulated() const {
    return !_connected && _simulate_if_missing;
}

void Mpu6050Driver::generateSimulatedData(int16_t &ax_mg, int16_t &ay_mg, int16_t &az_mg, int16_t &temp_cdeg) {
    _sim_step++;
    // Generate gentle sinusoidal tilt between -25° and +25°
    float angle_rad = (_sim_step % 100) * (2.0f * 3.14159265f / 100.0f);
    float sin_val = sinf(angle_rad);
    float cos_val = cosf(angle_rad);

    ax_mg = (int16_t)(sin_val * 420.0f);   // +/- 0.42g on X
    ay_mg = (int16_t)(cos_val * 250.0f);   // +/- 0.25g on Y
    az_mg = (int16_t)(980.0f - fabsf(sin_val) * 150.0f); // ~1.0g gravity vector
    temp_cdeg = 2500 + (int16_t)(sin_val * 150.0f); // 23.5°C to 26.5°C
}

bool Mpu6050Driver::readData(int16_t &ax_mg, int16_t &ay_mg, int16_t &az_mg, int16_t &temp_cdeg) {
    if (!_connected) {
        if (_simulate_if_missing) {
            generateSimulatedData(ax_mg, ay_mg, az_mg, temp_cdeg);
            return true;
        }
        return false;
    }

    uint8_t raw_buf[8]; // 6 bytes Accel + 2 bytes Temp
    if (!readRegisters(MPU6050_REG_ACCEL_XOUT_H, raw_buf, 8)) {
        return false;
    }

    int16_t raw_ax = (int16_t)((raw_buf[0] << 8) | raw_buf[1]);
    int16_t raw_ay = (int16_t)((raw_buf[2] << 8) | raw_buf[3]);
    int16_t raw_az = (int16_t)((raw_buf[4] << 8) | raw_buf[5]);
    int16_t raw_temp = (int16_t)((raw_buf[6] << 8) | raw_buf[7]);

    // Scale +/- 2g range: 16384 LSB = 1.0g = 1000 mg
    ax_mg = (int16_t)(((int32_t)raw_ax * 1000) / 16384);
    ay_mg = (int16_t)(((int32_t)raw_ay * 1000) / 16384);
    az_mg = (int16_t)(((int32_t)raw_az * 1000) / 16384);

    // Temperature formula: Temp_degC = (raw_temp / 340.0) + 36.53
    // In centi-degrees (0.01 °C):
    temp_cdeg = (int16_t)((((int32_t)raw_temp * 100) / 340) + 3653);

    return true;
}
