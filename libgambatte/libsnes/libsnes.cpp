#include <libsnes.hpp>

#define HAVE_STDINT_H
#include <gambatte.h>

#include <assert.h>

static snes_audio_sample_t audio_cb;
static snes_video_refresh_t video_cb;
static snes_input_poll_t input_poll_cb;
static snes_input_state_t input_state_cb;
static gambatte::GB gb;

namespace input
{
   struct map { unsigned snes; unsigned gb; };
   static const map btn_map[] = {
      { SNES_DEVICE_ID_JOYPAD_A, gambatte::InputGetter::A },
      { SNES_DEVICE_ID_JOYPAD_B, gambatte::InputGetter::B },
      { SNES_DEVICE_ID_JOYPAD_SELECT, gambatte::InputGetter::SELECT },
      { SNES_DEVICE_ID_JOYPAD_START, gambatte::InputGetter::START },
      { SNES_DEVICE_ID_JOYPAD_RIGHT, gambatte::InputGetter::RIGHT },
      { SNES_DEVICE_ID_JOYPAD_LEFT, gambatte::InputGetter::LEFT },
      { SNES_DEVICE_ID_JOYPAD_UP, gambatte::InputGetter::UP },
      { SNES_DEVICE_ID_JOYPAD_DOWN, gambatte::InputGetter::DOWN },
   };
}

class SNESInput : public gambatte::InputGetter
{
   public:
      unsigned operator()()
      {
         unsigned res = 0;
         for (unsigned i = 0; i < sizeof(input::btn_map) / sizeof(input::map); i++)
         {
            res |= input_state_cb(SNES_PORT_1,
                  SNES_DEVICE_JOYPAD, 0, input::btn_map[i].snes) ? input::btn_map[i].gb : 0;
         }
         return res;
      }
} static gb_input;


void snes_init()
{
   assert(sizeof(gambatte::uint_least32_t) == sizeof(uint32_t));
   gb.setInputGetter(&gb_input);
}
void snes_term() {}

void snes_set_video_refresh(snes_video_refresh_t cb) { video_cb = cb; }
void snes_set_audio_sample(snes_audio_sample_t cb) { audio_cb = cb; }
void snes_set_input_poll(snes_input_poll_t cb) { input_poll_cb = cb; }
void snes_set_input_state(snes_input_state_t cb) { input_state_cb = cb; }

void snes_set_controller_port_device(bool, unsigned) {}
void snes_set_cartridge_basename(const char *) {}

void snes_power() { gb.reset(); }
void snes_reset() { gb.reset(); }

unsigned snes_serialize_size() { return 0; }
bool snes_serialize(uint8_t *, unsigned) { return false; }
bool snes_unserialize(const uint8_t *, unsigned) { return false; }

void snes_cheat_reset() {}
void snes_cheat_set(unsigned, bool, const char *) {}

bool snes_load_cartridge_normal(
  const char *, const uint8_t *rom_data, unsigned rom_size
)
{
   if (gb.load(rom_data, rom_size))
      return false;

   if (!gb.isLoaded())
      return false;

   return true;
}

bool snes_load_cartridge_bsx_slotted(
  const char *, const uint8_t *, unsigned,
  const char *, const uint8_t *, unsigned
) { return false; }

bool snes_load_cartridge_bsx(
  const char *, const uint8_t *, unsigned,
  const char *, const uint8_t *, unsigned
) { return false; }

bool snes_load_cartridge_sufami_turbo(
  const char *, const uint8_t *, unsigned,
  const char *, const uint8_t *, unsigned,
  const char *, const uint8_t *, unsigned
) { return false; }

bool snes_load_cartridge_super_game_boy(
  const char *, const uint8_t *, unsigned,
  const char *, const uint8_t *, unsigned
) { return false; }

void snes_unload_cartridge() {}

bool snes_get_region() { return SNES_REGION_NTSC; }
uint8_t *snes_get_memory_data(unsigned) { return 0; }
unsigned snes_get_memory_size(unsigned) { return 0; }

static void convert_frame(uint16_t *output, const uint32_t *input)
{
   for (unsigned y = 0; y < 144; y++)
   {
      const uint32_t *src = input + y * 256;
      uint16_t *dst = output + y * 1024;

      for (unsigned x = 0; x < 160; x++)
      {
         unsigned color = src[x];
         unsigned out_color = 0;

         // ARGB8888 => XRGB555
         out_color |= (color & 0xf80000) >> (3 + 6);
         out_color |= (color & 0x00f800) >> (3 + 3);
         out_color |= (color & 0x0000f8) >> (3 + 0);

         dst[x] = out_color;
      }
   }
}

void snes_run()
{
   union
   {
      uint32_t u32[2064 + 2064];
      int16_t u16[2 * (2064 + 2064)];
   } sound_buf;
   unsigned samples = 2064;

   input_poll_cb();

   uint32_t video_buf[256 * 144];
   while (gb.runFor(video_buf, 256, sound_buf.u32, samples) == -1)
   {
      // TODO: Output audio here.
      samples = 2064;
   }

   // libsnes uses fixed pitch of 2048 bytes on non-interlaced video.
   uint16_t output_video[1024 * 144];
   convert_frame(output_video, video_buf);

   video_cb(output_video, 160, 144);
}

const char *snes_library_id() { return "libgambatte"; }
unsigned snes_library_revision_major() { return 1; }
unsigned snes_library_revision_minor() { return 3; }

