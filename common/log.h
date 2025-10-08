#ifndef COMMON_LOG_H_
#define COMMON_LOG_H_
#define RETRO_LOG_INFO 0
#define RETRO_LOG_ERROR 1
void gambatte_log(int unused, const char* fmt);
void gambatte_log(int unused, const char* fmt, unsigned arg);
#endif
