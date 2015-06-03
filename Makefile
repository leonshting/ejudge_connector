CC= gcc
CFLAGS= -c -Wall
CFLAGS += -O3

#SOURCES = cdecode.c cencode.c c_example1.c
#TARGETS = first

all: checker

checker: first.o usefull_trash.o cJSON.o cdecode.o cencode.o c_example1.o ezxml.o
	$(CC) first.o usefull_trash.o cJSON.o cdecode.o cencode.o c_example1.o ezxml.o -lm -lcurl -lssl -lcrypto -o checker

first.o: first.c cJSON.h usefull_trash.h c_example1.h first.h
	$(CC) $(CFLAGS) first.c

usefull_trash.o: usefull_trash.c
	$(CC) $(CFLAGS) usefull_trash.c

cJSON.o: cJSON.c
	$(CC) $(CFLAGS) cJSON.c

cdecode.o: cdecode.c cdecode.h
	$(CC) $(CFLAGS) cdecode.c

cencode.o: cencode.c cencode.h
	$(CC) $(CFLAGS) cencode.c

c_example1.o: c_example1.c c_example1.h cencode.h cdecode.h
	$(CC) $(CFLAGS) c_example1.c

ezxml.o: ezxml.c ezxml.h
	$(CC) $(CFLAGS) ezxml.c

clean:
	rm -f *o checker



