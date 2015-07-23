#include "libretro.h"
#include "blipper.h"
#include <gambatte.h>
#include "gbcpalettes.h"

#include <assert.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdlib.h>

#ifdef _3DS
extern "C" void* linearMemAlign(size_t size, size_t alignment);
extern "C" void linearFree(void* mem);
#endif

retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static gambatte::video_pixel_t* video_buf;
static gambatte::uint_least32_t video_pitch;
static gambatte::GB gb;

#include "cc_resampler.h"

namespace input
{
   struct map { unsigned snes; unsigned gb; };
   static const map btn_map[] = {
      { RETRO_DEVICE_ID_JOYPAD_A, gambatte::InputGetter::A },
      { RETRO_DEVICE_ID_JOYPAD_B, gambatte::InputGetter::B },
      { RETRO_DEVICE_ID_JOYPAD_SELECT, gambatte::InputGetter::SELECT },
      { RETRO_DEVICE_ID_JOYPAD_START, gambatte::InputGetter::START },
      { RETRO_DEVICE_ID_JOYPAD_RIGHT, gambatte::InputGetter::RIGHT },
      { RETRO_DEVICE_ID_JOYPAD_LEFT, gambatte::InputGetter::LEFT },
      { RETRO_DEVICE_ID_JOYPAD_UP, gambatte::InputGetter::UP },
      { RETRO_DEVICE_ID_JOYPAD_DOWN, gambatte::InputGetter::DOWN },
   };
}

class SNESInput : public gambatte::InputGetter
{
   public:
      unsigned operator()()
      {
         unsigned res = 0;
         for (unsigned i = 0; i < sizeof(input::btn_map) / sizeof(input::map); i++)
            res |= input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, input::btn_map[i].snes) ? input::btn_map[i].gb : 0;
         return res;
      }
} static gb_input;

static blipper_t *resampler_l;
static blipper_t *resampler_r;

void retro_get_system_info(struct retro_system_info *info)
{
   info->library_name = "Gambatte";
   info->library_version = "v0.5.0";
   info->need_fullpath = false;
   info->block_extract = false;
   info->valid_extensions = "gb|gbc|dmg";
}

static struct retro_system_timing g_timing;

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   retro_game_geometry geom = { 160, 144, 160, 144 };
   info->geometry = geom;
   info->timing   = g_timing;
}

static void check_system_specs(void)
{
   unsigned level = 4;
   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

static void log_null(enum retro_log_level level, const char *fmt, ...) {}

void retro_init(void)
{
   struct retro_log_callback log;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = log_null;

   // Using uint_least32_t in an audio interface expecting you to cast to short*? :( Weird stuff.
   assert(sizeof(gambatte::uint_least32_t) == sizeof(uint32_t));
   gb.setInputGetter(&gb_input);

   double fps = 4194304.0 / 70224.0;
   double sample_rate = fps * 35112;

#ifdef CC_RESAMPLER
   CC_init();
   if (environ_cb)
   {
      g_timing.fps = fps;
      g_timing.sample_rate = sample_rate / CC_DECIMATION_RATE; // ~64k
   }
#else
   resampler_l = blipper_new(32, 0.85, 6.5, 64, 1024, NULL);
   resampler_r = blipper_new(32, 0.85, 6.5, 64, 1024, NULL);

   if (environ_cb)
   {
      g_timing.fps = fps;
      g_timing.sample_rate = sample_rate / 64; // ~32k
   }
#endif

#ifdef _3DS
   video_buf = (gambatte::video_pixel_t*)
               linearMemAlign(256 * 144 * sizeof(gambatte::video_pixel_t), 128);
#else
   video_buf = (gambatte::video_pixel_t*)
               malloc(256 * 144 * sizeof(gambatte::video_pixel_t));
#endif
   video_pitch = 256;

   check_system_specs();
}

void retro_deinit()
{
#ifndef CC_RESAMPLER
   blipper_free(resampler_l);;
   blipper_free(resampler_r);;
#endif
#ifdef _3DS
   linearFree(video_buf);
#else
   free(video_buf);
#endif
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   static const struct retro_variable vars[] = {
      { "gambatte_gb_colorization", "GB Colorization; disabled|auto|internal|custom" },
      { "gambatte_gb_internal_palette", "Internal Palette; GBC - Blue|GBC - Brown|GBC - Dark Blue|GBC - Dark Brown|GBC - Dark Green|GBC - Grayscale|GBC - Green|GBC - Inverted|GBC - Orange|GBC - Pastel Mix|GBC - Red|GBC - Yellow|Special 1|Special 2|Special 3" },
      { "gambatte_gbc_color_correction", "Color correction; enabled|disabled" },
      { "gambatte_gb_hwmode", "Emulated hardware; Auto|GB|GBA" }, // unfortunately, libgambatte does not have a 'force GBC' flag
      { NULL, NULL },
   };

   cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)vars);
}

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t) { }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

