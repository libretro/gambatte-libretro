#include "corelib.h"
#include "libgambatte/include/gambatte.h"
#include "libgambatte/include/inputgetter.h"
#include <stdlib.h>

#define DBG 1

#ifdef DBG
#include <stdio.h>
#endif

class CInputGetter : public gambatte::InputGetter {
    public:
        virtual ~CInputGetter() = default;
        virtual unsigned operator()() {
            return value;
        }
        unsigned value = 0;
};

static CInputGetter s_input_getter;

bool bootloader_getter(void *, bool is_gbc, uint8_t* data, unsigned size) {
    return false;
}

gambatte::GB *gameboy_ = nullptr;
const int DISPLAY_WIDTH = 160;
const int DISPLAY_HEIGHT = 144;
const int FB_PITCH_PX = DISPLAY_WIDTH;
uint16_t fbuffer[FB_PITCH_PX * 144];
const size_t SOUND_SAMPLES_PER_FRAME = 35112;
const size_t SOUND_SAMPLES_PER_RUN = SOUND_SAMPLES_PER_FRAME;
const size_t SOUND_BUFF_SIZE = (SOUND_SAMPLES_PER_RUN + 2064);
gambatte::uint_least32_t sbuffer[SOUND_BUFF_SIZE];

void log(const char* msg) {
#ifdef DBG
    puts(msg);
#endif
}

extern "C" 
__attribute__((visibility("default")))
void set_key(size_t key, char val) {
}

void strncpy(const char* src, char* dst, size_t bytes) {
    for(size_t i = 0; i < bytes; i++) {
        dst[i] = src[i];
    }
}

extern "C" 
__attribute__((visibility("default")))
void init(const uint8_t* data, size_t len) {
    if (gameboy_ != nullptr) {
        delete gameboy_;
        gameboy_ = nullptr;
    }
    gameboy_ = new gambatte::GB();
    gameboy_->setBootloaderGetter(bootloader_getter);
    gameboy_->setInputGetter(&s_input_getter);
    int flags = 0;
    char internal_game_name[17];
    strncpy((const char*)(data + 0x134), internal_game_name, sizeof(internal_game_name));
    internal_game_name[16] = 0;
    log(internal_game_name);
    if (gameboy_->load(data, len, flags) != 0) {
        log("gb load failed.");
        exit(1);
    }
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
    unsigned samples = SOUND_SAMPLES_PER_RUN;
    while (-1 == gameboy_->runFor((gambatte::video_pixel_t*)fbuffer, FB_PITCH_PX,
                /*soundbuf*/ sbuffer, /*soundBufSize*/SOUND_BUFF_SIZE, samples)) {
        printf("got samples: %u\n", samples);
        samples = SOUND_SAMPLES_PER_RUN;
    }
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
