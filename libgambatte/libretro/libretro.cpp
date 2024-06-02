#include <stdlib.h>

#include <libretro.h>
#include <libretro_core_options.h>
#include "gambatte_log.h"
#include "blipper.h"
#include "cc_resampler.h"
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

#ifndef PATH_MAX_LENGTH
#if defined(_XBOX1) || defined(_3DS) || defined(PSP) || defined(PS2) || defined(GEKKO)|| defined(WIIU) || defined(ORBIS) || defined(__PSL1GHT__) || defined(__PS3__)
#define PATH_MAX_LENGTH 512
#else
#define PATH_MAX_LENGTH 4096
#endif
#endif

#include <string/stdstring.h>
#include <file/file_path.h>
#include <streams/file_stream.h>
#include <array/rhmap.h>

#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <algorithm>
#include <cmath>

#ifdef _3DS
extern "C" void* linearMemAlign(size_t size, size_t alignment);
extern "C" void linearFree(void* mem);
#endif

static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static gambatte::video_pixel_t* video_buf;
static gambatte::GB gb;

static bool libretro_supports_option_categories = false;
static bool libretro_supports_bitmasks          = false;
static bool libretro_supports_set_variable      = false;
static unsigned libretro_msg_interface_version  = 0;
static bool libretro_supports_ff_override       = false;
static bool libretro_ff_enabled                 = false;
static bool libretro_ff_enabled_prev            = false;

static bool show_gb_link_settings = true;

/* Minimum (and default) turbo pulse train
 * is 2 frames ON, 2 frames OFF */
#define TURBO_PERIOD_MIN      4
#define TURBO_PERIOD_MAX      120
#define TURBO_PULSE_WIDTH_MIN 2
#define TURBO_PULSE_WIDTH_MAX 15

static unsigned libretro_input_state = 0;
static bool up_down_allowed          = false;
static unsigned turbo_period         = TURBO_PERIOD_MIN;
static unsigned turbo_pulse_width    = TURBO_PULSE_WIDTH_MIN;
static unsigned turbo_a_counter      = 0;
static unsigned turbo_b_counter      = 0;

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

bool use_official_bootloader = false;

#define GB_SCREEN_WIDTH 160
#define VIDEO_WIDTH (GB_SCREEN_WIDTH * NUM_GAMEBOYS)
#define VIDEO_HEIGHT 144
/* Video buffer 'width' is 256, not 160 -> assume
 * there is a benefit to making this a power of 2 */
#define VIDEO_BUFF_SIZE (256 * NUM_GAMEBOYS * VIDEO_HEIGHT * sizeof(gambatte::video_pixel_t))
#define VIDEO_PITCH (256 * NUM_GAMEBOYS)
#define VIDEO_REFRESH_RATE (4194304.0 / 70224.0)

/*************************/
/* Audio Resampler START */
/*************************/

/* There are 35112 stereo sound samples in a video frame */
#define SOUND_SAMPLES_PER_FRAME   35112
/* We request 2064 samples from each call of GB::runFor() */
#define SOUND_SAMPLES_PER_RUN     2064
/* Native GB/GBC hardware audio sample rate (~2 MHz) */
#define SOUND_SAMPLE_RATE_NATIVE  (VIDEO_REFRESH_RATE * (double)SOUND_SAMPLES_PER_FRAME)

#define SOUND_SAMPLE_RATE_CC      (SOUND_SAMPLE_RATE_NATIVE / CC_DECIMATION_RATE) /* ~64k */
#define SOUND_SAMPLE_RATE_BLIPPER (SOUND_SAMPLE_RATE_NATIVE / 64) /* ~32k */

/* GB::runFor() nominally generates up to
 * (SOUND_SAMPLES_PER_RUN + 2064) samples, which
 * defines our sound buffer size
 * NOTE: This is in fact a lie - in the upstream code,
 * GB::runFor() can generate more than
 * (SOUND_SAMPLES_PER_RUN + 2064) samples, causing a
 * buffer overflow. It has been necessary to add an
 * internal hard cap/bail out in the event that
 * excess samples are detected... */
#define SOUND_BUFF_SIZE         (SOUND_SAMPLES_PER_RUN + 2064)

/* Blipper produces between 548 and 549 output samples
 * per frame. For safety, we want to keep the blip
 * buffer no more than ~50% full. (2 * 549) = 1098,
 * so add some padding and round up to (1024 + 512) */
#define BLIP_BUFFER_SIZE (1024 + 512)

static blipper_t *resampler_l = NULL;
static blipper_t *resampler_r = NULL;

static bool use_cc_resampler = false;

static int16_t *audio_out_buffer     = NULL;
static size_t audio_out_buffer_size  = 0;
static size_t audio_out_buffer_pos   = 0;
static size_t audio_batch_frames_max = (1 << 16);

static void audio_out_buffer_init(void)
{
   float sample_rate       = use_cc_resampler ?
         SOUND_SAMPLE_RATE_CC : SOUND_SAMPLE_RATE_BLIPPER;
   float samples_per_frame = sample_rate / VIDEO_REFRESH_RATE;
   size_t buffer_size      = ((size_t)samples_per_frame + 1) << 1;

   /* Create a buffer that is double the required size
    * to minimise the likelihood of resize operations
    * (core tends to produce very brief 'bursts' of high
    * sample counts depending upon the emulated content...) */
   buffer_size = (buffer_size << 1);

   audio_out_buffer        = (int16_t *)malloc(buffer_size * sizeof(int16_t));
   audio_out_buffer_size   = buffer_size;
   audio_out_buffer_pos    = 0;
   audio_batch_frames_max  = (1 << 16);
}

static void audio_out_buffer_deinit(void)
{
   if (audio_out_buffer)
      free(audio_out_buffer);

   audio_out_buffer       = NULL;
   audio_out_buffer_size  = 0;
   audio_out_buffer_pos   = 0;
   audio_batch_frames_max = (1 << 16);
}

static INLINE void audio_out_buffer_resize(size_t num_samples)
{
   size_t buffer_capacity = (audio_out_buffer_size -
         audio_out_buffer_pos) >> 1;

   if (buffer_capacity < num_samples)
   {
      int16_t *tmp_buffer = NULL;
      size_t tmp_buffer_size;

      tmp_buffer_size = audio_out_buffer_size +
            ((num_samples - buffer_capacity) << 1);
      tmp_buffer_size = (tmp_buffer_size << 1) - (tmp_buffer_size >> 1);
      tmp_buffer      = (int16_t *)malloc(tmp_buffer_size * sizeof(int16_t));

      memcpy(tmp_buffer, audio_out_buffer,
            audio_out_buffer_pos * sizeof(int16_t));

      free(audio_out_buffer);
      audio_out_buffer      = tmp_buffer;
      audio_out_buffer_size = tmp_buffer_size;
   }
}

void audio_out_buffer_write(int16_t *samples, size_t num_samples)
{
   audio_out_buffer_resize(num_samples);

   memcpy(audio_out_buffer + audio_out_buffer_pos,
         samples, (num_samples << 1) * sizeof(int16_t));

   audio_out_buffer_pos += num_samples << 1;
}

static void audio_out_buffer_read_blipper(size_t num_samples)
{
   int16_t *audio_out_buffer_ptr = NULL;

   audio_out_buffer_resize(num_samples);
   audio_out_buffer_ptr = audio_out_buffer + audio_out_buffer_pos;

   blipper_read(resampler_l, audio_out_buffer_ptr    , num_samples, 2);
   blipper_read(resampler_r, audio_out_buffer_ptr + 1, num_samples, 2);

   audio_out_buffer_pos += num_samples << 1;
}

static void audio_upload_samples(void)
{
   int16_t *audio_out_buffer_ptr = audio_out_buffer;
   size_t num_samples            = audio_out_buffer_pos >> 1;

   while (num_samples > 0)
   {
      size_t samples_to_write = (num_samples >
            audio_batch_frames_max) ?
                  audio_batch_frames_max :
                  num_samples;
      size_t samples_written = audio_batch_cb(
            audio_out_buffer_ptr, samples_to_write);

      if ((samples_written < samples_to_write) &&
          (samples_written > 0))
         audio_batch_frames_max = samples_written;

      num_samples          -= samples_to_write;
      audio_out_buffer_ptr += samples_to_write << 1;
   }

   audio_out_buffer_pos = 0;
}

static void blipper_renderaudio(const int16_t *samples, unsigned frames)
{
   if (!frames)
      return;

   blipper_push_samples(resampler_l, samples + 0, frames, 2);
   blipper_push_samples(resampler_r, samples + 1, frames, 2);
}

static void audio_resampler_deinit(void)
{
   if (resampler_l)
      blipper_free(resampler_l);

   if (resampler_r)
      blipper_free(resampler_r);

   resampler_l = NULL;
   resampler_r = NULL;

   audio_out_buffer_deinit();
}

