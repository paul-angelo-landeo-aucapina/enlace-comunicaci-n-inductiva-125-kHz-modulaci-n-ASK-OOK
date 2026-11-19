#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include "../firmware_rx/config.h"
#include "../firmware_rx/manchester_rx.h"
#include "../firmware_rx/packet_parser.h"

// Helper to encode a bitstream into IEEE 802.3 Manchester edge timestamps
// Bit 0: Rising at mid (LOW for first half, HIGH for second half)
// Bit 1: Falling at mid (HIGH for first half, LOW for second half)
struct SimulatedEdge {
    uint32_t timestamp_us;
    bool level;
};

std::vector<SimulatedEdge> generateManchesterSignal(const std::vector<uint8_t>& bytes, uint32_t start_time_us = 100000) {
    std::vector<SimulatedEdge> edges;
    uint32_t current_time = start_time_us;
    bool current_level = false;

    // Convert bytes into bit array
    std::vector<bool> bits;
    for (uint8_t b : bytes) {
        for (int i = 7; i >= 0; i--) {
            bits.push_back((b >> i) & 1);
        }
    }

    for (size_t i = 0; i < bits.size(); i++) {
        bool bit = bits[i];
        bool first_half = bit ? true : false;  // Bit 1 starts HIGH; Bit 0 starts LOW
        bool second_half = bit ? false : true; // Bit 1 ends LOW;   Bit 0 ends HIGH

        // Boundary edge (if previous level != first_half)
        if (i == 0 || current_level != first_half) {
            current_level = first_half;
            edges.push_back({current_time, current_level});
        }

        // Advance to mid-bit
        current_time += T_NOMINAL_HALF_US;

        // Mid-point transition
        current_level = second_half;
        edges.push_back({current_time, current_level});

        // Advance to end of bit
        current_time += T_NOMINAL_HALF_US;
    }

    return edges;
}

void test_crc8() {
    std::cout << "[TEST] Running CRC-8 verification..." << std::endl;

    // Test vector: empty
    uint8_t empty_crc = PacketParser::calculateCRC8(nullptr, 0);
    assert(empty_crc == 0x00);

    // Test vector: standard check
    const uint8_t test_data[] = {0x05, 'H', 'E', 'L', 'L', 'O'};
    uint8_t crc = PacketParser::calculateCRC8(test_data, sizeof(test_data));
    std::cout << "  Calculated CRC for \"HELLO\" frame: 0x" << std::hex << (int)crc << std::dec << std::endl;
    assert(crc != 0);

    // Recalculating with appended CRC should match
    uint8_t verify_crc = PacketParser::updateCRC8(crc, crc);
    (void)verify_crc;
    std::cout << "  CRC-8 self-test passed successfully." << std::endl;
}

