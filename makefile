CC = gcc
CFLAGS = -Wall
DEBUG_FLAGS = -g -DDEBUG

all: wordcount

threadpool.o: threadpool.c
	$(CC) $(CFLAGS) -c threadpool.c -o threadpool.o

mapreduce.o: mapreduce.c
	$(CC) $(CFLAGS) -c mapreduce.c -o mapreduce.o

distwc.o: distwc.c
	$(CC) $(CFLAGS) -c distwc.c -o distwc.o

wordcount: threadpool.o mapreduce.o distwc.o
	$(CC) $(CFLAGS) threadpool.o mapreduce.o distwc.o -o wordcount

debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean wordcount
	gdb ./wordcount

clean:
	rm -f *.o wordcount 