static void audio_resampler_init(bool startup)
{
   if (use_cc_resampler)
      CC_init();
   else
   {
      resampler_l = blipper_new(32, 0.85, 6.5, 64, BLIP_BUFFER_SIZE, NULL);
      resampler_r = blipper_new(32, 0.85, 6.5, 64, BLIP_BUFFER_SIZE, NULL);

      /* It is possible for blipper_new() to fail,
       * must handle errors */
      if (!resampler_l || !resampler_r)
      {
         /* Display warning message */
         if (libretro_msg_interface_version >= 1)
         {
            struct retro_message_ext msg = {
               "Sinc resampler unsupported on this platform - using Cosine",
               2000,
               1,
               RETRO_LOG_WARN,
               RETRO_MESSAGE_TARGET_OSD,
               RETRO_MESSAGE_TYPE_NOTIFICATION,
               -1
            };
            environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg);
         }
         else
         {
            struct retro_message msg = {
               "Sinc resampler unsupported on this platform - using Cosine",
               120
            };
            environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
         }

         /* Force CC resampler */
         audio_resampler_deinit();
         use_cc_resampler = true;
         CC_init();

         /* Notify frontend of option value change */
         if (libretro_supports_set_variable)
         {
            struct retro_variable var = {0};
            var.key   = "gambatte_audio_resampler";
            var.value = "cc";
            environ_cb(RETRO_ENVIRONMENT_SET_VARIABLE, &var);
         }

         /* Notify frontend of sample rate change */
         if (!startup)
         {
            struct retro_system_av_info av_info;
            retro_get_system_av_info(&av_info);
            environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &av_info);
         }
      }
   }

   audio_out_buffer_init();
}

/***********************/
/* Audio Resampler END */
/***********************/

/***************************/
/* Palette Switching START */
/***************************/

static bool internal_palette_active    = false;
static size_t internal_palette_index   = 0;
static unsigned palette_switch_counter = 0;

/* Period in frames between palette switches
 * when holding RetroPad L/R */
#define PALETTE_SWITCH_PERIOD 30

/* Note: These must be updated if the internal
 * palette options in libretro_core_options.h
 * are changed
 * > We could count the palettes at runtime,
 *   but this adds unnecessary performance
 *   overheads and seems futile given that
 *   a number of other parameters must be
 *   hardcoded anyway... */
#define NUM_PALETTES_DEFAULT       51
#define NUM_PALETTES_TWB64_1      100
#define NUM_PALETTES_TWB64_2      100
#define NUM_PALETTES_TWB64_3      100
#define NUM_PALETTES_PIXELSHIFT_1  45
#define NUM_PALETTES_TOTAL        (NUM_PALETTES_DEFAULT + NUM_PALETTES_TWB64_1 + NUM_PALETTES_TWB64_2 + \
                                   NUM_PALETTES_TWB64_3 + NUM_PALETTES_PIXELSHIFT_1)

struct retro_core_option_value *palettes_default_opt_values      = NULL;
struct retro_core_option_value *palettes_twb64_1_opt_values      = NULL;
struct retro_core_option_value *palettes_twb64_2_opt_values      = NULL;
struct retro_core_option_value *palettes_twb64_3_opt_values      = NULL;
struct retro_core_option_value *palettes_pixelshift_1_opt_values = NULL;

static const char *internal_palette_labels[NUM_PALETTES_TOTAL] = {0};

static size_t *palettes_default_index_map      = NULL;
static size_t *palettes_twb64_1_index_map      = NULL;
static size_t *palettes_twb64_2_index_map      = NULL;
static size_t *palettes_twb64_3_index_map      = NULL;
static size_t *palettes_pixelshift_1_index_map = NULL;

static void parse_internal_palette_values(const char *key,
      struct retro_core_option_v2_definition *opt_defs_intl,
      size_t num_palettes, size_t palette_offset,
      struct retro_core_option_value **opt_values,
      size_t **index_map)
{
   size_t i;
   struct retro_core_option_v2_definition *opt_defs     = option_defs_us;
   struct retro_core_option_v2_definition *opt_def      = NULL;
   size_t label_index                                   = 0;
#ifndef HAVE_NO_LANGEXTRA
   struct retro_core_option_v2_definition *opt_def_intl = NULL;
#endif
   /* Find option corresponding to key */
   for (opt_def = opt_defs; !string_is_empty(opt_def->key); opt_def++)
      if (string_is_equal(opt_def->key, key))
         break;

   /* Cache option values array for fast access
    * when setting palette index */
   *opt_values = opt_def->values;

   /* Loop over all palette values for specified
    * option and:
    * - Generate palette index maps
    * - Fetch palette labels for notification
    *   purposes
    * Note: We perform no error checking here,
    * since we are operating on hardcoded structs
    * over which the core has full control */
   for (i = 0; i < num_palettes; i++)
   {
      const char *value       = opt_def->values[i].value;
      const char *value_label = NULL;

      /* Add entry to hash map
       * > Note that we have to set index+1, since
       *   a return value of 0 from RHMAP_GET_STR()
       *   indicates that the key string was not found */
      RHMAP_SET_STR((*index_map), value, i + 1);

      /* Check if we have a localised palette label */
#ifndef HAVE_NO_LANGEXTRA
      if (opt_defs_intl)
      {
         /* Find localised option corresponding to key */
         for (opt_def_intl = opt_defs_intl;
              !string_is_empty(opt_def_intl->key);
              opt_def_intl++)
         {
            if (string_is_equal(opt_def_intl->key, key))
            {
               size_t j = 0;

               /* Search for current option value */
               for (;;)
               {
                  const char *value_intl = opt_def_intl->values[j].value;

                  if (string_is_empty(value_intl))
                     break;

                  if (string_is_equal(value, value_intl))
                  {
                     /* We have a match; fetch localised label */
                     value_label = opt_def_intl->values[j].label;
                     break;
                  }

                  j++;
               }

               break;
            }
         }
      }
#endif
      /* If localised palette label is unset,
       * use value itself from option_defs_us */
      if (!value_label)
         value_label = value;

      /* Cache label for 'consolidated' palette index */
      internal_palette_labels[palette_offset + label_index++] = value_label;
   }
}

static void init_palette_switch(void)
{
   struct retro_core_option_v2_definition *opt_defs_intl = NULL;
#ifndef HAVE_NO_LANGEXTRA
   unsigned language                                     = 0;
#endif

   libretro_supports_set_variable = false;
   if (environ_cb(RETRO_ENVIRONMENT_SET_VARIABLE, NULL))
      libretro_supports_set_variable = true;

   libretro_msg_interface_version = 0;
   environ_cb(RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION,
         &libretro_msg_interface_version);

   internal_palette_active = false;
   internal_palette_index  = 0;
   palette_switch_counter  = 0;

#ifndef HAVE_NO_LANGEXTRA
   if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
       (language < RETRO_LANGUAGE_LAST) &&
       (language != RETRO_LANGUAGE_ENGLISH) &&
       options_intl[language])
      opt_defs_intl = options_intl[language]->definitions;
#endif

   /* Parse palette values for each palette group
    * > Default palettes */
   parse_internal_palette_values("gambatte_gb_internal_palette",
         opt_defs_intl, NUM_PALETTES_DEFAULT,
         0,
         &palettes_default_opt_values,
         &palettes_default_index_map);
   /* > TWB64 Pack 1 palettes */
   parse_internal_palette_values("gambatte_gb_palette_twb64_1",
         opt_defs_intl, NUM_PALETTES_TWB64_1,
         NUM_PALETTES_DEFAULT,
         &palettes_twb64_1_opt_values,
         &palettes_twb64_1_index_map);
   /* > TWB64 Pack 2 palettes */
   parse_internal_palette_values("gambatte_gb_palette_twb64_2",
         opt_defs_intl, NUM_PALETTES_TWB64_2,
         NUM_PALETTES_DEFAULT + NUM_PALETTES_TWB64_1,
         &palettes_twb64_2_opt_values,
         &palettes_twb64_2_index_map);
   /* > TWB64 Pack 3 palettes */
   parse_internal_palette_values("gambatte_gb_palette_twb64_3",
         opt_defs_intl, NUM_PALETTES_TWB64_3,
         NUM_PALETTES_DEFAULT + NUM_PALETTES_TWB64_1 + NUM_PALETTES_TWB64_2,
         &palettes_twb64_3_opt_values,
         &palettes_twb64_3_index_map);
   /* > PixelShift - Pack 1 palettes */
   parse_internal_palette_values("gambatte_gb_palette_pixelshift_1",
         opt_defs_intl, NUM_PALETTES_PIXELSHIFT_1,
         NUM_PALETTES_DEFAULT + NUM_PALETTES_TWB64_1 + NUM_PALETTES_TWB64_2 +
         NUM_PALETTES_TWB64_3,
         &palettes_pixelshift_1_opt_values,
         &palettes_pixelshift_1_index_map);
}

static void deinit_palette_switch(void)
{
   libretro_supports_set_variable   = false;
   libretro_msg_interface_version   = 0;
   internal_palette_active          = false;
   internal_palette_index           = 0;
   palette_switch_counter           = 0;
   palettes_default_opt_values      = NULL;
   palettes_twb64_1_opt_values      = NULL;
   palettes_twb64_2_opt_values      = NULL;
   palettes_twb64_3_opt_values      = NULL;
   palettes_pixelshift_1_opt_values = NULL;

   RHMAP_FREE(palettes_default_index_map);
   RHMAP_FREE(palettes_twb64_1_index_map);
   RHMAP_FREE(palettes_twb64_2_index_map);
   RHMAP_FREE(palettes_twb64_3_index_map);
   RHMAP_FREE(palettes_pixelshift_1_index_map);
}

