CC         := clang
LIBRARIES  := glfw3 SDL2 libavcodec libavformat libavutil libswscale libavdevice
FRAMEWORKS := -framework OpenGL

CFLAGS  := -Os
CFLAGS  := $(shell pkg-config --cflags $(LIBRARIES)) $(CFLAGS)
LDFLAGS := $(shell pkg-config --libs $(LIBRARIES)) $(FRAMEWORKS)

TARGETS := ffprobe opengl hplayer parse rtsp

all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all
