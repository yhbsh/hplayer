CC       := clang
CFLAGS   := $(shell pkg-config --cflags libavcodec libavformat libavdevice libswscale libswresample libavutil)
CFLAGS   += -Wall -O3
LFLAGS   := $(shell pkg-config --libs libavcodec libavformat libavdevice libswscale libswresample libavutil glfw3)


main: main.c pl.c
	cc $(CFLAGS) main.c pl.c -o pl $(LFLAGS)

clean:
	rm -rf pl *.dSYM