static void palette_switch_set_index(size_t palette_index)
{
   const char *palettes_default_value = NULL;
   const char *palettes_ext_key       = NULL;
   const char *palettes_ext_value     = NULL;
   size_t opt_index                   = 0;
   struct retro_variable var          = {0};

   if (palette_index >= NUM_PALETTES_TOTAL)
      palette_index = NUM_PALETTES_TOTAL - 1;

   /* Check which palette group the specified
    * index corresponds to */
   if (palette_index < NUM_PALETTES_DEFAULT)
   {
      /* This is a palette from the default group */
      opt_index              = palette_index;
      palettes_default_value = palettes_default_opt_values[opt_index].value;
   }
   else if (palette_index <
         NUM_PALETTES_DEFAULT + NUM_PALETTES_TWB64_1)
   {
      /* This is a palette from the TWB64 Pack 1 group */
      palettes_default_value = "TWB64 - Pack 1";

      opt_index              = palette_index - NUM_PALETTES_DEFAULT;
      palettes_ext_key       = "gambatte_gb_palette_twb64_1";
      palettes_ext_value     = palettes_twb64_1_opt_values[opt_index].value;
   }
   else if (palette_index <
         NUM_PALETTES_DEFAULT + NUM_PALETTES_TWB64_1 + NUM_PALETTES_TWB64_2)
   {
      /* This is a palette from the TWB64 Pack 2 group */
      palettes_default_value = "TWB64 - Pack 2";

      opt_index              = palette_index -
            (NUM_PALETTES_DEFAULT + NUM_PALETTES_TWB64_1);
      palettes_ext_key       = "gambatte_gb_palette_twb64_2";
      palettes_ext_value     = palettes_twb64_2_opt_values[opt_index].value;
   }
   else if (palette_index <
         NUM_PALETTES_DEFAULT + NUM_PALETTES_TWB64_1 + NUM_PALETTES_TWB64_2 +
         NUM_PALETTES_TWB64_3)
   {
      /* This is a palette from the TWB64 Pack 3 group */
      palettes_default_value = "TWB64 - Pack 3";

      opt_index              = palette_index -
            (NUM_PALETTES_DEFAULT + NUM_PALETTES_TWB64_1 + NUM_PALETTES_TWB64_2);
      palettes_ext_key       = "gambatte_gb_palette_twb64_3";
      palettes_ext_value     = palettes_twb64_3_opt_values[opt_index].value;
   }
   else
   {
      /* This is a palette from the PixelShift Pack 1 group */
      palettes_default_value = "PixelShift - Pack 1";

      opt_index              = palette_index -
            (NUM_PALETTES_DEFAULT + NUM_PALETTES_TWB64_1 + NUM_PALETTES_TWB64_2 +
             NUM_PALETTES_TWB64_3);
      palettes_ext_key       = "gambatte_gb_palette_pixelshift_1";
      palettes_ext_value     = palettes_pixelshift_1_opt_values[opt_index].value;
   }

   /* Notify frontend of option value changes */
   var.key   = "gambatte_gb_internal_palette";
   var.value = palettes_default_value;
   environ_cb(RETRO_ENVIRONMENT_SET_VARIABLE, &var);

   if (palettes_ext_key)
   {
      var.key   = palettes_ext_key;
      var.value = palettes_ext_value;
      environ_cb(RETRO_ENVIRONMENT_SET_VARIABLE, &var);
   }

   /* Display notification message */
   if (libretro_msg_interface_version >= 1)
   {
      struct retro_message_ext msg = {
         internal_palette_labels[palette_index],
         2000,
         1,
         RETRO_LOG_INFO,
         RETRO_MESSAGE_TARGET_OSD,
         RETRO_MESSAGE_TYPE_NOTIFICATION_ALT,
         -1
      };
      environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE_EXT, &msg);
   }
   else
   {
      struct retro_message msg = {
         internal_palette_labels[palette_index],
         120
      };
      environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
   }
}

/*************************/
/* Palette Switching END */
/*************************/

/*****************************/
/* Interframe blending START */
/*****************************/

#define LCD_RESPONSE_TIME 0.333f
/* > 'LCD Ghosting (Fast)' method does not
 *   correctly interpret the set response time,
 *   leading to an artificially subdued blur effect.
 *   We have to compensate for this by increasing
 *   the response time, hence this 'fake' value */
#define LCD_RESPONSE_TIME_FAKE 0.5f

enum frame_blend_method
{
   FRAME_BLEND_NONE = 0,
   FRAME_BLEND_MIX,
   FRAME_BLEND_LCD_GHOSTING,
   FRAME_BLEND_LCD_GHOSTING_FAST
};

static enum frame_blend_method frame_blend_type  = FRAME_BLEND_NONE;
static gambatte::video_pixel_t* video_buf_prev_1 = NULL;
static gambatte::video_pixel_t* video_buf_prev_2 = NULL;
static gambatte::video_pixel_t* video_buf_prev_3 = NULL;
static gambatte::video_pixel_t* video_buf_prev_4 = NULL;
static float* video_buf_acc_r                    = NULL;
static float* video_buf_acc_g                    = NULL;
static float* video_buf_acc_b                    = NULL;
static float frame_blend_response[4]             = {0.0f};
static bool frame_blend_response_set             = false;
static void (*blend_frames)(void)                = NULL;

/* > Note: The individual frame blending functions
 *   are somewhat WET (Write Everything Twice), in that
 *   we duplicate the entire nested for loop.
 *   This code is performance-critical, so we want to
 *   minimise logic in the inner loops where possible  */
static void blend_frames_mix(void)
{
   gambatte::video_pixel_t *curr = video_buf;
   gambatte::video_pixel_t *prev = video_buf_prev_1;
   size_t x, y;

   for (y = 0; y < VIDEO_HEIGHT; y++)
   {
      for (x = 0; x < VIDEO_WIDTH; x++)
      {
         /* Get colours from current + previous frames */
         gambatte::video_pixel_t rgb_curr = *(curr + x);
         gambatte::video_pixel_t rgb_prev = *(prev + x);

         /* Store colours for next frame */
         *(prev + x) = rgb_curr;

         /* Mix colours
          * > "Mixing Packed RGB Pixels Efficiently"
          *   http://blargg.8bitalley.com/info/rgb_mixing.html */
#ifdef VIDEO_RGB565
         *(curr + x) = (rgb_curr + rgb_prev + ((rgb_curr ^ rgb_prev) & 0x821)) >> 1;
#elif defined(VIDEO_ABGR1555)
         *(curr + x) = (rgb_curr + rgb_prev + ((rgb_curr ^ rgb_prev) & 0x521)) >> 1;
#else
         *(curr + x) = (rgb_curr + rgb_prev + ((rgb_curr ^ rgb_prev) & 0x10101)) >> 1;
#endif
      }

      curr += VIDEO_PITCH;
      prev += VIDEO_PITCH;
   }
}

