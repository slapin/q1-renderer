CFLAGS = -g -O0 -Wall
all: poly
poly: poly.o
	$(CC) $(CFLAGS) -o poly poly.o -lm

