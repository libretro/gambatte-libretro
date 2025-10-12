#define DEBUG 1
#include "corelib.h"
#include <SDL2/SDL.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

void sdl_update_keys(const uint8_t *sdlkeys) {
    set_key(BTN_A, sdlkeys[SDL_SCANCODE_X]);      // A
    set_key(BTN_B, sdlkeys[SDL_SCANCODE_Z]);      // B
    set_key(BTN_Sel, sdlkeys[SDL_SCANCODE_TAB]);    // Select
    set_key(BTN_Start, sdlkeys[SDL_SCANCODE_RETURN]); // Start
    set_key(BTN_Up, sdlkeys[SDL_SCANCODE_UP]);     // Dpad Up
    set_key(BTN_Down, sdlkeys[SDL_SCANCODE_DOWN]);   // Dpad Down
    set_key(BTN_Left, sdlkeys[SDL_SCANCODE_LEFT]);   // Dpad Left
    set_key(BTN_Right, sdlkeys[SDL_SCANCODE_RIGHT]);  // Dpad Right
}

char run = 1;
const uint8_t *sdlkeys;
void input() {
    for (SDL_Event event; SDL_PollEvent(&event);) {
        if (event.type == SDL_QUIT) {
            run = 0;
        }
    }
  sdl_update_keys(sdlkeys);
}

size_t slurp(const char*filename, uint8_t* out, size_t capacity) {
    size_t pos = 0;
    int fd = open(filename, 0);
    if (fd == -1) {
        perror("open fail: ");
        exit(1);
    }
    while (1) {
        ssize_t count = read(fd, out + pos, capacity - pos);
        if (count == -1) {
            perror("read fail: ");
            exit(1);
        }
        if (count == 0) break;
        pos += count;
    }
    return pos;
}

typedef struct {
    SDL_AudioDeviceID device;
} SoundCard;

void audio_callback(void *userdata, uint8_t* data, int bytes) {
    // puts("Audio");
    // printf("attempt to dequeue %d bytes\n", bytes);
    // apu_sample_variable((int16_t*)data, bytes / sizeof(uint16_t));
    // FIXME
}

void soundcard_init(SoundCard *snd) {
    SDL_AudioSpec desired;
    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_U16LSB;   // intel and aarch64 are both LE
    desired.channels = 1;
    desired.samples = 0;
    desired.callback = audio_callback;
    SDL_AudioSpec actual;
    int res = SDL_OpenAudio(&desired, &actual);
    assert(res == 0);
    assert(desired.freq == actual.freq);
    // assert(desired.format == actual.format);  // some bits may change
    assert(desired.channels == actual.channels);
    // assert(desired.samples == actual.samples);

    snd->device = 1;
    SDL_PauseAudioDevice(snd->device, 0);
}


void soundcard_queue(SoundCard *snd, uint8_t* data, size_t bytes) {
    if(SDL_QueueAudio(snd->device, (void*)data, bytes) != 0) {
        printf("audio queue fail: '%s'\n", SDL_GetError());
    }
    SDL_PauseAudioDevice(snd->device, 0);
}

const int BYTES_PER_PIXEL=4;
const int VIDEO_WIDTH = 160;
const int VIDEO_HEIGHT = 144;
const int SCALE = 2;
int main(int argc, char **argv) {
  uint8_t buffer[1024 * 1024];
  size_t bytes_read = slurp(argv[1], buffer, sizeof(buffer));
  printf("read bytes: %lu\n", bytes_read);
  init(buffer, bytes_read);

  int audio_raw_lr = open("audio_raw_lr.bin", 
          O_CREAT | O_TRUNC | O_WRONLY , 0700);
  if (audio_raw_lr == -1) {
      perror("raw open failed.");
      return 1;
  }

  sdlkeys = (uint8_t*)SDL_GetKeyboardState(0);


  char save_file[1024];
  snprintf(save_file, 1023, "%s.sav", argv[1]);
  if (argc == 3) {
      printf("skipping load.\n");
  } else {
      printf("Loading %s\n", save_file);
      load_state(save_file);
  }

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Renderer *renderer = SDL_CreateRenderer(
      SDL_CreateWindow("picoboy", 0, 0, VIDEO_WIDTH*SCALE, VIDEO_HEIGHT*SCALE, SDL_WINDOW_SHOWN), -1,
      SDL_RENDERER_ACCELERATED);
  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
  // SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
                                    SDL_TEXTUREACCESS_STREAMING, VIDEO_WIDTH, VIDEO_HEIGHT);
  SoundCard soundcard;
  soundcard_init(&soundcard);
  // soundcard_test();

  clock_t last = clock();
  clock_t start = clock();
  assert(CLOCKS_PER_SEC == 1'000'000);
  while(run) {
      input();
      frame();
      apu_sample_variable((int16_t*)&audio_raw_lr, 0); // DEBUG FIXME
      SDL_UpdateTexture(texture, 0, framebuffer(), VIDEO_WIDTH*BYTES_PER_PIXEL);
      SDL_RenderCopy(renderer, texture, 0, 0);
      SDL_RenderPresent(renderer);
      // soundcard_queue(&soundcard, audio_buffer, audio_bytes);
      // SDL_UpdateTexture(texture, 0, finished_frame + 2048, 512);
      // SDL_RenderCopy(renderer, texture, 0, 0);
      // SDL_RenderPresent(renderer);
      clock_t now = clock();
      // printf("now: %ld\n", now);
      ssize_t delay = (last + 166'666) - now;
      if (delay > 0) {
          usleep(delay);
      } else {
          puts("lagging");
      }
      last = now;
  }
  printf("save file: %s\n", save_file);
  dump_state(save_file);
  close(audio_raw_lr);
}
