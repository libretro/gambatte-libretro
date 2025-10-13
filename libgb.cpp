#include "corelib.h"
#include "libgambatte/include/gambatte.h"
#include "libgambatte/libretro/blipper.h"
#include "libgambatte/include/inputgetter.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <mutex>

#define DBG true

#if DBG
#include <stdio.h>
#include <assert.h>
#else
void printf(const char* msg, ...) {}
#endif

unsigned input_value = 0;
class CInputGetter : public gambatte::InputGetter {
    public:
        virtual ~CInputGetter() = default;
        virtual unsigned operator()() {
            return input_value;
        }
};


bool bootloader_getter(void *, bool is_gbc, uint8_t* data, unsigned size) {
    return false;
}

std::mutex abuff_mutex;
CInputGetter *s_input_getter = nullptr;
gambatte::GB *gameboy_ = nullptr;
const int DISPLAY_WIDTH = 160;
const int DISPLAY_HEIGHT = 144;
const int FB_PITCH_PX = DISPLAY_WIDTH;
uint32_t fbuffer[FB_PITCH_PX * 144];
const size_t SOUND_SAMPLES_PER_FRAME = 35112;
const size_t SOUND_SAMPLES_PER_RUN = 2064;
const size_t SOUND_BUFF_SIZE = (SOUND_SAMPLES_PER_RUN + 2064);

// Audio buffering and downsampling.
// Incoming audio is at a rate of 35112 per 59.727hz frame,
// or 2.097 mHz
// To get to approximately 44100 hz, we downsample by
// 48x (47.554 would be perfect).
// With incoming buffers of 2064, each chunk from the
// core should result in 2064/48 = 43 samples.
gambatte::uint_least32_t sbuffer[SOUND_BUFF_SIZE];
const size_t DOWNSAMPLE_MULTIPLE = 64;
const size_t BLIP_BUFFER_SIZE = SOUND_SAMPLES_PER_FRAME*2;
blipper_t *resampler_left = nullptr;
blipper_t *resampler_right = nullptr;

void log(const char* msg) {
#ifdef DBG
    printf("%s\n", msg);
#endif
}

extern "C" 
__attribute__((visibility("default")))
void set_key(size_t key, char val) {
    // gambatte keys are slightly out of order.
    // corelib uses a=0, ab sel start updownleftright
    // gambatte uses a=1, ab sel start right left up down
    // corelib has 0 1 2 3 corresponding with shifts
    // gambatte uses masks directly.
    switch (key) {
        case Keys::BTN_A:     key = gambatte::InputGetter::A; break;
        case Keys::BTN_B:     key = gambatte::InputGetter::B; break;
        case Keys::BTN_Sel:   key = gambatte::InputGetter::SELECT; break;
        case Keys::BTN_Start: key = gambatte::InputGetter::START; break;
        case Keys::BTN_Up:    key = gambatte::InputGetter::UP; break;
        case Keys::BTN_Down:  key = gambatte::InputGetter::DOWN; break;
        case Keys::BTN_Left:  key = gambatte::InputGetter::LEFT; break;
        case Keys::BTN_Right: key = gambatte::InputGetter::RIGHT; break;
    }

    if (val) 
        input_value |= key;
    else
        input_value &= ~key;
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
        delete s_input_getter;
        s_input_getter = nullptr;
        blipper_free(resampler_left);
        blipper_free(resampler_right);
    }
    gameboy_ = new gambatte::GB();
    s_input_getter = new CInputGetter();
    resampler_left = blipper_new(32, 0.85, 6.5, DOWNSAMPLE_MULTIPLE, BLIP_BUFFER_SIZE, 0);
    gameboy_->setBootloaderGetter(bootloader_getter);
    gameboy_->setInputGetter(s_input_getter);
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

// Note: Framebuffer creates a u32 RGBA buffer.
extern "C" 
__attribute__((visibility("default")))
const uint8_t *framebuffer() {
    return (const uint8_t*)fbuffer;
}

void writeall(int fd, uint8_t* buff, size_t count) {
    while (count > 0) {
        ssize_t written = write(fd, buff, count);
        if (written <= 0) {
            perror("write fail.");
            exit(1);
        }
        count -= written;
        buff += written;
    }
}

size_t min(size_t a, size_t b) {
    return a < b ? a : b;
}

