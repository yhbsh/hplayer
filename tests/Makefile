CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 $(shell pkg-config --cflags libavcodec libavformat libavdevice libavutil sdl2 glfw3)
LDFLAGS = $(shell pkg-config --libs libavcodec libavformat libavdevice libavutil sdl2 glfw3) -framework OpenGL

# List of source files
SOURCES = ffprobe.c hplayer.c monitor.c opengl.c parse.c rtmp.c

# Generate a list of executables by replacing .c with nothing
EXECUTABLES = $(SOURCES:.c=)

# Default target
all: $(EXECUTABLES)

# Rule to build each executable
%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Clean up built executables
clean:
	rm -f $(EXECUTABLES)

.PHONY: all clean
