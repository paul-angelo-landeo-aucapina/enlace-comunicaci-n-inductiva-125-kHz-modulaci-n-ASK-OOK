#ifndef MANCHESTER_TX_H
#define MANCHESTER_TX_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

#ifdef ESP32
#include <esp_attr.h>
#include <esp_timer.h>
#else
#define IRAM_ATTR
#endif

struct TxStats {
    uint32_t packets_sent;
    uint32_t total_symbols_sent;
    uint32_t last_tx_duration_ms;
};

class ManchesterTx {
public:
    ManchesterTx();

    // Initialize pins and timer
    void begin(uint8_t tx_pin = TX_PIN, uint8_t led_pin = LED_PIN);

    // Encode raw bytes into Manchester symbols and begin transmission
    // Returns true if transmission successfully started, false if already busy
    bool sendPacket(const uint8_t *frame_data, uint16_t frame_len);

    // Continuous carrier mode for testbench / oscilloscope calibration
    void setContinuousCarrier(bool enable);
    bool isContinuousCarrier() const;

    // Check if transmitter is actively sending symbols
    bool isBusy() const;

    // Stop transmission immediately and reset line to LOW
    void stop();

    // Timer ISR tick handler (called every T_NOMINAL_HALF_US, e.g. 2000 us)
    void IRAM_ATTR tick();

    // Telemetry and statistics
    TxStats getStats() const;

    // Direct symbol inspection (useful for unit tests and simulations)
    uint16_t getSymbolCount() const { return _total_symbols; }
    uint8_t  getSymbol(uint16_t index) const { return (index < _total_symbols) ? _symbols[index] : 0; }

private:
    uint8_t  _tx_pin;
    uint8_t  _led_pin;

    volatile bool _is_transmitting;
    volatile bool _continuous_carrier;

    // Pre-computed Manchester symbols (0 or 1 for each half-bit)
    uint8_t  _symbols[MAX_SYMBOLS];
    volatile uint16_t _total_symbols;
    volatile uint16_t _current_symbol_idx;

    uint32_t _tx_start_time_ms;
    TxStats  _stats;

#ifdef ESP32
    esp_timer_handle_t _timer_handle;
    static void IRAM_ATTR onTimerTick(void* arg);
#endif

    // Internal helper to convert byte array to IEEE 802.3 Manchester symbols
    void encodeManchester(const uint8_t *data, uint16_t len);
};

#endif // MANCHESTER_TX_H
