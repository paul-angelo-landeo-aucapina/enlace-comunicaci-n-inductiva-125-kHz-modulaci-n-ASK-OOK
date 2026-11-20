#include "packet_parser.h"
#include <string.h>

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <time.h>
static inline uint32_t millis() { return 0; }
#endif

PacketParser::PacketParser() {
    reset();
}

void PacketParser::reset() {
    _state = PARSER_SEARCH_SFD;
    _shift_reg = 0;
    _bit_acc = 0;
    _bits_collected = 0;
    _payload_idx = 0;
    memset(&_current_packet, 0, sizeof(_current_packet));
}

uint8_t PacketParser::updateCRC8(uint8_t crc, uint8_t byte) {
    crc ^= byte;
    for (uint8_t i = 0; i < 8; i++) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ CRC8_POLYNOMIAL;
        } else {
            crc = (crc << 1);
        }
    }
    return crc;
}

uint8_t PacketParser::calculateCRC8(const uint8_t *data, uint8_t len) {
    uint8_t crc = CRC8_INIT;
    for (uint8_t i = 0; i < len; i++) {
        crc = updateCRC8(crc, data[i]);
    }
    return crc;
}

bool PacketParser::processBit(uint8_t bit) {
    bit &= 0x01;

    switch (_state) {
        case PARSER_SEARCH_SFD:
            _shift_reg = (_shift_reg << 1) | bit;
            if (_shift_reg == SFD_BYTE) {
                // SFD matched! Align to byte boundaries starting now
                _state = PARSER_READ_LEN;
                _bit_acc = 0;
                _bits_collected = 0;
                _payload_idx = 0;
                memset(&_current_packet, 0, sizeof(_current_packet));
                _current_packet.timestamp_ms = millis();
            }
            break;

        case PARSER_READ_LEN:
            _bit_acc = (_bit_acc << 1) | bit;
            _bits_collected++;
            if (_bits_collected == 8) {
                uint8_t len = _bit_acc;
                if (len == 0 || len > MAX_PAYLOAD_LEN) {
                    // Invalid length; abort and re-search SFD
                    _state = PARSER_SEARCH_SFD;
                    _shift_reg = 0;
                } else {
                    _current_packet.length = len;
                    _state = PARSER_READ_PAYLOAD;
                    _bit_acc = 0;
                    _bits_collected = 0;
                }
            }
            break;

        case PARSER_READ_PAYLOAD:
            _bit_acc = (_bit_acc << 1) | bit;
            _bits_collected++;
            if (_bits_collected == 8) {
                _current_packet.payload[_payload_idx++] = _bit_acc;
                _bit_acc = 0;
                _bits_collected = 0;

                if (_payload_idx >= _current_packet.length) {
                    _current_packet.payload[_payload_idx] = '\0'; // Null-terminate for string safety
                    _state = PARSER_READ_CRC;
                }
            }
            break;

        case PARSER_READ_CRC:
            _bit_acc = (_bit_acc << 1) | bit;
            _bits_collected++;
            if (_bits_collected == 8) {
                _current_packet.received_crc = _bit_acc;

                // Compute CRC over [Length] + [Payload]
                uint8_t crc = calculateCRC8(&_current_packet.length, 1);
                for (uint8_t i = 0; i < _current_packet.length; i++) {
                    crc = updateCRC8(crc, _current_packet.payload[i]);
                }
                _current_packet.calculated_crc = crc;
                _current_packet.crc_ok = (_current_packet.received_crc == crc);

                // Prepare for next packet
                _state = PARSER_SEARCH_SFD;
                _shift_reg = 0;
                return true; // Complete packet assembled!
            }
            break;
    }

    return false;
}

const RxPacket& PacketParser::getPacket() const {
    return _current_packet;
}
