LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

COMMON_DIR := ../../../common
INCLUDE_DIR := ../../include
RESAMPLER_DIR := $(COMMON_DIR)/resample/src
RESAMPLER_DIR_INCLUDE := $(COMMON_DIR)/resample
EMU_DIR := ../../src
LIBRETRO_DIR := ../

LOCAL_MODULE    := libretro

LOCAL_SRC_FILES := $(EMU_DIR)/bitmap_font.cpp $(EMU_DIR)/cpu.cpp $(EMU_DIR)/gambatte.cpp $(EMU_DIR)/initstate.cpp $(EMU_DIR)/interrupter.cpp $(EMU_DIR)/interruptrequester.cpp $(EMU_DIR)/memory.cpp $(EMU_DIR)/sound.cpp $(EMU_DIR)/state_osd_elements.cpp $(EMU_DIR)/statesaver.cpp $(EMU_DIR)/tima.cpp $(EMU_DIR)/video.cpp $(EMU_DIR)/file/file.cpp $(EMU_DIR)/mem/cartridge.cpp $(EMU_DIR)/mem/memptrs.cpp $(EMU_DIR)/mem/rtc.cpp $(EMU_DIR)/sound/channel1.cpp $(EMU_DIR)/sound/channel2.cpp $(EMU_DIR)/sound/channel3.cpp $(EMU_DIR)/sound/channel4.cpp $(EMU_DIR)/sound/duty_unit.cpp $(EMU_DIR)/sound/envelope_unit.cpp $(EMU_DIR)/sound/length_counter.cpp $(EMU_DIR)/video/ly_counter.cpp $(EMU_DIR)/video/lyc_irq.cpp $(EMU_DIR)/video/next_m0_time.cpp $(EMU_DIR)/video/ppu.cpp $(EMU_DIR)/video/sprite_mapper.cpp $(LIBRETRO_DIR)/libretro.cpp $(RESAMPLER_DIR)/chainresampler.cpp $(RESAMPLER_DIR)/i0.cpp $(RESAMPLER_DIR)/makesinckernel.cpp $(RESAMPLER_DIR)/resamplerinfo.cpp $(RESAMPLER_DIR)/u48div.cpp
LOCAL_CXXFLAGS = -DINLINE=inline -DHAVE_STDINT_H -DHAVE_INTTYPES_H -DLSB_FIRST -D__LIBRETRO__
LOCAL_C_INCLUDES = $(EMU_DIR) $(COMMON_DIR) $(RESAMPLER_DIR) $(RESAMPLER_DIR_INCLUDE) $(LIBRETRO_DIR) $(INCLUDE_DIR)

include $(BUILD_SHARED_LIBRARY)