static void blend_frames_lcd_ghost(void)
{
   gambatte::video_pixel_t *curr   = video_buf;
   gambatte::video_pixel_t *prev_1 = video_buf_prev_1;
   gambatte::video_pixel_t *prev_2 = video_buf_prev_2;
   gambatte::video_pixel_t *prev_3 = video_buf_prev_3;
   gambatte::video_pixel_t *prev_4 = video_buf_prev_4;
   float *response                 = frame_blend_response;
   size_t x, y;

   for (y = 0; y < VIDEO_HEIGHT; y++)
   {
      for (x = 0; x < VIDEO_WIDTH; x++)
      {
         /* Get colours from current + previous frames */
         gambatte::video_pixel_t rgb_curr   = *(curr + x);
         gambatte::video_pixel_t rgb_prev_1 = *(prev_1 + x);
         gambatte::video_pixel_t rgb_prev_2 = *(prev_2 + x);
         gambatte::video_pixel_t rgb_prev_3 = *(prev_3 + x);
         gambatte::video_pixel_t rgb_prev_4 = *(prev_4 + x);

         /* Store colours for next frame */
         *(prev_1 + x) = rgb_curr;
         *(prev_2 + x) = rgb_prev_1;
         *(prev_3 + x) = rgb_prev_2;
         *(prev_4 + x) = rgb_prev_3;

         /* Unpack colours and convert to float */
#ifdef VIDEO_RGB565
         float r_curr = static_cast<float>(rgb_curr >> 11 & 0x1F);
         float g_curr = static_cast<float>(rgb_curr >>  6 & 0x1F);
         float b_curr = static_cast<float>(rgb_curr       & 0x1F);

         float r_prev_1 = static_cast<float>(rgb_prev_1 >> 11 & 0x1F);
         float g_prev_1 = static_cast<float>(rgb_prev_1 >>  6 & 0x1F);
         float b_prev_1 = static_cast<float>(rgb_prev_1       & 0x1F);

         float r_prev_2 = static_cast<float>(rgb_prev_2 >> 11 & 0x1F);
         float g_prev_2 = static_cast<float>(rgb_prev_2 >>  6 & 0x1F);
         float b_prev_2 = static_cast<float>(rgb_prev_2       & 0x1F);

         float r_prev_3 = static_cast<float>(rgb_prev_3 >> 11 & 0x1F);
         float g_prev_3 = static_cast<float>(rgb_prev_3 >>  6 & 0x1F);
         float b_prev_3 = static_cast<float>(rgb_prev_3       & 0x1F);

         float r_prev_4 = static_cast<float>(rgb_prev_4 >> 11 & 0x1F);
         float g_prev_4 = static_cast<float>(rgb_prev_4 >>  6 & 0x1F);
         float b_prev_4 = static_cast<float>(rgb_prev_4       & 0x1F);
#elif defined(VIDEO_ABGR1555)
         float r_curr = static_cast<float>(rgb_curr       & 0x1F);
         float g_curr = static_cast<float>(rgb_curr >>  5 & 0x1F);
         float b_curr = static_cast<float>(rgb_curr >> 10 & 0x1F);

         float r_prev_1 = static_cast<float>(rgb_prev_1       & 0x1F);
         float g_prev_1 = static_cast<float>(rgb_prev_1 >>  5 & 0x1F);
         float b_prev_1 = static_cast<float>(rgb_prev_1 >> 10 & 0x1F);

         float r_prev_2 = static_cast<float>(rgb_prev_2       & 0x1F);
         float g_prev_2 = static_cast<float>(rgb_prev_2 >>  5 & 0x1F);
         float b_prev_2 = static_cast<float>(rgb_prev_2 >> 10 & 0x1F);

         float r_prev_3 = static_cast<float>(rgb_prev_3       & 0x1F);
         float g_prev_3 = static_cast<float>(rgb_prev_3 >>  5 & 0x1F);
         float b_prev_3 = static_cast<float>(rgb_prev_3 >> 10 & 0x1F);

         float r_prev_4 = static_cast<float>(rgb_prev_4       & 0x1F);
         float g_prev_4 = static_cast<float>(rgb_prev_4 >>  5 & 0x1F);
         float b_prev_4 = static_cast<float>(rgb_prev_4 >> 10 & 0x1F);
#else
         float r_curr = static_cast<float>(rgb_curr >> 16 & 0x1F);
         float g_curr = static_cast<float>(rgb_curr >>  8 & 0x1F);
         float b_curr = static_cast<float>(rgb_curr       & 0x1F);

         float r_prev_1 = static_cast<float>(rgb_prev_1 >> 16 & 0x1F);
         float g_prev_1 = static_cast<float>(rgb_prev_1 >>  8 & 0x1F);
         float b_prev_1 = static_cast<float>(rgb_prev_1       & 0x1F);

         float r_prev_2 = static_cast<float>(rgb_prev_2 >> 16 & 0x1F);
         float g_prev_2 = static_cast<float>(rgb_prev_2 >>  8 & 0x1F);
         float b_prev_2 = static_cast<float>(rgb_prev_2       & 0x1F);

         float r_prev_3 = static_cast<float>(rgb_prev_3 >> 16 & 0x1F);
         float g_prev_3 = static_cast<float>(rgb_prev_3 >>  8 & 0x1F);
         float b_prev_3 = static_cast<float>(rgb_prev_3       & 0x1F);

         float r_prev_4 = static_cast<float>(rgb_prev_4 >> 16 & 0x1F);
         float g_prev_4 = static_cast<float>(rgb_prev_4 >>  8 & 0x1F);
         float b_prev_4 = static_cast<float>(rgb_prev_4       & 0x1F);
#endif
         /* Mix colours for current frame and convert back to video_pixel_t
          * > Response time effect implemented via an exponential
          *   drop-off algorithm, taken from the 'Gameboy Classic Shader'
          *   by Harlequin:
          *      https://github.com/libretro/glsl-shaders/blob/master/handheld/shaders/gameboy/shader-files/gb-pass0.glsl */
         r_curr += (r_prev_1 - r_curr) * *response;
         r_curr += (r_prev_2 - r_curr) * *(response + 1);
         r_curr += (r_prev_3 - r_curr) * *(response + 2);
         r_curr += (r_prev_4 - r_curr) * *(response + 3);
         gambatte::video_pixel_t r_mix = static_cast<gambatte::video_pixel_t>(r_curr + 0.5f) & 0x1F;

         g_curr += (g_prev_1 - g_curr) * *response;
         g_curr += (g_prev_2 - g_curr) * *(response + 1);
         g_curr += (g_prev_3 - g_curr) * *(response + 2);
         g_curr += (g_prev_4 - g_curr) * *(response + 3);
         gambatte::video_pixel_t g_mix = static_cast<gambatte::video_pixel_t>(g_curr + 0.5f) & 0x1F;

         b_curr += (b_prev_1 - b_curr) * *response;
         b_curr += (b_prev_2 - b_curr) * *(response + 1);
         b_curr += (b_prev_3 - b_curr) * *(response + 2);
         b_curr += (b_prev_4 - b_curr) * *(response + 3);
         gambatte::video_pixel_t b_mix = static_cast<gambatte::video_pixel_t>(b_curr + 0.5f) & 0x1F;

         /* Repack colours for current frame */
#ifdef VIDEO_RGB565
         *(curr + x) = r_mix << 11 | g_mix << 6 | b_mix;
#elif defined(VIDEO_ABGR1555)
         *(curr + x) = b_mix << 10 | g_mix << 5 | b_mix;
#else
         *(curr + x) = r_mix << 16 | g_mix << 8 | b_mix;
#endif
      }

      curr   += VIDEO_PITCH;
      prev_1 += VIDEO_PITCH;
      prev_2 += VIDEO_PITCH;
      prev_3 += VIDEO_PITCH;
      prev_4 += VIDEO_PITCH;
   }
}

static void blend_frames_lcd_ghost_fast(void)
{
   gambatte::video_pixel_t *curr = video_buf;
   float *prev_r                 = video_buf_acc_r;
   float *prev_g                 = video_buf_acc_g;
   float *prev_b                 = video_buf_acc_b;
   size_t x, y;

   for (y = 0; y < VIDEO_HEIGHT; y++)
   {
      for (x = 0; x < VIDEO_WIDTH; x++)
      {
         /* Get colours from current + previous frames */
         gambatte::video_pixel_t rgb_curr   = *(curr + x);
         float r_prev                       = *(prev_r + x);
         float g_prev                       = *(prev_g + x);
         float b_prev                       = *(prev_b + x);

         /* Unpack current colours and convert to float */
#ifdef VIDEO_RGB565
         float r_curr = static_cast<float>(rgb_curr >> 11 & 0x1F);
         float g_curr = static_cast<float>(rgb_curr >>  6 & 0x1F);
         float b_curr = static_cast<float>(rgb_curr       & 0x1F);
#elif defined(VIDEO_ABGR1555)
         float r_curr = static_cast<float>(rgb_curr       & 0x1F);
         float g_curr = static_cast<float>(rgb_curr >>  5 & 0x1F);
         float b_curr = static_cast<float>(rgb_curr >> 10 & 0x1F);
#else
         float r_curr = static_cast<float>(rgb_curr >> 16 & 0x1F);
         float g_curr = static_cast<float>(rgb_curr >>  8 & 0x1F);
         float b_curr = static_cast<float>(rgb_curr       & 0x1F);
#endif
         /* Mix colours for current frame */
         float r_mix = (r_curr * (1.0f - LCD_RESPONSE_TIME_FAKE)) + (LCD_RESPONSE_TIME_FAKE * r_prev);
         float g_mix = (g_curr * (1.0f - LCD_RESPONSE_TIME_FAKE)) + (LCD_RESPONSE_TIME_FAKE * g_prev);
         float b_mix = (b_curr * (1.0f - LCD_RESPONSE_TIME_FAKE)) + (LCD_RESPONSE_TIME_FAKE * b_prev);

         /* Store colours for next frame */
         *(prev_r + x) = r_mix;
         *(prev_g + x) = g_mix;
         *(prev_b + x) = b_mix;

         /* Convert, repack and assign colours for current frame */
#ifdef VIDEO_RGB565
         *(curr + x) =   (static_cast<gambatte::video_pixel_t>(r_mix + 0.5f) & 0x1F) << 11
                       | (static_cast<gambatte::video_pixel_t>(g_mix + 0.5f) & 0x1F) << 6
                       | (static_cast<gambatte::video_pixel_t>(b_mix + 0.5f) & 0x1F);
#elif defined(ABGR1555)
         *(curr + x) =   (static_cast<gambatte::video_pixel_t>(r_mix + 0.5f) & 0x1F)
                       | (static_cast<gambatte::video_pixel_t>(g_mix + 0.5f) & 0x1F) << 5
                       | (static_cast<gambatte::video_pixel_t>(b_mix + 0.5f) & 0x1F) << 10;
#else
         *(curr + x) =   (static_cast<gambatte::video_pixel_t>(r_mix + 0.5f) & 0x1F) << 16
                       | (static_cast<gambatte::video_pixel_t>(g_mix + 0.5f) & 0x1F) << 8
                       | (static_cast<gambatte::video_pixel_t>(b_mix + 0.5f) & 0x1F);
#endif
      }

      curr   += VIDEO_PITCH;
      prev_r += VIDEO_PITCH;
      prev_g += VIDEO_PITCH;
      prev_b += VIDEO_PITCH;
   }
}

static bool allocate_video_buf_prev(gambatte::video_pixel_t** buf)
{
   if (!*buf)
   {
      *buf = (gambatte::video_pixel_t*)malloc(VIDEO_BUFF_SIZE);
      if (!*buf)
         return false;
   }
   memset(*buf, 0, VIDEO_BUFF_SIZE);
   return true;
}

static bool allocate_video_buf_acc(void)
{
   size_t i;
   size_t buf_size = 256 * NUM_GAMEBOYS * VIDEO_HEIGHT * sizeof(float);

   if (!video_buf_acc_r)
   {
      video_buf_acc_r = (float*)malloc(buf_size);
      if (!video_buf_acc_r)
         return false;
   }

   if (!video_buf_acc_g)
   {
      video_buf_acc_g = (float*)malloc(buf_size);
      if (!video_buf_acc_g)
         return false;
   }

   if (!video_buf_acc_b)
   {
      video_buf_acc_b = (float*)malloc(buf_size);
      if (!video_buf_acc_b)
         return false;
   }

   /* Cannot use memset() on arrays of floats... */
   for (i = 0; i < (256 * NUM_GAMEBOYS * VIDEO_HEIGHT); i++)
   {
      video_buf_acc_r[i] = 0.0f;
      video_buf_acc_g[i] = 0.0f;
      video_buf_acc_b[i] = 0.0f;
   }
   return true;
}

