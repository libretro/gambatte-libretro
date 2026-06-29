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
 * Note (2026 audit): this resampler is no longer a drop-in copy of the
 * upstream dual-precision blipper. The optional floating-point build
 * (BLIPPER_FIXED_POINT 0 / BLIPPER_REAL_T) has been removed and the
 * sample types are pinned to int16/int32. The Kaiser-windowed sinc
 * filter bank that upstream synthesized at runtime with double-precision
 * libm is now precomputed and baked as a const int16 table
 * (blipper_filter_bank.h), so this translation unit pulls in no libm and
 * emits no floating-point instructions, and the resampled output is
 * bit-identical across every target and compiler. The remaining public
 * surface matches the subset gambatte-libretro uses.
 */

#ifndef BLIPPER_H__
#define BLIPPER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct blipper blipper_t;

/* Fixed-point only. int16 in/out, int32 intermediate. */
typedef int16_t blipper_sample_t;
typedef int32_t blipper_long_sample_t;

/* Create a new blipper.
 * taps: Number of filter taps per impulse.
 *
 * cutoff: Cutoff frequency in the passband. Has a range of [0, 1].
 *
 * beta: Beta used for Kaiser window.
 *
 * decimation: Sets decimation rate. Must be power-of-two (2^n).
 * The input sampling rate is then output_rate * 2^decimation.
 *
 * buffer_samples: The maximum number of processed output samples that can be
 * buffered up by blipper.
 *
 * filter_bank: An optional filter which has already been created by
 * blipper_create_filter_bank(). blipper_new() does not take ownership
 * of the buffer and the caller must keep it alive for the blipper's
 * lifetime. If NULL, the baked-in const filter bank is used, which is
 * valid only for the (decimation, taps) pair the table was generated
 * for (see blipper_filter_bank.h). cutoff and beta are encoded in the
 * baked table and are ignored when filter_bank is NULL.
 *
 * The configuration shipped by gambatte-libretro is:
 *   taps = 32, cutoff = 0.85, beta = 6.5, decimation = 64
 */
blipper_t *blipper_new(unsigned taps, double cutoff, double beta,
      unsigned decimation, unsigned buffer_samples, const blipper_sample_t *filter_bank);

/* Reset the blipper to its initiate state. */
void blipper_reset(blipper_t *blip);

/* Returns a pointer to the baked-in const filter bank for the supported
 * (decimation, taps) configuration, or NULL if the requested parameters
 * do not match the precomputed table. The returned pointer is owned by
 * the library (it points at static const storage) and must NOT be freed.
 * cutoff and beta are accepted for source compatibility but are encoded
 * in the baked table; mismatching values are not validated. */
const blipper_sample_t *blipper_create_filter_bank(unsigned decimation,
      unsigned taps, double cutoff, double beta);

/* Frees the blipper. blip can be NULL (no-op). */
void blipper_free(blipper_t *blip);

/* Data pushing interfaces. One of these should be used exclusively. */

/* Push a single delta, which occurs clock_step input samples after the
 * last time a delta was pushed. The delta value is the difference signal
 * between the new sample and the previous.
 * It is unnecessary to pass a delta of 0.
 * If the deltas are known beforehand (e.g. when synthesizing a waveform),
 * this is a more efficient interface than blipper_push_samples().
 *
 * The caller must ensure not to push deltas in a way that can destabilize
 * the final integration.
 */
void blipper_push_delta(blipper_t *blip, blipper_long_sample_t delta, unsigned clocks_step);

/* Push raw samples. blipper will find the deltas themself and push them.
 * stride is the number of samples between each sample to be used.
 * This can be used to push interleaved stereo data to two independent
 * blippers.
 */
void blipper_push_samples(blipper_t *blip, const blipper_sample_t *delta,
      unsigned samples, unsigned stride);

/* Returns the number of samples available for reading using
 * blipper_read().
 */
unsigned blipper_read_avail(blipper_t *blip);

/* Reads processed samples. The caller must ensure to not read
 * more than what is returned from blipper_read_avail().
 * As in blipper_push_samples(), stride is the number of samples
 * between each output sample in output.
 * Can be used to write to an interleaved stereo buffer.
 */
void blipper_read(blipper_t *blip, blipper_sample_t *output, unsigned samples,
      unsigned stride);

#ifdef __cplusplus
}
#endif

#endif
