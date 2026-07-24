#ifndef SHIFT_DETAILS_H
#define SHIFT_DETAILS_H

/*
 * shift_details.h — Shift configuration storage & helpers
 *
 * Stores 1-3 shift definitions in NVS.
 * Each shift has: name, start/end time, up to 3 break pairs.
 * Time format : "HH:MM:SS"  (compatible with shift_timing.h)
 *
 * NVS namespace : "shift_cfg"
 * Keys:
 *   enabled       (bool)   — feature enable
 *   num_shifts    (uint8)  — 1..3
 *   s<N>_name     (String) — shift N name  (N = 1..3)
 *   s<N>_start    (String) — shift start time
 *   s<N>_end      (String) — shift end time
 *   s<N>_b<M>s    (String) — break M start (M = 1..3)
 *   s<N>_b<M>e    (String) — break M end
 *
 * Usage:
 *   #include "shift_details.h"   (after iotboard.h)
 *   shiftDetailsInit();          // call once in setup / boardinit tail
 *   JsonObject sh = shiftGetObject(1);   // zero-based index 0..num_shifts-1
 *   // then pass to shift_timing.h functions:
 *   int secs = getShiftWorkingTime(sh);
 */

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// ── NVS namespace ─────────────────────────────────────────────────────────────
#define SHIFT_PREF_NS "shift_cfg"

// ── Globals ───────────────────────────────────────────────────────────────────
Preferences shiftPref;
bool        shiftEnabled   = false;
uint8_t     shiftNumShifts = 1;          // 1..3

// ── Per-shift data ────────────────────────────────────────────────────────────
struct ShiftBreak {
    char start[6];   // "HH:MM\0"
    char end[6];
};

struct ShiftEntry {
    char       name[24];
    char       start[6];
    char       end[6];
    ShiftBreak brk[3];
};

static ShiftEntry _shifts[3];            // indices 0..2 for shifts 1..3

// ── Default times ─────────────────────────────────────────────────────────────
static const char* _DEFAULT_SHIFT_STARTS[3] = { "06:00", "14:00", "22:00" };
static const char* _DEFAULT_SHIFT_ENDS[3]   = { "14:00", "22:00", "06:00" };
static const char* _DEFAULT_SHIFT_NAMES[3]  = { "Morning", "Afternoon", "Night" };

// ── Key builder helpers ───────────────────────────────────────────────────────
static void _shiftKey(char* buf, uint8_t n, const char* suffix) {
    // n = 1..3
    buf[0] = 's'; buf[1] = '0' + n; buf[2] = '_';
    uint8_t i = 3;
    for (const char* p = suffix; *p; p++, i++) buf[i] = *p;
    buf[i] = '\0';
}

static void _shiftBrkKey(char* buf, uint8_t n, uint8_t m, const char* se) {
    // n = 1..3, m = 1..3, se = "s" or "e"
    buf[0] = 's'; buf[1] = '0' + n; buf[2] = '_';
    buf[3] = 'b'; buf[4] = '0' + m;
    buf[5] = se[0]; buf[6] = '\0';
}

// ── Load from NVS ─────────────────────────────────────────────────────────────
void shiftDetailsLoad() {
    shiftPref.begin(SHIFT_PREF_NS, true);
    shiftEnabled   = shiftPref.getBool("enabled",    false);
    shiftNumShifts = shiftPref.getUChar("num_shifts", 1);
    if (shiftNumShifts < 1) shiftNumShifts = 1;
    if (shiftNumShifts > 3) shiftNumShifts = 3;

    char key[12];
    for (uint8_t n = 1; n <= 3; n++) {
        uint8_t i = n - 1;

        _shiftKey(key, n, "name");
        String nm = shiftPref.getString(key, _DEFAULT_SHIFT_NAMES[i]);
        strncpy(_shifts[i].name,  nm.c_str(), sizeof(_shifts[i].name) - 1);
        _shifts[i].name[sizeof(_shifts[i].name) - 1] = '\0';

        _shiftKey(key, n, "start");
        String st = shiftPref.getString(key, _DEFAULT_SHIFT_STARTS[i]);
        strncpy(_shifts[i].start, st.c_str(), 5); _shifts[i].start[5] = '\0';

        _shiftKey(key, n, "end");
        String en = shiftPref.getString(key, _DEFAULT_SHIFT_ENDS[i]);
        strncpy(_shifts[i].end,   en.c_str(), 5); _shifts[i].end[5] = '\0';

        for (uint8_t m = 1; m <= 3; m++) {
            uint8_t bi = m - 1;
            _shiftBrkKey(key, n, m, "s");
            String bs = shiftPref.getString(key, "00:00");
            strncpy(_shifts[i].brk[bi].start, bs.c_str(), 5);
            _shifts[i].brk[bi].start[5] = '\0';

            _shiftBrkKey(key, n, m, "e");
            String be = shiftPref.getString(key, "00:00");
            strncpy(_shifts[i].brk[bi].end,   be.c_str(), 5);
            _shifts[i].brk[bi].end[5] = '\0';
        }
    }
    shiftPref.end();
}

