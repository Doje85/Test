#include "Ring.h"
#include "config.h"

const char* const EFFECT_NAMES[FX_COUNT] = {
  "Farbe", "Regenbogen", "Lauflicht", "Atmen", "Komet", "Funkeln", "Feuer",
};

Ring::Ring(uint8_t pin, uint16_t count, neoPixelType type)
    : _strip(count, pin, type), _pin(pin), _count(count) {}

void Ring::begin() {
  _strip.begin();
  _strip.clear();
  _strip.show();
  _heat = new uint8_t[_count]();
}

uint32_t Ring::scaled(uint32_t c, uint8_t f) const {
  uint8_t r = ((c >> 16) & 0xFF) * f / 255;
  uint8_t g = ((c >> 8) & 0xFF) * f / 255;
  uint8_t b = (c & 0xFF) * f / 255;
  return Adafruit_NeoPixel::Color(r, g, b);
}

void Ring::update(uint32_t now) {
  if (!state.on) {
    if (_wasOn || _dirty) {
      _strip.clear();
      _strip.show();
      _wasOn = false;
      _dirty = false;
    }
    return;
  }
  _wasOn = true;

  // Geschwindigkeit 1..100 -> Bildabstand ~200ms..5ms
  uint32_t interval = map(constrain(state.speed, 1, 100), 1, 100, 200, 5);
  if (!_dirty && now - _lastFrame < interval) return;
  if (state.effect == FX_SOLID && !_dirty) return;  // statisch, nichts zu tun

  _lastFrame = now;
  _dirty = false;
  render();
  _step++;
}

void Ring::render() {
  _strip.setBrightness(min<uint8_t>(state.brightness, MAX_BRIGHTNESS));
  const uint32_t col = state.color;

  switch (state.effect) {
    case FX_SOLID:
      for (uint16_t i = 0; i < _count; i++) _strip.setPixelColor(i, col);
      break;

    case FX_RAINBOW: {
      uint16_t base = (_step * 256) & 0xFFFF;
      for (uint16_t i = 0; i < _count; i++) {
        uint16_t hue = base + (uint32_t)i * 65536 / _count;
        _strip.setPixelColor(idx(i), _strip.gamma32(_strip.ColorHSV(hue)));
      }
      break;
    }

    case FX_CHASE: {
      // Drei Punkte, gleichmäßig verteilt, laufen im Kreis
      uint16_t pos = _step % _count;
      _strip.clear();
      for (uint8_t k = 0; k < 3; k++) {
        uint16_t p = (pos + k * _count / 3) % _count;
        _strip.setPixelColor(idx(p), col);
      }
      break;
    }

    case FX_BREATHE: {
      // Sinusförmiges Auf- und Abblenden
      uint8_t phase = _step & 0xFF;
      uint8_t level = (uint8_t)((sin(phase * TWO_PI / 256.0) + 1.0) * 127.5);
      level = Adafruit_NeoPixel::gamma8(level);
      for (uint16_t i = 0; i < _count; i++) _strip.setPixelColor(i, scaled(col, level));
      break;
    }

    case FX_COMET: {
      uint16_t head = _step % _count;
      uint16_t tail = max<uint16_t>(3, _count / 2);
      _strip.clear();
      for (uint16_t t = 0; t < tail; t++) {
        uint16_t p = (head + _count - t) % _count;
        uint8_t level = 255 - (uint32_t)t * 255 / tail;
        _strip.setPixelColor(idx(p), scaled(col, Adafruit_NeoPixel::gamma8(level)));
      }
      break;
    }

    case FX_SPARKLE: {
      // Vorhandene Pixel ausblenden, zufällig neue aufblitzen lassen
      for (uint16_t i = 0; i < _count; i++) {
        uint32_t c = _strip.getPixelColor(i);
        _strip.setPixelColor(i, scaled(c, 200));
      }
      if (random(3) == 0) _strip.setPixelColor(random(_count), col);
      break;
    }

    case FX_FIRE: {
      // Einfache Feuersimulation: zufällige Hitze, die abkühlt und geglättet wird
      for (uint16_t i = 0; i < _count; i++) {
        uint8_t cool = random(0, 40);
        _heat[i] = _heat[i] > cool ? _heat[i] - cool : 0;
        if (random(100) < 35) _heat[i] = min<long>(255, _heat[i] + random(120, 255));
      }
      for (uint16_t i = 0; i < _count; i++) {
        uint8_t h = (_heat[i] + _heat[(i + 1) % _count] + _heat[(i + _count - 1) % _count]) / 3;
        uint8_t r = h > 85 ? 255 : h * 3;
        uint8_t g = h > 170 ? 255 : (h > 85 ? (h - 85) * 3 : 0);
        uint8_t b = h > 170 ? (h - 170) * 3 : 0;
        _strip.setPixelColor(i, _strip.gamma32(Adafruit_NeoPixel::Color(r, g / 2, b / 4)));
      }
      break;
    }
  }
  _strip.show();
}