static void init_frame_blending(void)
{
   blend_frames = NULL;

   /* Allocate interframe blending buffers, as required
    * NOTE: In all cases, any used buffers are 'reset'
    * to avoid drawing garbage on the next frame */
   switch (frame_blend_type)
   {
      case FRAME_BLEND_MIX:
         /* Simple 50:50 blending requires a single buffer */
         if (!allocate_video_buf_prev(&video_buf_prev_1))
            return;
         break;
      case FRAME_BLEND_LCD_GHOSTING:
         /* 'Accurate' LCD ghosting requires four buffers */
         if (!allocate_video_buf_prev(&video_buf_prev_1))
            return;
         if (!allocate_video_buf_prev(&video_buf_prev_2))
            return;
         if (!allocate_video_buf_prev(&video_buf_prev_3))
            return;
         if (!allocate_video_buf_prev(&video_buf_prev_4))
            return;
         break;
      case FRAME_BLEND_LCD_GHOSTING_FAST:
         /* 'Fast' LCD ghosting requires three (RGB)
          * 'accumulator' buffers */
         if (!allocate_video_buf_acc())
            return;
         break;
      case FRAME_BLEND_NONE:
      default:
         /* Error condition - cannot happen
          * > Just leave blend_frames() function set to NULL */
         return;
   }

   /* Set LCD ghosting response time factors,
    * if required */
   if ((frame_blend_type == FRAME_BLEND_LCD_GHOSTING) &&
       !frame_blend_response_set)
   {
      /* For the default response time of 0.333,
       * only four previous samples are required
       * since the response factor for the fifth
       * is:
       *    pow(LCD_RESPONSE_TIME, 5.0f) -> 0.00409
       * ...which is less than half a percent, and
       * therefore irrelevant.
       * If the response time were significantly
       * increased, we may need to rethink this
       * (but more samples == greater performance
       * overheads) */
      frame_blend_response[0] = LCD_RESPONSE_TIME;
      frame_blend_response[1] = std::pow(LCD_RESPONSE_TIME, 2.0f);
      frame_blend_response[2] = std::pow(LCD_RESPONSE_TIME, 3.0f);
      frame_blend_response[3] = std::pow(LCD_RESPONSE_TIME, 4.0f);

      frame_blend_response_set = true;
   }

   /* Assign frame blending function */
   switch (frame_blend_type)
   {
      case FRAME_BLEND_MIX:
         blend_frames = blend_frames_mix;
         return;
      case FRAME_BLEND_LCD_GHOSTING:
         blend_frames = blend_frames_lcd_ghost;
         return;
      case FRAME_BLEND_LCD_GHOSTING_FAST:
         blend_frames = blend_frames_lcd_ghost_fast;
         return;
      case FRAME_BLEND_NONE:
      default:
         /* Error condition - cannot happen
          * > Just leave blend_frames() function set to NULL */
         return;
   }
}

static void deinit_frame_blending(void)
{
   if (video_buf_prev_1)
   {
      free(video_buf_prev_1);
      video_buf_prev_1 = NULL;
   }

   if (video_buf_prev_2)
   {
      free(video_buf_prev_2);
      video_buf_prev_2 = NULL;
   }

   if (video_buf_prev_3)
   {
      free(video_buf_prev_3);
      video_buf_prev_3 = NULL;
   }

   if (video_buf_prev_4)
   {
      free(video_buf_prev_4);
      video_buf_prev_4 = NULL;
   }

   if (video_buf_acc_r)
   {
      free(video_buf_acc_r);
      video_buf_acc_r = NULL;
   }

   if (video_buf_acc_g)
   {
      free(video_buf_acc_g);
      video_buf_acc_g = NULL;
   }

   if (video_buf_acc_b)
   {
      free(video_buf_acc_b);
      video_buf_acc_b = NULL;
   }

   frame_blend_type         = FRAME_BLEND_NONE;
   frame_blend_response_set = false;
}

static void check_frame_blend_variable(void)
{
   struct retro_variable var;
   enum frame_blend_method old_frame_blend_type = frame_blend_type;

   frame_blend_type = FRAME_BLEND_NONE;

   var.key = "gambatte_mix_frames";
   var.value = 0;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "mix"))
         frame_blend_type = FRAME_BLEND_MIX;
      else if (!strcmp(var.value, "lcd_ghosting"))
         frame_blend_type = FRAME_BLEND_LCD_GHOSTING;
      else if (!strcmp(var.value, "lcd_ghosting_fast"))
         frame_blend_type = FRAME_BLEND_LCD_GHOSTING_FAST;
   }

   if (frame_blend_type == FRAME_BLEND_NONE)
      blend_frames = NULL;
   else if (frame_blend_type != old_frame_blend_type)
      init_frame_blending();
}

/***************************/
/* Interframe blending END */
/***************************/

/************************/
/* Rumble support START */
/************************/

static struct retro_rumble_interface rumble = {0};
static uint16_t rumble_strength_last        = 0;
static uint16_t rumble_strength_up          = 0;
static uint16_t rumble_strength_down        = 0;
static uint16_t rumble_level                = 0;
static bool rumble_active                   = false;

void cartridge_set_rumble(unsigned active)
{
   if (!rumble.set_rumble_state ||
       !rumble_level)
      return;

   if (active)
      rumble_strength_up++;
   else
      rumble_strength_down++;

   rumble_active = true;
}

static void apply_rumble(void)
{
   uint16_t strength;

   if (!rumble.set_rumble_state ||
       !rumble_level)
      return;

   strength = (rumble_strength_up > 0) ?
         (rumble_strength_up * rumble_level) /
               (rumble_strength_up + rumble_strength_down) : 0;

   rumble_strength_up   = 0;
   rumble_strength_down = 0;

   if (strength == rumble_strength_last)
      return;

   rumble.set_rumble_state(0, RETRO_RUMBLE_WEAK, strength);
   rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, strength);

   rumble_strength_last = strength;
}

static void deactivate_rumble(void)
{
   rumble_strength_up   = 0;
   rumble_strength_down = 0;
   rumble_active        = false;

   if (!rumble.set_rumble_state ||
       (rumble_strength_last == 0))
      return;

   rumble.set_rumble_state(0, RETRO_RUMBLE_WEAK, 0);
   rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, 0);

   rumble_strength_last = 0;
}

/**********************/
/* Rumble support END */
/**********************/

#ifdef HAVE_NETWORK
/* Core options 'update display' callback */
static bool update_option_visibility(void)
{
   struct retro_variable var = {0};
   bool updated              = false;
   unsigned i;

   /* If frontend supports core option categories,
    * then gambatte_show_gb_link_settings is ignored
    * and no options should be hidden */
   if (libretro_supports_option_categories)
      return false;

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

         updated = true;
      }
   }

   return updated;
}
#endif

/* Fast forward override */
static void set_fastforward_override(bool fastforward)
{
   struct retro_fastforwarding_override ff_override;

   if (!libretro_supports_ff_override)
      return;

   ff_override.ratio        = -1.0f;
   ff_override.notification = true;

   if (fastforward)
   {
      ff_override.fastforward    = true;
      ff_override.inhibit_toggle = true;
   }
   else
   {
      ff_override.fastforward    = false;
      ff_override.inhibit_toggle = false;
   }

   environ_cb(RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE, &ff_override);
}

static bool file_present_in_system(const char *fname)
{
   const char *system_dir = NULL;
   char full_path[PATH_MAX_LENGTH];

   full_path[0] = '\0';

   if (string_is_empty(fname))
      return false;

   /* Get system directory */
   if (!environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) ||
       !system_dir)
   {
      gambatte_log(RETRO_LOG_WARN,
            "No system directory defined, unable to look for '%s'.\n", fname);
      return false;
   }

   fill_pathname_join(full_path, system_dir,
         fname, sizeof(full_path));

   return path_is_valid(full_path);
}

static bool get_bootloader_from_file(void* userdata, bool isgbc, uint8_t* data, uint32_t buf_size)
{
   const char *system_dir = NULL;
   const char *bios_name  = NULL;
   RFILE *bios_file       = NULL;
   int64_t bios_size      = 0;
   int64_t bytes_read     = 0;
   char bios_path[PATH_MAX_LENGTH];

   bios_path[0] = '\0';

   if (!use_official_bootloader)
      return false;

   /* Get system directory */
   if (!environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) ||
       !system_dir)
   {
      gambatte_log(RETRO_LOG_WARN,
            "No system directory defined, unable to look for bootloader.\n");
      return false;
   }

   /* Get BIOS type */
   if (isgbc)
   {
      bios_name = "gbc_bios.bin";
      bios_size = 0x900;
   }
   else
   {
      bios_name = "gb_bios.bin";
      bios_size = 0x100;
   }

   if (bios_size > buf_size)
      return false;

   /* Get BIOS path */
   fill_pathname_join(bios_path, system_dir,
         bios_name, sizeof(bios_path));

   /* Read BIOS file */
   bios_file = filestream_open(bios_path,
         RETRO_VFS_FILE_ACCESS_READ,
         RETRO_VFS_FILE_ACCESS_HINT_NONE);

   if (!bios_file)
      return false;

   bytes_read = filestream_read(bios_file,
         data, bios_size);
   filestream_close(bios_file);

   if (bytes_read != bios_size)
      return false;

   gambatte_log(RETRO_LOG_INFO, "Read bootloader: %s\n", bios_path);

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

