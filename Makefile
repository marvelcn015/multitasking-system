CC=gcc
CFLAGS=-m32

all: mtx

mtx: t.o ts.o
	$(CC) $(CFLAGS) t.o ts.o -o mtx

t.o: t.c
	$(CC) $(CFLAGS) -c t.c

ts.o: ts.s
	$(CC) $(CFLAGS) -c ts.s

clean:
	rm -f *.o mtx 