# Variables
TARGET = txt
SRC = src/main.c
CC = gcc
CFLAGS = -Wall -Wextra

# Default target
all: linux windows bundle

# Compile the target
linux: $(SRC)
	$(CC) $(CFLAGS) -I./ext/raylib/include -L./ext/raylib/lib -o $(TARGET) $^ -l:libraylib.a -lm -ldl -ggdb

windows: $(SRC)
	x86_64-w64-mingw32-windres assets/app.rc -O coff -o app.res
	x86_64-w64-mingw32-$(CC) -mwindows $(CFLAGS) -I./ext/raylib-win/include -L./ext/raylib-win/lib -o $(TARGET).exe $^ app.res -l:libraylib.a -lwinmm -lgdi32

windows-console: $(SRC)
	x86_64-w64-mingw32-$(CC) $(CFLAGS) -I./ext/raylib-win/include -L./ext/raylib-win/lib -o $(TARGET).exe $^ -l:libraylib.a -lwinmm -lgdi32

bundle: src/bundle.c
	cc -o bundle src/bundle.c

# Clean up build artifacts
clean:
	rm -f $(TARGET) $(TARGET).exe bundle bundle.exe

.PHONY: all clean linux windows windows-console