static void update_input_state(void)
{
   unsigned i;
   unsigned res                = 0;
   bool turbo_a                = false;
   bool turbo_b                = false;
   bool palette_prev           = false;
   bool palette_next           = false;
   bool palette_switch_enabled = (libretro_supports_set_variable &&
         internal_palette_active);

   if (libretro_supports_bitmasks)
   {
      int16_t ret = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
      for (i = 0; i < sizeof(input::btn_map) / sizeof(input::map); i++)
         res |= (ret & (1 << input::btn_map[i].snes)) ? input::btn_map[i].gb : 0;

      libretro_ff_enabled = libretro_supports_ff_override &&
            (ret & (1 << RETRO_DEVICE_ID_JOYPAD_R2));

      turbo_a = (ret & (1 << RETRO_DEVICE_ID_JOYPAD_X));
      turbo_b = (ret & (1 << RETRO_DEVICE_ID_JOYPAD_Y));

      if (palette_switch_enabled)
      {
         palette_prev = (bool)(ret & (1 << RETRO_DEVICE_ID_JOYPAD_L));
         palette_next = (bool)(ret & (1 << RETRO_DEVICE_ID_JOYPAD_R));
      }
   }
   else
   {
      for (i = 0; i < sizeof(input::btn_map) / sizeof(input::map); i++)
         res |= input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, input::btn_map[i].snes) ? input::btn_map[i].gb : 0;

      libretro_ff_enabled = libretro_supports_ff_override &&
            input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);

      turbo_a = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
      turbo_b = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);

      if (palette_switch_enabled)
      {
         palette_prev = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L);
         palette_next = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R);
      }
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

   /* Handle fast forward button */
   if (libretro_ff_enabled != libretro_ff_enabled_prev)
   {
      set_fastforward_override(libretro_ff_enabled);
      libretro_ff_enabled_prev = libretro_ff_enabled;
   }

   /* Handle turbo buttons */
   if (turbo_a)
   {
      res |= (turbo_a_counter < turbo_pulse_width) ?
            gambatte::InputGetter::A : 0;

      turbo_a_counter++;
      if (turbo_a_counter >= turbo_period)
         turbo_a_counter = 0;
   }
   else
      turbo_a_counter = 0;

   if (turbo_b)
   {
      res |= (turbo_b_counter < turbo_pulse_width) ?
            gambatte::InputGetter::B : 0;

      turbo_b_counter++;
      if (turbo_b_counter >= turbo_period)
         turbo_b_counter = 0;
   }
   else
      turbo_b_counter = 0;

   /* Handle internal palette switching */
   if (palette_prev || palette_next)
   {
      if (palette_switch_counter == 0)
      {
         size_t palette_index = internal_palette_index;

         if (palette_prev)
         {
            if (palette_index > 0)
               palette_index--;
            else
               palette_index = NUM_PALETTES_TOTAL - 1;
         }
         else /* palette_next */
         {
            if (palette_index < NUM_PALETTES_TOTAL - 1)
               palette_index++;
            else
               palette_index = 0;
         }

         palette_switch_set_index(palette_index);
      }

      palette_switch_counter++;
      if (palette_switch_counter >= PALETTE_SWITCH_PERIOD)
         palette_switch_counter = 0;
   }
   else
      palette_switch_counter = 0;

   libretro_input_state = res;
}

/* gb_input is called multiple times per frame.
 * Determine input state once per frame using
 * update_input_state(), and simply return
 * cached value here */
