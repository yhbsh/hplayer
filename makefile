CFLAGS=-O3 `pkg-config --cflags glfw3 libavcodec libavformat sdl2`
LDFLAGS=`pkg-config --libs glfw3 libavcodec libavformat sdl2` -framework opengl

CC=clang

all: ffprobe opengl hplayer parse

hplayer: hplayer.c
	$(CC) $(CFLAGS) hplayer.c -o hplayer $(LDFLAGS)

opengl: opengl.c
	$(CC) $(CFLAGS) opengl.c  -o opengl $(LDFLAGS)

ffprobe: ffprobe.c
	$(CC) $(CFLAGS) ffprobe.c -o ffprobe $(LDFLAGS)

parse: parse.c
	$(CC) $(CFLAGS) parse.c -o parse $(LDFLAGS)
