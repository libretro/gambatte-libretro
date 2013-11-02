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

#include "blipper.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

struct blipper
{
   blipper_fixed_t *output_buffer;
   unsigned output_avail;
   unsigned output_buffer_samples;

   blipper_sample_t *filter_bank;

   unsigned phase;
   unsigned phases;
   unsigned phases_log2;
   unsigned taps;

   blipper_fixed_t integrator;
   blipper_sample_t last_sample;
};

void blipper_free(blipper_t *blip)
{
   if (blip)
   {
      free(blip->filter_bank);
      free(blip->output_buffer);
      free(blip);
   }
}

static double besseli0(double x)
{
   unsigned i;
   double sum = 0.0;

   double factorial = 1.0;
   double factorial_mult = 0.0;
   double x_pow = 1.0;
   double two_div_pow = 1.0;
   double x_sqr = x * x;

   /* Approximate. This is an infinite sum.
    * Luckily, it converges rather fast. */
   for (i = 0; i < 18; i++)
   {
      sum += x_pow * two_div_pow / (factorial * factorial);

      factorial_mult += 1.0;
      x_pow *= x_sqr;
      two_div_pow *= 0.25;
      factorial *= factorial_mult;
   }

   return sum;
}

static double sinc(double v)
{
   if (fabs(v) < 0.00001)
      return 1.0;
   else
      return sin(v) / v;
}

