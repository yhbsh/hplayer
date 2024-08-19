CC       := clang
CFLAGS   := -I./include -Ofast
LFLAGS   := -lavformat -lavcodec -lavdevice -lswscale -lswresample -lavutil -lglfw3 -framework OpenGL -framework IOKit -framework Cocoa


main: main.c pl.c
	cc $(CFLAGS) main.c pl.c -o pl $(LFLAGS)

clean:
	rm -rf pl *.dSYM
