# force rebuild, while deps isn't working
.PHONY: libgb.so

# TODO: re-add O3
CFLAGS=-fvisibility=hidden -fPIC -Wfatal-errors -Werror -Wno-narrowing
# CFLAGS=-fvisibility=hidden -ffreestanding -nostdlib -fPIC -O3 -Wfatal-errors -Werror
SRCS := $(wildcard libgambatte/src/**/*.cpp libgambatte/src/*.cpp)
libgb.so: libgb.cpp $(SRCS)
	time $(CXX) $(CFLAGS) -D__LIBRETRO__ -I libgambatte/include -I libgambatte/src -I common/ -shared -o libgb.so $(SRCS) libgb.cpp


main: libgb.so
	time $(CXX) -O3 -o main main.c -L. -l:libgb.so -lSDL2 -lc -lm ${WARN}
	echo "done"
