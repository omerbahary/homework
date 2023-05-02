CC=gcc
CFLAGS=-Wall -g

hw1shell: hw1shell.o
	$(CC) $(CFLAGS) -o hw1shell hw1shell.o

hw1shell.o: hw1shell.c hw1shell.h
	$(CC) $(CFLAGS) -c hw1shell.c

clean:
	rm -f hw1shell hw1shell.o
