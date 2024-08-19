# Variables
CC = cc
CFLAGS = -Wall -Wextra -I./ext/raylib/include -L./ext/raylib/lib
LIBS = -l:libraylib.a -lm -ldl -ggdb
TARGET = txt
SRC = src/main.c

# Detect OS
ifeq ($(OS),Windows_NT)
    CC = x86_64-w64-mingw32-cc
    CFLAGS = -mwindows -Wall -Wextra -I./ext/raylib-win/include -L./ext/raylib-win/lib
    LIBS = -l:libraylib.a -lwinmm -lgdi32
endif

# Default target
all: $(TARGET)

# Compile the target
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Clean up build artifacts
clean:
	rm -f $(TARGET)

.PHONY: all clean

