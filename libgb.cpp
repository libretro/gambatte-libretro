#include "corelib.h"
#include "libgambatte/include/gambatte.h"

gambatte::GB *gameboy_ = nullptr;

extern "C" 
__attribute__((visibility("default")))
void set_key(size_t key, char val) {
}

extern "C" 
__attribute__((visibility("default")))
void init(const uint8_t* data, size_t len) {
    if (gameboy_ != nullptr) {
        delete gameboy_;
        gameboy_ = nullptr;
    }
    gameboy_ = new gambatte::GB();
}

// Note: Framebuffer creates a BGR565 formatted buffer.
extern "C" 
__attribute__((visibility("default")))
const uint8_t *framebuffer() {
    return 0;
}

extern "C"
__attribute__((visibility("default")))
void frame() {
}

extern "C"
__attribute__((visibility("default")))
void dump_state(const char* save_path) {
}

extern "C"
__attribute__((visibility("default")))
void load_state(const char* save_path) {
}

extern "C"
void apu_tick_60hz() {
}

extern "C"
__attribute__((visibility("default")))
void apu_sample_60hz(int16_t *output) {
}

extern "C"
__attribute__((visibility("default")))
long apu_sample_variable(int16_t *output, int32_t frames) {
    return 0;
}