class SNESInput : public gambatte::InputGetter
{
   public:
      unsigned operator()()
      {
         return libretro_input_state;
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

void retro_get_system_info(struct retro_system_info *info)
{
   info->library_name = "Gambatte";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
#ifdef HAVE_NETWORK
   info->library_version = "v0.5.0-netlink" GIT_VERSION;
#else
   info->library_version = "v0.5.0" GIT_VERSION;
#endif
   info->need_fullpath = false;
   info->block_extract = false;
   info->valid_extensions = "gb|gbc|dmg";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{

   info->geometry.base_width   = VIDEO_WIDTH;
   info->geometry.base_height  = VIDEO_HEIGHT;
   info->geometry.max_width    = VIDEO_WIDTH;
   info->geometry.max_height   = VIDEO_HEIGHT;
   info->geometry.aspect_ratio = (float)GB_SCREEN_WIDTH / (float)VIDEO_HEIGHT;

   info->timing.fps            = VIDEO_REFRESH_RATE;
   info->timing.sample_rate    = use_cc_resampler ?
         SOUND_SAMPLE_RATE_CC : SOUND_SAMPLE_RATE_BLIPPER;
}

static void check_system_specs(void)
{
   unsigned level = 4;
   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

void retro_init(void)
{
   struct retro_log_callback log;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      gambatte_log_set_cb(log.log);
   else
      gambatte_log_set_cb(NULL);

   // Using uint_least32_t in an audio interface expecting you to cast to short*? :( Weird stuff.
   assert(sizeof(gambatte::uint_least32_t) == sizeof(uint32_t));
   gb.setInputGetter(&gb_input);
#ifdef DUAL_MODE
   gb2.setInputGetter(&gb_input);
#endif

#ifdef _3DS
   video_buf = (gambatte::video_pixel_t*)linearMemAlign(VIDEO_BUFF_SIZE, 128);
#else
   video_buf = (gambatte::video_pixel_t*)malloc(VIDEO_BUFF_SIZE);
#endif

   check_system_specs();
   
   //gb/gbc bootloader support
   gb.setBootloaderGetter(get_bootloader_from_file);
#ifdef DUAL_MODE
   gb2.setBootloaderGetter(get_bootloader_from_file);
#endif

   // Initialise internal palette maps
   initPaletteMaps();

   // Initialise palette switching functionality
   init_palette_switch();

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

   libretro_supports_bitmasks = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

   libretro_supports_ff_override = false;
   if (environ_cb(RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE, NULL))
      libretro_supports_ff_override = true;
}

void retro_deinit(void)
{
#ifdef _3DS
   linearFree(video_buf);
#else
   free(video_buf);
#endif
   video_buf = NULL;
   deinit_frame_blending();
   audio_resampler_deinit();

   freePaletteMaps();
   deinit_palette_switch();

   if (libretro_ff_enabled)
      set_fastforward_override(false);

   libretro_supports_option_categories = false;
   libretro_supports_bitmasks          = false;
   libretro_supports_ff_override       = false;
   libretro_ff_enabled                 = false;
   libretro_ff_enabled_prev            = false;

   libretro_input_state = 0;
   up_down_allowed      = false;
   turbo_period         = TURBO_PERIOD_MIN;
   turbo_pulse_width    = TURBO_PULSE_WIDTH_MIN;
   turbo_a_counter      = 0;
   turbo_b_counter      = 0;

   deactivate_rumble();
   memset(&rumble, 0, sizeof(struct retro_rumble_interface));
   rumble_level = 0;
}

void retro_set_environment(retro_environment_t cb)
{
   struct retro_vfs_interface_info vfs_iface_info;
   bool option_categories = false;
   environ_cb = cb;

   /* Set core options
    * An annoyance: retro_set_environment() can be called
    * multiple times, and depending upon the current frontend
    * state various environment callbacks may be disabled.
    * This means the reported 'categories_supported' status
    * may change on subsequent iterations. We therefore have
    * to record whether 'categories_supported' is true on any
    * iteration, and latch the result */
   libretro_set_core_options(environ_cb, &option_categories);
   libretro_supports_option_categories |= option_categories;

#ifdef HAVE_NETWORK
   /* If frontend supports core option categories,
    * gambatte_show_gb_link_settings is unused and
    * should be hidden */
   if (libretro_supports_option_categories)
   {
      struct retro_core_option_display option_display;

      option_display.visible = false;
      option_display.key     = "gambatte_show_gb_link_settings";

      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY,
            &option_display);
   }
   /* If frontend does not support core option
    * categories, core options may be shown/hidden
    * at runtime. In this case, register 'update
    * display' callback, so frontend can update
    * core options menu without calling retro_run() */
   else
   {
      struct retro_core_options_update_display_callback update_display_cb;
      update_display_cb.callback = update_option_visibility;

      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK,
            &update_display_cb);
   }
#endif

   vfs_iface_info.required_interface_version = 2;
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

enum gb_colorization_enable_type
{
   GB_COLORIZATION_DISABLED = 0,
   GB_COLORIZATION_AUTO     = 1,
   GB_COLORIZATION_CUSTOM   = 2,
   GB_COLORIZATION_INTERNAL = 3,
   GB_COLORIZATION_GBC      = 4,
   GB_COLORIZATION_SGB      = 5
};

static enum gb_colorization_enable_type gb_colorization_enable = GB_COLORIZATION_DISABLED;

static std::string rom_path;
static char internal_game_name[17];

static void load_custom_palette(void)
{
   const char *system_dir = NULL;
   const char *rom_file   = NULL;
   char *rom_name         = NULL;
   RFILE *palette_file    = NULL;
   unsigned line_index    = 0;
   bool path_valid        = false;
   unsigned rgb32         = 0;
   char palette_path[PATH_MAX_LENGTH];

   palette_path[0] = '\0';

   /* Get system directory */
   if (!environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) ||
       !system_dir)
   {
      gambatte_log(RETRO_LOG_WARN,
            "No system directory defined, unable to look for custom palettes.\n");
      return;
   }

   /* Look for palette named after ROM file */
   rom_file = path_basename(rom_path.c_str());
   if (!string_is_empty(rom_file))
   {
      size_t len = (strlen(rom_file) + 1) * sizeof(char);
      rom_name = (char*)malloc(len);
      strlcpy(rom_name, rom_file, len);
      path_remove_extension(rom_name);
      if (!string_is_empty(rom_name))
      {
         fill_pathname_join_special_ext(palette_path,
               system_dir, "palettes", rom_name, ".pal",
               sizeof(palette_path));
         path_valid = path_is_valid(palette_path);
      }
      free(rom_name);
      rom_name = NULL;
   }

   if (!path_valid)
   {
      /* Look for palette named after the internal game
       * name in the ROM header */
      fill_pathname_join_special_ext(palette_path,
            system_dir, "palettes", internal_game_name, ".pal",
            sizeof(palette_path));
      path_valid = path_is_valid(palette_path);
   }

   if (!path_valid)
   {
      /* Look for default custom palette file (default.pal) */
      fill_pathname_join_special_ext(palette_path,
            system_dir, "palettes", "default", ".pal",
            sizeof(palette_path));
      path_valid = path_is_valid(palette_path);
   }

   if (!path_valid)
      return; /* Unable to find any custom palette file */

   palette_file = filestream_open(palette_path,
         RETRO_VFS_FILE_ACCESS_READ,
         RETRO_VFS_FILE_ACCESS_HINT_NONE);

   if (!palette_file)
   {
      gambatte_log(RETRO_LOG_WARN,
            "Failed to open custom palette: %s\n", palette_path);
      return;
   }

   gambatte_log(RETRO_LOG_INFO, "Using custom palette: %s\n", palette_path);

   /* Iterate over palette file lines */
   while (!filestream_eof(palette_file))
   {
      char *line            = filestream_getline(palette_file);
      const char *value_str = NULL;

      if (!line)
         break;

      /* Remove any leading/trailing whitespace
       * > Additionally handles 'leftovers' from
       *   CRLF line terminators if palette file
       *   happens to be in DOS format */
      string_trim_whitespace(line);

      if (string_is_empty(line) || /* Skip empty lines */
          (*line == '[') ||        /* Skip ini sections */
          (*line == ';'))          /* Skip ini comments */
         goto palette_line_end;

      /* Supposed to be a typo here... */
      if (string_starts_with(line, "slectedScheme="))
         goto palette_line_end;

      /* Get substring after first '=' character */
      value_str = strchr(line, '=');
      if (!value_str ||
          string_is_empty(++value_str))
      {
         gambatte_log(RETRO_LOG_WARN,
               "Error in %s, line %u (color left as default)\n",
               palette_path, line_index);
         goto palette_line_end;
      }

      /* Extract colour value */
      rgb32 = string_to_unsigned(value_str);
      if (rgb32 == 0)
      {
         /* string_to_unsigned() will return 0 if
          * string is invalid, so perform a manual
          * validity check... */
         for (; *value_str != '\0'; value_str++)
         {
            if (*value_str != '0')
            {
               gambatte_log(RETRO_LOG_WARN,
                     "Unable to read palette color in %s, line %u (color left as default)\n",
                     palette_path, line_index);
               goto palette_line_end;
            }
         }
      }

#ifdef VIDEO_RGB565
      rgb32 = (rgb32 & 0x0000F8) >>  3 | /* blue */
              (rgb32 & 0x00FC00) >>  5 | /* green */
              (rgb32 & 0xF80000) >>  8;  /* red */
#elif defined(VIDEO_ABGR1555)
      rgb32 = (rgb32 & 0x0000F8) <<  7 | /* blue */
              (rgb32 & 0xF800)   >>  6 | /* green */
              (rgb32 & 0xF80000) >> 19;  /* red */
#endif

      if (     string_starts_with(line, "Background0="))
         gb.setDmgPaletteColor(0, 0, rgb32);
      else if (string_starts_with(line, "Background1="))
         gb.setDmgPaletteColor(0, 1, rgb32);
      else if (string_starts_with(line, "Background2="))
         gb.setDmgPaletteColor(0, 2, rgb32);       
      else if (string_starts_with(line, "Background3="))
         gb.setDmgPaletteColor(0, 3, rgb32);
      else if (string_starts_with(line, "Sprite%2010="))
         gb.setDmgPaletteColor(1, 0, rgb32);
      else if (string_starts_with(line, "Sprite%2011="))
         gb.setDmgPaletteColor(1, 1, rgb32);
      else if (string_starts_with(line, "Sprite%2012="))
         gb.setDmgPaletteColor(1, 2, rgb32);
      else if (string_starts_with(line, "Sprite%2013="))
         gb.setDmgPaletteColor(1, 3, rgb32);
      else if (string_starts_with(line, "Sprite%2020="))
         gb.setDmgPaletteColor(2, 0, rgb32);
      else if (string_starts_with(line, "Sprite%2021="))
         gb.setDmgPaletteColor(2, 1, rgb32);
      else if (string_starts_with(line, "Sprite%2022="))
         gb.setDmgPaletteColor(2, 2, rgb32);  
      else if (string_starts_with(line, "Sprite%2023="))
         gb.setDmgPaletteColor(2, 3, rgb32);
      else
         gambatte_log(RETRO_LOG_WARN,
               "Error in %s, line %u (color left as default)\n",
               palette_path, line_index);

palette_line_end:
      line_index++;
      free(line);
      line = NULL;
   }

   filestream_close(palette_file);
}

static void find_internal_palette(const unsigned short **palette, bool *is_gbc)
{
   const char *palette_title = NULL;
   size_t index              = 0;
   struct retro_variable var = {0};

   // Read main internal palette setting
   var.key   = "gambatte_gb_internal_palette";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      // Handle TWB64 packs
      if (string_is_equal(var.value, "TWB64 - Pack 1"))
      {
         var.key   = "gambatte_gb_palette_twb64_1";
         var.value = NULL;

         if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            palette_title = var.value;

         // Determine 'consolidated' palette index
         if (palette_title)
            index = RHMAP_GET_STR(palettes_twb64_1_index_map, palette_title);
         if (index > 0)
            index--;
         internal_palette_index = NUM_PALETTES_DEFAULT + index;
      }
      else if (string_is_equal(var.value, "TWB64 - Pack 2"))
      {
         var.key   = "gambatte_gb_palette_twb64_2";
         var.value = NULL;

         if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            palette_title = var.value;

         // Determine 'consolidated' palette index
         if (palette_title)
            index = RHMAP_GET_STR(palettes_twb64_2_index_map, palette_title);
         if (index > 0)
            index--;
         internal_palette_index = NUM_PALETTES_DEFAULT +
               NUM_PALETTES_TWB64_1 + index;
      }
      else if (string_is_equal(var.value, "TWB64 - Pack 3"))
      {
         var.key   = "gambatte_gb_palette_twb64_3";
         var.value = NULL;

         if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            palette_title = var.value;

         // Determine 'consolidated' palette index
         if (palette_title)
            index = RHMAP_GET_STR(palettes_twb64_3_index_map, palette_title);
         if (index > 0)
            index--;
         internal_palette_index = NUM_PALETTES_DEFAULT +
               NUM_PALETTES_TWB64_1 + NUM_PALETTES_TWB64_2 + index;
      }
      // Handle PixelShift packs
      else if (string_is_equal(var.value, "PixelShift - Pack 1"))
      {
         var.key   = "gambatte_gb_palette_pixelshift_1";
         var.value = NULL;

         if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            palette_title = var.value;

         // Determine 'consolidated' palette index
         if (palette_title)
            index = RHMAP_GET_STR(palettes_pixelshift_1_index_map, palette_title);
         if (index > 0)
            index--;
         internal_palette_index = NUM_PALETTES_DEFAULT + NUM_PALETTES_TWB64_1 +
               NUM_PALETTES_TWB64_2 + NUM_PALETTES_TWB64_3 + index;
      }
      else
      {
         palette_title = var.value;

         // Determine 'consolidated' palette index
         index = RHMAP_GET_STR(palettes_default_index_map, palette_title);
         if (index > 0)
            index--;
         internal_palette_index = index;
      }
   }

   // Ensure we have a valid palette title
   if (!palette_title)
   {
      palette_title          = "GBC - Grayscale";
      internal_palette_index = 8;
   }

   // Search for requested palette
   *palette = findGbcDirPal(palette_title);

   // If palette is not found (i.e. if a palette
   // is removed from the core, and a user loads
   // old core options settings), fall back to
   // black and white
   if (!(*palette))
   {
      palette_title          = "GBC - Grayscale";
      *palette               = findGbcDirPal(palette_title);
      internal_palette_index = 8;
      // No error check here - if this fails,
      // the core is entirely broken...
   }

   // Check whether this is a GBC palette
   if (!strncmp("GBC", palette_title, 3))
      *is_gbc = true;
   else
      *is_gbc = false;

   // Record that an internal palette is
   // currently in use
   internal_palette_active = true;
}

static void check_variables(bool startup)
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
         colorCorrectionBrightness = 1.0f;
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

