#ifndef SERIALPORT_H
#define SERIALPORT_H

/*
 * serialport.h — Generic UART2 serial port wrapper
 *
 * Uses TX2_PIN / RX2_PIN defined in pindefinition.h.
 * Preferences namespace : "serialport"
 *
 * NOTE: This port shares UART2 with RS485 Modbus RTU.
 *       Do NOT enable both simultaneously.
 *
 * Usage in main sketch:
 *   #include "iotboard.h"   // serialPortInit() is called inside boardinit()
 *
 *   // Write
 *   serialPort.println("Hello");
 *
 *   // Read
 *   if (serialPort.available()) {
 *       String data = serialPort.readStringUntil('\n');
 *   }
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include <Preferences.h>
#include "pindefinition.h"

// ── Preferences key names ────────────────────────────────────────────────────
#define SERIALPORT_PREF_NS "serialport"

// ── Public object ─────────────────────────────────────────────────────────────
// Serial2 is the ESP32 UART2 peripheral. Aliased here so user code can write:
//   serialPort.println("hello");
#define serialPort Serial2

// ── Internal state ────────────────────────────────────────────────────────────
static bool serialPortInitialized = false;

// ── Helper: build Arduino serial config word ──────────────────────────────────
static uint32_t serialport_buildConfig(uint8_t dataBits, char parity, uint8_t stopBits) {
    // dataBits : 5-8  |  parity : 'N','E','O'  |  stopBits : 1,2
    if (dataBits == 5) {
        if (parity == 'E') return (stopBits == 2) ? SERIAL_5E2 : SERIAL_5E1;
        if (parity == 'O') return (stopBits == 2) ? SERIAL_5O2 : SERIAL_5O1;
        return (stopBits == 2) ? SERIAL_5N2 : SERIAL_5N1;
    }
    if (dataBits == 6) {
        if (parity == 'E') return (stopBits == 2) ? SERIAL_6E2 : SERIAL_6E1;
        if (parity == 'O') return (stopBits == 2) ? SERIAL_6O2 : SERIAL_6O1;
        return (stopBits == 2) ? SERIAL_6N2 : SERIAL_6N1;
    }
    if (dataBits == 7) {
        if (parity == 'E') return (stopBits == 2) ? SERIAL_7E2 : SERIAL_7E1;
        if (parity == 'O') return (stopBits == 2) ? SERIAL_7O2 : SERIAL_7O1;
        return (stopBits == 2) ? SERIAL_7N2 : SERIAL_7N1;
    }
    // default 8
    if (parity == 'E') return (stopBits == 2) ? SERIAL_8E2 : SERIAL_8E1;
    if (parity == 'O') return (stopBits == 2) ? SERIAL_8O2 : SERIAL_8O1;
    return (stopBits == 2) ? SERIAL_8N2 : SERIAL_8N1;
}

// ── Helper: decode Arduino serial config word ─────────────────────────────────
static void serialport_decodeConfig(uint32_t cfg,
                                    uint8_t &dataBits,
                                    char    &parity,
                                    uint8_t &stopBits) {
    dataBits = 8; parity = 'N'; stopBits = 1;
    if      (cfg == SERIAL_5N1) { dataBits=5; parity='N'; stopBits=1; }
    else if (cfg == SERIAL_5N2) { dataBits=5; parity='N'; stopBits=2; }
    else if (cfg == SERIAL_5E1) { dataBits=5; parity='E'; stopBits=1; }
    else if (cfg == SERIAL_5E2) { dataBits=5; parity='E'; stopBits=2; }
    else if (cfg == SERIAL_5O1) { dataBits=5; parity='O'; stopBits=1; }
    else if (cfg == SERIAL_5O2) { dataBits=5; parity='O'; stopBits=2; }
    else if (cfg == SERIAL_6N1) { dataBits=6; parity='N'; stopBits=1; }
    else if (cfg == SERIAL_6N2) { dataBits=6; parity='N'; stopBits=2; }
    else if (cfg == SERIAL_6E1) { dataBits=6; parity='E'; stopBits=1; }
    else if (cfg == SERIAL_6E2) { dataBits=6; parity='E'; stopBits=2; }
    else if (cfg == SERIAL_6O1) { dataBits=6; parity='O'; stopBits=1; }
    else if (cfg == SERIAL_6O2) { dataBits=6; parity='O'; stopBits=2; }
    else if (cfg == SERIAL_7N1) { dataBits=7; parity='N'; stopBits=1; }
    else if (cfg == SERIAL_7N2) { dataBits=7; parity='N'; stopBits=2; }
    else if (cfg == SERIAL_7E1) { dataBits=7; parity='E'; stopBits=1; }
    else if (cfg == SERIAL_7E2) { dataBits=7; parity='E'; stopBits=2; }
    else if (cfg == SERIAL_7O1) { dataBits=7; parity='O'; stopBits=1; }
    else if (cfg == SERIAL_7O2) { dataBits=7; parity='O'; stopBits=2; }
    else if (cfg == SERIAL_8N1) { dataBits=8; parity='N'; stopBits=1; }
    else if (cfg == SERIAL_8N2) { dataBits=8; parity='N'; stopBits=2; }
    else if (cfg == SERIAL_8E1) { dataBits=8; parity='E'; stopBits=1; }
    else if (cfg == SERIAL_8E2) { dataBits=8; parity='E'; stopBits=2; }
    else if (cfg == SERIAL_8O1) { dataBits=8; parity='O'; stopBits=1; }
    else if (cfg == SERIAL_8O2) { dataBits=8; parity='O'; stopBits=2; }
}

// ── Initialise Serial2 from preferences ───────────────────────────────────────
void serialPortInit() {
    if (serialPortInitialized) {
        Serial.println("[SerialPort] Already initialized, skipping");
        return;
    }

    Preferences pref;
    pref.begin(SERIALPORT_PREF_NS, true);
    bool enabled = pref.getBool("enabled", false);
    if (!enabled) {
        pref.end();
        Serial.println("[SerialPort] Disabled in preferences.");
        return;
    }

    uint32_t baud = pref.getULong("baudrate", 9600);
    uint32_t cfg  = pref.getULong("config",   SERIAL_8N1);
    pref.end();

    Serial2.begin(baud, cfg, RX2_PIN, TX2_PIN);
    serialPortInitialized = true;

    uint8_t bits; char par; uint8_t stop;
    serialport_decodeConfig(cfg, bits, par, stop);
    Serial.printf("[SerialPort] Initialized  RX=%d  TX=%d  baud=%lu  %d%c%d\n",
                  RX2_PIN, TX2_PIN, baud, bits, par, stop);
}

// ── Runtime status ─────────────────────────────────────────────────────────────
bool serialPortIsRunning() {
    return serialPortInitialized;
}

#endif // SERIALPORT_H
