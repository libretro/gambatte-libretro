/*
 * Convoluted Cosine Resampler
 * Copyright (C) 2014 - Bouhlel Ali ( aliaspider@gmail.com )
 *
 * licence: GPLv3
 */

#ifndef CC_RESAMPLER_H
#define CC_RESAMPLER_H

#ifdef CC_RESAMPLER

#ifdef __cplusplus
extern "C" {
#endif

typedef union audio_frame
{
   struct
   {
      int16_t l;
      int16_t r;
   };
   uint32_t val;

}audio_frame_t;

#define CC_DECIMATION_RATE 32

static void CC_renderaudio(audio_frame_t* sound_buf, unsigned samples)
{

   static const int16_t CC_kernel[32]=
   {
      0x01FF, 0x01FE, 0x01FA, 0x01F4, 0x01EC, 0x01E1, 0x01D4, 0x01C5,
      0x01B4, 0x01A2, 0x018E, 0x0178, 0x0161, 0x014A, 0x0131, 0x0119,
      0x0100, 0x00E6, 0x00CE, 0x00B5, 0x009E, 0x0087, 0x0071, 0x005D,
      0x004B, 0x003A, 0x002B, 0x001E, 0x0013, 0x000B, 0x0005, 0x0001
   };
   static const int16_t CC_kernel_r[32]=
   {
      0x0000, 0x0001, 0x0005, 0x000B, 0x0013, 0x001E, 0x002B, 0x003A,
      0x004B, 0x005D, 0x0071, 0x0087, 0x009E, 0x00B5, 0x00CE, 0x00E6,
      0x0100, 0x0119, 0x0131, 0x014A, 0x0161, 0x0178, 0x018E, 0x01A2,
      0x01B4, 0x01C5, 0x01D4, 0x01E1, 0x01EC, 0x01F4, 0x01FA, 0x01FE
   };

   static int32_t current_l = 0;
   static int32_t current_r = 0;
   static int32_t next_l    = 0;
   static int32_t next_r    = 0;
   static unsigned int accumulated_samples = 0;
   static unsigned int write_pos = 0;

#ifndef CC_RESAMPLER_NO_HIGHPASS
   static int32_t capacitor = 0;
#endif

   static int16_t out_buf[1024];
   unsigned i;

   for (i=0; i!=samples; i++)
   {

      current_l += sound_buf[i].l * CC_kernel[accumulated_samples];
      current_r += sound_buf[i].r * CC_kernel[accumulated_samples];
      next_l    += sound_buf[i].l * CC_kernel_r[accumulated_samples];
      next_r    += sound_buf[i].r * CC_kernel_r[accumulated_samples];

      accumulated_samples++;
      if (accumulated_samples == 32)
      {
#ifdef CC_RESAMPLER_NO_HIGHPASS
         out_buf[write_pos++] = (current_l>>14);
         out_buf[write_pos++] = (current_r>>14);
#else
         capacitor += ((current_l ) - capacitor) >> 14;
         out_buf[write_pos++] = (current_l>>14) - (capacitor >> 14);
         out_buf[write_pos++] = (current_r>>14) - (capacitor >> 14);
#endif

         accumulated_samples = 0;
         current_l = next_l;
         current_r = next_r;
         next_l = 0;
         next_r = 0;

         if (write_pos == 1024)
         {
            audio_batch_cb(out_buf, 512);
            write_pos = 0;
         }
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif // CC_RESAMPLER
#endif // CC_RESAMPLER_H
