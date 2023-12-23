clean: run
	rm a.out


run: build
	./a.out

debug:
	mkdir -p build
	clang -Wall -Wextra -pedantic -I/opt/homebrew/include -g3 -o build/main main.c -L/opt/homebrew/lib -lavformat -lavcodec


build: main.c
	clang -Wall -Wextra -pedantic -I/opt/homebrew/include -O3 main.c -L/opt/homebrew/lib -lavformat -lavcodec -lsdl2 -lsdl2_image
