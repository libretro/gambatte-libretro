#include "log.h"
#include <stdio.h>

void gambatte_log(int unused, const char* msg) {
    printf("gambatte: %s\n", msg);
};

void gambatte_log(int unused, const char* fmt, unsigned arg) {
    printf(fmt, arg);
};