// ── Save to NVS ───────────────────────────────────────────────────────────────
void shiftDetailsSave() {
    shiftPref.begin(SHIFT_PREF_NS, false);
    shiftPref.putBool("enabled",    shiftEnabled);
    shiftPref.putUChar("num_shifts", shiftNumShifts);

    char key[12];
    for (uint8_t n = 1; n <= 3; n++) {
        uint8_t i = n - 1;

        _shiftKey(key, n, "name");
        shiftPref.putString(key, _shifts[i].name);

        _shiftKey(key, n, "start");
        shiftPref.putString(key, _shifts[i].start);

        _shiftKey(key, n, "end");
        shiftPref.putString(key, _shifts[i].end);

        for (uint8_t m = 1; m <= 3; m++) {
            uint8_t bi = m - 1;
            _shiftBrkKey(key, n, m, "s");
            shiftPref.putString(key, _shifts[i].brk[bi].start);
            _shiftBrkKey(key, n, m, "e");
            shiftPref.putString(key, _shifts[i].brk[bi].end);
        }
    }
    shiftPref.end();
}

// ── Init (call once from setup) ───────────────────────────────────────────────
void shiftDetailsInit() {
    shiftDetailsLoad();
    Serial.printf("[ShiftDetails] loaded  enabled=%d  num=%d\n",
                  shiftEnabled, shiftNumShifts);
}

// ── Build JsonObject for shift N (1-based) ────────────────────────────────────
// Returns a JsonObject inside the provided JsonDocument.
// The returned object is compatible with shift_timing.h functions.
// Call:  StaticJsonDocument<1024> doc;
//        JsonObject sh = shiftGetObject(doc, 1);
//        int secs = getShiftWorkingTime(sh);
JsonObject shiftGetObject(JsonDocument& doc, uint8_t n) {
    if (n < 1 || n > 3) n = 1;
    uint8_t i = n - 1;
    doc.clear();
    JsonObject obj = doc.to<JsonObject>();
    // Append ":00" seconds so shift_timing.h gets "HH:MM:SS" format
    char stBuf[9], enBuf[9];
    snprintf(stBuf, sizeof(stBuf), "%s:00", _shifts[i].start);
    snprintf(enBuf, sizeof(enBuf), "%s:00", _shifts[i].end);
    obj["shift_start_time"] = stBuf;
    obj["shift_end_time"]   = enBuf;
    for (uint8_t m = 1; m <= 3; m++) {
        uint8_t bi = m - 1;
        char ks[20], ke[20], bsBuf[9], beBuf[9];
        snprintf(ks,    sizeof(ks),    "break%d_start_time", m);
        snprintf(ke,    sizeof(ke),    "break%d_end_time",   m);
        snprintf(bsBuf, sizeof(bsBuf), "%s:00", _shifts[i].brk[bi].start);
        snprintf(beBuf, sizeof(beBuf), "%s:00", _shifts[i].brk[bi].end);
        obj[ks] = bsBuf;
        obj[ke] = beBuf;
    }
    return obj;
}

// ── Get shift name (1-based) ──────────────────────────────────────────────────
const char* shiftGetName(uint8_t n) {
    if (n < 1 || n > 3) return "";
    return _shifts[n - 1].name;
}

// ── Internal: parse "HH:MM" string to minutes since midnight ─────────────────
static uint16_t _shiftTimeToMin(const char* t) {
    if (!t || t[0] == '\0') return 0;
    uint16_t h = (uint8_t)(t[0] - '0') * 10 + (uint8_t)(t[1] - '0');
    uint16_t m = (uint8_t)(t[3] - '0') * 10 + (uint8_t)(t[4] - '0');
    return h * 60 + m;
}

