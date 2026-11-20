#include "manchester_tx.h"
#include <string.h>

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <time.h>
#include <iostream>
static inline uint32_t millis() { return 0; }
static inline void digitalWrite(uint8_t pin, uint8_t val) { (void)pin; (void)val; }
static inline void pinMode(uint8_t pin, uint8_t mode) { (void)pin; (void)mode; }
#ifndef HIGH
#define HIGH 1
#endif
#ifndef LOW
#define LOW  0
#endif
#ifndef OUTPUT
#define OUTPUT 1
#endif
#endif

#ifdef ESP32
void IRAM_ATTR ManchesterTx::onTimerTick(void* arg) {
    ManchesterTx* instance = reinterpret_cast<ManchesterTx*>(arg);
    if (instance) {
        instance->tick();
    }
}
#endif

ManchesterTx::ManchesterTx()
    : _tx_pin(TX_PIN),
      _led_pin(LED_PIN),
      _is_transmitting(false),
      _continuous_carrier(false),
      _total_symbols(0),
      _current_symbol_idx(0),
      _tx_start_time_ms(0),
      _stats{0, 0, 0}
#ifdef ESP32
      , _timer_handle(nullptr)
#endif
{
    memset(_symbols, 0, sizeof(_symbols));
}

void ManchesterTx::begin(uint8_t tx_pin, uint8_t led_pin) {
    _tx_pin = tx_pin;
    _led_pin = led_pin;

    pinMode(_tx_pin, OUTPUT);
    digitalWrite(_tx_pin, LOW);

    pinMode(_led_pin, OUTPUT);
    digitalWrite(_led_pin, LOW);

    _is_transmitting = false;
    _continuous_carrier = false;
    _total_symbols = 0;
    _current_symbol_idx = 0;

#ifdef ESP32
    if (_timer_handle == nullptr) {
        const esp_timer_create_args_t timer_args = {
            .callback = &ManchesterTx::onTimerTick,
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "manchester_tx_timer"
        };
        esp_timer_create(&timer_args, &_timer_handle);
    }
#endif
}

void ManchesterTx::encodeManchester(const uint8_t *data, uint16_t len) {
    uint16_t sym_idx = 0;

    for (uint16_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        // Send MSB first (bit 7 down to bit 0)
        for (int8_t b = 7; b >= 0; b--) {
            uint8_t bit = (byte >> b) & 0x01;

            // IEEE 802.3 Manchester definition:
            // Bit 0: Rising edge at midpoint -> LOW first half, HIGH second half
            // Bit 1: Falling edge at midpoint -> HIGH first half, LOW second half
            if (bit == 0) {
                _symbols[sym_idx++] = 0;
                _symbols[sym_idx++] = 1;
            } else {
                _symbols[sym_idx++] = 1;
                _symbols[sym_idx++] = 0;
            }
        }
    }

    _total_symbols = sym_idx;
    _current_symbol_idx = 0;
}

bool ManchesterTx::sendPacket(const uint8_t *frame_data, uint16_t frame_len) {
    if (!frame_data || frame_len == 0 || frame_len > MAX_FRAME_BYTES) {
        return false;
    }

    if (_is_transmitting || _continuous_carrier) {
        return false; // Transmitter busy
    }

    // Pre-encode bits into Manchester half-bit symbols
    encodeManchester(frame_data, frame_len);

    _tx_start_time_ms = millis();
    _is_transmitting = true;

    // Turn on status LED to indicate active transmission
    digitalWrite(_led_pin, HIGH);

#ifdef ESP32
    if (_timer_handle != nullptr) {
        // Start periodic hardware timer at 2000 us (500 Hz)
        esp_timer_start_periodic(_timer_handle, T_NOMINAL_HALF_US);
    }
#endif

    return true;
}

void IRAM_ATTR ManchesterTx::tick() {
    if (!_is_transmitting) {
        return;
    }

    if (_current_symbol_idx < _total_symbols) {
        uint8_t symbol = _symbols[_current_symbol_idx];
        _current_symbol_idx = _current_symbol_idx + 1;
        digitalWrite(_tx_pin, symbol ? HIGH : LOW);
        _stats.total_symbols_sent++;
    } else {
        // Transmission finished: hold carrier LOW (idle state)
        digitalWrite(_tx_pin, LOW);
        digitalWrite(_led_pin, LOW);
        _is_transmitting = false;

#ifdef ESP32
        if (_timer_handle != nullptr) {
            esp_timer_stop(_timer_handle);
        }
#endif

        _stats.packets_sent++;
        _stats.last_tx_duration_ms = millis() - _tx_start_time_ms;
    }
}

void ManchesterTx::setContinuousCarrier(bool enable) {
    if (_is_transmitting) {
        stop();
    }

    _continuous_carrier = enable;
    if (_continuous_carrier) {
        digitalWrite(_tx_pin, HIGH);
        digitalWrite(_led_pin, HIGH);
    } else {
        digitalWrite(_tx_pin, LOW);
        digitalWrite(_led_pin, LOW);
    }
}

bool ManchesterTx::isContinuousCarrier() const {
    return _continuous_carrier;
}

bool ManchesterTx::isBusy() const {
    return _is_transmitting || _continuous_carrier;
}

void ManchesterTx::stop() {
#ifdef ESP32
    if (_timer_handle != nullptr && _is_transmitting) {
        esp_timer_stop(_timer_handle);
    }
#endif

    _is_transmitting = false;
    _continuous_carrier = false;
    _current_symbol_idx = 0;
    _total_symbols = 0;

    digitalWrite(_tx_pin, LOW);
    digitalWrite(_led_pin, LOW);
}

TxStats ManchesterTx::getStats() const {
    return _stats;
}
