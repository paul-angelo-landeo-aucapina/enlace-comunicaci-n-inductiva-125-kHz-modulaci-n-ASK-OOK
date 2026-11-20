/**
 * ==============================================================================
 * Project: 125 kHz Inductive Communication Link (ASK/OOK)
 * Target:  ESP32 DevKitC (Receiver Stage)
 * File:    firmware_rx.ino
 * License: MIT
 *
 * Description:
 *   Asynchronous, interrupt-driven receiver decoding 250 bps Manchester (IEEE 802.3)
 *   from an inductive envelope demodulator + LM393 comparator on GPIO19.
 *   Provides clock recovery, glitch filtering, frame synchronization (SFD 0x7E),
 *   CRC-8 error detection, and real-time USB Serial diagnostics.
 * ==============================================================================
 */

#include <Arduino.h>
#include "config.h"
#include "manchester_rx.h"
#include "packet_parser.h"
#include "imu_telemetry.h"

// Instantiate decoder and parser
ManchesterRx rxDecoder;
PacketParser parser;

// Packet tracking
static uint32_t packet_counter = 0;
static uint32_t led_off_time_ms = 0;
static uint32_t last_heartbeat_ms = 0;

// Interrupt Service Routine (IRAM_ATTR ensures execution from internal RAM)
void IRAM_ATTR isr_rx_edge() {
    uint32_t now_us = (uint32_t)esp_timer_get_time();
    bool pin_level = (digitalRead(RX_PIN) == HIGH);
    rxDecoder.handleEdge(now_us, pin_level);
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000); // Allow USB-CDC / UART to stabilize

    Serial.println();
    Serial.println(F("=========================================================="));
    Serial.println(F("  ESP32 125 kHz ASK/OOK Inductive Receiver (250 bps)     "));
    Serial.println(F("  Modulation: OOK | Encoding: Manchester (IEEE 802.3)     "));
    Serial.println(F("  Input Pin : GPIO19 (LM393 Comparator Output)           "));
    Serial.println(F("  Status LED: GPIO2                                      "));
    Serial.println(F("=========================================================="));

    // Configure GPIOs
    pinMode(RX_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Initialize decoder
    rxDecoder.begin(RX_PIN);
    parser.reset();

    // Attach edge interrupt on CHANGE (both rising and falling edges)
    attachInterrupt(digitalPinToInterrupt(RX_PIN), isr_rx_edge, CHANGE);

    Serial.println(F("[SYSTEM] Interrupts configured. Listening for incoming carrier pulses..."));
}

void loop() {
    // 1. Process all bits currently buffered in the ISR ring buffer
    while (rxDecoder.available()) {
        uint8_t bit = rxDecoder.read();
        
        if (parser.processBit(bit)) {
            // Complete packet assembled!
            packet_counter++;
            const RxPacket& pkt = parser.getPacket();

            // Pulse status LED
            digitalWrite(LED_PIN, HIGH);
            led_off_time_ms = millis() + 80;

            // Formatted telemetry output
            Serial.println();
            Serial.printf(">>> [PKT #%u] Timestamp: %u ms <<<\n", packet_counter, pkt.timestamp_ms);
            Serial.printf("  Length  : %u bytes\n", pkt.length);

            // Print HEX Dump
            Serial.print(F("  HEX     : "));
            for (uint8_t i = 0; i < pkt.length; i++) {
                if (pkt.payload[i] < 0x10) Serial.print('0');
                Serial.print(pkt.payload[i], HEX);
                Serial.print(' ');
            }
            Serial.println();

            // Print ASCII Payload
            Serial.printf("  ASCII   : \"%s\"\n", (const char*)pkt.payload);

            // CRC-8 Verification Status
            Serial.printf("  CRC-8   : 0x%02X (Calc: 0x%02X) -> %s\n",
                          pkt.received_crc,
                          pkt.calculated_crc,
                          pkt.crc_ok ? "[PASS]" : "[ERROR - CRC MISMATCH]");

            // Check if this is an IMU Telemetry Packet (0x01, 10 bytes)
            if (pkt.crc_ok && pkt.length == sizeof(ImuTelemetryPacket) && pkt.payload[0] == PKT_TYPE_IMU) {
                const ImuTelemetryPacket* imu_pkt = reinterpret_cast<const ImuTelemetryPacket*>(pkt.payload);
                float ax_g = (float)imu_pkt->accel_x_mg / 1000.0f;
                float ay_g = (float)imu_pkt->accel_y_mg / 1000.0f;
                float az_g = (float)imu_pkt->accel_z_mg / 1000.0f;
                float pitch = imuCalculatePitchDeg(imu_pkt->accel_x_mg, imu_pkt->accel_y_mg, imu_pkt->accel_z_mg);
                float roll  = imuCalculateRollDeg(imu_pkt->accel_y_mg, imu_pkt->accel_z_mg);
                float temp_c = (float)imu_pkt->temp_cdeg / 100.0f;

                Serial.println(F("  === [DECODED MPU6050 IMU TELEMETRY] ==="));
                Serial.printf("    Sequence   : #%u\n", (unsigned int)imu_pkt->seq);
                Serial.printf("    Accel (g)  : X=%+0.3f | Y=%+0.3f | Z=%+0.3f\n", ax_g, ay_g, az_g);
                Serial.printf("    Attitude   : Pitch=%+0.1f deg | Roll=%+0.1f deg\n", pitch, roll);
                Serial.printf("    Temperature: %0.2f deg C\n", temp_c);
            }

            // Timing Quality & Link Diagnostics
            RxStats stats = rxDecoder.getStats();
            Serial.printf("  Link Quality: Jitter: %+d us | Glitches: %u | Sync Errors: %u\n",
                          stats.last_jitter_us,
                          stats.glitches_rejected,
                          stats.sync_errors);
            Serial.println(F("----------------------------------------------------------"));
        }
    }

    // 2. Turn off status LED after pulse timeout
    if (led_off_time_ms > 0 && millis() >= led_off_time_ms) {
        digitalWrite(LED_PIN, LOW);
        led_off_time_ms = 0;
    }

    // 3. Periodic bench diagnostic heartbeat (every 10 seconds)
    if (millis() - last_heartbeat_ms >= 10000) {
        last_heartbeat_ms = millis();
        RxStats stats = rxDecoder.getStats();
        Serial.printf("[DIAGNOSTIC] Total Edges: %u | Glitches Filtered: %u | Sync Resets: %u | Decoded Bits: %u\n",
                      stats.total_edges,
                      stats.glitches_rejected,
                      stats.sync_errors,
                      stats.bits_decoded);
    }

    yield(); // FreeRTOS watchdog feeding
}
