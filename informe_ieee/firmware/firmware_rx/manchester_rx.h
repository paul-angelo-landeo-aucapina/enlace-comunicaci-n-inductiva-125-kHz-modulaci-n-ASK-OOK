#ifndef MANCHESTER_RX_H
#define MANCHESTER_RX_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

#ifdef ESP32
#include <esp_attr.h>
#else
#define IRAM_ATTR
#endif

enum ManchesterState {
    STATE_UNSYNCED,     // Waiting for steady preamble edges
    STATE_EXPECT_MID,   // Last edge was a boundary; next MUST be midpoint
    STATE_EXPECT_ANY    // Last edge was midpoint; next can be boundary or midpoint
};

struct RxStats {
    uint32_t total_edges;
    uint32_t glitches_rejected;
    uint32_t sync_errors;
    uint32_t bits_decoded;
    int32_t  last_jitter_us;
};

class ManchesterRx {
public:
    ManchesterRx();

    void begin(uint8_t pin);
    void reset();

    // Called strictly from the GPIO ISR
    void IRAM_ATTR handleEdge(uint32_t now_us, bool pin_level);

    // FIFO interface (call from main loop / task context)
    bool available() const;
    uint8_t read();
    uint16_t availableBytes() const;

    // Diagnostic metrics
    RxStats getStats() const;

private:
    uint8_t _pin;
    ManchesterState _state;
    volatile uint32_t _last_edge_us;
    uint8_t  _sync_count;

    // Bit assembly
    uint8_t _current_byte;
    uint8_t _bit_count;

    // Ring buffer for raw received bytes/bits
    volatile uint8_t _fifo[RX_FIFO_SIZE];
    volatile uint8_t _fifo_head;
    volatile uint8_t _fifo_tail;

    // Telemetry / diagnostics
    RxStats _stats;

    void IRAM_ATTR pushBit(uint8_t bit);
};

#endif // MANCHESTER_RX_H
