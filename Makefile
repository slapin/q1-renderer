CFLAGS = -g -ggdb2 -O0 -Wall -Wuninitialized
SRCM_NORMAL = main.c
SRCM_NULL = main_null.c
SRCS = poly.c triangle.c
OBJS_NORMAL = $(SRCM_NORMAL:.c=.o) $(SRCS:.c=.o)
OBJS_NULL = $(SRCM_NULL:.c=.o) $(SRCS:.c=.o)
all: poly poly-null
poly: $(OBJS_NORMAL)
	$(CC) $(CFLAGS) -o poly $(OBJS_NORMAL) -lpng -lm `allegro-config --libs`

poly-null: $(OBJS_NULL)
	$(CC) $(CFLAGS) -o poly-null $(OBJS_NULL) -lpng -lm
clean:
	rm -f $(OBJS_NORMAL) $(OBJS_NULL)

.PHONY: all clean

poly.o: poly.c triangle.h
triangle.o: triangle.c triangle.h

