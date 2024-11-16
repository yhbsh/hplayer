CFLAGS     = -D__STDC_CONSTANT_MACROS -I./deps/include -DGL_SILENCE_DEPRECATION
LIBS       = -L./deps/lib -lglfw -lavformat -lavcodec -lswscale -lswresample -lavutil -lportaudio
FRAMEWORKS = -framework CoreMedia -framework CoreVideo -framework Videotoolbox -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework AudioToolbox

all: main miniaudio portaudio video video_many video_objc image

main: main.c
	clang main.c -o main

video_many: video_many.cpp
	clang++ -std=c++11 $(CFLAGS) video_many.cpp -o video_many $(LIBS) $(FRAMEWORKS)

video: video.cpp
	clang++ -std=c++11 $(CFLAGS) video.cpp -o video $(LIBS) $(FRAMEWORKS)

video_objc: video.m
	clang $(CFLAGS) video.m -o video_objc $(LIBS) $(FRAMEWORKS)

portaudio: portaudio.c
	clang $(CFLAGS) portaudio.c -o portaudio $(LIBS) $(FRAMEWORKS)

miniaudio: miniaudio.c
	clang $(CFLAGS) miniaudio.c -o miniaudio $(LIBS) $(FRAMEWORKS)

image: image.c
	clang $(CFLAGS) image.c -o image $(LIBS) $(FRAMEWORKS)

clean:
	rm -rf image video_objc video_many video portaudio miniaudio *.dSYM
