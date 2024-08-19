CC       := clang
CFLAGS   := -I./include -Ofast
LFLAGS   := -lavformat -lavcodec -lavdevice -lswscale -lswresample -lavutil -lglfw3 -framework OpenGL -framework IOKit -framework Cocoa


main: main.c ffplay.c
	cc $(CFLAGS) main.c ffplay.c -o ffplay $(LFLAGS)

clean:
	rm -r ffplay *.dSYM
