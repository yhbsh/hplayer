CFLAGS     = -Wall -Wextra -O3 -I./deps/include -DGL_SILENCE_DEPRECATION
LIBS       = -L./deps/lib -lglfw -lavformat -lavcodec -lswscale -lswresample -lavutil
FRAMEWORKS = -framework CoreMedia -framework CoreVideo -framework Videotoolbox -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework AudioToolbox

all: video video_many video_objc image

video_many: video_many.cpp
	clang++ -std=c++11 $(CFLAGS) video_many.cpp -o video_many $(LIBS) $(FRAMEWORKS)

video: video.cpp
	clang++ -std=c++11 $(CFLAGS) video.cpp -o video $(LIBS) $(FRAMEWORKS)

video_objc: video.m
	clang $(CFLAGS) video.m -o video_objc $(LIBS) $(FRAMEWORKS)

image: image.c
	clang $(CFLAGS) image.c -o image $(LIBS) $(FRAMEWORKS)
