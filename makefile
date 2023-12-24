CFLAGS = `pkg-config --cflags libavformat libavcodec libavutil sdl2 sdl2_image` -Wall -Wextra -pedantic
LIBS = `pkg-config --libs libavformat libavcodec libavutil sdl2 sdl2_image` -Wall -Wextra -pedantic



build: hplayer.c
	clang $(CFLAGS) -O3 -o hplayer hplayer.c $(LIBS)

debug:
	mkdir -p build
	clang $(CFLAGS) -g3 -o build/hplayer hplayer.c $(LIBS)

clean:
	rm hplayer

