CFLAGS = -std=c17 -Wall -Wextra -Werror
all:
	gcc main.c -o chip8 $(CFLAGS) `sdl2-config --cflags --libs`
debug :
	gcc main.c -o chip8 $(CFLAGS) `sdl2-config --cflags --libs` -DDEBUG