   bool old_use_cc_resampler = use_cc_resampler;
   use_cc_resampler          = false;
   var.key                   = "gambatte_audio_resampler";
   var.value                 = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) &&
       var.value && !strcmp(var.value, "cc"))
      use_cc_resampler = true;

   if (!startup && (use_cc_resampler != old_use_cc_resampler))
   {
      struct retro_system_av_info av_info;
      audio_resampler_deinit();
      audio_resampler_init(false);
      retro_get_system_av_info(&av_info);
      environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &av_info);
   }

   up_down_allowed = false;
   var.key         = "gambatte_up_down_allowed";
   var.value       = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "enabled"))
         up_down_allowed = true;
      else
         up_down_allowed = false;
   }

   turbo_period      = TURBO_PERIOD_MIN;
   turbo_pulse_width = TURBO_PULSE_WIDTH_MIN;
   var.key           = "gambatte_turbo_period";
   var.value         = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      turbo_period = atoi(var.value);
      turbo_period = (turbo_period < TURBO_PERIOD_MIN) ?
            TURBO_PERIOD_MIN : turbo_period;
      turbo_period = (turbo_period > TURBO_PERIOD_MAX) ?
            TURBO_PERIOD_MAX : turbo_period;

      turbo_pulse_width = turbo_period >> 1;
      turbo_pulse_width = (turbo_pulse_width < TURBO_PULSE_WIDTH_MIN) ?
            TURBO_PULSE_WIDTH_MIN : turbo_pulse_width;
      turbo_pulse_width = (turbo_pulse_width > TURBO_PULSE_WIDTH_MAX) ?
            TURBO_PULSE_WIDTH_MAX : turbo_pulse_width;

      turbo_a_counter = 0;
      turbo_b_counter = 0;
   }

   rumble_level = 0;
   var.key      = "gambatte_rumble_level";
   var.value    = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      rumble_level = atoi(var.value);
      rumble_level = (rumble_level > 10) ? 10 : rumble_level;
      rumble_level = (rumble_level > 0)  ? ((0x1999 * rumble_level) + 0x5) : 0;
   }
   if (rumble_level == 0)
      deactivate_rumble();

   /* Interframe blending option has its own handler */
   check_frame_blend_variable();

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
   update_option_visibility();

#endif

   internal_palette_active = false;
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

   if (strcmp(var.value, "disabled") == 0)
      gb_colorization_enable = GB_COLORIZATION_DISABLED;
   else if (strcmp(var.value, "auto") == 0)
      gb_colorization_enable = GB_COLORIZATION_AUTO;
   else if (strcmp(var.value, "custom") == 0)
      gb_colorization_enable = GB_COLORIZATION_CUSTOM;
   else if (strcmp(var.value, "internal") == 0)
      gb_colorization_enable = GB_COLORIZATION_INTERNAL;
   else if (strcmp(var.value, "GBC") == 0)
      gb_colorization_enable = GB_COLORIZATION_GBC;
   else if (strcmp(var.value, "SGB") == 0)
      gb_colorization_enable = GB_COLORIZATION_SGB;

   // Containers for GBC/SGB BIOS built-in palettes
   const unsigned short *gbc_bios_palette = NULL;
   const unsigned short *sgb_bios_palette = NULL;
   bool isGbcPalette = false;

   switch (gb_colorization_enable)
   {
      case GB_COLORIZATION_AUTO:
         // Automatic colourisation
         // Order of preference:
         // 1 - SGB, if more colourful than GBC
         // 2 - GBC, if more colourful than SGB
         // 3 - SGB, if no GBC palette defined
         // 4 - User-defined internal palette, if neither GBC nor SGB palettes defined
         //
         // Load GBC BIOS built-in palette
         gbc_bios_palette = findGbcTitlePal(internal_game_name);
         // Load SGB BIOS built-in palette
         sgb_bios_palette = findSgbTitlePal(internal_game_name);
         // If both GBC and SGB palettes are defined,
         // use whichever is more colourful
         if (gbc_bios_palette)
         {
            isGbcPalette = true;
            if (sgb_bios_palette)
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
         if (!gbc_bios_palette)
         {
            gbc_bios_palette = sgb_bios_palette;
         }
         // If neither GBC nor SGB palettes are defined, set
         // user-defined internal palette
         if (!gbc_bios_palette)
         {
            find_internal_palette(&gbc_bios_palette, &isGbcPalette);
         }
         break;
      case GB_COLORIZATION_CUSTOM:
         load_custom_palette();
         break;
      case GB_COLORIZATION_INTERNAL:
         find_internal_palette(&gbc_bios_palette, &isGbcPalette);
         break;
      case GB_COLORIZATION_GBC:
         // Force GBC colourisation
         gbc_bios_palette = findGbcTitlePal(internal_game_name);
         if (!gbc_bios_palette)
         {
            gbc_bios_palette = findGbcDirPal("GBC - Dark Green"); // GBC Default
         }
         isGbcPalette = true;
         break;
      case GB_COLORIZATION_SGB:
         // Force SGB colourisation
         gbc_bios_palette = findSgbTitlePal(internal_game_name);
         if (!gbc_bios_palette)
         {
            gbc_bios_palette = findGbcDirPal("SGB - 1A"); // SGB Default
         }
         break;
      default: // GB_COLORIZATION_DISABLED
         gbc_bios_palette = findGbcDirPal("GBC - Grayscale");
         break;
   }
   
   // Enable colour correction, if required
   gb.setColorCorrection((colorCorrection == 2) || ((colorCorrection == 1) && isGbcPalette));
   
   // If gambatte is using custom colourisation
   // then we have already loaded the palette.
   // In this case we can therefore skip this loop.
   if (gb_colorization_enable != GB_COLORIZATION_CUSTOM)
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
      gambatte_log(RETRO_LOG_ERROR, "Cannot dupe frames!\n");
      return false;
   }

   if (environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble))
      gambatte_log(RETRO_LOG_INFO, "Rumble environment supported.\n");
   else
      gambatte_log(RETRO_LOG_INFO, "Rumble environment not supported.\n");

   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Turbo B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Turbo A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },
      { 0 },
   };

   struct retro_input_descriptor desc_ff[] = { /* ff: fast forward */
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Turbo B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Turbo A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Fast Forward" },
      { 0 },
   };

   struct retro_input_descriptor desc_ps[] = { /* ps: palette switching */
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Turbo B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Turbo A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Prev. Internal Palette" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Next Internal Palette" },
      { 0 },
   };

   struct retro_input_descriptor desc_ff_ps[] = { /* ff: fast forward, ps: palette switching */
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Turbo B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Turbo A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Prev. Internal Palette" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Next Internal Palette" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Fast Forward" },
      { 0 },
   };

   if (libretro_supports_ff_override)
   {
      if (libretro_supports_set_variable)
         environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc_ff_ps);
      else
         environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc_ff);
   }
   else
   {
      if (libretro_supports_set_variable)
         environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc_ps);
      else
         environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
   }

#if defined(VIDEO_RGB565) || defined(VIDEO_ABGR1555)
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      gambatte_log(RETRO_LOG_ERROR, "RGB565 is not supported.\n");
      return false;
   }
#else
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      gambatte_log(RETRO_LOG_ERROR, "XRGB8888 is not supported.\n");
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

   gambatte_log(RETRO_LOG_INFO, "Got internal game name: %s.\n", internal_game_name);

   check_variables(true);
   audio_resampler_init(true);

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

void retro_run()
{
   static uint64_t samples_count = 0;
   static uint64_t frames_count = 0;

   input_poll_cb();
   update_input_state();

   uint64_t expected_frames = samples_count / SOUND_SAMPLES_PER_FRAME;
   if (frames_count < expected_frames) // Detect frame dupes.
   {
      video_cb(NULL, VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_PITCH * sizeof(gambatte::video_pixel_t));
      frames_count++;
      return;
   }

   union
   {
      gambatte::uint_least32_t u32[SOUND_BUFF_SIZE];
      int16_t i16[2 * SOUND_BUFF_SIZE];
   } static sound_buf;
   unsigned samples = SOUND_SAMPLES_PER_RUN;

   while (gb.runFor(video_buf, VIDEO_PITCH, sound_buf.u32, SOUND_BUFF_SIZE, samples) == -1)
   {
      if (use_cc_resampler)
         CC_renderaudio((audio_frame_t*)sound_buf.u32, samples);
      else
      {
         blipper_renderaudio(sound_buf.i16, samples);

         unsigned read_avail = blipper_read_avail(resampler_l);
         if (read_avail >= (BLIP_BUFFER_SIZE >> 1))
            audio_out_buffer_read_blipper(read_avail);
      }

      samples_count += samples;
      samples = SOUND_SAMPLES_PER_RUN;
   }
#ifdef DUAL_MODE
   while (gb2.runFor(video_buf + GB_SCREEN_WIDTH, VIDEO_PITCH, sound_buf.u32, samples) == -1) {}
#endif

   /* Perform interframe blending, if required */
   if (blend_frames)
      blend_frames();

   video_cb(video_buf, VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_PITCH * sizeof(gambatte::video_pixel_t));

   if (use_cc_resampler)
      CC_renderaudio((audio_frame_t*)sound_buf.u32, samples);
   else
   {
      blipper_renderaudio(sound_buf.i16, samples);

      unsigned read_avail = blipper_read_avail(resampler_l);
      audio_out_buffer_read_blipper(read_avail);
   }
   samples_count += samples;
   audio_upload_samples();

   /* Apply any 'pending' rumble effects */
   if (rumble_active)
      apply_rumble();

   frames_count++;

   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables(false);
}

unsigned retro_api_version() { return RETRO_API_VERSION; }

