#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Ringe: ein Eintrag pro Ring -> { Daten-Pin, Anzahl LEDs }
// Sichere Pins am ESP32-DevKit: 4, 5, 13, 14, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
// (16/17 meiden, falls ein WROVER-Modul mit PSRAM verwendet wird)
// Maximal 8 Ringe (RMT-Kanäle des ESP32).
// ---------------------------------------------------------------------------
struct RingConfig {
  uint8_t pin;
  uint16_t ledCount;
};

static const RingConfig RING_CONFIG[] = {
  {25, 12},
  {26, 16},
  {27, 24},
};
static const uint8_t RING_COUNT = sizeof(RING_CONFIG) / sizeof(RING_CONFIG[0]);

// Farbreihenfolge/Typ der LEDs (WS2812B = NEO_GRB, SK6812 RGBW = NEO_GRBW)
#define LED_TYPE (NEO_GRB + NEO_KHZ800)

// Obergrenze für die Helligkeit (0-255), schützt Netzteil und Kabel.
#define MAX_BRIGHTNESS 160

// ---------------------------------------------------------------------------
// WLAN: Zugangsdaten für dein Heimnetz in include/secrets.h eintragen
// (siehe secrets.example.h). Ohne Verbindung startet ein eigener Access Point.
// ---------------------------------------------------------------------------
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

#define AP_SSID "NeoPixel-Ringe"
#define AP_PASSWORD "neopixel123"  // mindestens 8 Zeichen
#define HOSTNAME "neopixel-ringe"  // erreichbar als http://neopixel-ringe.local
