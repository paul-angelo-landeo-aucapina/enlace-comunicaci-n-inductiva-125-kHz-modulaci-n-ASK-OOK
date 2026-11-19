#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <string>

// Include TX headers
#include "../firmware_tx/config.h"
#include "../firmware_tx/packet_builder.h"
#include "../firmware_tx/manchester_tx.h"

// Include RX headers for loopback test
#include "../firmware_rx/manchester_rx.h"
#include "../firmware_rx/packet_parser.h"

void test_packet_builder() {
    std::cout << "[TEST 1] Testing PacketBuilder frame assembly and CRC-8..." << std::endl;

    PacketBuilder builder;
    uint8_t frame_buf[MAX_FRAME_BYTES];
    uint16_t frame_len = 0;

    // Test rejection of invalid lengths
    assert(builder.buildPacket(nullptr, 5, frame_buf, sizeof(frame_buf), &frame_len) == false);
    assert(builder.buildPacket((const uint8_t*)"A", 0, frame_buf, sizeof(frame_buf), &frame_len) == false);
    assert(builder.buildPacket((const uint8_t*)"A", 33, frame_buf, sizeof(frame_buf), &frame_len) == false);

    // Test valid frame construction
    const char* payload = "HELLO";
    uint8_t payload_len = strlen(payload);
    bool ok = builder.buildPacket((const uint8_t*)payload, payload_len, frame_buf, sizeof(frame_buf), &frame_len);
    assert(ok == true);

    // Expected frame: [0xAA, 0xAA, 0x7E, 0x05, 'H', 'E', 'L', 'L', 'O', CRC]
    uint16_t expected_total_len = 2 + 1 + 1 + 5 + 1; // 10 bytes
    assert(frame_len == expected_total_len);
    assert(frame_buf[0] == 0xAA);
    assert(frame_buf[1] == 0xAA);
    assert(frame_buf[2] == 0x7E);
    assert(frame_buf[3] == 0x05);
    assert(memcmp(&frame_buf[4], payload, 5) == 0);

    // Check CRC matches PacketParser CRC-8 algorithm
    uint8_t expected_crc = PacketParser::calculateCRC8(&frame_buf[3], 1); // length byte
    for (int i = 0; i < 5; i++) {
        expected_crc = PacketParser::updateCRC8(expected_crc, (uint8_t)payload[i]);
    }
    assert(frame_buf[frame_len - 1] == expected_crc);

    std::cout << "  Frame assembled successfully (" << frame_len << " bytes, CRC: 0x"
              << std::hex << (int)expected_crc << std::dec << ")." << std::endl;
}

void test_manchester_encoding() {
    std::cout << "[TEST 2] Testing Manchester IEEE 802.3 Symbol Encoding..." << std::endl;

    ManchesterTx tx;
    tx.begin(18, 2);

    // Test pattern: single byte 0xA5 (binary: 10100101)
    uint8_t test_data[] = {0xA5};
    bool ok = tx.sendPacket(test_data, 1);
    assert(ok == true);

    // 1 byte = 8 bits = 16 symbols
    assert(tx.getSymbolCount() == 16);

    // Expected symbols:
    // Bit 1: 1, 0
    // Bit 0: 0, 1
    // Bit 1: 1, 0
    // Bit 0: 0, 1
    // Bit 0: 0, 1
    // Bit 1: 1, 0
    // Bit 0: 0, 1
    // Bit 1: 1, 0
    uint8_t expected_symbols[16] = {
        1, 0,  // 1
        0, 1,  // 0
        1, 0,  // 1
        0, 1,  // 0
        0, 1,  // 0
        1, 0,  // 1
        0, 1,  // 0
        1, 0   // 1
    };

    for (int i = 0; i < 16; i++) {
        assert(tx.getSymbol(i) == expected_symbols[i]);
    }

    std::cout << "  Manchester symbols verified against IEEE 802.3 bit transitions." << std::endl;
    tx.stop();
}

