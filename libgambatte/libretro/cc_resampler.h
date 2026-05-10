/*
 * Convoluted Cosine Resampler
 * Copyright (C) 2014 - Bouhlel Ali ( aliaspider@gmail.com )
 *
 * licence: GPLv3
 */

/* Note (2026 audit): the previous version of this header defined
 * file-static module state (CC_accumulated_samples and the rest)
 * directly in the header. If the header was ever included from
 * more than one translation unit, each TU got its own copy of
 * that state and the resampler silently desynchronized. The
 * definitions have been moved into cc_resampler.c so there is
 * exactly one instance of every variable. The header now only
 * exposes prototypes. */

#ifndef CC_RESAMPLER_H
#define CC_RESAMPLER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct audio_frame
{
   int16_t l;
   int16_t r;
} audio_frame_t;

#define CC_DECIMATION_RATE 32

extern void audio_out_buffer_write(int16_t *samples, size_t num_samples);

void CC_init(void);
void CC_renderaudio(audio_frame_t *sound_buf, unsigned samples);

#ifdef __cplusplus
}
#endif

#endif /* CC_RESAMPLER_H */
