#include <stdlib.h>

#include <libretro.h>
#include <libretro_core_options.h>
#include "blipper.h"
#include "gambatte.h"
#include "gbcpalettes.h"
#include "bootloader.h"
#ifdef HAVE_NETWORK
#include "net_serial.h"
#endif

#if defined(__DJGPP__) && defined(__STRICT_ANSI__)
/* keep this above libretro-common includes */
#undef __STRICT_ANSI__
#endif

#include <streams/file_stream.h>

#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <algorithm>

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

static bool libretro_supports_bitmasks = false;

static bool show_gb_link_settings = true;

static bool up_down_allowed = false;
static bool rom_loaded = false;

//Dual mode runs two GBCs side by side.
//Currently, they load the same ROM, take the same input, and only the left one supports SRAM, cheats, savestates, or sound.
//Can be made useful later, but for now, it's just a tech demo.
//#define DUAL_MODE

#ifdef DUAL_MODE
static gambatte::GB gb2;
#define NUM_GAMEBOYS 2
#else
#define NUM_GAMEBOYS 1
#endif

#ifdef CC_RESAMPLER
#include "cc_resampler.h"
#endif

bool use_official_bootloader = false;

// Colours from previous frame
static gambatte::video_pixel_t prev_colours[160 * NUM_GAMEBOYS * 144] = {0};
static unsigned mix_frames_mode = 0;

bool file_present_in_system(std::string fname)
{
   const char *systemdirtmp = NULL;
   bool worked = environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &systemdirtmp);
   if (!worked)
      return false;
   
   std::string fullpath  = systemdirtmp;
   fullpath             += "/";
   fullpath             += fname;
   
   RFILE *fp             = filestream_open(fullpath.c_str(), RETRO_VFS_FILE_ACCESS_READ, 
         RETRO_VFS_FILE_ACCESS_HINT_NONE);
   
   if (fp)
   {
      filestream_close(fp);
      return true;
   }
   
   return false;
}

bool get_bootloader_from_file(void* userdata, bool isgbc, uint8_t* data, uint32_t buf_size)
{
   std::string path;
   unsigned int size;
   RFILE *fp                = NULL;
   int64_t n                = 0;
   bool worked              = false;
   const char *systemdirtmp = NULL;
   if (!use_official_bootloader)
      return false;

   // get path
   worked = environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &systemdirtmp);
   if (!worked)
      return false;

   path  = systemdirtmp;
   path += "/"; //retroarch/libretro does not add a slash at the end of directory names
   
   if (isgbc)
   {
      path += "gbc_bios.bin";
      size = 0x900;
   }
   else
   {
      path += "gb_bios.bin";
      size = 0x100;
   }

   if (size > buf_size)
      return false;
   
   // open file
   fp = filestream_open(path.c_str(), RETRO_VFS_FILE_ACCESS_READ,
         RETRO_VFS_FILE_ACCESS_HINT_NONE);

   if (!fp)
      return false;

   n = filestream_read(fp, data, size);
   filestream_close(fp);
   
   if (n != size)
      return false;
   
   return true;
}

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
         unsigned i;
         unsigned res = 0;
         if (libretro_supports_bitmasks)
         {
            int16_t ret = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
            for (i = 0; i < sizeof(input::btn_map) / sizeof(input::map); i++)
               res |= (ret & (1 << input::btn_map[i].snes)) ? input::btn_map[i].gb : 0;
         }
         else
         {
            for (i = 0; i < sizeof(input::btn_map) / sizeof(input::map); i++)
               res |= input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, input::btn_map[i].snes) ? input::btn_map[i].gb : 0;
         }

         if (!up_down_allowed)
         {
            if (res & gambatte::InputGetter::UP)
               if (res & gambatte::InputGetter::DOWN)
                  res &= ~(gambatte::InputGetter::UP | gambatte::InputGetter::DOWN);

            if (res & gambatte::InputGetter::LEFT)
               if (res & gambatte::InputGetter::RIGHT)
                  res &= ~(gambatte::InputGetter::LEFT | gambatte::InputGetter::RIGHT);
         }

         return res;
      }
} static gb_input;

#ifdef HAVE_NETWORK
enum SerialMode {
   SERIAL_NONE,
   SERIAL_SERVER,
   SERIAL_CLIENT
};
static NetSerial gb_net_serial;
static SerialMode gb_serialMode = SERIAL_NONE;
static int gb_NetworkPort = 12345;
static std::string gb_NetworkClientAddr;
#endif

