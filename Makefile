CC=gcc
CFLAGS = -g -W -Wall

all: KeepTrollin

KeepTrollin: KeepTrollin.o
	$(CC) -o KeepTrollin KeepTrollin.o 

KeepTrollin.o: KeepTrollin.c 
	$(CC) $(CFLAGS) -c KeepTrollin.c

clean:
	rm -f KeepTrollin KeepTrollin.o
