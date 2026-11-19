#include "manchester_rx.h"

#define BIT_FIFO_SIZE 256

ManchesterRx::ManchesterRx()
    : _pin(RX_PIN),
      _state(STATE_UNSYNCED),
      _last_edge_us(0),
      _sync_count(0),
      _current_byte(0),
      _bit_count(0),
      _fifo_head(0),
      _fifo_tail(0),
      _stats{0, 0, 0, 0, 0} {
}

void ManchesterRx::begin(uint8_t pin) {
    _pin = pin;
    reset();
}

void ManchesterRx::reset() {
    _state = STATE_UNSYNCED;
    _sync_count = 0;
    _last_edge_us = 0;
    _current_byte = 0;
    _bit_count = 0;
    _fifo_head = 0;
    _fifo_tail = 0;
}

void IRAM_ATTR ManchesterRx::pushBit(uint8_t bit) {
    uint8_t next_head = (_fifo_head + 1) % RX_FIFO_SIZE;
    if (next_head != _fifo_tail) {
        _fifo[_fifo_head] = bit & 0x01;
        _fifo_head = next_head;
        _stats.bits_decoded++;
    }
}

void IRAM_ATTR ManchesterRx::handleEdge(uint32_t now_us, bool pin_level) {
    _stats.total_edges++;

    if (_last_edge_us == 0) {
        _last_edge_us = now_us;
        return;
    }

    uint32_t dt = now_us - _last_edge_us;

    // 1. Timeout Check: If silence persists for > 14 ms, link went idle -> reset
    if (dt > T_TIMEOUT_US) {
        _state = STATE_UNSYNCED;
        _sync_count = 0;
        _last_edge_us = now_us;
        return;
    }

    // 2. Glitch Filter: Reject sub-millisecond inductive switching spikes
    if (dt < T_GLITCH_MIN_US) {
        _stats.glitches_rejected++;
        return; // Retain _last_edge_us to measure from the authentic edge
    }

    _last_edge_us = now_us;

    bool is_half = (dt >= T_HALF_MIN_US && dt <= T_HALF_MAX_US);
    bool is_full = (dt >= T_BIT_MIN_US && dt <= T_BIT_MAX_US);

    // IEEE 802.3 Manchester definition:
    // Rising  edge at midpoint = '0'
    // Falling edge at midpoint = '1'
    uint8_t decoded_bit = (pin_level) ? 0 : 1;

    switch (_state) {
        case STATE_UNSYNCED:
            // Preamble (0xAA = 10101010) produces alternating edges separated by full-bit intervals (T_bit)
            if (is_full) {
                _sync_count++;
                if (_sync_count >= 2) {
                    _state = STATE_EXPECT_ANY;
                    pushBit(decoded_bit);
                    _stats.last_jitter_us = (int32_t)dt - T_NOMINAL_BIT_US;
                }
            } else {
                _sync_count = 0;
            }
            break;

        case STATE_EXPECT_ANY:
            // Last edge was a MIDPOINT edge.
            if (is_full) {
                // Alternating bit: no boundary edge occurred. This edge IS the next midpoint!
                pushBit(decoded_bit);
                _stats.last_jitter_us = (int32_t)dt - T_NOMINAL_BIT_US;
                _state = STATE_EXPECT_ANY;
            } else if (is_half) {
                // Same bit: this is a boundary transition. Wait for midpoint transition.
                _stats.last_jitter_us = (int32_t)dt - T_NOMINAL_HALF_US;
                _state = STATE_EXPECT_MID;
            } else {
                // Invalid edge interval -> sync lost
                _stats.sync_errors++;
                _state = STATE_UNSYNCED;
                _sync_count = 0;
            }
            break;

        case STATE_EXPECT_MID:
            // Last edge was a BOUNDARY edge. Next edge MUST be midpoint (~T_half).
            if (is_half) {
                pushBit(decoded_bit);
                _stats.last_jitter_us = (int32_t)dt - T_NOMINAL_HALF_US;
                _state = STATE_EXPECT_ANY;
            } else {
                // Expected midpoint edge missing or distorted
                _stats.sync_errors++;
                _state = STATE_UNSYNCED;
                _sync_count = 0;
            }
            break;
    }
}

bool ManchesterRx::available() const {
    return (_fifo_head != _fifo_tail);
}

uint16_t ManchesterRx::availableBytes() const {
    if (_fifo_head >= _fifo_tail) {
        return _fifo_head - _fifo_tail;
    }
    return RX_FIFO_SIZE - (_fifo_tail - _fifo_head);
}

uint8_t ManchesterRx::read() {
    if (_fifo_head == _fifo_tail) {
        return 0;
    }
    uint8_t bit = _fifo[_fifo_tail];
    _fifo_tail = (_fifo_tail + 1) % RX_FIFO_SIZE;
    return bit;
}

RxStats ManchesterRx::getStats() const {
    return _stats;
}
