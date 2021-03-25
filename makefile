CC=gcc
CFLAGS=-I. -std=c99
DEP=goatmalloc.h
OBJ=goatmalloc.o test_goatmalloc.o

%.o: %.c $(DEP)
		$(CC) -c -o $@ $< $(CFLAGS)

all: goatmalloc

goatmalloc: $(OBJ)
		$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm goatmalloc
