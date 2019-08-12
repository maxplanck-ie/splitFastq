CC ?= gcc
LDFLAGS =
CFLAGS = 
PREFIX ?= /usr/local/bin

all: splitFastq

splitFastq:
	$(CC) -o $@ splitFastq.c $(CFLAGS) $(LDFLAGS) -lz

install: splitFastq
	install splitFastq $(PREFIX)/bin

clean:
	rm -f *.o splitFastq
