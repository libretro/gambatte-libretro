
TARGET := libgambatte.a

GAMBATTE_SRC := src/bitmap_font.cpp \
	src/cpu.cpp \
	src/gambatte.cpp \
	src/initstate.cpp \
	src/interrupter.cpp \
	src/interruptrequester.cpp \
	src/memory.cpp \
	src/sound.cpp \
	src/state_osd_elements.cpp \
	src/statesaver.cpp \
	src/tima.cpp \
	src/video.cpp \
	src/mem/cartridge.cpp \
	src/mem/memptrs.cpp \
	src/mem/rtc.cpp \
	src/sound/channel1.cpp \
	src/sound/channel2.cpp \
	src/sound/channel3.cpp \
	src/sound/channel4.cpp \
	src/sound/duty_unit.cpp \
	src/sound/envelope_unit.cpp \
	src/sound/length_counter.cpp \
	src/video/ly_counter.cpp \
	src/video/lyc_irq.cpp \
	src/video/next_m0_time.cpp \
	src/video/ppu.cpp \
	src/video/sprite_mapper.cpp \
	src/file/file.cpp

GAMBATTE_OBJ := $(GAMBATTE_SRC:.cpp=.o)

#GAMBATTE_FLAGS := -O2 -fomit-frame-pointer -Wall -Wextra -fno-exceptions -fno-rtti -DHAVE_STDINT_H -fPIC -Isrc -Iinclude -I../common
GAMBATTE_FLAGS := -O0 -g -Wall -Wextra -fno-exceptions -fno-rtti -DHAVE_STDINT_H -fPIC -Isrc -Iinclude -I../common

all: $(TARGET)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(GAMBATTE_FLAGS)

$(TARGET): $(GAMBATTE_OBJ)
	$(AR) rcs $@ $(GAMBATTE_OBJ)

clean:
	rm -f $(TARGET)
	rm -f $(GAMBATTE_OBJ)

.PHONY: clean