static blipper_t *resampler_l;
static blipper_t *resampler_r;

void retro_get_system_info(struct retro_system_info *info)
{
   info->library_name = "Gambatte";
#ifdef HAVE_NETWORK
   info->library_version = "v0.5.0-netlink";
#else
   info->library_version = "v0.5.0";
#endif
   info->need_fullpath = false;
   info->block_extract = false;
   info->valid_extensions = "gb|gbc|dmg";
}

static struct retro_system_timing g_timing;

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   retro_game_geometry geom = { 160*NUM_GAMEBOYS, 144, 160*NUM_GAMEBOYS, 144, 160.0f/144.0f };
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
#ifdef DUAL_MODE
   gb2.setInputGetter(&gb_input);
#endif

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
               linearMemAlign(sizeof(gambatte::video_pixel_t) * 256 * NUM_GAMEBOYS * 144, 128);
#else
   video_buf = (gambatte::video_pixel_t*)
               malloc(sizeof(gambatte::video_pixel_t) * 256 * NUM_GAMEBOYS * 144);
#endif
   video_pitch = 256 * NUM_GAMEBOYS;

   check_system_specs();
   
   //gb/gbc bootloader support
   gb.setBootloaderGetter(get_bootloader_from_file);
#ifdef DUAL_MODE
   gb2.setBootloaderGetter(get_bootloader_from_file);
#endif
   
   struct retro_variable var = {0};
   var.key = "gambatte_gb_bootloader";


   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "enabled"))
         use_official_bootloader = true;
      else
         use_official_bootloader = false;
   }
   else
      use_official_bootloader = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

}

void retro_deinit(void)
{
#ifndef CC_RESAMPLER
   blipper_free(resampler_l);
   blipper_free(resampler_r);
#endif
#ifdef _3DS
   linearFree(video_buf);
#else
   free(video_buf);
#endif
   video_buf = NULL;
   libretro_supports_bitmasks = false;
}

void retro_set_environment(retro_environment_t cb)
{
   struct retro_vfs_interface_info vfs_iface_info;
   environ_cb = cb;

   libretro_set_core_options(environ_cb);

   vfs_iface_info.required_interface_version = 1;
   vfs_iface_info.iface                      = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
	   filestream_vfs_init(&vfs_iface_info);
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
   uint8_t *rtc = 0;
   if (gb.savedata_size())
   {
      sram = new uint8_t[gb.savedata_size()];
      memcpy(sram, gb.savedata_ptr(), gb.savedata_size());
   }
   if (gb.rtcdata_size())
   {
      rtc = new uint8_t[gb.rtcdata_size()];
      memcpy(rtc, gb.rtcdata_ptr(), gb.rtcdata_size());
   }

   gb.reset();
#ifdef DUAL_MODE
   gb2.reset();
#endif

   if (sram)
   {
      memcpy(gb.savedata_ptr(), sram, gb.savedata_size());
      delete[] sram;
   }
   if (rtc)
   {
      memcpy(gb.rtcdata_ptr(), rtc, gb.rtcdata_size());
      delete[] rtc;
   }

   memset(prev_colours, 0, sizeof(gambatte::video_pixel_t) * 160 * NUM_GAMEBOYS * 144);
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
   std::string code_str(code);

   replace(code_str.begin(), code_str.end(), '+', ';');

   if (code_str.find("-") != std::string::npos) {
      gb.setGameGenie(code_str);
   } else {
      gb.setGameShark(code_str);
   }
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
static char internal_game_name[17];

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
   unsigned i, j;

   unsigned colorCorrection = 0;
   struct retro_variable var = {0};
   var.key = "gambatte_gbc_color_correction";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "GBC only"))
         colorCorrection = 1;
      else if (!strcmp(var.value, "always"))
         colorCorrection = 2;
   }
   
   unsigned colorCorrectionMode = 0;
   var.key   = "gambatte_gbc_color_correction_mode";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && !strcmp(var.value, "fast")) {
      colorCorrectionMode = 1;
   }
   gb.setColorCorrectionMode(colorCorrectionMode);
   
   float colorCorrectionBrightness = 0.5f; /* central */
   var.key   = "gambatte_gbc_frontlight_position";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "above screen"))
         colorCorrectionBrightness = 1.2f;
      else if (!strcmp(var.value, "below screen"))
         colorCorrectionBrightness = 0.0f;
   }
   gb.setColorCorrectionBrightness(colorCorrectionBrightness);
   
   unsigned darkFilterLevel = 0;
   var.key   = "gambatte_dark_filter_level";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      darkFilterLevel = static_cast<unsigned>(atoi(var.value));
   }
   gb.setDarkFilterLevel(darkFilterLevel);
   
   var.key   = "gambatte_up_down_allowed";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "enabled"))
         up_down_allowed = true;
      else
         up_down_allowed = false;
   }
   else
      up_down_allowed = false;

   unsigned prev_mix_frames_mode = mix_frames_mode;
   mix_frames_mode = 0;
   var.key   = "gambatte_mix_frames";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "accurate"))
         mix_frames_mode = 1;
      else if (!strcmp(var.value, "fast"))
         mix_frames_mode = 2;
   }
   // Must reset previous colours when turning 'mix frames'
   // on, otherwise first rendered frame may contain garbage
   if ((prev_mix_frames_mode == 0) && (mix_frames_mode != 0)) {
      memset(prev_colours, 0, sizeof(gambatte::video_pixel_t) * 160 * NUM_GAMEBOYS * 144);
   }

