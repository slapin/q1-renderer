all: poly
poly: poly.c
	$(CC) -Wall -o poly poly.c -lm

