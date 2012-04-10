
include platform.mk

RESAMPLE_DIR := ../../common/resample
RESAMPLE_SRC := $(wildcard $(RESAMPLE_DIR)/src/*.cpp)
LIBRETRO_OBJ := libretro.o $(RESAMPLE_SRC:.cpp=.o)

LIBRETRO_FLAGS := -I../include -I. -Wall -pedantic -ansi $(fpic) -O3 -I$(RESAMPLE_DIR) -I$(RESAMPLE_DIR)/..

all: $(TARGET)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(LIBRETRO_FLAGS)

$(TARGET): $(LIBRETRO_OBJ) ../libgambatte.a
ifeq ($(platform), wii)
	$(AR) x ../libgambatte.a
	$(AR) rcs $@ $(LIBRETRO_OBJ) $(shell $(AR) t ../libgambatte.a)
else
	$(CXX) -o $@ $(LIBRETRO_OBJ) $(SHARED) ../libgambatte.a -Wl,--no-undefined $(LDFLAGS)
endif

../libgambatte.a:
	$(MAKE) -C ../

clean:
	rm -f $(TARGET) $(LIBRETRO_OBJ)

.PHONY: clean