void retro_set_controller_port_device(unsigned, unsigned) {}

void retro_reset()
{
   // gambatte seems to clear out SRAM on reset.
   uint8_t *sram = 0;
   if (gb.savedata_size())
   {
      sram = new uint8_t[gb.savedata_size()];
      memcpy(sram, gb.savedata_ptr(), gb.savedata_size());
   }

   gb.reset();

   if (sram)
   {
      memcpy(gb.savedata_ptr(), sram, gb.savedata_size());
      delete[] sram;
   }
}

static size_t serialize_size = 0;
size_t retro_serialize_size(void)
{
   return gb.stateSize();
}

bool retro_serialize(void *data, size_t size)
{
   serialize_size = retro_serialize_size();

   if (size != serialize_size)
      return false;

   gb.saveState(data);
   return true;
}

bool retro_unserialize(const void *data, size_t size)
{
   serialize_size = retro_serialize_size();

   if (size != serialize_size)
      return false;

   gb.loadState(data);
   return true;
}

void retro_cheat_reset()
{
   gb.clearCheats();
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   std::string s = code;
   if (s.find("-") != std::string::npos)
      gb.setGameGenie(code);
   else
      gb.setGameShark(code);
}

   
static std::string basename(std::string filename)
{
   // Remove directory if present.
   // Do this before extension removal incase directory has a period character.
   const size_t last_slash_idx = filename.find_last_of("\\/");
   if (std::string::npos != last_slash_idx)
      filename.erase(0, last_slash_idx + 1);

   // Remove extension if present.
   const size_t period_idx = filename.rfind('.');
   if (std::string::npos != period_idx)
      filename.erase(period_idx);

   return filename;
}

static bool startswith(const std::string s1, const std::string prefix)
{
    return s1.compare(0, prefix.length(), prefix) == 0;
}

static int gb_colorization_enable = 0;

static std::string rom_path;
char internal_game_name[17];

static void load_custom_palette(void)
{
   unsigned rgb32 = 0;

   const char *system_directory_c = NULL;
   environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_directory_c);
   if (!system_directory_c)
   {
      log_cb(RETRO_LOG_WARN, "[Gambatte]: no system directory defined, unable to look for custom palettes.\n");
      return;
   }

   std::string system_directory(system_directory_c);
   std::string custom_palette_path = system_directory + "/palettes/" + basename(rom_path) + ".pal";
   std::ifstream palette_file(custom_palette_path.c_str()); // try to open the palette file in read-only mode

   if (!palette_file.is_open())
   {
      // try again with the internal game name from the ROM header
      custom_palette_path = system_directory + "/palettes/" + std::string(internal_game_name) + ".pal";
      palette_file.open(custom_palette_path.c_str());
   }

   if (!palette_file.is_open())// && !findGbcTitlePal(internal_game_name))
   {
      // try again with default.pal
	  //- removed last line if colorization is enabled
      custom_palette_path = system_directory + "/palettes/" + "default.pal";
      palette_file.open(custom_palette_path.c_str());
   }

   if (!palette_file.is_open())
      return;  // unable to find any custom palette file

#if 0
   fprintf(RETRO_LOG_INFO, "[Gambatte]: using custom palette %s.\n", custom_palette_path.c_str());
