
ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
endif
endif

ifeq ($(platform), unix)
   TARGET := libsnes.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T
else ifeq ($(platform), osx)
   TARGET := libsnes.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
else ifeq ($(platform), wii)
   TARGET := libsnes.a
   CXX = powerpc-eabi-g++
   AR = powerpc-eabi-ar
   CXXFLAGS += -mrvl -mcpu=750 -DGEKKO -meabi -mhard-float
else
   TARGET := snes.dll
   CC = gcc
   CXX = g++
   SHARED := -shared -Wl,--version-script=link.T
   LDFLAGS += -static-libgcc -static-libstdc++ -s
endif

