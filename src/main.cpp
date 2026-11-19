#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>

#include "config.h"
#include "Ring.h"
#include "web_ui.h"

static Ring* rings[RING_COUNT];
static WebServer server(80);
static Preferences prefs;

// ---------------------------------------------------------------------------
// Einstellungen im Flash speichern / laden
// ---------------------------------------------------------------------------
static void saveState() {
  prefs.begin("rings", false);
  for (uint8_t i = 0; i < RING_COUNT; i++) {
    char key[8];
    snprintf(key, sizeof(key), "r%u", i);
    prefs.putBytes(key, &rings[i]->state, sizeof(RingState));
  }
  prefs.end();
}

static void loadState() {
  prefs.begin("rings", true);
  for (uint8_t i = 0; i < RING_COUNT; i++) {
    char key[8];
    snprintf(key, sizeof(key), "r%u", i);
    RingState s;
    if (prefs.getBytesLength(key) == sizeof(RingState)) {
      prefs.getBytes(key, &s, sizeof(RingState));
      if (s.effect >= FX_COUNT) s.effect = FX_SOLID;
      rings[i]->state = s;
    }
  }
  prefs.end();
}

// ---------------------------------------------------------------------------
// JSON (von Hand, damit keine weitere Bibliothek nötig ist)
// ---------------------------------------------------------------------------
static String stateJson() {
  String j = "{\"effects\":[";
  for (uint8_t e = 0; e < FX_COUNT; e++) {
    if (e) j += ',';
    j += '"';
    j += EFFECT_NAMES[e];
    j += '"';
  }
  j += "],\"maxBrightness\":";
  j += MAX_BRIGHTNESS;
  j += ",\"rings\":[";
  for (uint8_t i = 0; i < RING_COUNT; i++) {
    const RingState& s = rings[i]->state;
    char buf[192];
    snprintf(buf, sizeof(buf),
             "%s{\"id\":%u,\"pin\":%u,\"leds\":%u,\"on\":%s,\"brightness\":%u,"
             "\"color\":\"#%06lX\",\"effect\":%u,\"speed\":%u,\"reverse\":%s}",
             i ? "," : "", i, rings[i]->pin(), rings[i]->count(), s.on ? "true" : "false",
             s.brightness, (unsigned long)(s.color & 0xFFFFFF), s.effect, s.speed,
             s.reverse ? "true" : "false");
    j += buf;
  }
  j += "]}";
  return j;
}

// Übernimmt alle übergebenen Parameter in einen Ring
static void applyArgs(Ring& r) {
  RingState& s = r.state;
  if (server.hasArg("on")) s.on = server.arg("on") == "1" || server.arg("on") == "true";
  if (server.hasArg("brightness")) s.brightness = constrain(server.arg("brightness").toInt(), 0, 255);
  if (server.hasArg("color")) {
    String c = server.arg("color");
    c.replace("#", "");
    s.color = strtoul(c.c_str(), nullptr, 16) & 0xFFFFFF;
  }
  if (server.hasArg("effect")) {
    int e = server.arg("effect").toInt();
    if (e >= 0 && e < FX_COUNT) s.effect = e;
  }
  if (server.hasArg("speed")) s.speed = constrain(server.arg("speed").toInt(), 1, 100);
  if (server.hasArg("reverse")) s.reverse = server.arg("reverse") == "1" || server.arg("reverse") == "true";
  r.invalidate();
}

// ---------------------------------------------------------------------------
// HTTP-Routen
//   GET  /                 Weboberfläche
//   GET  /api/state        aktueller Zustand aller Ringe (JSON)
//   POST /api/ring?id=N&…  einen Ring ändern (on, brightness, color, effect, speed, reverse)
//   POST /api/all?…        alle Ringe gleichzeitig ändern
//   POST /api/save         Zustand dauerhaft speichern
// ---------------------------------------------------------------------------
static void setupRoutes() {
  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", INDEX_HTML); });

  server.on("/api/state", HTTP_GET, [] { server.send(200, "application/json", stateJson()); });

  server.on("/api/ring", HTTP_POST, [] {
    int id = server.hasArg("id") ? server.arg("id").toInt() : -1;
    if (id < 0 || id >= RING_COUNT) {
      server.send(400, "application/json", "{\"error\":\"ungueltige id\"}");
      return;
    }
    applyArgs(*rings[id]);
    server.send(200, "application/json", stateJson());
  });

  server.on("/api/all", HTTP_POST, [] {
    for (uint8_t i = 0; i < RING_COUNT; i++) applyArgs(*rings[i]);
    server.send(200, "application/json", stateJson());
  });

  server.on("/api/save", HTTP_POST, [] {
    saveState();
    server.send(200, "application/json", "{\"saved\":true}");
  });

  server.onNotFound([] { server.send(404, "text/plain", "Nicht gefunden"); });
}

static void setupWifi() {
  WiFi.setHostname(HOSTNAME);
  if (strlen(WIFI_SSID) > 0) {
    Serial.printf("Verbinde mit WLAN \"%s\" ", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      delay(250);
      Serial.print('.');
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      WiFi.setAutoReconnect(true);
      Serial.printf("Verbunden, IP: %s\n", WiFi.localIP().toString().c_str());
      return;
    }
    Serial.println("WLAN-Verbindung fehlgeschlagen, starte Access Point.");
  }
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("Access Point \"%s\" (Passwort: %s), IP: %s\n", AP_SSID, AP_PASSWORD,
                WiFi.softAPIP().toString().c_str());
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\nNeoPixel-Ringe: %u Ringe\n", RING_COUNT);

  for (uint8_t i = 0; i < RING_COUNT; i++) {
    rings[i] = new Ring(RING_CONFIG[i].pin, RING_CONFIG[i].ledCount, LED_TYPE);
    rings[i]->begin();
    Serial.printf("  Ring %u: GPIO %u, %u LEDs\n", i, RING_CONFIG[i].pin, RING_CONFIG[i].ledCount);
  }
  loadState();

  setupWifi();
  if (MDNS.begin(HOSTNAME)) MDNS.addService("http", "tcp", 80);
  setupRoutes();
  server.begin();
  Serial.printf("Weboberflaeche: http://%s.local/\n", HOSTNAME);
}

void loop() {
  server.handleClient();
  uint32_t now = millis();
  for (uint8_t i = 0; i < RING_COUNT; i++) rings[i]->update(now);
  delay(1);
}
