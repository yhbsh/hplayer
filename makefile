build: hplayer.c
	clang -Wall -Wextra -pedantic -I/opt/homebrew/include -O3 -o hplayer hplayer.c -L/opt/homebrew/lib -lavformat -lavcodec -lswscale -lavutil -lsdl2 -lsdl2_image

debug:
	mkdir -p build
	clang -Wall -Wextra -pedantic -I/opt/homebrew/include -g3 -o build/main main.c -L/opt/homebrew/lib -lavformat -lavcodec -lswscale -lavutil -lsdl2 -lsdl2_image



clean:
	rm hplayer

