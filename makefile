all: ./src/main.c video.o
	gcc ./src/main.c ./obj/video.o -lSDL2 -o main

video.o: src/video.c obj
	gcc -c src/video.c  -o ./obj/video.o

obj: 
	mkdir obj

debug:
	gcc -g src/main.c src/video.c -lSDL2 -o debug

clear:
	rm -rf obj
