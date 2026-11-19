#pragma once
#include <Adafruit_NeoPixel.h>

enum Effect : uint8_t {
  FX_SOLID = 0,
  FX_RAINBOW,
  FX_CHASE,
  FX_BREATHE,
  FX_COMET,
  FX_SPARKLE,
  FX_FIRE,
  FX_COUNT
};

extern const char* const EFFECT_NAMES[FX_COUNT];

struct RingState {
  bool on = true;
  uint8_t brightness = 80;     // 0-255 (wird auf MAX_BRIGHTNESS begrenzt)
  uint32_t color = 0x0080FF;   // 0xRRGGBB
  uint8_t effect = FX_RAINBOW;
  uint8_t speed = 50;          // 1-100
  bool reverse = false;
};

class Ring {
 public:
  Ring(uint8_t pin, uint16_t count, neoPixelType type);
  void begin();
  // Nicht blockierend: in jedem loop()-Durchlauf aufrufen.
  void update(uint32_t nowMs);
  // Erzwingt beim nächsten update() ein neues Bild (z. B. nach Änderung).
  void invalidate() { _dirty = true; }

  uint16_t count() const { return _count; }
  uint8_t pin() const { return _pin; }

  RingState state;

 private:
  void render();
  uint16_t idx(uint16_t i) const { return state.reverse ? _count - 1 - i : i; }
  uint32_t scaled(uint32_t color, uint8_t factor) const;

  Adafruit_NeoPixel _strip;
  uint8_t _pin;
  uint16_t _count;
  uint32_t _lastFrame = 0;
  uint32_t _step = 0;
  bool _dirty = true;
  bool _wasOn = true;
  uint8_t* _heat = nullptr;  // für den Feuer-Effekt
};