void run_closed_loop_test(const std::string& test_name, const uint8_t* payload, uint8_t len) {
    std::cout << "[LOOPBACK] Running: " << test_name << " (" << (int)len << " bytes)..." << std::endl;

    PacketBuilder builder;
    ManchesterTx  tx;
    ManchesterRx  rx;
    PacketParser  parser;

    tx.begin(18, 2);
    rx.begin(19);

    // 1. Build packet at TX
    uint8_t frame_buf[MAX_FRAME_BYTES];
    uint16_t frame_len = 0;
    bool built = builder.buildPacket(payload, len, frame_buf, sizeof(frame_buf), &frame_len);
    assert(built == true);

    // 2. Encode to Manchester symbols at TX
    bool sent = tx.sendPacket(frame_buf, frame_len);
    assert(sent == true);

    uint16_t total_symbols = tx.getSymbolCount();
    assert(total_symbols == frame_len * 8 * 2);

    // 3. Convert symbol stream into physical edge transitions and inject into RX
    // Every symbol lasts T_NOMINAL_HALF_US (2000 us).
    // An edge is emitted whenever the symbol level changes, or at the initial transition.
    uint32_t current_time_us = 500000; // Start at 500 ms
    bool current_line_level = false;   // Idle line is LOW

    bool packet_decoded = false;

    for (uint16_t i = 0; i < total_symbols; i++) {
        bool symbol_level = (tx.getSymbol(i) != 0);

        // If line state changes, fire GPIO edge interrupt in receiver
        if (i == 0 || symbol_level != current_line_level) {
            current_line_level = symbol_level;
            rx.handleEdge(current_time_us, current_line_level);
        }

        // Advance half-bit interval
        current_time_us += T_NOMINAL_HALF_US;

        // Drain bits from RX FIFO into PacketParser
        while (rx.available()) {
            uint8_t bit = rx.read();
            if (parser.processBit(bit)) {
                packet_decoded = true;
                const RxPacket& pkt = parser.getPacket();

                assert(pkt.crc_ok == true);
                assert(pkt.length == len);
                assert(memcmp(pkt.payload, payload, len) == 0);

                std::cout << "  -> SUCCESS: Decoded " << (int)pkt.length
                          << " bytes | CRC: 0x" << std::hex << (int)pkt.received_crc << std::dec
                          << " | CRC Status: OK" << std::endl;
            }
        }
    }

    // Trailing idle transition
    if (current_line_level != false) {
        current_line_level = false;
        rx.handleEdge(current_time_us, current_line_level);
    }

    // Drain remaining bits if any
    while (rx.available()) {
        uint8_t bit = rx.read();
        if (parser.processBit(bit)) {
            packet_decoded = true;
        }
    }

    assert(packet_decoded == true);
    tx.stop();
}

void test_end_to_end_loopback() {
    std::cout << "[TEST 3] Running End-to-End Closed-Loop TX -> RX Simulation..." << std::endl;

    // Case 1: Standard short string
    const char* str1 = "HELLO";
    run_closed_loop_test("ASCII Standard", (const uint8_t*)str1, strlen(str1));

    // Case 2: Telemetry message with numbers and symbols
    const char* str2 = "CAMPOS 125kHz OOK OK #42";
    run_closed_loop_test("Telemetry Frame", (const uint8_t*)str2, strlen(str2));

    // Case 3: Maximum length payload (32 bytes)
    const char* str3 = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
    run_closed_loop_test("Max Payload (32 Bytes)", (const uint8_t*)str3, 32);

    // Case 4: Binary data containing edge-case byte values (0x00, 0xFF, 0x55, 0xAA, 0x7E)
    uint8_t bin_data[] = {0x00, 0xFF, 0x55, 0xAA, 0x7E, 0x12, 0x34, 0x89};
    run_closed_loop_test("Arbitrary Binary Payload", bin_data, sizeof(bin_data));

    std::cout << "  All closed-loop TX -> RX scenarios passed with 0 bit errors!\n" << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  ESP32 125 kHz TX Firmware & Loopback Test Suite         " << std::endl;
    std::cout << "==========================================================" << std::endl;

    test_packet_builder();
    test_manchester_encoding();
    test_end_to_end_loopback();

    std::cout << ">>> ALL TRANSMITTER & LOOPBACK TESTS PASSED SUCCESSFULLY! <<<\n" << std::endl;
    return 0;
}
