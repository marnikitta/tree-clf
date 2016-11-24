CC=gcc

CFLAG=-c -Wall -std=c99

#all: target target/main
all: target target/main

target/main: target/main.o target/tree.o
	$(CC) target/main.o target/tree.o -o target/main

target/main.o: main.c tree.h
	$(CC) $(CFLAG) main.c -o target/main.o

target/tree.o: tree.c tree.h
	$(CC) $(CFLAG) tree.c -o target/tree.o

pt: pth-test.c
	$(CC) -pthread pth-test.c -o target/pt

target:
	mkdir -p target

clean:
	rm -rf target
