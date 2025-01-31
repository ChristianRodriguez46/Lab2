all: lab2

lab2:
	g++ lab2.cpp libggfonts.a -Wall -lX11 -lGL -lGLU -lm -o lab2
clean:
	rm -f lab2
