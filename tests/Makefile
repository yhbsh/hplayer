HEADERS    = -Wall -Wextra -pedantic -fobjc-arc -std=c11 -I../deps/include
LIBRARIES  = -L../deps/lib -lglfw -lavformat -lavcodec -lswscale -lswresample -lavutil -lsdl
FRAMEWORKS =  -framework  CoreVideo       \
              -framework  CoreAudio       \
              -framework  CoreMedia       \
              -framework  CoreGraphics    \
              -framework  CoreFoundation  \
              -framework  CoreHaptics     \
              -framework  CoreServices    \
              -framework  Cocoa           \
              -framework  Carbon          \
              -framework  IOKit           \
              -framework  Opengl          \
              -framework  Videotoolbox    \
              -framework  AudioToolbox    \
              -framework  GameController  \
              -framework  ForceFeedback   \
              -framework  Metal    

SOURCES    = ffprobe.c hplayer.c opengl.c parse.c rtmp.c
TARGETS    = $(SOURCES:.c=)

all: $(TARGETS)

%: %.c
	clang $(HEADERS) $< -o $@ $(LIBRARIES) $(FRAMEWORKS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
