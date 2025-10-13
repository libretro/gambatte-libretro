#pragma once

#include <stdint.h>
#include <stddef.h>

enum Keys {
    BTN_A = 0,
    BTN_B,
    BTN_Sel,
    BTN_Start,
    BTN_Up,
    BTN_Down,
    BTN_Left,
    BTN_Right,
    NUM_KEYS
};

extern "C" void set_key(size_t key, char val);
extern "C" void init(const uint8_t* data, size_t len);

// Note: Framebuffer creates a BGR565 formatted buffer.
extern "C" const uint8_t *framebuffer();

extern "C" void frame();
extern "C" void dump_state(const char* save_path);
extern "C" void load_state(const char* save_path);

// APU
// const int SAMPLE_RATE = 35112 * 59.727 / 64;
const int SAMPLE_RATE = 44100; // Original
const int SAMPLES_PER_FRAME = SAMPLE_RATE / 60;
extern "C" void apu_tick_60hz();
extern "C" void apu_sample_60hz(int16_t *output);
extern "C" long apu_sample_variable(int16_t *output, int32_t frames);
