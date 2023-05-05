CC=gcc
CFLAGS=-Wall -g

homework_2: homework_2.o
	$(CC) $(CFLAGS) -o homework_2 homework_2.o

homework_2.o: homework_2.c 
	$(CC) $(CFLAGS) -c homework_2.c

clean:
	rm -f homework_2 homework_2.o
