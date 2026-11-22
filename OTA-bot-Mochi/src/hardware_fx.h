#pragma once
#include <Arduino.h>
#include "../pin_config.h"

#ifndef MOCHI_LED_PIN
#define MOCHI_LED_PIN -1
#endif
#ifndef MOCHI_LED_COUNT
#define MOCHI_LED_COUNT 6
#endif
#ifndef MOCHI_MOTOR_PIN
#define MOCHI_MOTOR_PIN -1
#endif

#if MOCHI_LED_PIN >= 0
#include <Adafruit_NeoPixel.h>
#endif

enum class FxEvent : uint8_t {
    None = 0,
    Startup,
    Tap,
    DoubleTap,
    Pet,
    Notification,
    CallRing
};

namespace hardware_fx {

#if MOCHI_LED_PIN >= 0
static Adafruit_NeoPixel strip(MOCHI_LED_COUNT, MOCHI_LED_PIN, NEO_GRB + NEO_KHZ800);
#endif

static uint32_t motor_off_ms = 0;
static uint32_t pulse2_start_ms = 0;
static uint16_t pulse2_dur_ms = 0;
static uint32_t fx_until_ms = 0;
static uint8_t fx_r = 0, fx_g = 170, fx_b = 255;
static uint32_t last_led_ms = 0;

inline void haptic_now(uint32_t now, uint16_t ms1, uint16_t gap_ms = 0, uint16_t ms2 = 0) {
#if MOCHI_MOTOR_PIN >= 0
    digitalWrite(MOCHI_MOTOR_PIN, HIGH);
    motor_off_ms = now + ms1;
    if (ms2 > 0 && gap_ms > 0) {
        pulse2_start_ms = now + ms1 + gap_ms;
        pulse2_dur_ms = ms2;
    } else {
        pulse2_start_ms = 0;
    }
#else
    (void)now; (void)ms1; (void)gap_ms; (void)ms2;
#endif
}

inline void init() {
#if MOCHI_MOTOR_PIN >= 0
    pinMode(MOCHI_MOTOR_PIN, OUTPUT);
    digitalWrite(MOCHI_MOTOR_PIN, LOW);
#endif
#if MOCHI_LED_PIN >= 0
    strip.begin();
    strip.setBrightness(95); // ~37% brightness: ideal for 0.4mm Light Diffusion Black PETG
    strip.clear();
    strip.show();
#endif
}

inline void trigger(FxEvent ev, uint32_t now) {
    switch (ev) {
        case FxEvent::Startup:
            haptic_now(now, 55, 75, 55);
            fx_r = 0; fx_g = 200; fx_b = 255;
            fx_until_ms = now + 1800;
            break;
        case FxEvent::Tap:
            haptic_now(now, 28);
            fx_r = 0; fx_g = 180; fx_b = 255;
            fx_until_ms = now + 450;
            break;
        case FxEvent::DoubleTap:
            haptic_now(now, 30, 55, 30);
            fx_r = 120; fx_g = 80; fx_b = 255;
            fx_until_ms = now + 650;
            break;
        case FxEvent::Pet:
            haptic_now(now, 22);
            fx_r = 255; fx_g = 110; fx_b = 180;
            fx_until_ms = now + 600;
            break;
        case FxEvent::Notification:
            haptic_now(now, 70, 80, 90);
            fx_r = 255; fx_g = 140; fx_b = 0;
            fx_until_ms = now + 2200;
            break;
        case FxEvent::CallRing:
            haptic_now(now, 120, 100, 120);
            fx_r = 0; fx_g = 255; fx_b = 130;
            fx_until_ms = now + 1200;
            break;
        default:
            break;
    }
}

inline void poll(uint32_t now, bool calling) {
#if MOCHI_MOTOR_PIN >= 0
    if (motor_off_ms && (int32_t)(now - motor_off_ms) >= 0) {
        digitalWrite(MOCHI_MOTOR_PIN, LOW);
        motor_off_ms = 0;
    }
    if (pulse2_start_ms && (int32_t)(now - pulse2_start_ms) >= 0) {
        digitalWrite(MOCHI_MOTOR_PIN, HIGH);
        motor_off_ms = now + pulse2_dur_ms;
        pulse2_start_ms = 0;
    }
#endif

#if MOCHI_LED_PIN >= 0
    if ((uint32_t)(now - last_led_ms) < 25) return;
    last_led_ms = now;

    if (calling && (int32_t)(now - fx_until_ms) >= 0) {
        trigger(FxEvent::CallRing, now);
    }

    uint8_t r = 0, g = 45, b = 85; // Subtle ambient ice-blue idle glow
    if ((int32_t)(fx_until_ms - now) > 0) {
        r = fx_r; g = fx_g; b = fx_b;
    } else {
        // Gentle slow breathing on idle
        uint8_t phase = (now / 24) & 0xFF;
        uint8_t wave = (phase < 128) ? phase : (255 - phase);
        r = 0;
        g = 15 + (wave >> 2);
        b = 35 + (wave >> 1);
    }

    for (uint16_t i = 0; i < MOCHI_LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
#else
    (void)calling;
#endif
}

} // namespace hardware_fx