/* index range = [-1, 1) */
static double kaiser_window(double index, double beta)
{
   return besseli0(beta * sqrt(1.0 - index * index));
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float *blipper_create_sinc(unsigned phases, unsigned taps,
      double cutoff, double beta)
{
   unsigned i, filter_len;
   double sidelobes, window_mod, window_phase, sinc_phase;
   float *filter;

   filter = malloc(phases * taps * sizeof(*filter));
   if (!filter)
      return NULL;

   sidelobes = taps / 2.0;
   window_mod = 1.0 / kaiser_window(0.0, beta);
   filter_len = phases * taps;
   for (i = 0; i < filter_len; i++)
   {
      window_phase = (double)i / filter_len; /* [0, 1) */
      window_phase = 2.0 * window_phase - 1.0; /* [-1, 1) */
      sinc_phase = window_phase * sidelobes; /* [-taps / 2, taps / 2) */

      filter[i] = cutoff * sinc(M_PI * sinc_phase * cutoff) *
         kaiser_window(window_phase, beta) * window_mod;
   }

   return filter;
}

/* We differentiate and integrate at different sample rates.
 * Differentiation is D(z) = 1 - z^-1 and happens when delta impulses
 * are convolved. Integration step after decimation by D is 1 / (1 - z^-D).
 *
 * If our sinc filter is S(z) we'd have a response of
 * S(z) * (1 - z^-1) / (1 - z^-D) after blipping.
 *
 * Compensate by prefiltering S(z) with the inverse (1 - z^-D) / (1 - z^-1).
 * This filtering creates a finite length filter, albeit slightly longer.
 *
 * phases is the same as decimation rate. */
static float *blipper_prefilter_sinc(float *filter, unsigned phases,
      unsigned *out_taps)
{
   unsigned i;
   unsigned taps = *out_taps;
   float *tmp_filter;
   float *new_filter = malloc((phases * taps + phases) * sizeof(*filter));
   if (!new_filter)
      goto error;

   tmp_filter = realloc(filter, (phases * taps + phases) * sizeof(*filter));
   if (!tmp_filter)
      goto error;
   filter = tmp_filter;

   /* Integrate. */
   new_filter[0] = filter[0];
   for (i = 1; i < phases * taps; i++)
      new_filter[i] = new_filter[i - 1] + filter[i];
   for (i = phases * taps; i < phases * taps + phases; i++)
      new_filter[i] = new_filter[phases * taps - 1];

   /* Differentiate with offset of D. */
   memcpy(filter, new_filter, phases * sizeof(*filter));
   for (i = phases; i < phases * taps + phases; i++)
      filter[i] = new_filter[i] - new_filter[i - phases];

   *out_taps = taps + 1;
   free(new_filter);
   return filter;

error:
   free(new_filter);
   free(filter);
   return NULL;
}

/* Creates a polyphase filter bank.
 * Interleaves the filter for cache coherency and possibilities
 * for SIMD processing. */
static float *blipper_interleave_sinc(float *filter, unsigned phases,
      unsigned taps)
{
   unsigned t, p;
   float *new_filter = malloc(phases * taps * sizeof(*filter));
   if (!new_filter)
      goto error;

   for (t = 0; t < taps; t++)
      for (p = 0; p < phases; p++)
         new_filter[p * taps + t] = filter[t * phases + p];

   free(filter);
   return new_filter;

error:
   free(new_filter);
   free(filter);
   return NULL;
}

static blipper_sample_t *blipper_quantize_sinc(float *filter, unsigned taps, float amp)
{
   unsigned t;
   blipper_sample_t *filt = malloc(taps * sizeof(*filt));
   if (!filt)
      goto error;

   for (t = 0; t < taps; t++)
      filt[t] = (blipper_sample_t)floor(filter[t] * 0x7fff * amp + 0.5);

   free(filter);
   return filt;

error:
   free(filter);
   free(filt);
   return NULL;
}

static int blipper_create_filter_bank(blipper_t *blip, unsigned taps,
      double cutoff, double beta)
{
   float *sinc_filter;

   sinc_filter = blipper_create_sinc(blip->phases, taps, cutoff, beta);
   if (!sinc_filter)
      return 0;

   sinc_filter = blipper_prefilter_sinc(sinc_filter, blip->phases, &taps);
   if (!sinc_filter)
      return 0;
   sinc_filter = blipper_interleave_sinc(sinc_filter, blip->phases, taps);
   if (!sinc_filter)
      return 0;

   blip->filter_bank = blipper_quantize_sinc(sinc_filter, blip->phases * taps, 0.85f / blip->phases);
   if (!blip->filter_bank)
      return 0;

   blip->taps = taps;
   return 1;
}

static unsigned log2_int(unsigned v)
{
   unsigned ret;
   v >>= 1;
   for (ret = 0; v; v >>= 1, ret++);
   return ret;
}

blipper_t *blipper_new(unsigned taps, double cutoff, double beta,
      unsigned decimation, unsigned buffer_samples)
{
   blipper_t *blip = NULL;

   /* Sanity check. Not strictly required to be supported in C. */
   if ((-3 >> 2) != -1)
   {
      fprintf(stderr, "Integer right shift not supported.\n");
      return NULL;
   }

   if ((decimation & (decimation - 1)) != 0)
   {
      fprintf(stderr, "[blipper]: Decimation factor must be POT.\n");
      return NULL;
   }

   blip = calloc(1, sizeof(*blip));
   if (!blip)
      return NULL;

   blip->phases = decimation;
   blip->phases_log2 = log2_int(decimation);

   if (!blipper_create_filter_bank(blip, taps, cutoff, beta))
      goto error;

   blip->output_buffer = calloc(buffer_samples + blip->taps,
         sizeof(*blip->output_buffer));
   if (!blip->output_buffer)
      goto error;
   blip->output_buffer_samples = buffer_samples + blip->taps;

   return blip;

error:
   blipper_free(blip);
   return NULL;
}

void blipper_push_delta(blipper_t *blip, blipper_fixed_t delta, unsigned clocks_step)
{
   unsigned target_output, filter_phase, taps, i;
   const blipper_sample_t *response;
   blipper_fixed_t *target;

   blip->phase += clocks_step;

   target_output = (blip->phase + blip->phases - 1) >> blip->phases_log2;

   filter_phase = (target_output << blip->phases_log2) - blip->phase;
   response = blip->filter_bank + blip->taps * filter_phase;

   target = blip->output_buffer + target_output;
   taps = blip->taps;

   /* SIMD target */
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
         blipper_push_delta(blip, (blipper_fixed_t)val - (blipper_fixed_t)last, clocks_skip + 1);
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
   blipper_sample_t quant;
   blipper_fixed_t sum = blip->integrator;
   const blipper_fixed_t *out = blip->output_buffer;

   for (s = 0; s < samples; s++, output += stride)
   {
      /* Should do a saturated add here. */
      sum += out[s];

      quant = (sum + 0x4000) >> 15;
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

