# Variables
TARGET = txt
SRC = src/main.c

# Default target
all: linux windows bundle

# Compile the target
linux: $(SRC)
	gcc -Wall -Wextra -I./ext/raylib/include -L./ext/raylib/lib -o $(TARGET) $^ -l:libraylib.a -lm -ldl -ggdb

windows: $(SRC)
	x86_64-w64-mingw32-gcc -mwindows -Wall -Wextra -I./ext/raylib-win/include -L./ext/raylib-win/lib -o $(TARGET).exe $^ -l:libraylib.a -lwinmm -lgdi32

windows-console: $(SRC)
	x86_64-w64-mingw32-gcc -Wall -Wextra -I./ext/raylib-win/include -L./ext/raylib-win/lib -o $(TARGET).exe $^ -l:libraylib.a -lwinmm -lgdi32

bundle: src/bundle.c
	cc -o bundle src/bundle.c

# Clean up build artifacts
clean:
	rm -f $(TARGET) $(TARGET).exe bundle

.PHONY: all clean linux windows windows-console

