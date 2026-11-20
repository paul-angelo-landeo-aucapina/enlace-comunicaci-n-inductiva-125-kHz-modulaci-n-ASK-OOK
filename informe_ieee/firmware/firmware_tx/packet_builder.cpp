#include "packet_builder.h"
#include <string.h>

PacketBuilder::PacketBuilder() {
    reset();
}

void PacketBuilder::reset() {
    // Stateless helper, reset hook provided for consistency
}

uint8_t PacketBuilder::updateCRC8(uint8_t crc, uint8_t byte) {
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

uint8_t PacketBuilder::calculateCRC8(const uint8_t *data, uint8_t len) {
    uint8_t crc = CRC8_INIT;
    for (uint8_t i = 0; i < len; i++) {
        crc = updateCRC8(crc, data[i]);
    }
    return crc;
}

bool PacketBuilder::buildPacket(const uint8_t *payload, uint8_t length, uint8_t *out_buffer, uint16_t max_out_len, uint16_t *actual_out_len) {
    if (!payload || length == 0 || length > MAX_PAYLOAD_LEN || !out_buffer || !actual_out_len) {
        return false;
    }

    uint16_t total_len = PREAMBLE_COUNT + 1 + 1 + length + 1; // Preambles + SFD + Len + Payload + CRC
    if (total_len > max_out_len) {
        return false;
    }

    uint16_t idx = 0;

    // 1. Preamble bytes (0xAA)
    for (uint8_t i = 0; i < PREAMBLE_COUNT; i++) {
        out_buffer[idx++] = PREAMBLE_BYTE;
    }

    // 2. Start-of-Frame Delimiter (0x7E)
    out_buffer[idx++] = SFD_BYTE;

    // 3. Length byte
    out_buffer[idx++] = length;

    // 4. Payload bytes
    memcpy(&out_buffer[idx], payload, length);
    idx += length;

    // 5. CRC-8 calculation (over [Length] + [Payload])
    uint8_t crc = calculateCRC8(&length, 1);
    for (uint8_t i = 0; i < length; i++) {
        crc = updateCRC8(crc, payload[i]);
    }
    out_buffer[idx++] = crc;

    *actual_out_len = idx;
    return true;
}
