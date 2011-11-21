
include platform.mk

RESAMPLE_DIR := ../../common/resample
RESAMPLE_SRC := $(wildcard $(RESAMPLE_DIR)/src/*.cpp)
LIBSNES_OBJ := libsnes.o $(RESAMPLE_SRC:.cpp=.o)

LIBSNES_FLAGS := -I../include -I. -Wall -pedantic -ansi $(fpic) -O3 -I$(RESAMPLE_DIR) -I$(RESAMPLE_DIR)/..

all: $(TARGET)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(LIBSNES_FLAGS)

$(TARGET): $(LIBSNES_OBJ) ../libgambatte.a
	$(CXX) -o $@ $(LIBSNES_OBJ) $(SHARED) ../libgambatte.a -Wl,--no-undefined $(LDFLAGS)

../libgambatte.a:
	$(MAKE) -C ../

clean:
	rm -f $(TARGET) $(LIBSNES_OBJ)

.PHONY: clean
