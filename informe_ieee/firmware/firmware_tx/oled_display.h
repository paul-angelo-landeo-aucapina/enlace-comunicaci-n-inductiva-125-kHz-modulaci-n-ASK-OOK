#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#define OLED_I2C_ADDR_DEFAULT  0x3C  // Default I2C address for SSD1306 OLED
#define OLED_I2C_ADDR_ALT      0x3D

class OledDisplay {
public:
    OledDisplay(uint8_t address = OLED_I2C_ADDR_DEFAULT);

    // Initialize display over I2C without external library dependencies
    bool begin(uint8_t sda_pin = 21, uint8_t scl_pin = 22, uint32_t frequency = 400000);

    // Check if the physical display hardware was detected
    bool isConnected() const;

    // Clear display contents
    void clear();

    // Print text on a specific line (page 0 to 7, col 0 to 127)
    // Supports standard printable ASCII characters (32 to 126)
    void printLine(uint8_t line, const char *text, uint8_t start_col = 0);

    // Formatted Telemetry Screen:
    //   Line 0: Header (Link & Carrier Status)
    //   Line 2: Packet Sequence & Status
    //   Line 4: Accel components (Ax, Ay, Az in g)
    //   Line 6: Pitch, Roll & Temp
    void updateTelemetry(bool is_transmitting, uint32_t pkt_count, uint8_t seq,
                         int16_t ax_mg, int16_t ay_mg, int16_t az_mg,
                         int16_t temp_cdeg, bool imu_simulated);

private:
    uint8_t _address;
    bool    _connected;

    void sendCommand(uint8_t cmd);
    void sendData(const uint8_t *data, uint16_t len);
    void setCursor(uint8_t page, uint8_t col);
};

#endif // OLED_DISPLAY_H