#ifdef HAVE_NETWORK

   gb_serialMode = SERIAL_NONE;
   var.key = "gambatte_gb_link_mode";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
      if (!strcmp(var.value, "Network Server")) {
         gb_serialMode = SERIAL_SERVER;
      } else if (!strcmp(var.value, "Network Client")) {
         gb_serialMode = SERIAL_CLIENT;
      }
   }

   var.key = "gambatte_gb_link_network_port";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
      gb_NetworkPort=atoi(var.value);
   }

   unsigned ip_index = 1;
   gb_NetworkClientAddr = "";

   for (i = 0; i < 4; i++)
   {
      std::string octet = "0";
      char tmp[8] = {0};

      for (j = 0; j < 3; j++)
      {
         char key[64] = {0};

         /* Should be using std::to_string() here, but some
          * compilers don't support it... */
         sprintf(key, "%s%u",
              "gambatte_gb_link_network_server_ip_", ip_index);

         var.key = key;
         var.value = NULL;

         if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            octet += std::string(var.value);

         ip_index++;
      }

      /* Remove leading zeros
       * Should be using std::stoul() here, but some compilers
       * don't support it... */
      sprintf(tmp, "%u", atoi(octet.c_str()));
      octet = std::string(tmp);

      if (i < 3)
         octet += ".";

      gb_NetworkClientAddr += octet;
   }

   switch(gb_serialMode)
   {
      case SERIAL_SERVER:
         gb_net_serial.start(true, gb_NetworkPort, gb_NetworkClientAddr);
         gb.setSerialIO(&gb_net_serial);
         break;
      case SERIAL_CLIENT:
         gb_net_serial.start(false, gb_NetworkPort, gb_NetworkClientAddr);
         gb.setSerialIO(&gb_net_serial);
         break;
      default:
         gb_net_serial.stop();
         gb.setSerialIO(NULL);
         break;
   }

   /* Show/hide core options */

   var.key = "gambatte_show_gb_link_settings";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      bool show_gb_link_settings_prev = show_gb_link_settings;

      show_gb_link_settings = true;
      if (strcmp(var.value, "disabled") == 0)
         show_gb_link_settings = false;

      if (show_gb_link_settings != show_gb_link_settings_prev)
      {
         struct retro_core_option_display option_display;

         option_display.visible = show_gb_link_settings;

         option_display.key = "gambatte_gb_link_mode";
         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

         option_display.key = "gambatte_gb_link_network_port";
         environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

         for (i = 0; i < 12; i++)
         {
            char key[64] = {0};

            /* Should be using std::to_string() here, but some
             * compilers don't support it... */
            sprintf(key, "%s%u",
                 "gambatte_gb_link_network_server_ip_", i + 1);

            option_display.key = key;

            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
         }
      }
   }

