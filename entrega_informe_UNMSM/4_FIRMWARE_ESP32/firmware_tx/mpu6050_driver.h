#ifndef MPU6050_DRIVER_H
#define MPU6050_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// MPU6050 Default I2C Addresses
#define MPU6050_I2C_ADDR_DEFAULT  0x68  // AD0 = GND
#define MPU6050_I2C_ADDR_ALT      0x69  // AD0 = VCC

// Key Registers
#define MPU6050_REG_SMPLRT_DIV    0x19
#define MPU6050_REG_CONFIG        0x1A
#define MPU6050_REG_ACCEL_CONFIG  0x1C
#define MPU6050_REG_ACCEL_XOUT_H  0x3B
#define MPU6050_REG_TEMP_OUT_H    0x41
#define MPU6050_REG_PWR_MGMT_1    0x6B
#define MPU6050_REG_WHO_AM_I      0x75

class Mpu6050Driver {
public:
    Mpu6050Driver(uint8_t address = MPU6050_I2C_ADDR_DEFAULT);

    // Initialize I2C and configure sensor (scale +/- 2g, DLPF 44 Hz, wake from sleep)
    // Returns true if sensor was detected on the bus and configured
    bool begin(uint8_t sda_pin = 21, uint8_t scl_pin = 22, uint32_t frequency = 100000);

    // Check if the physical hardware is detected and responding
    bool isConnected() const;

    // Read converted values:
    //   ax_mg, ay_mg, az_mg: Acceleration in milli-g (1000 mg = 1.0 g)
    //   temp_cdeg: Temperature in centi-degrees Celsius (2540 = 25.40 °C)
    bool readData(int16_t &ax_mg, int16_t &ay_mg, int16_t &az_mg, int16_t &temp_cdeg);

    // Configure fallback to synthetic test data if physical sensor is not plugged in
    void setSimulateIfMissing(bool enable);
    bool isSimulated() const;

private:
    uint8_t  _address;
    bool     _connected;
    bool     _simulate_if_missing;
    uint32_t _sim_step;

    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegisters(uint8_t start_reg, uint8_t *buffer, uint8_t length);
    void generateSimulatedData(int16_t &ax_mg, int16_t &ay_mg, int16_t &az_mg, int16_t &temp_cdeg);
};

#endif // MPU6050_DRIVER_H
