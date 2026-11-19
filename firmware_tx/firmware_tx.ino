/**
 * ==============================================================================
 * Project: 125 kHz Inductive Communication Link (ASK/OOK)
 * Target:  ESP32 DevKitC (Transmitter Stage)
 * File:    firmware_tx.ino
 * License: MIT
 *
 * Description:
 *   High-precision, non-blocking transmitter modulating a 125 kHz inductive carrier
 *   via TLC555 ~RST gating on GPIO18 using IEEE 802.3 Manchester encoding at 250 bps.
 *   Utilizes ESP32 hardware timer (esp_timer @ 500 Hz / 2000 us ticks) for jitter-free
 *   symbol timing without blocking FreeRTOS tasks.
 *
 * Features:
 *   - Frame assembly: [0xAA, 0xAA] [0x7E] [Len] [Payload] [CRC-8]
 *   - Interactive USB Serial CLI (115200 baud) for diagnostics and manual commands
 *   - Continuous carrier mode ('carrier on/off') for tank tuning on oscilloscope
 *   - Automated benchmark burst mode ('bench <N>') for Packet Error Rate (PER) testing
 *   - Optional autonomous periodic beacon
 *   - Status LED on GPIO2
 * ==============================================================================
 */

#include <Arduino.h>
#include "config.h"
#include "packet_builder.h"
#include "manchester_tx.h"

// Instances
PacketBuilder builder;
ManchesterTx  tx;

// Transmission buffers
static uint8_t frame_buffer[MAX_FRAME_BYTES];
static uint16_t frame_len = 0;

// Benchmarking & Beacon state
static bool beacon_enabled = false;
static uint32_t last_beacon_ms = 0;
static uint32_t beacon_counter = 0;

static uint32_t bench_remaining = 0;
static uint32_t bench_total = 0;
static uint32_t last_packet_end_ms = 0;

// Command line input buffer
static String serial_input = "";

