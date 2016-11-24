CC=gcc

CFLAG=-c -D_GNU_SOURCE -Wall -std=c99 -pthread
LFLAG=-pthread

#all: target target/main
all: target target/main

target/main: target/main.o target/tree.o
	$(CC) $(LFLAG) target/main.o target/tree.o -o target/main

target/main.o: main.c tree.h
	$(CC) $(CFLAG) main.c -o target/main.o

target/tree.o: tree.c tree.h
	$(CC) $(CFLAG) tree.c -o target/tree.o

target:
	mkdir -p target

clean:
	rm -rf target
