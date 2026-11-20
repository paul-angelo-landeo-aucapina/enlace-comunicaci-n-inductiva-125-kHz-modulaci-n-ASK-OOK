#ifndef PACKET_BUILDER_H
#define PACKET_BUILDER_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

class PacketBuilder {
public:
    PacketBuilder();

    // Reset internal state
    void reset();

    // Assemble a complete frame into out_buffer
    // Frame format: [PREAMBLE x N] [SFD (0x7E)] [Length] [Payload...] [CRC-8]
    // Returns true on success, false if payload length is invalid or buffer is too small
    bool buildPacket(const uint8_t *payload, uint8_t length, uint8_t *out_buffer, uint16_t max_out_len, uint16_t *actual_out_len);

    // CRC-8 calculation helper (Polynomial: 0x07, Init: 0x00)
    // Matches the receiver polynomial and initial value
    static uint8_t calculateCRC8(const uint8_t *data, uint8_t len);
    static uint8_t updateCRC8(uint8_t crc, uint8_t byte);
};

#endif // PACKET_BUILDER_H