// Helper to trigger packet transmission
bool transmitPayload(const uint8_t *payload, uint8_t len) {
    if (tx.isBusy()) {
        Serial.println(F("[ERROR] Transmitter is busy! Cannot enqueue packet."));
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

    uint8_t crc = frame_buffer[frame_len - 1];
    Serial.printf("[TX] Sending Frame: Len=%u bytes | CRC=0x%02X | Payload=\"%s\"\n",
                  len, crc, (const char*)payload);
    return true;
}

void printHelp() {
    Serial.println(F("Available Commands:"));
    Serial.println(F("  send <text>      : Transmit custom text string"));
    Serial.println(F("  ping             : Transmit standard \"PING\" test packet"));
    Serial.println(F("  bench <count>    : Send a burst of N consecutive packets for PER tests"));
    Serial.println(F("  carrier on|off   : Continuous 125 kHz carrier for oscilloscope calibration"));
    Serial.println(F("  beacon on|off    : Periodic telemetry heartbeat every 3 seconds"));
    Serial.println(F("  status           : Report transmitter hardware and packet telemetry"));
    Serial.println(F("  help             : Show this command menu"));
}

void processCommand(String cmd) {
    cmd.trim();
    if (cmd.length() == 0) return;

    if (cmd.equalsIgnoreCase("help")) {
        printHelp();
    } else if (cmd.equalsIgnoreCase("ping")) {
        const char msg[] = "PING";
        transmitPayload((const uint8_t*)msg, strlen(msg));
    } else if (cmd.startsWith("send ") || cmd.startsWith("SEND ")) {
        String payload_str = cmd.substring(5);
        if (payload_str.length() == 0) {
            Serial.println(F("[ERROR] Empty payload."));
        } else if (payload_str.length() > MAX_PAYLOAD_LEN) {
            Serial.printf("[ERROR] Payload exceeds %u bytes limit.\n", MAX_PAYLOAD_LEN);
        } else {
            transmitPayload((const uint8_t*)payload_str.c_str(), payload_str.length());
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
        Serial.println(F("          Verify 125 kHz sine wave on TX resonant tank with oscilloscope."));
    } else if (cmd.equalsIgnoreCase("carrier off")) {
        tx.setContinuousCarrier(false);
        Serial.println(F("[CARRIER] Continuous carrier disabled (GPIO18 Low)."));
    } else if (cmd.equalsIgnoreCase("beacon on")) {
        beacon_enabled = true;
        last_beacon_ms = millis();
        Serial.println(F("[BEACON] Periodic autonomous beacon enabled (every 3000 ms)."));
    } else if (cmd.equalsIgnoreCase("beacon off")) {
        beacon_enabled = false;
        Serial.println(F("[BEACON] Periodic beacon disabled."));
    } else if (cmd.equalsIgnoreCase("status")) {
        TxStats stats = tx.getStats();
        Serial.println(F("--- [Transmitter Status] ---"));
        Serial.printf("  Hardware Pin   : GPIO%u (OOK Gating)\n", TX_PIN);
        Serial.printf("  Status LED Pin : GPIO%u\n", LED_PIN);
        Serial.printf("  State          : %s\n", tx.isContinuousCarrier() ? "CONTINUOUS CARRIER" : (tx.isBusy() ? "TRANSMITTING" : "IDLE"));
        Serial.printf("  Packets Sent   : %u\n", stats.packets_sent);
        Serial.printf("  Symbols Sent   : %u\n", stats.total_symbols_sent);
        Serial.printf("  Last TX Time   : %u ms\n", stats.last_tx_duration_ms);
        Serial.printf("  Beacon Mode    : %s\n", beacon_enabled ? "ON" : "OFF");
        Serial.println(F("----------------------------"));
    } else {
        // Fallback: send input string directly as packet payload
        if (cmd.length() <= MAX_PAYLOAD_LEN) {
            transmitPayload((const uint8_t*)cmd.c_str(), cmd.length());
        } else {
            Serial.printf("[ERROR] Unrecognized command or message too long (%u > %u). Type 'help'.\n",
                          (unsigned int)cmd.length(), MAX_PAYLOAD_LEN);
        }
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000); // Allow USB-CDC / UART bridge to stabilize

    Serial.println();
    Serial.println(F("=========================================================="));
    Serial.println(F("  ESP32 125 kHz ASK/OOK Inductive Transmitter (250 bps)  "));
    Serial.println(F("  Modulation: OOK | Encoding: Manchester (IEEE 802.3)     "));
    Serial.println(F("  Output Pin: GPIO18 (TLC555 ~RST Gating)                "));
    Serial.println(F("  Status LED: GPIO2                                      "));
    Serial.println(F("  Timer Mode: esp_timer hardware ISR @ 500 Hz (2000 us)  "));
    Serial.println(F("=========================================================="));

    // Initialize transmitter engine
    tx.begin(TX_PIN, LED_PIN);
    builder.reset();

    Serial.println(F("[SYSTEM] Transmitter initialized and ready in IDLE state."));
    Serial.println(F("[SYSTEM] Type 'help' for command options or type text to send."));
}

void loop() {
    // 1. Process incoming Serial CLI commands
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') {
            continue; // Ignore carriage return
        }
        if (c == '\n') {
            processCommand(serial_input);
            serial_input = "";
        } else {
            serial_input += c;
        }
    }

    // 2. Handle Benchmark Burst Progression
    if (bench_remaining > 0 && !tx.isBusy()) {
        // Apply guard interval between consecutive benchmark frames
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

    // 3. Handle Periodic Beacon (if enabled and transmitter is idle)
    if (beacon_enabled && !tx.isBusy() && bench_remaining == 0) {
        if (millis() - last_beacon_ms >= 3000) {
            last_beacon_ms = millis();
            beacon_counter++;
            char beacon_msg[32];
            snprintf(beacon_msg, sizeof(beacon_msg), "BEACON #%u", (unsigned int)beacon_counter);
            transmitPayload((const uint8_t*)beacon_msg, strlen(beacon_msg));
        }
    }

    yield(); // FreeRTOS watchdog feeding
}
