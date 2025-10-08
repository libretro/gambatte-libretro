#include "corelib.h"
#include "libgambatte/include/gambatte.h"

gambatte::GB *gameboy_ = nullptr;
uint32_t fbuffer[160 * 144];
const int FB_PITCH_PX = 160;
const size_t SOUND_SAMPLES_PER_RUN = 2064;
const size_t SOUND_BUFF_SIZE = (SOUND_SAMPLES_PER_RUN + 2064);
gambatte::uint_least32_t sbuffer[SOUND_BUFF_SIZE];

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
    return (const uint8_t*)fbuffer;
}

extern "C"
__attribute__((visibility("default")))
void frame() {
    if (gameboy_ == nullptr) return;
    unsigned samples;
    while (-1 == gameboy_->runFor((long unsigned int*)fbuffer, FB_PITCH_PX,
                /*soundbuf*/ sbuffer, /*soundBufSize*/SOUND_BUFF_SIZE, samples)) {}
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
