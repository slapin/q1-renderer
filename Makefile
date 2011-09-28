CFLAGS = -g -ggdb2 -O0 -Wall
all: poly
poly: poly.o
	$(CC) $(CFLAGS) -o poly poly.o -lpng -lm

clean:
	rm -f poly.o poly

.PHONY: all clean

