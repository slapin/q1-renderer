CFLAGS = -g -ggdb2 -O0 -Wall
all: poly poly-null
poly: poly.o main.o
	$(CC) $(CFLAGS) -o poly main.o poly.o -lpng -lm `allegro-config --libs`

poly-null: poly.o main_null.o
	$(CC) $(CFLAGS) -o poly-null main_null.o poly.o -lpng -lm
clean:
	rm -f poly.o main.o poly poly_null.o

.PHONY: all clean

