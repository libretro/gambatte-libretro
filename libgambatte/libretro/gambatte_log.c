#include <stdio.h>
#include <stdarg.h>
#include <string/stdstring.h>
#include "gambatte_log.h"

static retro_log_printf_t gambatte_log_cb = NULL;

void gambatte_log_set_cb(retro_log_printf_t log_cb)
{
   gambatte_log_cb = log_cb;
}

void gambatte_log(enum retro_log_level level, const char *format, ...)
{
   char msg[512];
   va_list ap;

   msg[0] = '\0';

   if (string_is_empty(format))
      return;

   va_start(ap, format);
   vsprintf(msg, format, ap);
   va_end(ap);

   if (gambatte_log_cb)
      gambatte_log_cb(level, "[Gambatte] %s", msg);
   else
      fprintf((level == RETRO_LOG_ERROR) ? stderr : stdout,
            "[Gambatte] %s", msg);
}
