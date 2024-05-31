CC         := clang
LIBRARIES  := glfw3 libavcodec libavformat libavutil SDL2
FRAMEWORKS := -framework OpenGL

CFLAGS  := -Os
CFLAGS  := $(shell pkg-config --cflags $(LIBRARIES)) $(CFLAGS)
LDFLAGS := $(shell pkg-config --libs $(LIBRARIES)) $(FRAMEWORKS)

TARGETS := ffprobe opengl hplayer parse

all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all
