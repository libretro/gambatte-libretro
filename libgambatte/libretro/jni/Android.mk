LOCAL_PATH := $(call my-dir)

ROOT_DIR     := $(LOCAL_PATH)/../../..
CORE_DIR     := $(ROOT_DIR)/libgambatte/src
LIBRETRO_DIR := $(ROOT_DIR)/libgambatte/libretro

HAVE_NETWORK := 1

include $(ROOT_DIR)/Makefile.common

COREFLAGS := -DINLINE=inline -DHAVE_STDINT_H -DHAVE_INTTYPES_H -D__LIBRETRO__ -DVIDEO_RGB565

ifeq ($(HAVE_NETWORK),1)
  COREFLAGS += -DHAVE_NETWORK
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_CXX) $(SOURCES_C)
LOCAL_CXXFLAGS  := $(COREFLAGS) $(INCFLAGS)
LOCAL_CFLAGS    := $(INCFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(LIBRETRO_DIR)/link.T
include $(BUILD_SHARED_LIBRARY)
