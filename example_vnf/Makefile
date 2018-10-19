CC = gcc
OPTS = -O3
DEBUG_OPTS = -pg
CFLAGS = -g

all: udp-mirror

udp-mirror: main.o
	$(CC) $(OPTS) -o udp-mirror main.o -lm

debug: main.o newhash.o
	$(CC) $(DEBUG_OPTS)  -o udp-mirror main.o -lm

main.o: main.c

clean:
	rm -f udp-mirror main.o