#endif
   unsigned line_count = 0;
   for (std::string line; getline(palette_file, line); ) // iterate over file lines
   {
      line_count++;

      if (line[0]=='[') // skip ini sections
         continue;

      if (line[0]==';') // skip ini comments
         continue;

      if (line[0]=='\n') // skip empty lines
         continue;

      if (line.find("=") == std::string::npos)
      {
         log_cb(RETRO_LOG_WARN, "[Gambatte]: error in %s, line %d (color left as default).\n", custom_palette_path.c_str(), line_count);
         continue; // current line does not contain a palette color definition, so go to next line
      }

      // Supposed to be a typo here.
      if (startswith(line, "slectedScheme="))
         continue;

      std::string line_value = line.substr(line.find("=") + 1); // extract the color value string
      std::stringstream ss(line_value); // convert the color value to int
      ss >> rgb32;
      if (!ss)
      {
         log_cb(RETRO_LOG_WARN, "[Gambatte]: unable to read palette color in %s, line %d (color left as default).\n",
               custom_palette_path.c_str(), line_count);
         continue;
      }
#ifdef VIDEO_RGB565
      rgb32=(rgb32&0x0000F8)>>3 |//red
            (rgb32&0x00FC00)>>5 |//green
            (rgb32&0xF80000)>>8;//blue
#endif

      if (startswith(line, "Background0="))
         gb.setDmgPaletteColor(0, 0, rgb32);
      else if (startswith(line, "Background1="))
         gb.setDmgPaletteColor(0, 1, rgb32);
      else if (startswith(line, "Background2="))
         gb.setDmgPaletteColor(0, 2, rgb32);      	
      else if (startswith(line, "Background3="))
         gb.setDmgPaletteColor(0, 3, rgb32);
      else if (startswith(line, "Sprite%2010="))
         gb.setDmgPaletteColor(1, 0, rgb32);
      else if (startswith(line, "Sprite%2011="))
         gb.setDmgPaletteColor(1, 1, rgb32);
      else if (startswith(line, "Sprite%2012="))
         gb.setDmgPaletteColor(1, 2, rgb32);
      else if (startswith(line, "Sprite%2013="))
         gb.setDmgPaletteColor(1, 3, rgb32);
      else if (startswith(line, "Sprite%2020="))
         gb.setDmgPaletteColor(2, 0, rgb32);
      else if (startswith(line, "Sprite%2021="))
         gb.setDmgPaletteColor(2, 1, rgb32);
      else if (startswith(line, "Sprite%2022="))
         gb.setDmgPaletteColor(2, 2, rgb32);  
      else if (startswith(line, "Sprite%2023="))
         gb.setDmgPaletteColor(2, 3, rgb32);
      else log_cb(RETRO_LOG_WARN, "[Gambatte]: error in %s, line %d (color left as default).\n", custom_palette_path.c_str(), line_count);
   } // endfor
}