// ── Get current shift number (1-based) from RTC system time ──────────────────
// Returns 0 if time is unavailable or current time is not within any shift.
uint8_t getCurrentShiftNumber() {
    if (!shiftEnabled || shiftNumShifts == 0) return 0;
    struct tm ti;
    if (!getLocalTime(&ti)) return 0;   // RTC / NTP not synced yet
    uint16_t nowMin = (uint16_t)ti.tm_hour * 60 + (uint16_t)ti.tm_min;
    for (uint8_t i = 0; i < shiftNumShifts; i++) {
        uint16_t sMin = _shiftTimeToMin(_shifts[i].start);
        uint16_t eMin = _shiftTimeToMin(_shifts[i].end);
        bool inShift;
        if (sMin <= eMin) {
            // Normal (same-day) shift: e.g. 06:00 – 14:00
            inShift = (nowMin >= sMin && nowMin < eMin);
        } else {
            // Overnight shift: e.g. 22:00 – 06:00
            inShift = (nowMin >= sMin || nowMin < eMin);
        }
        if (inShift) return i + 1;   // 1-based
    }
    return 0;   // not in any defined shift
}

// ── Get current shift name ────────────────────────────────────────────────────
// Returns "" if unavailable or outside all shifts.
const char* getCurrentShiftName() {
    uint8_t n = getCurrentShiftNumber();
    return shiftGetName(n);   // shiftGetName(0) already returns ""
}

// ── Serialise all shifts to JSON string ───────────────────────────────────────
String shiftDetailsToJson() {
    DynamicJsonDocument doc(1536);
    doc["success"]    = true;
    doc["enabled"]    = shiftEnabled;
    doc["num_shifts"] = shiftNumShifts;
    JsonArray arr = doc.createNestedArray("shifts");
    for (uint8_t i = 0; i < 3; i++) {
        JsonObject s = arr.createNestedObject();
        s["name"]  = _shifts[i].name;
        s["start"] = _shifts[i].start;
        s["end"]   = _shifts[i].end;
        JsonArray brks = s.createNestedArray("breaks");
        for (uint8_t m = 0; m < 3; m++) {
            JsonObject b = brks.createNestedObject();
            b["start"] = _shifts[i].brk[m].start;
            b["end"]   = _shifts[i].brk[m].end;
        }
    }
    String out;
    serializeJson(doc, out);
    return out;
}

// ── Apply JSON body to config ─────────────────────────────────────────────────
// Returns true if anything changed.
bool shiftDetailsFromJson(const String& body) {
    DynamicJsonDocument doc(1536);
    if (deserializeJson(doc, body) != DeserializationError::Ok) return false;
    bool changed = false;

    if (doc.containsKey("enabled"))    { shiftEnabled    = doc["enabled"].as<bool>();   changed = true; }
    if (doc.containsKey("num_shifts")) {
        uint8_t v = doc["num_shifts"].as<uint8_t>();
        if (v >= 1 && v <= 3) { shiftNumShifts = v; changed = true; }
    }

    JsonArray arr = doc["shifts"].as<JsonArray>();
    for (uint8_t i = 0; i < 3 && i < arr.size(); i++) {
        JsonObject s = arr[i].as<JsonObject>();
        if (s.isNull()) continue;

        if (s.containsKey("name")) {
            String nm = s["name"].as<String>();
            strncpy(_shifts[i].name, nm.c_str(), sizeof(_shifts[i].name) - 1);
            _shifts[i].name[sizeof(_shifts[i].name) - 1] = '\0';
            changed = true;
        }
        if (s.containsKey("start")) {
            String st = s["start"].as<String>();
            strncpy(_shifts[i].start, st.c_str(), 5); _shifts[i].start[5] = '\0';
            changed = true;
        }
        if (s.containsKey("end")) {
            String en = s["end"].as<String>();
            strncpy(_shifts[i].end, en.c_str(), 5); _shifts[i].end[5] = '\0';
            changed = true;
        }

        JsonArray brks = s["breaks"].as<JsonArray>();
        for (uint8_t m = 0; m < 3 && m < brks.size(); m++) {
            JsonObject b = brks[m].as<JsonObject>();
            if (b.isNull()) continue;
            if (b.containsKey("start")) {
                String bs = b["start"].as<String>();
                strncpy(_shifts[i].brk[m].start, bs.c_str(), 5);
                _shifts[i].brk[m].start[5] = '\0'; changed = true;
            }
            if (b.containsKey("end")) {
                String be = b["end"].as<String>();
                strncpy(_shifts[i].brk[m].end, be.c_str(), 5);
                _shifts[i].brk[m].end[5] = '\0'; changed = true;
            }
        }
    }

    if (changed) shiftDetailsSave();
    return changed;
}


#endif // SHIFT_DETAILS_H
