#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ==============================================================================
// Hardware Configuration (ESP32 DevKitC Transmitter)
// ==============================================================================
#define TX_PIN                 18       // GPIO18: OOK Gating (~RST Pin 4 of TLC555)
#define LED_PIN                2        // GPIO2: Onboard LED for TX activity indicator
#define SERIAL_BAUD            115200   // USB Serial diagnostic baudrate

// ==============================================================================
// Manchester 250 bps Timing Specifications
// ==============================================================================
// At 250 bps:
//   T_bit  = 1 / 250 bps = 4000 us
//   T_half = 2000 us (Half-bit interval for IEEE 802.3 Manchester transition)
#define T_NOMINAL_BIT_US       4000
#define T_NOMINAL_HALF_US      2000

// Guard interval: silence period between consecutive frames (ensures RX timeout reset)
#define T_GUARD_INTERVAL_MS    25

// ==============================================================================
// Protocol & Frame Definitions
// ==============================================================================
#define PREAMBLE_BYTE          0xAA     // 10101010 clock training pattern
#define PREAMBLE_COUNT         2        // Send 2 preamble bytes (16 bit transitions)
#define SFD_BYTE               0x7E     // 01111110 Start-of-Frame Delimiter
#define MAX_PAYLOAD_LEN        32       // Maximum payload bytes per packet
#define RX_FIFO_SIZE           64       // Compatible FIFO size definition for loopback tests

// Maximum frame size: 2 Preambles + 1 SFD + 1 Len + 32 Payload + 1 CRC = 37 bytes
#define MAX_FRAME_BYTES        (PREAMBLE_COUNT + 1 + 1 + MAX_PAYLOAD_LEN + 1)
// Each bit requires 2 symbols: 37 bytes * 8 bits * 2 symbols = 592 symbols
#define MAX_SYMBOLS            (MAX_FRAME_BYTES * 8 * 2)

// CRC-8 Configuration (Polynomial: x^8 + x^2 + x + 1 -> 0x07, Initial value: 0x00)
#define CRC8_POLYNOMIAL        0x07
#define CRC8_INIT              0x00

#endif // CONFIG_H
