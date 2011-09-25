TARGET := libsnes.so

RESAMPLE_DIR := ../../common/resample
RESAMPLE_SRC := $(wildcard $(RESAMPLE_DIR)/src/*.cpp)
LIBSNES_OBJ := libsnes.o $(RESAMPLE_SRC:.cpp=.o)

LIBSNES_FLAGS := -I../include -I. -Wall -pedantic -ansi -fPIC -O3 -g -I$(RESAMPLE_DIR) -I$(RESAMPLE_DIR)/..

all: $(TARGET)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(LIBSNES_FLAGS)

$(TARGET): $(LIBSNES_OBJ) ../libgambatte.a
	$(CXX) -o $@ $(LIBSNES_OBJ) -shared ../libgambatte.a -Wl,--no-undefined

../libgambatte.a:
	$(MAKE) -C ../

clean:
	rm -f $(TARGET) $(LIBSNES_OBJ)

.PHONY: clean