#endif

   var.key = "gambatte_gb_colorization";

   if (!environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || !var.value) {
      // Should really wait until the end to call setColorCorrection(),
      // but don't want to have to change the indentation of all the
      // following code... (makes it too difficult to see the changes in
      // a git diff...)
      gb.setColorCorrection(gb.isCgb() && (colorCorrection != 0));
      return;
   }
   
   if (gb.isCgb()) {
      gb.setColorCorrection(colorCorrection != 0);
      return;
   }

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
   else if (strcmp(var.value, "GBC") == 0)
      gb_colorization_enable = 4;
   else if (strcmp(var.value, "SGB") == 0)
      gb_colorization_enable = 5;

   //std::string internal_game_name = gb.romTitle(); // available only in latest Gambatte
   //std::string internal_game_name = reinterpret_cast<const char *>(info->data + 0x134); // buggy with some games ("YOSSY NO COOKIE", "YOSSY NO PANEPON, etc.)

   // Containers for GBC/SGB BIOS built-in palettes
   unsigned short* gbc_bios_palette = NULL;
   unsigned short* sgb_bios_palette = NULL;
   bool isGbcPalette = false;

   switch (gb_colorization_enable)
   {
      case 1:
         // Automatic colourisation
         // Order of preference:
         // 1 - SGB, if more colourful than GBC
         // 2 - GBC, if more colourful than SGB
         // 3 - SGB, if no GBC palette defined
         // 4 - User-defined internal palette, if neither GBC nor SGB palettes defined
         //
         // Load GBC BIOS built-in palette
         gbc_bios_palette = const_cast<unsigned short*>(findGbcTitlePal(internal_game_name));
         // Load SGB BIOS built-in palette
         sgb_bios_palette = const_cast<unsigned short*>(findSgbTitlePal(internal_game_name));
         // If both GBC and SGB palettes are defined,
         // use whichever is more colourful
         if (gbc_bios_palette != 0)
         {
            isGbcPalette = true;
            if (sgb_bios_palette != 0)
            {
               if (gbc_bios_palette != p005 &&
                   gbc_bios_palette != p006 &&
                   gbc_bios_palette != p007 &&
                   gbc_bios_palette != p008 &&
                   gbc_bios_palette != p012 &&
                   gbc_bios_palette != p013 &&
                   gbc_bios_palette != p016 &&
                   gbc_bios_palette != p017 &&
                   gbc_bios_palette != p01B)
               {
                  // Limited color GBC palette -> use SGB equivalent
                  gbc_bios_palette = sgb_bios_palette;
                  isGbcPalette = false;
               }
            }
         }
         // If no GBC palette is defined, use SGB palette
         if (gbc_bios_palette == 0)
         {
            gbc_bios_palette = sgb_bios_palette;
         }
         // If neither GBC nor SGB palettes are defined, set
         // user-defined internal palette
         if (gbc_bios_palette == 0)
         {
            var.key = "gambatte_gb_internal_palette";
            if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            {
               // Load the selected internal palette
               gbc_bios_palette = const_cast<unsigned short*>(findGbcDirPal(var.value));
               if (!strncmp("GBC", var.value, 3)) {
                  isGbcPalette = true;
               }
            }
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
            if (!strncmp("GBC", var.value, 3)) {
               isGbcPalette = true;
            }
         }
         break;
      case 4:
         // Force GBC colourisation
         gbc_bios_palette = const_cast<unsigned short*>(findGbcTitlePal(internal_game_name));
         if (gbc_bios_palette == 0)
         {
            gbc_bios_palette = const_cast<unsigned short*>(findGbcDirPal("GBC - Dark Green")); // GBC Default
         }
         isGbcPalette = true;
         break;
      case 5:
         // Force SGB colourisation
         gbc_bios_palette = const_cast<unsigned short*>(findSgbTitlePal(internal_game_name));
         if (gbc_bios_palette == 0)
         {
            gbc_bios_palette = const_cast<unsigned short*>(findGbcDirPal("SGB - 1A")); // SGB Default
         }
         break;
      default:
         gbc_bios_palette = const_cast<unsigned short*>(findGbcDirPal("GBC - Grayscale"));
         isGbcPalette = true;
         break;
   }
   
   // Enable colour correction, if required
   gb.setColorCorrection((colorCorrection == 2) || ((colorCorrection == 1) && isGbcPalette));
   
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
   
   bool has_gbc_bootloader = file_present_in_system("gbc_bios.bin");

   unsigned flags = 0;
   struct retro_variable var = {0};
   var.key = "gambatte_gb_hwmode";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "GB"))
      {
          flags |= gambatte::GB::FORCE_DMG;
      }
      
      if (!strcmp(var.value, "GBC"))
      {
         if (has_gbc_bootloader && use_official_bootloader)
            flags |= gambatte::GB::FORCE_CGB;
      }

      if (!strcmp(var.value, "GBA"))
      {
         flags |= gambatte::GB::GBA_CGB;
         if (has_gbc_bootloader && use_official_bootloader)
            flags |= gambatte::GB::FORCE_CGB;
      }
   }

   if (gb.load(info->data, info->size, flags) != 0)
      return false;
#ifdef DUAL_MODE
   if (gb2.load(info->data, info->size, flags) != 0)
      return false;
