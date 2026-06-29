/*
 * Copyright (C) 2013 - Hans-Kristian Arntzen
 *
 * Permission is hereby granted, free of charge, 
 * to any person obtaining a copy of this software and
 * associated documentation files (the "Software"),
 * to deal in the Software without restriction,
 * including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/*
 * Note (2026 audit): the upstream blipper synthesized its Kaiser-windowed
 * sinc filter bank at runtime using double-precision libm (besseli0, sinc,
 * sqrt, sin) and then quantized the result to int16. That made the exact
 * int16 taps a function of the host libm and floating-point rounding mode,
 * and dragged libm / floating point into an otherwise pure-integer audio
 * path. The synthesis (blipper_create_sinc / _prefilter_sinc /
 * _interleave_sinc / _quantize_sinc and the besseli0 / sinc / kaiser_window
 * helpers) has been removed and the resulting coefficients are baked as a
 * const int16 table in blipper_filter_bank.h. The optional floating-point
 * build (BLIPPER_FIXED_POINT 0 / BLIPPER_REAL_T) and the performance-timing
 * code (which used double get_time()) have also been removed. This TU now
 * contains no floating point and no libm dependency, and produces output
 * that is bit-identical on every target.
 */

#include "blipper.h"
#include "blipper_filter_bank.h"

#include <stdlib.h>
#include <string.h>

struct blipper
{
   blipper_long_sample_t *output_buffer;
   unsigned output_avail;
   unsigned output_buffer_samples;

   const blipper_sample_t *filter_bank;

   unsigned phase;
   unsigned phases;
   unsigned phases_log2;
   unsigned taps;

   blipper_long_sample_t integrator;
   blipper_sample_t last_sample;
};

void blipper_free(blipper_t *blip)
{
   if (blip)
   {
      /* The filter bank is const static storage (see
       * blipper_filter_bank.h) or caller-owned; blipper never owns it. */
      free(blip->output_buffer);
      free(blip);
   }
}

const blipper_sample_t *blipper_create_filter_bank(unsigned decimation,
      unsigned taps, double cutoff, double beta)
{
   /* cutoff and beta are encoded in the baked table; accepted for
    * source compatibility but intentionally unused. */
   (void)cutoff;
   (void)beta;

   if (decimation == BLIPPER_FILTER_BANK_PHASES &&
       taps       == BLIPPER_FILTER_BANK_TAPS)
      return blipper_filter_bank;

   return NULL;
}

static unsigned log2_int(unsigned v)
{
   unsigned ret;
   v >>= 1;
   for (ret = 0; v; v >>= 1, ret++);
   return ret;
}

void blipper_reset(blipper_t *blip)
{
   blip->phase = 0;
   memset(blip->output_buffer, 0,
         (blip->output_avail + blip->taps) * sizeof(*blip->output_buffer));
   blip->output_avail = 0;
   blip->last_sample = 0;
   blip->integrator = 0;
}

blipper_t *blipper_new(unsigned taps, double cutoff, double beta,
      unsigned decimation, unsigned buffer_samples,
      const blipper_sample_t *filter_bank)
{
   blipper_t *blip = NULL;

   /* Sanity check. Not strictly required to be supported in C. */
   if ((-3 >> 2) != -1)
      return NULL;

   if ((decimation & (decimation - 1)) != 0)
      return NULL;

   /* Without a caller-supplied bank we use the baked-in const table,
    * which is only valid for the configuration it was generated for. */
   if (!filter_bank)
   {
      filter_bank = blipper_create_filter_bank(decimation, taps, cutoff, beta);
      if (!filter_bank)
         return NULL;
   }

   blip = (blipper_t*)calloc(1, sizeof(*blip));
   if (!blip)
      return NULL;

   blip->phases      = decimation;
   blip->phases_log2 = log2_int(decimation);
   blip->taps        = taps;
   blip->filter_bank = filter_bank;

   blip->output_buffer = (blipper_long_sample_t*)calloc(buffer_samples + blip->taps,
         sizeof(*blip->output_buffer));
   if (!blip->output_buffer)
      goto error;
   blip->output_buffer_samples = buffer_samples + blip->taps;

   return blip;

error:
   blipper_free(blip);
   return NULL;
}

void blipper_push_delta(blipper_t *blip, blipper_long_sample_t delta, unsigned clocks_step)
{
   unsigned target_output, filter_phase, taps, i;
   const blipper_sample_t *response;
   blipper_long_sample_t *target;

   blip->phase += clocks_step;

   target_output = (blip->phase + blip->phases - 1) >> blip->phases_log2;

   filter_phase = (target_output << blip->phases_log2) - blip->phase;
   response = blip->filter_bank + blip->taps * filter_phase;

   target = blip->output_buffer + target_output;
   taps = blip->taps;

   for (i = 0; i < taps; i++)
      target[i] += delta * response[i];

   blip->output_avail = target_output;
}

void blipper_push_samples(blipper_t *blip, const blipper_sample_t *data,
      unsigned samples, unsigned stride)
{
   unsigned s;
   unsigned clocks_skip = 0;
   blipper_sample_t last = blip->last_sample;

   for (s = 0; s < samples; s++, data += stride)
   {
      blipper_sample_t val = *data;
      if (val != last)
      {
         blipper_push_delta(blip, (blipper_long_sample_t)val - (blipper_long_sample_t)last, clocks_skip + 1);
         clocks_skip = 0;
         last = val;
      }
      else
         clocks_skip++;
   }

   blip->phase += clocks_skip;
   blip->output_avail = (blip->phase + blip->phases - 1) >> blip->phases_log2;
   blip->last_sample = last;
}

unsigned blipper_read_avail(blipper_t *blip)
{
   return blip->output_avail;
}

void blipper_read(blipper_t *blip, blipper_sample_t *output, unsigned samples,
      unsigned stride)
{
   unsigned s;
   blipper_long_sample_t sum = blip->integrator;
   const blipper_long_sample_t *out = blip->output_buffer;

   for (s = 0; s < samples; s++, output += stride)
   {
      blipper_long_sample_t quant;

      /* Cannot overflow. Also add a leaky integrator.
         Mitigates DC shift numerical instability which is
         inherent for integrators. */
      sum += (out[s] >> 1) - (sum >> 9);

      /* Rounded. With leaky integrator, this cannot overflow. */
      quant = (sum + 0x4000) >> 15;

      /* Clamp. quant can potentially have range [-0x10000, 0xffff] here.
       * In both cases, top 16-bits will have a uniform bit pattern which can be exploited. */
      if ((blipper_sample_t)quant != quant)
      {
         quant = (quant >> 16) ^ 0x7fff;
         sum = quant << 15;
      }

      *output = quant;
   }

   /* Don't bother with ring buffering.
    * The entire buffer should be read out ideally anyways. */
   memmove(blip->output_buffer, blip->output_buffer + samples,
         (blip->output_avail + blip->taps - samples) * sizeof(*out));
   memset(blip->output_buffer + blip->taps, 0, samples * sizeof(*out));
   blip->output_avail -= samples;
   blip->phase -= samples << blip->phases_log2;

   blip->integrator = sum;
}
