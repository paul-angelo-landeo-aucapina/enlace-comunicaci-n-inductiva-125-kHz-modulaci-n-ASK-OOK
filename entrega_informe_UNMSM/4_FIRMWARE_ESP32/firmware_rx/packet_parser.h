#ifndef PACKET_PARSER_H
#define PACKET_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

enum ParserState {
    PARSER_SEARCH_SFD,
    PARSER_READ_LEN,
    PARSER_READ_PAYLOAD,
    PARSER_READ_CRC
};

struct RxPacket {
    uint8_t length;
    uint8_t payload[MAX_PAYLOAD_LEN + 1]; // Extra byte for null terminator
    uint8_t received_crc;
    uint8_t calculated_crc;
    bool    crc_ok;
    uint32_t timestamp_ms;
};

class PacketParser {
public:
    PacketParser();

    void reset();

    // Feed one bit recovered from Manchester decoder
    // Returns true when a complete packet is assembled (check packet.crc_ok)
    bool processBit(uint8_t bit);

    const RxPacket& getPacket() const;

    // CRC-8 calculation helper (Polynomial: 0x07)
    static uint8_t calculateCRC8(const uint8_t *data, uint8_t len);
    static uint8_t updateCRC8(uint8_t crc, uint8_t byte);

private:
    ParserState _state;
    uint8_t     _shift_reg;
    uint8_t     _bit_acc;
    uint8_t     _bits_collected;
    uint8_t     _payload_idx;

    RxPacket    _current_packet;
};

#endif // PACKET_PARSER_H
