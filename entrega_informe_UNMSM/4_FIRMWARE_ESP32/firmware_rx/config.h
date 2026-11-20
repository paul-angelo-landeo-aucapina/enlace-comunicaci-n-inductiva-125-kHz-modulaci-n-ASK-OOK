#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ==============================================================================
// Hardware Configuration
// ==============================================================================
#define RX_PIN            19       // GPIO19: Output from LM393 comparator (with 4.7k pull-up)
#define LED_PIN           2        // GPIO2: Onboard LED for packet reception indicator
#define SERIAL_BAUD       115200   // USB Serial diagnostic baudrate

// ==============================================================================
// Manchester 250 bps Timing Specifications
// ==============================================================================
// At 250 bps:
//   T_bit  = 1 / 250 bps = 4000 us
//   T_half = 2000 us
#define T_NOMINAL_BIT_US   4000
#define T_NOMINAL_HALF_US  2000

// Acceptance windows (approx +/- 25% tolerance for oscillator & RC jitter)
#define T_HALF_MIN_US      1400
#define T_HALF_MAX_US      2600

#define T_BIT_MIN_US       2800
#define T_BIT_MAX_US       5200

// Glitch filter: any pulse shorter than 600 us is rejected as noise
#define T_GLITCH_MIN_US    600

// Silence timeout: reset decoder if no edges received for > 3 bit intervals
#define T_TIMEOUT_US       14000

// ==============================================================================
// Protocol & Frame Definitions
// ==============================================================================
#define PREAMBLE_BYTE      0xAA    // 10101010 training pattern for clock recovery
#define SFD_BYTE           0x7E    // 01111110 Start-of-Frame Delimiter
#define MAX_PAYLOAD_LEN    32      // Maximum payload bytes per packet
#define RX_FIFO_SIZE       64      // Internal ring buffer capacity for received bytes

// CRC-8 Configuration (Polynomial: x^8 + x^2 + x + 1 -> 0x07, Initial value: 0x00)
#define CRC8_POLYNOMIAL    0x07
#define CRC8_INIT          0x00

#endif // CONFIG_H