static void check_variables(void)
{
   bool colorCorrection=true;
   struct retro_variable var = {0};
   var.key = "gambatte_gbc_color_correction";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && !strcmp(var.value, "disabled")) colorCorrection=false;
   gb.setColorCorrection(colorCorrection);

   var.key = "gambatte_gb_colorization";

   if (!environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || !var.value)
      return;

   if (gb.isCgb())
      return;

   // else it is a GB-mono game -> set a color palette
   //bool gb_colorization_old = gb_colorization_enable;

   if (strcmp(var.value, "disabled") == 0)
      gb_colorization_enable = 0;
   else if (strcmp(var.value, "auto") == 0)
      gb_colorization_enable = 1;
   else if (strcmp(var.value, "custom") == 0)
      gb_colorization_enable = 2;
   else if (strcmp(var.value, "internal") == 0)
      gb_colorization_enable = 3;

   //std::string internal_game_name = gb.romTitle(); // available only in latest Gambatte
   //std::string internal_game_name = reinterpret_cast<const char *>(info->data + 0x134); // buggy with some games ("YOSSY NO COOKIE", "YOSSY NO PANEPON, etc.)

   // load a GBC BIOS builtin palette
   unsigned short* gbc_bios_palette = NULL;

   switch (gb_colorization_enable)
   {
      case 1:   
        gbc_bios_palette = const_cast<unsigned short*>(findGbcTitlePal(internal_game_name));
        if (!gbc_bios_palette)
        {
           // no custom palette found, load the default (blue)
           gbc_bios_palette = const_cast<unsigned short*>(findGbcDirPal("GBC - Blue"));
        }
      break;
        
      case 2:
	    load_custom_palette();
      break;
	  
      case 3:
	    var.key = "gambatte_gb_internal_palette";
	    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
		{
			// Load the selected internal palette
			gbc_bios_palette = const_cast<unsigned short*>(findGbcDirPal(var.value));
		}
      break;

      default:
	    gbc_bios_palette = const_cast<unsigned short*>(findGbcDirPal("GBC - Grayscale"));
	  break;
   }
   //gambatte is using custom colorization then we have a previously palette loaded, 
   //skip this loop then
   if (gb_colorization_enable != 2)
   {
     unsigned rgb32 = 0;
     for (unsigned palnum = 0; palnum < 3; ++palnum)
     {
        for (unsigned colornum = 0; colornum < 4; ++colornum)
        {
           rgb32 = gb.gbcToRgb32(gbc_bios_palette[palnum * 4 + colornum]);
           gb.setDmgPaletteColor(palnum, colornum, rgb32);
        }
     }
   }
}

static unsigned pow2ceil(unsigned n) {
   --n;
   n |= n >> 1;
   n |= n >> 2;
   n |= n >> 4;
   n |= n >> 8;
   ++n;

   return n;
}

bool retro_load_game(const struct retro_game_info *info)
{
   bool can_dupe = false;
   environ_cb(RETRO_ENVIRONMENT_GET_CAN_DUPE, &can_dupe);
   if (!can_dupe)
   {
      log_cb(RETRO_LOG_ERROR, "[Gambatte]: Cannot dupe frames!\n");
      return false;
   }

   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },

      { 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

#ifdef VIDEO_RGB565
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_ERROR, "[Gambatte]: RGB565 is not supported.\n");
      return false;
   }
#else
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      log_cb(RETRO_LOG_ERROR, "[Gambatte]: XRGB8888 is not supported.\n");
      return false;
   }
#endif

   unsigned flags = 0;
   struct retro_variable var = {0};
   var.key = "gambatte_gb_hwmode";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "GB")) flags |= gambatte::GB::FORCE_DMG;
      if (!strcmp(var.value, "GBA")) flags |= gambatte::GB::GBA_CGB;
   }

   if (gb.load(info->data, info->size, flags) != 0)
      return false;

   rom_path = info->path ? info->path : "";
   strncpy(internal_game_name, (const char*)info->data + 0x134, sizeof(internal_game_name) - 1);
   internal_game_name[sizeof(internal_game_name)-1]='\0';

   log_cb(RETRO_LOG_INFO, "[Gambatte]: Got internal game name: %s.\n", internal_game_name);

   check_variables();

   //Ugly hack alert: This entire thing depends upon cartridge.cpp and memptrs.cpp not changing in weird ways.
   unsigned sramsize = gb.savedata_size();
   if (sramsize)
   {
      unsigned romsize = pow2ceil(info->size) & ~0x4000;
      unsigned ramsize = (gb.isCgb() ? 8 : 2) * 0x1000ul;
      char * sramdata = (char*)gb.savedata_ptr();
      char * romdata = sramdata - romsize;
      char * ramdata = sramdata + sramsize;
      struct retro_memory_descriptor descs[3];
      memset(descs, 0, sizeof(descs));
      descs[0].ptr=ramdata;
      descs[0].len=ramsize;
      descs[0].addrspace="";
      descs[1].ptr=sramdata;
      descs[1].len=sramsize;
      descs[1].addrspace="S";
      descs[2].ptr=romdata;
      descs[2].len=romsize;
      descs[2].addrspace="R";
      descs[2].flags=RETRO_MEMDESC_CONST;
      struct retro_memory_map maps={descs, 3};
      environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &maps);
   }

   return true;
}