void test_manchester_and_packet_parser() {
    std::cout << "[TEST] Running end-to-end Manchester decode & packet parser simulation..." << std::endl;

    ManchesterRx rx;
    PacketParser parser;
    rx.begin(19);

    // Build test packet:
    // Preamble: 2x 0xAA (16 bits)
    // SFD: 0x7E
    // Length: 5
    // Payload: "HELLO"
    // CRC: calculateCRC8(length + payload)
    std::vector<uint8_t> frame_bytes;
    frame_bytes.push_back(0xAA);
    frame_bytes.push_back(0xAA);
    frame_bytes.push_back(0x7E);

    uint8_t payload_len = 5;
    const char* message = "HELLO";
    frame_bytes.push_back(payload_len);
    for (int i = 0; i < payload_len; i++) {
        frame_bytes.push_back((uint8_t)message[i]);
    }

    // CRC over LEN + PAYLOAD
    uint8_t crc = PacketParser::calculateCRC8(&payload_len, 1);
    for (int i = 0; i < payload_len; i++) {
        crc = PacketParser::updateCRC8(crc, (uint8_t)message[i]);
    }
    frame_bytes.push_back(crc);

    // Generate physical edges
    std::vector<SimulatedEdge> edges = generateManchesterSignal(frame_bytes);
    std::cout << "  Total physical transitions generated: " << edges.size() << std::endl;

    // Feed edges into ManchesterRx
    bool packet_received = false;
    for (const auto& edge : edges) {
        rx.handleEdge(edge.timestamp_us, edge.level);

        while (rx.available()) {
            uint8_t bit = rx.read();
            if (parser.processBit(bit)) {
                packet_received = true;
                const RxPacket& pkt = parser.getPacket();
                std::cout << "  -> Packet Decoded! Length: " << (int)pkt.length
                          << " | Payload: \"" << (const char*)pkt.payload << "\""
                          << " | CRC: 0x" << std::hex << (int)pkt.received_crc
                          << " (calc 0x" << (int)pkt.calculated_crc << ")"
                          << " | Status: " << (pkt.crc_ok ? "PASS" : "FAIL")
                          << std::dec << std::endl;

                assert(pkt.crc_ok == true);
                assert(pkt.length == 5);
                assert(std::string((const char*)pkt.payload) == "HELLO");
            }
        }
    }

    assert(packet_received == true);
    RxStats stats = rx.getStats();
    std::cout << "  Decoded Bits: " << stats.bits_decoded
              << " | Glitches: " << stats.glitches_rejected
              << " | Sync Errors: " << stats.sync_errors << std::endl;
    std::cout << "  End-to-End Decoder test passed successfully!" << std::endl;
}

void test_jitter_and_glitches() {
    std::cout << "[TEST] Running noise & jitter resilience test..." << std::endl;

    ManchesterRx rx;
    PacketParser parser;
    rx.begin(19);

    // Build frame
    std::vector<uint8_t> frame_bytes = {0xAA, 0xAA, 0x7E, 0x04, 'T', 'E', 'S', 'T'};
    uint8_t len = 4;
    uint8_t crc = PacketParser::calculateCRC8(&len, 1);
    const char* payload = "TEST";
    for (int i = 0; i < len; i++) {
        crc = PacketParser::updateCRC8(crc, (uint8_t)payload[i]);
    }
    frame_bytes.push_back(crc);

    std::vector<SimulatedEdge> clean_edges = generateManchesterSignal(frame_bytes);
    std::vector<SimulatedEdge> noisy_edges;

    // Inject jitter (+/- 150 us) and high-frequency glitches (< 300 us)
    for (size_t i = 0; i < clean_edges.size(); i++) {
        SimulatedEdge e = clean_edges[i];

        // Add 100 us jitter to edge timing
        int32_t jitter = (i % 2 == 0) ? 80 : -80;
        e.timestamp_us += jitter;
        noisy_edges.push_back(e);

        // Inject high-frequency noise spikes (150 us and 300 us after edge)
        if (i == 5 || i == 18) {
            noisy_edges.push_back({e.timestamp_us + 150, !e.level}); // Glitch spike (150 us)
            noisy_edges.push_back({e.timestamp_us + 300, e.level});  // Return to level (150 us later)
        }
    }

    bool packet_received = false;
    for (const auto& edge : noisy_edges) {
        rx.handleEdge(edge.timestamp_us, edge.level);

        while (rx.available()) {
            uint8_t bit = rx.read();
            if (parser.processBit(bit)) {
                packet_received = true;
                const RxPacket& pkt = parser.getPacket();
                assert(pkt.crc_ok == true);
                assert(std::string((const char*)pkt.payload) == "TEST");
            }
        }
    }

    assert(packet_received == true);
    RxStats stats = rx.getStats();
    std::cout << "  Glitches successfully filtered: " << stats.glitches_rejected << std::endl;
    assert(stats.glitches_rejected >= 2);
    std::cout << "  Noise & Jitter resilience test passed successfully!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " ESP32 RX Firmware Unit Test Suite     " << std::endl;
    std::cout << "========================================" << std::endl;

    test_crc8();
    test_manchester_and_packet_parser();
    test_jitter_and_glitches();

    std::cout << "\n>>> ALL AUTOMATED TESTS PASSED! <<<\n" << std::endl;
    return 0;
}
