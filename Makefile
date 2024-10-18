HEADERS    = -Wall -Wextra -std=c11 -O3 -I./deps/include 
LIBRARIES  = -L./deps/lib -lglfw -lavformat -lavcodec -lswscale -lswresample -lavutil
FRAMEWORKS = -framework CoreMedia -framework CoreVideo -framework Videotoolbox -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework AudioToolbox

main: main.c
	clang $(HEADERS) main.c -o main $(LIBRARIES) $(FRAMEWORKS)
