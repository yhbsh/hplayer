HEADERS    = -Wall -Wextra -std=c11 -O3 -I./deps/include -DGL_SILENCE_DEPRECATION
LIBRARIES  = -L./deps/lib -lglfw -lavformat -lavcodec -lswscale -lswresample -lavutil
FRAMEWORKS = -framework CoreMedia -framework CoreVideo -framework Videotoolbox -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework AudioToolbox

all: video video_objc image

video: video.c
	clang $(HEADERS) video.c -o video $(LIBRARIES) $(FRAMEWORKS)

video_objc: video.m
	clang $(HEADERS) video.m -o video_objc $(LIBRARIES) $(FRAMEWORKS)

image: image.c
	clang $(HEADERS) image.c -o image $(LIBRARIES) $(FRAMEWORKS)
