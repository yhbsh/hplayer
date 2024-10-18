HEADERS    = -Wall -Wextra -std=c11 -O3 -I./deps/include -DGL_SILENCE_DEPRECATION 
LIBRARIES  = -L./deps/lib -lglfw -lavformat -lavcodec -lswscale -lswresample -lavutil
FRAMEWORKS = -framework CoreMedia -framework CoreVideo -framework Videotoolbox -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework AudioToolbox

all: main main_objc

main: main.c
	clang $(HEADERS) main.c -o main $(LIBRARIES) $(FRAMEWORKS)

main_objc: main.m
	clang $(HEADERS) main.m -o main $(LIBRARIES) $(FRAMEWORKS)