#endif

   rom_path = info->path ? info->path : "";
   strncpy(internal_game_name, (const char*)info->data + 0x134, sizeof(internal_game_name) - 1);
   internal_game_name[sizeof(internal_game_name)-1]='\0';

   log_cb(RETRO_LOG_INFO, "[Gambatte]: Got internal game name: %s.\n", internal_game_name);

   check_variables();

   unsigned sramlen       = gb.savedata_size();
   const uint64_t rom     = RETRO_MEMDESC_CONST;
   const uint64_t mainram = RETRO_MEMDESC_SYSTEM_RAM;
   struct retro_memory_map mmaps;

   struct retro_memory_descriptor descs[10] =
   {
      { mainram, gb.rambank0_ptr(),     0, 0xC000,          0, 0, 0x1000, NULL },
      { mainram, gb.rambank1_ptr(),     0, 0xD000,          0, 0, 0x1000, NULL },
      { mainram, gb.zeropage_ptr(),     0, 0xFF80,          0, 0, 0x0080, NULL },
      {       0, gb.vram_ptr(),         0, 0x8000,          0, 0, 0x2000, NULL },
      {       0, gb.oamram_ptr(),       0, 0xFE00, 0xFFFFFFE0, 0, 0x00A0, NULL },
      {     rom, gb.rombank0_ptr(),     0, 0x0000,          0, 0, 0x4000, NULL },
      {     rom, gb.rombank1_ptr(),     0, 0x4000,          0, 0, 0x4000, NULL },
      {       0, gb.oamram_ptr(),   0x100, 0xFF00,          0, 0, 0x0080, NULL },
      {       0, 0,                     0,      0,          0, 0,      0,    0 },
      {       0, 0,                     0,      0,          0, 0,      0,    0 }
   };

   unsigned i = 8;
   if (sramlen)
   {
      descs[i].ptr    = gb.savedata_ptr();
      descs[i].start  = 0xA000;
      descs[i].select = (size_t)~0x1FFF;
      descs[i].len    = sramlen;
      i++;
   }

   if (gb.isCgb())
   {
      descs[i].flags  = mainram;
      descs[i].ptr    = gb.rambank2_ptr();
      descs[i].start  = 0x10000;
      descs[i].select = 0xFFFFA000;
      descs[i].len    = 0x6000;
      i++;
   }

   mmaps.descriptors     = descs;
   mmaps.num_descriptors = i;   
   environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &mmaps);
   
   bool yes = true;
   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &yes);

   rom_loaded = true;
   return true;
}


bool retro_load_game_special(unsigned, const struct retro_game_info*, size_t) { return false; }

void retro_unload_game()
{
   rom_loaded = false;
}

unsigned retro_get_region() { return RETRO_REGION_NTSC; }

void *retro_get_memory_data(unsigned id)
{
   if (rom_loaded) switch (id)
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
         return gb.rambank0_ptr();
   }

   return 0;
}

