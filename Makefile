# force rebuild, while deps isn't working
.PHONY: libgb.so clean run runc

all: libgb.so main

# TODO: re-add O3
EMBEDFLAGS=-g -fvisibility=hidden -static-libstdc++ -fPIC -Wfatal-errors -Werror -Wno-narrowing
# CFLAGS=-fvisibility=hidden -ffreestanding -nostdlib -fPIC -O3 -Wfatal-errors -Werror
SRCS := $(wildcard libgambatte/src/**/*.cpp libgambatte/src/*.cpp)
libgb.so: libgb.cpp $(SRCS)
	time $(CXX) $(CFLAGS) $(EMBEDFLAGS) -D__LIBRETRO__ -I libgambatte/include -I libgambatte/src -I common/ -shared -o libgb.so libgb.cpp $(SRCS)
	cp libgb.so libapu.so

main:
	time $(CXX) -O3 -o main main.c -L. -l:libgb.so -lSDL2 -lc -lm ${WARN}
	echo "done"

clean:
	rm -f libgb.so

run:
	LD_LIBRARY_PATH=$(shell pwd) ./main "$(ROM)"
runc:
	LD_LIBRARY_PATH=$(shell pwd) ./main "$(ROM)" c
	
