#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <cmath>

#include "../firmware_tx/config.h"
#include "../firmware_tx/imu_telemetry.h"
#include "../firmware_tx/packet_builder.h"
#include "../firmware_tx/manchester_tx.h"

#include "../firmware_rx/manchester_rx.h"
#include "../firmware_rx/packet_parser.h"

void test_imu_packet_structure() {
    std::cout << "[TEST 1] Verifying ImuTelemetryPacket binary packing..." << std::endl;

    // Must be strictly 10 bytes for our 250 bps bandwidth budget
    assert(sizeof(ImuTelemetryPacket) == 10);
    std::cout << "  ImuTelemetryPacket size = " << sizeof(ImuTelemetryPacket)
              << " bytes (Strict packing verified)." << std::endl;

    // Test attitude math
    float p0 = imuCalculatePitchDeg(0, 0, 1000);
    float r0 = imuCalculateRollDeg(0, 1000);
    assert(fabsf(p0) < 0.001f);
    assert(fabsf(r0) < 0.001f);

    float p90 = imuCalculatePitchDeg(1000, 0, 0);
    assert(fabsf(p90 - 90.0f) < 0.1f);

    float r90 = imuCalculateRollDeg(1000, 0);
    assert(fabsf(r90 - 90.0f) < 0.1f);

    std::cout << "  Attitude pitch/roll trigonometry verified." << std::endl;
}

void test_imu_inductive_loopback() {
    std::cout << "[TEST 2] Running Closed-Loop Inductive IMU Telemetry Stream..." << std::endl;

    PacketBuilder builder;
    ManchesterTx  tx;
    ManchesterRx  rx;
    PacketParser  parser;

    tx.begin(18, 2);
    rx.begin(19);

    uint32_t current_time_us = 100000;

    // Simulate sending 5 consecutive IMU telemetry frames (Stream mode)
    for (uint8_t seq = 0; seq < 5; seq++) {
        ImuTelemetryPacket tx_pkt;
        tx_pkt.type = PKT_TYPE_IMU;
        tx_pkt.seq = seq;
        tx_pkt.accel_x_mg = (int16_t)(150 + seq * 40);  // Dynamic acceleration
        tx_pkt.accel_y_mg = (int16_t)(-80 - seq * 25);
        tx_pkt.accel_z_mg = (int16_t)(980 - seq * 10);
        tx_pkt.temp_cdeg = (int16_t)(2520 + seq * 15);  // 25.20°C to 25.80°C

        uint8_t frame_buf[MAX_FRAME_BYTES];
        uint16_t frame_len = 0;
        bool built = builder.buildPacket((const uint8_t*)&tx_pkt, sizeof(tx_pkt),
                                         frame_buf, sizeof(frame_buf), &frame_len);
        assert(built == true);
        assert(frame_len == 2 + 1 + 1 + 10 + 1); // 15 bytes

        bool sent = tx.sendPacket(frame_buf, frame_len);
        assert(sent == true);

        uint16_t total_symbols = tx.getSymbolCount();
        assert(total_symbols == frame_len * 8 * 2); // 240 symbols

        bool current_line = false;
        bool packet_received = false;

        for (uint16_t s = 0; s < total_symbols; s++) {
            bool sym_level = (tx.getSymbol(s) != 0);
            if (s == 0 || sym_level != current_line) {
                current_line = sym_level;
                rx.handleEdge(current_time_us, current_line);
            }
            current_time_us += T_NOMINAL_HALF_US;

            while (rx.available()) {
                uint8_t bit = rx.read();
                if (parser.processBit(bit)) {
                    packet_received = true;
                    const RxPacket& rx_pkt = parser.getPacket();

                    assert(rx_pkt.crc_ok == true);
                    assert(rx_pkt.length == sizeof(ImuTelemetryPacket));

                    const ImuTelemetryPacket* rx_imu = reinterpret_cast<const ImuTelemetryPacket*>(rx_pkt.payload);
                    assert(rx_imu->type == PKT_TYPE_IMU);
                    assert(rx_imu->seq == seq);
                    assert(rx_imu->accel_x_mg == tx_pkt.accel_x_mg);
                    assert(rx_imu->accel_y_mg == tx_pkt.accel_y_mg);
                    assert(rx_imu->accel_z_mg == tx_pkt.accel_z_mg);
                    assert(rx_imu->temp_cdeg == tx_pkt.temp_cdeg);

                    float pitch = imuCalculatePitchDeg(rx_imu->accel_x_mg, rx_imu->accel_y_mg, rx_imu->accel_z_mg);
                    float roll  = imuCalculateRollDeg(rx_imu->accel_y_mg, rx_imu->accel_z_mg);

                    std::cout << "  -> Received IMU Frame [Seq #" << (int)rx_imu->seq
                              << "]: Ax=" << (float)rx_imu->accel_x_mg / 1000.0f << "g"
                              << " | Ay=" << (float)rx_imu->accel_y_mg / 1000.0f << "g"
                              << " | Az=" << (float)rx_imu->accel_z_mg / 1000.0f << "g"
                              << " | Pitch=" << pitch << " deg"
                              << " | Roll=" << roll << " deg"
                              << " | Temp=" << (float)rx_imu->temp_cdeg / 100.0f << " C [PASS]"
                              << std::endl;
                }
            }
        }

        // Return to idle LOW
        if (current_line != false) {
            current_line = false;
            rx.handleEdge(current_time_us, current_line);
        }

        while (rx.available()) {
            uint8_t bit = rx.read();
            if (parser.processBit(bit)) {
                packet_received = true;
            }
        }

        assert(packet_received == true);
        tx.stop();

        // Advance guard interval (25 ms)
        current_time_us += (T_GUARD_INTERVAL_MS * 1000);
    }

    std::cout << "  5/5 IMU telemetry frames successfully received and verified with 0 bit errors!\n" << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  ESP32 MPU6050 IMU Inductive Telemetry Test Suite        " << std::endl;
    std::cout << "==========================================================" << std::endl;

    test_imu_packet_structure();
    test_imu_inductive_loopback();

    std::cout << ">>> ALL IMU TELEMETRY & ATTITUDE TESTS PASSED! <<<\n" << std::endl;
    return 0;
}