int workers = 0; // debug: check for overlapping sample/queue
extern "C"
__attribute__((visibility("default")))
long apu_sample_variable(int16_t* output, int32_t samples) {
    std::lock_guard guard(abuff_mutex);
    workers++;
    assert(workers==1);
    const int requested = samples;
    unsigned avail = blipper_read_avail(resampler_left);
    if (avail < samples) {
        samples = avail;
    }
    if (samples == 0) {
        workers--;
        return 0;
    }

    constexpr int stride = 1; // TODO: fixup for l + r
    blipper_read(resampler_left, output, samples, 1);
    for (int i = 0; i < requested - samples; i++) {
        output[i*stride] = 0;
    }
    workers--;
    return samples;
}

void queue_samples(size_t num_samples) {
    std::lock_guard guard(abuff_mutex);
    workers++;
    assert(workers == 1);
    // Milestones:
    // x dump raw audio from core (2mhz), confirm
    // x resampler for left (mono) audio (64x)
    // - mono audio distortions
    // - resampler for left (mono) audio (48x)
    // - resampler for right (mono)
    // - merged audio
    // - working in android
    const int16_t* samples = (const int16_t*)&sbuffer;
    blipper_push_samples(resampler_left, samples + 0, num_samples, 2);
    // blipper_push_samples(resampler_right, samples + 1, num_samples, 2);
    workers--;
}

extern "C"
__attribute__((visibility("default")))
void frame() {
    if (gameboy_ == nullptr) return;
    unsigned samples = SOUND_SAMPLES_PER_RUN;
    while (-1 == gameboy_->runFor((gambatte::video_pixel_t*)fbuffer, FB_PITCH_PX,
                /*soundbuf*/ sbuffer, /*soundBufSize*/SOUND_BUFF_SIZE, samples)) {
        // printf("got samples: %u\n", samples);
        queue_samples(samples);
        samples = SOUND_SAMPLES_PER_RUN;
    }
}


void checksum(uint8_t* data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
       sum += data[i];
    }
    printf("checksum: %d\n", sum);
} 

extern "C"
__attribute__((visibility("default")))
void save(int fd) {
    if (gameboy_ == nullptr)
        return;
    size_t state_size = gameboy_->stateSize();
    printf("save state is %zu bytes\n", state_size);
    uint8_t *buffer = (uint8_t*)malloc(state_size);
    uint8_t* const orig_buffer = buffer;
    gameboy_->saveState(buffer);
    // checksum(buffer, state_size);
    while (state_size > 0) {
        ssize_t written = write(fd, buffer, state_size);
        if (written <= 0) {
            perror("Save failed: ");
            return;
        }
        state_size -= written;
        buffer += written;
    }
    free(orig_buffer);
    close(fd);
}


extern "C"
__attribute__((visibility("default")))
void dump_state(const char* filename) {
    int fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY , 0700);
    if (fd == -1) {
        perror("failed to open:");
        return;
    }
    printf("saving to %s\n", filename);
    save(fd);
}

extern "C"
__attribute__((visibility("default")))
void load(int fd) {
    ssize_t bytes = lseek(fd, 0, SEEK_END);
    if (bytes <= 0) {
        perror("Failed to seek while loading: ");
        return;
    }
    printf("Loading %zu bytes\n", bytes);
    size_t state_size = gameboy_->stateSize();
    if (bytes != state_size) {
        puts("Invalid state size");
        return;
    }
    lseek(fd, 0, SEEK_SET);
    uint8_t *buffer = (uint8_t*)malloc(bytes);
    uint8_t *write = buffer;
    while (bytes > 0) {
        ssize_t read_bytes = read(fd, write, bytes);
        if (read_bytes <= 0) {
            perror("Read failure during load: ");
            return;
        }
        printf("read returned %zu bytes\n", read_bytes);
        write += read_bytes;
        bytes -= read_bytes;
    }
    // checksum(buffer, state_size);
    gameboy_->loadState(buffer);
    free(buffer);
    close(fd);
}

extern "C"
__attribute__((visibility("default")))
void load_state(const char* filename) {
    int fd = open(filename,  O_RDONLY , 0700);
    if (fd == -1) {
        perror("Failed to open: ");
        return;
    }
    load(fd);
}

extern "C"
__attribute__((visibility("default")))
void apu_tick_60hz() { /* unused */ }

extern "C"
__attribute__((visibility("default")))
void apu_sample_60hz(int16_t *output) { /* unused */ }

