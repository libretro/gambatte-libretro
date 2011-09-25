TARGET := libsnes.so

LIBSNES_OBJ := libsnes.o

#LIBSNES_FLAGS := -I../include -I. -Wall -pedantic -ansi -fPIC
LIBSNES_FLAGS := -I../include -I. -Wall -pedantic -ansi -fPIC -O0 -g

all: $(TARGET)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(LIBSNES_FLAGS)

$(TARGET): $(LIBSNES_OBJ)
	$(CXX) -o $@ $(LIBSNES_OBJ) -shared ../libgambatte.a -Wl,--no-undefined

clean:
	rm -f $(TARGET) $(LIBSNES_OBJ)

.PHONY: clean
