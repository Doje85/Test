# ESP32-Steuerung für mehrere NeoPixel-Ringe

Steuert beliebig viele (bis 8) WS2812B/NeoPixel-Ringe, jeden an einem eigenen GPIO,
über eine Weboberfläche im Browser (Handy oder PC).

**Funktionen**
- Pro Ring: An/Aus, Farbe, Effekt, Helligkeit, Tempo, Laufrichtung
- „Alle Ringe“: eine Einstellung für alle gleichzeitig
- Effekte: Farbe, Regenbogen, Lauflicht, Atmen, Komet, Funkeln, Feuer
- Einstellungen werden mit „Speichern“ im Flash abgelegt und beim Start geladen
- WLAN-Client oder eigener Access Point als Fallback
- Nicht blockierend: alle Ringe laufen unabhängig mit eigenem Tempo

## Verkabelung

```
 5V-Netzteil (+) ──┬───────────┬───────────┬──────────── ESP32 VIN (oder USB)
                   │           │           │
                 Ring 1      Ring 2      Ring 3       (je 5V / GND / DIN)
                   │ DIN       │ DIN       │ DIN
 GPIO 25 ──[330Ω]──┘           │           │
 GPIO 26 ──[330Ω]──────────────┘           │
 GPIO 27 ──[330Ω]──────────────────────────┘

 Netzteil (−) ──── GND aller Ringe ──── ESP32 GND   (gemeinsame Masse!)
 1000 µF Elko zwischen 5V und GND direkt am Netzteil
```

Hinweise:
- **Gemeinsame Masse** zwischen ESP32, Netzteil und Ringen ist Pflicht.
- Der ESP32 liefert 3,3 V-Pegel. Meist klappt das; bei Flackern einen Pegelwandler
  (z. B. 74AHCT125) zwischen GPIO und DIN setzen.
- Stromverbrauch: bis ~60 mA pro LED bei vollem Weiß. 52 LEDs ≈ 3 A max. –
  `MAX_BRIGHTNESS` in `include/config.h` begrenzt das (Standard 160/255).
- Ringe nicht über den 5V-Pin des ESP32 versorgen, wenn sie viele LEDs haben.

## Konfiguration

In `include/config.h`:

```cpp
static const RingConfig RING_CONFIG[] = {
  {25, 12},   // GPIO 25, 12 LEDs
  {26, 16},   // GPIO 26, 16 LEDs
  {27, 24},   // GPIO 27, 24 LEDs
};
```

Einfach Zeilen hinzufügen/entfernen. RGBW-Ringe (SK6812): `LED_TYPE` auf
`NEO_GRBW + NEO_KHZ800` ändern.

**WLAN:** `include/secrets.example.h` nach `include/secrets.h` kopieren und SSID/Passwort
eintragen. Ohne (funktionierendes) WLAN öffnet der ESP32 den Access Point
**NeoPixel-Ringe** (Passwort `neopixel123`), Oberfläche dann unter `http://192.168.4.1`.

## Flashen

Mit [PlatformIO](https://platformio.org/) (VS Code-Erweiterung oder CLI):

```bash
pio run -t upload
pio device monitor
```

Im seriellen Monitor erscheint die IP-Adresse. Alternativ: `http://neopixel-ringe.local`.

## HTTP-API

Für Automatisierung (Home Assistant, Skripte, …):

| Methode | Pfad | Parameter |
|---|---|---|
| GET | `/api/state` | – (liefert JSON aller Ringe) |
| POST | `/api/ring` | `id` + beliebig: `on`, `color` (`ff8800`), `brightness` (0–255), `effect` (0–6), `speed` (1–100), `reverse` |
| POST | `/api/all` | wie oben, ohne `id` |
| POST | `/api/save` | – (aktuellen Zustand speichern) |

Beispiel: `curl -X POST "http://neopixel-ringe.local/api/ring?id=1&effect=6&brightness=120"`

## Projektstruktur

```
include/config.h      Pins, LED-Anzahl, WLAN, Helligkeitsgrenze
src/main.cpp          WLAN, Webserver, API, Speichern/Laden
src/Ring.h/.cpp       Ring-Klasse mit allen Effekten
src/web_ui.h          Weboberfläche (HTML/JS, im Flash abgelegt)
```
