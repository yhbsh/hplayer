CC       := clang
CFLAGS   := $(shell pkg-config --cflags libavcodec libavformat libavutil glfw3)
CFLAGS   += -Wall -Wextra -O2
LFLAGS   := $(shell pkg-config --libs libavcodec libavformat libavutil glfw3)

main: main.c pl.c
	cc $(CFLAGS) main.c pl.c -o pl $(LFLAGS)

clean:
	rm -rf pl *.dSYM
