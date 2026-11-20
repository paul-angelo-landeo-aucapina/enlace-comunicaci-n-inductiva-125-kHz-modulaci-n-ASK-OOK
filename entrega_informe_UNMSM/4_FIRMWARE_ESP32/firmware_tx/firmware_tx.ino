/**
 * ==============================================================================
 * Project: 125 kHz Inductive Communication Link (ASK/OOK)
 * Target:  ESP32 DevKitC (Transmitter Stage with MPU6050 IMU & OLED Display)
 * File:    firmware_tx.ino
 * License: MIT
 *
 * Description:
 *   High-precision, non-blocking transmitter modulating a 125 kHz inductive carrier
 *   via TLC555 ~RST gating on GPIO18 using IEEE 802.3 Manchester encoding at 250 bps.
 *   Integrates an MPU6050 (6-DOF IMU) and SSD1306 OLED display on I2C (GPIO21/22).
 *
 * Features:
 *   - Compact 10-byte binary IMU telemetry: [0x01] [Seq] [Ax, Ay, Az] [Temp]
 *   - Frame assembly: [0xAA, 0xAA] [0x7E] [Len] [Payload] [CRC-8]
 *   - Hardware esp_timer ISR @ 500 Hz (2000 us) for jitter-free line coding
 *   - Shared I2C Bus on GPIO21 (SDA) and GPIO22 (SCL):
 *       * MPU6050 IMU: Address 0x68
 *       * SSD1306 OLED: Address 0x3C
 *   - Real-time local telemetry dashboard on OLED display
 *   - Fault-tolerant auto-fallback to simulated tilt if sensor not detected
 *   - Interactive USB Serial CLI (115200 baud)
 * ==============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "packet_builder.h"
#include "manchester_tx.h"
#include "imu_telemetry.h"
#include "mpu6050_driver.h"
#include "oled_display.h"

// Core Communication Instances
PacketBuilder builder;
ManchesterTx  tx;

// Sensor and Display Instances (Shared I2C on GPIO21 / GPIO22)
Mpu6050Driver imu;
OledDisplay   oled;

// Transmission buffers
static uint8_t frame_buffer[MAX_FRAME_BYTES];
static uint16_t frame_len = 0;

// IMU Streaming State
static bool     imu_streaming = false;
static uint8_t  imu_seq = 0;
static uint32_t last_imu_tx_ms = 0;
static const uint32_t IMU_STREAM_INTERVAL_MS = 600; // 480 ms TX time + 120 ms guard

// Benchmarking & Beacon state
static bool beacon_enabled = false;
static uint32_t last_beacon_ms = 0;
static uint32_t beacon_counter = 0;

static uint32_t bench_remaining = 0;
static uint32_t bench_total = 0;
static uint32_t last_packet_end_ms = 0;

// Command line input buffer
static String serial_input = "";

// Helper to trigger raw packet transmission
bool transmitPayload(const uint8_t *payload, uint8_t len) {
    if (tx.isBusy()) {
        return false;
    }

    if (!builder.buildPacket(payload, len, frame_buffer, sizeof(frame_buffer), &frame_len)) {
        Serial.println(F("[ERROR] Packet assembly failed. Check payload length (1-32 bytes)."));
        return false;
    }

    if (!tx.sendPacket(frame_buffer, frame_len)) {
        Serial.println(F("[ERROR] Failed to start Manchester transmitter engine."));
        return false;
    }

    return true;
}

// Helper to sample MPU6050, update OLED, and transmit binary telemetry
bool sendImuTelemetry() {
    int16_t ax_mg = 0, ay_mg = 0, az_mg = 0, temp_cdeg = 0;
    if (!imu.readData(ax_mg, ay_mg, az_mg, temp_cdeg)) {
        Serial.println(F("[IMU] Failed to read sensor data."));
        return false;
    }

    // Build compact 10-byte telemetry packet
    ImuTelemetryPacket pkt;
    pkt.type = PKT_TYPE_IMU;
    pkt.seq = imu_seq++;
    pkt.accel_x_mg = ax_mg;
    pkt.accel_y_mg = ay_mg;
    pkt.accel_z_mg = az_mg;
    pkt.temp_cdeg = temp_cdeg;

    if (!transmitPayload((const uint8_t*)&pkt, sizeof(pkt))) {
        return false;
    }

    // Update local OLED display with live measurements
    oled.updateTelemetry(true, tx.getStats().packets_sent, pkt.seq,
                         ax_mg, ay_mg, az_mg, temp_cdeg, imu.isSimulated());

    float pitch = imuCalculatePitchDeg(ax_mg, ay_mg, az_mg);
    float roll  = imuCalculateRollDeg(ay_mg, az_mg);

    Serial.printf("[TX-IMU #%u] Seq:%u | Accel:[%+0.2fg, %+0.2fg, %+0.2fg] | Pitch:%+0.1f° Roll:%+0.1f° | Temp:%0.1f°C %s\n",
                  tx.getStats().packets_sent, pkt.seq,
                  (float)ax_mg / 1000.0f, (float)ay_mg / 1000.0f, (float)az_mg / 1000.0f,
                  pitch, roll, (float)temp_cdeg / 100.0f,
                  imu.isSimulated() ? "(SIM)" : "(HW)");

    return true;
}

void printHelp() {
    Serial.println(F("Available Commands:"));
    Serial.println(F("  imu on|off       : Continuous MPU6050 inductive telemetry streaming"));
    Serial.println(F("  read             : Read MPU6050 sensor once and print to Serial/OLED"));
    Serial.println(F("  send <text>      : Transmit custom text string"));
    Serial.println(F("  ping             : Transmit standard \"PING\" test packet"));
    Serial.println(F("  bench <count>    : Send a burst of N consecutive packets for PER tests"));
    Serial.println(F("  carrier on|off   : Continuous 125 kHz carrier for oscilloscope calibration"));
    Serial.println(F("  beacon on|off    : Periodic telemetry heartbeat every 3 seconds"));
    Serial.println(F("  status           : Report transmitter, sensor, and display status"));
    Serial.println(F("  help             : Show this command menu"));
}

void processCommand(String cmd) {
    cmd.trim();
    if (cmd.length() == 0) return;

    if (cmd.equalsIgnoreCase("help")) {
        printHelp();
    } else if (cmd.equalsIgnoreCase("imu on")) {
        imu_streaming = true;
        Serial.println(F("[IMU] Continuous telemetry streaming ENABLED (~1.7 Hz)."));
    } else if (cmd.equalsIgnoreCase("imu off")) {
        imu_streaming = false;
        Serial.println(F("[IMU] Continuous telemetry streaming DISABLED."));
    } else if (cmd.equalsIgnoreCase("read")) {
        int16_t ax, ay, az, t;
        imu.readData(ax, ay, az, t);
        float pitch = imuCalculatePitchDeg(ax, ay, az);
        float roll  = imuCalculateRollDeg(ay, az);
        Serial.printf("[IMU READ] Ax=%+0.3fg Ay=%+0.3fg Az=%+0.3fg | Pitch=%+0.1f° Roll=%+0.1f° | Temp=%0.2f°C %s\n",
                      (float)ax / 1000.0f, (float)ay / 1000.0f, (float)az / 1000.0f,
                      pitch, roll, (float)t / 100.0f,
                      imu.isSimulated() ? "(SIMULATED)" : "(HARDWARE)");
        oled.updateTelemetry(tx.isBusy(), tx.getStats().packets_sent, imu_seq, ax, ay, az, t, imu.isSimulated());
    } else if (cmd.equalsIgnoreCase("ping")) {
        const char msg[] = "PING";
        transmitPayload((const uint8_t*)msg, strlen(msg));
        Serial.println(F("[TX] PING packet sent."));
    } else if (cmd.startsWith("send ") || cmd.startsWith("SEND ")) {
        String payload_str = cmd.substring(5);
        if (payload_str.length() == 0) {
            Serial.println(F("[ERROR] Empty payload."));
        } else if (payload_str.length() > MAX_PAYLOAD_LEN) {
            Serial.printf("[ERROR] Payload exceeds %u bytes limit.\n", MAX_PAYLOAD_LEN);
        } else {
            transmitPayload((const uint8_t*)payload_str.c_str(), payload_str.length());
            Serial.printf("[TX] Sent text: \"%s\"\n", payload_str.c_str());
        }
    } else if (cmd.startsWith("bench ") || cmd.startsWith("BENCH ")) {
        int count = cmd.substring(6).toInt();
        if (count <= 0) {
            Serial.println(F("[ERROR] Count must be > 0."));
        } else {
            bench_total = count;
            bench_remaining = count;
            Serial.printf("[BENCH] Starting burst transmission of %u packets...\n", bench_total);
            char bench_msg[32];
            snprintf(bench_msg, sizeof(bench_msg), "B#%04u", (unsigned int)(bench_total - bench_remaining + 1));
            transmitPayload((const uint8_t*)bench_msg, strlen(bench_msg));
            bench_remaining--;
        }
    } else if (cmd.equalsIgnoreCase("carrier on")) {
        tx.setContinuousCarrier(true);
        Serial.println(F("[CARRIER] Continuous 125 kHz carrier enabled on GPIO18 (~RST High)."));
        oled.printLine(0, "TX: CARRIER ON ");
    } else if (cmd.equalsIgnoreCase("carrier off")) {
        tx.setContinuousCarrier(false);
        Serial.println(F("[CARRIER] Continuous carrier disabled (GPIO18 Low)."));
        oled.printLine(0, "TX: CARRIER OFF");
    } else if (cmd.equalsIgnoreCase("beacon on")) {
        beacon_enabled = true;
        last_beacon_ms = millis();
        Serial.println(F("[BEACON] Periodic autonomous beacon enabled (every 3000 ms)."));
    } else if (cmd.equalsIgnoreCase("beacon off")) {
        beacon_enabled = false;
        Serial.println(F("[BEACON] Periodic beacon disabled."));
    } else if (cmd.equalsIgnoreCase("status")) {
        TxStats stats = tx.getStats();
        Serial.println(F("--- [Transmitter & Peripherals Status] ---"));
        Serial.printf("  OOK Modulation : GPIO%u (~RST TLC555)\n", TX_PIN);
        Serial.printf("  Status LED     : GPIO%u\n", LED_PIN);
        Serial.printf("  I2C Bus        : SDA=GPIO21, SCL=GPIO22\n");
        Serial.printf("  MPU6050 Status : %s\n", imu.isConnected() ? "CONNECTED (0x68)" : (imu.isSimulated() ? "SIMULATED FALLBACK" : "DISCONNECTED"));
        Serial.printf("  OLED Status    : %s\n", oled.isConnected() ? "CONNECTED (0x3C)" : "DISCONNECTED");
        Serial.printf("  Transmitter    : %s\n", tx.isContinuousCarrier() ? "CONTINUOUS CARRIER" : (tx.isBusy() ? "TRANSMITTING" : "IDLE"));
        Serial.printf("  IMU Streaming  : %s\n", imu_streaming ? "ACTIVE" : "OFF");
        Serial.printf("  Packets Sent   : %u\n", stats.packets_sent);
        Serial.printf("  Symbols Sent   : %u\n", stats.total_symbols_sent);
        Serial.printf("  Last TX Time   : %u ms\n", stats.last_tx_duration_ms);
        Serial.println(F("------------------------------------------"));
    } else {
        if (cmd.length() <= MAX_PAYLOAD_LEN) {
            transmitPayload((const uint8_t*)cmd.c_str(), cmd.length());
            Serial.printf("[TX] Sent text: \"%s\"\n", cmd.c_str());
        } else {
            Serial.printf("[ERROR] Unrecognized command. Type 'help'.\n");
        }
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000); // Allow USB UART to settle

    Serial.println();
    Serial.println(F("=========================================================="));
    Serial.println(F("  ESP32 125 kHz TX with MPU6050 IMU & OLED Display       "));
    Serial.println(F("  Modulation: OOK | Encoding: Manchester (IEEE 802.3)     "));
    Serial.println(F("  Output Pin: GPIO18 (TLC555 ~RST Gating)                "));
    Serial.println(F("  I2C Bus   : SDA=GPIO21, SCL=GPIO22                     "));
    Serial.println(F("=========================================================="));

    // 1. Initialize Transmitter Engine
    tx.begin(TX_PIN, LED_PIN);
    builder.reset();

    // 2. Initialize Shared I2C Bus (GPIO21 = SDA, GPIO22 = SCL)
    Wire.begin(21, 22, 400000);

    // 3. Initialize OLED Display (0x3C)
    if (oled.begin(21, 22)) {
        Serial.println(F("[I2C] SSD1306 OLED Display detected at 0x3C. [OK]"));
        oled.printLine(0, "CAMPOS 125kHz TX");
        oled.printLine(2, "Initializing...");
        oled.printLine(4, "MPU6050 + OLED");
        oled.printLine(6, "Ready in IDLE");
    } else {
        Serial.println(F("[I2C] SSD1306 OLED Display NOT found at 0x3C (Running headless)."));
    }

    // 4. Initialize MPU6050 IMU (0x68)
    if (imu.begin(21, 22)) {
        Serial.println(F("[I2C] MPU6050 6-DOF IMU detected at 0x68. [OK]"));
    } else {
        Serial.println(F("[I2C] MPU6050 NOT detected on I2C. Fallback to SIMULATED tilt active."));
    }

    Serial.println(F("[SYSTEM] Ready. Type 'imu on' to start streaming or 'help' for menu."));
}

void loop() {
    // 1. Handle Serial CLI input
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            processCommand(serial_input);
            serial_input = "";
        } else {
            serial_input += c;
        }
    }

    // 2. Handle Continuous IMU Telemetry Streaming
    if (imu_streaming && !tx.isBusy() && bench_remaining == 0) {
        if (millis() - last_imu_tx_ms >= IMU_STREAM_INTERVAL_MS) {
            last_imu_tx_ms = millis();
            sendImuTelemetry();
        }
    }

    // 3. Handle Benchmark Burst Progression
    if (bench_remaining > 0 && !tx.isBusy()) {
        if (millis() - last_packet_end_ms >= T_GUARD_INTERVAL_MS) {
            char bench_msg[32];
            snprintf(bench_msg, sizeof(bench_msg), "B#%04u", (unsigned int)(bench_total - bench_remaining + 1));
            transmitPayload((const uint8_t*)bench_msg, strlen(bench_msg));
            bench_remaining--;
            last_packet_end_ms = millis();

            if (bench_remaining == 0) {
                Serial.printf("[BENCH] Burst completed. Sent %u frames.\n", bench_total);
            }
        }
    } else if (!tx.isBusy()) {
        last_packet_end_ms = millis();
    }

    // 4. Handle Periodic Beacon
    if (beacon_enabled && !tx.isBusy() && bench_remaining == 0 && !imu_streaming) {
        if (millis() - last_beacon_ms >= 3000) {
            last_beacon_ms = millis();
            beacon_counter++;
            char beacon_msg[32];
            snprintf(beacon_msg, sizeof(beacon_msg), "BEACON #%u", (unsigned int)beacon_counter);
            transmitPayload((const uint8_t*)beacon_msg, strlen(beacon_msg));
            Serial.printf("[BEACON] Sent: \"%s\"\n", beacon_msg);
        }
    }

    yield(); // FreeRTOS watchdog feeding
}
