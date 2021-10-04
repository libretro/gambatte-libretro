#ifndef _GAMBATTE_LOG_H
#define _GAMBATTE_LOG_H

#include <libretro.h>

#ifdef __cplusplus
extern "C" {
#endif

void gambatte_log_set_cb(retro_log_printf_t log_cb);
void gambatte_log(enum retro_log_level level, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