size_t retro_get_memory_size(unsigned id)
{
   if (rom_loaded) switch (id)
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

static void mix_frames_fast(void)
{
   // Simple frame blending: mixes current frame 50:50 with
   // previous one.
   // Uses fast bit twiddling method, suitable for very low
   // end devices (NB: rounding errors will cause darkening
   // of colours - this is fairly innocuous, but may annoy
   // some users)
   unsigned offset = 0;
   unsigned colour_index = 0;
   for (unsigned i = 0; i < 144; i++)
   {
      for (unsigned j = 0; j < 160 * NUM_GAMEBOYS; j++)
      {
         // Get colours from current frame + previous frame
         unsigned buff_index = offset + j;
         gambatte::video_pixel_t rgb = video_buf[buff_index];
         gambatte::video_pixel_t rgb_prev = prev_colours[colour_index];
         
         // Store current colours for next frame
         prev_colours[colour_index] = rgb;
         colour_index++;
         
         // Mix colours for current frame
         // Do this in one shot to minimise unnecessary variables...
         // > Unpack current/previous frame colours and divide by 2
         // > Mix and repack colours for current frame
#ifdef VIDEO_RGB565
         video_buf[buff_index] =   (((rgb >> 11 & 0x1F) >> 1) + ((rgb_prev >> 11 & 0x1F) >> 1)) << 11
                                 | (((rgb >>  6 & 0x1F) >> 1) + ((rgb_prev >>  6 & 0x1F) >> 1)) << 6
                                 | (((rgb       & 0x1F) >> 1) + ((rgb_prev       & 0x1F) >> 1));
#else
         video_buf[buff_index] =   (((rgb >> 16 & 0x1F) >> 1) + ((rgb_prev >> 16 & 0x1F) >> 1)) << 16
                                 | (((rgb >>  8 & 0x1F) >> 1) + ((rgb_prev >>  8 & 0x1F) >> 1)) << 8
                                 | (((rgb       & 0x1F) >> 1) + ((rgb_prev       & 0x1F) >> 1));
#endif
      }
      offset += video_pitch;
   }
}

static void mix_frames_accurate(void)
{
   // Simple frame blending: mixes current frame 50:50 with
   // previous one.
   // Uses slow and accurate floating point conversion
   // method. Not suitable for very low end devices, but
   // should leave colour levels intact.
   // NB: We're repeating some code here, just for performance
   // reasons (i.e. putting an 'if' statement inside the inner
   // loop to select between fast and accurate methods makes the
   // fast method just a tiny bit too slow...)
   unsigned offset = 0;
   unsigned colour_index = 0;
   for (unsigned i = 0; i < 144; i++)
   {
      for (unsigned j = 0; j < 160 * NUM_GAMEBOYS; j++)
      {
         // Get colours from current frame + previous frame
         unsigned buff_index = offset + j;
         gambatte::video_pixel_t rgb = video_buf[buff_index];
         gambatte::video_pixel_t rgb_prev = prev_colours[colour_index];
         
         // Store current colours for next frame
         prev_colours[colour_index] = rgb;
         colour_index++;
         
         // Unpack current/previous frame colours and convert to float
#ifdef VIDEO_RGB565
         float r = static_cast<float>(rgb >> 11 & 0x1F);
         float g = static_cast<float>(rgb >>  6 & 0x1F);
         float b = static_cast<float>(rgb       & 0x1F);
         
         float r_prev = static_cast<float>(rgb_prev >> 11 & 0x1F);
         float g_prev = static_cast<float>(rgb_prev >>  6 & 0x1F);
         float b_prev = static_cast<float>(rgb_prev       & 0x1F);
#else
         float r = static_cast<float>(rgb >> 16 & 0x1F);
         float g = static_cast<float>(rgb >>  8 & 0x1F);
         float b = static_cast<float>(rgb       & 0x1F);
         
         float r_prev = static_cast<float>(rgb_prev >> 16 & 0x1F);
         float g_prev = static_cast<float>(rgb_prev >>  8 & 0x1F);
         float b_prev = static_cast<float>(rgb_prev       & 0x1F);
#endif
         // Mix colours for current frame and convert back to unsigned
         unsigned r_mix = static_cast<unsigned>(((r * 0.5) + (r_prev * 0.5)) + 0.5) & 0x1F;
         unsigned g_mix = static_cast<unsigned>(((g * 0.5) + (g_prev * 0.5)) + 0.5) & 0x1F;
         unsigned b_mix = static_cast<unsigned>(((b * 0.5) + (b_prev * 0.5)) + 0.5) & 0x1F;
         
         // Repack colours for current frame
#ifdef VIDEO_RGB565
         video_buf[buff_index] = r_mix << 11 | g_mix << 6 | b_mix;
#else
         video_buf[buff_index] = r_mix << 16 | g_mix << 8 | b_mix;
#endif
      }
      offset += video_pitch;
   }
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
      video_cb(NULL, 160*NUM_GAMEBOYS, 144, 512*NUM_GAMEBOYS);
#else
      video_cb(NULL, 160*NUM_GAMEBOYS, 144, 1024*NUM_GAMEBOYS);
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
#ifdef DUAL_MODE
   while (gb2.runFor(video_buf+160, video_pitch, sound_buf.u32, samples) == -1) {}
#endif

   samples_count += samples;

#ifdef CC_RESAMPLER
   CC_renderaudio((audio_frame_t*)sound_buf.u32, samples);
#else
   render_audio(sound_buf.i16, samples);
#endif

   switch (mix_frames_mode)
   {
      case 1:
         mix_frames_accurate();
         break;
      case 2:
         mix_frames_fast();
         break;
      default:
         // Do nothing
         // (defensive coding - could remove this...)
         break;
   }

#ifdef VIDEO_RGB565
   video_cb(video_buf, 160*NUM_GAMEBOYS, 144, 512*NUM_GAMEBOYS);
#else
   video_cb(video_buf, 160*NUM_GAMEBOYS, 144, 1024*NUM_GAMEBOYS);
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