bool retro_load_game_special(unsigned, const struct retro_game_info*, size_t) { return false; }

void retro_unload_game()
{}

unsigned retro_get_region() { return RETRO_REGION_NTSC; }

void *retro_get_memory_data(unsigned id)
{
   switch (id)
   {
      case RETRO_MEMORY_SAVE_RAM:
         return gb.savedata_ptr();
      case RETRO_MEMORY_RTC:
         return gb.rtcdata_ptr();
      case RETRO_MEMORY_SYSTEM_RAM:
         /* Really ugly hack here, relies upon 
          * libgambatte/src/memory/memptrs.cpp MemPtrs::reset not
          * realizing that that memchunk hack is ugly, or 
          * otherwise getting rearranged. */
         return (char*)gb.savedata_ptr() + gb.savedata_size();
   }

   return 0;
}

size_t retro_get_memory_size(unsigned id)
{
   switch (id)
   {
      case RETRO_MEMORY_SAVE_RAM:
         return gb.savedata_size();
      case RETRO_MEMORY_RTC:
         return gb.rtcdata_size();
      case RETRO_MEMORY_SYSTEM_RAM:
         /* This is rather hacky too... it relies upon 
          * libgambatte/src/memory/cartridge.cpp not changing
          * the call to memptrs.reset, but this is 
          * probably mostly safe.
          *
          * GBC will probably not get a
          * hardware upgrade anytime soon. */
         return (gb.isCgb() ? 8 : 2) * 0x1000ul;
   }

   return 0;
}

static void render_audio(const int16_t *samples, unsigned frames)
{
   if (!frames)
      return;

   blipper_push_samples(resampler_l, samples + 0, frames, 2);
   blipper_push_samples(resampler_r, samples + 1, frames, 2);
}

void retro_run()
{
   static uint64_t samples_count = 0;
   static uint64_t frames_count = 0;

   input_poll_cb();

   uint64_t expected_frames = samples_count / 35112;
   if (frames_count < expected_frames) // Detect frame dupes.
   {
#ifdef VIDEO_RGB565
      video_cb(0, 160, 144, 512);
#else
      video_cb(0, 160, 144, 1024);
#endif
      frames_count++;
      return;
   }

   union
   {
      gambatte::uint_least32_t u32[2064 + 2064];
      int16_t i16[2 * (2064 + 2064)];
   } static sound_buf;
   unsigned samples = 2064;

   while (gb.runFor(video_buf, video_pitch, sound_buf.u32, samples) == -1)
   {
#ifdef CC_RESAMPLER
      CC_renderaudio((audio_frame_t*)sound_buf.u32, samples);
#else
      render_audio(sound_buf.i16, samples);

      unsigned read_avail = blipper_read_avail(resampler_l);
      if (read_avail >= 512)
      {
         blipper_read(resampler_l, sound_buf.i16 + 0, read_avail, 2);
         blipper_read(resampler_r, sound_buf.i16 + 1, read_avail, 2);
         audio_batch_cb(sound_buf.i16, read_avail);
      }

#endif
      samples_count += samples;
      samples = 2064;
   }

   samples_count += samples;

#ifdef CC_RESAMPLER
   CC_renderaudio((audio_frame_t*)sound_buf.u32, samples);
#else
   render_audio(sound_buf.i16, samples);
#endif

#ifdef VIDEO_RGB565
   video_cb(video_buf, 160, 144, 512);
#else
   video_cb(video_buf, 160, 144, 1024);
#endif


#ifndef CC_RESAMPLER
   unsigned read_avail = blipper_read_avail(resampler_l);
   blipper_read(resampler_l, sound_buf.i16 + 0, read_avail, 2);
   blipper_read(resampler_r, sound_buf.i16 + 1, read_avail, 2);
   audio_batch_cb(sound_buf.i16, read_avail);
#endif

   frames_count++;

   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();
}

unsigned retro_api_version() { return RETRO_API_VERSION; }

