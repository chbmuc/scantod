OBJECTS=scanto.o
HEADERS=scanto.h

CC=gcc
CFLAGS=-g -Wall -I/usr/include/libxml2 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include

$(OBJECTS):	$(@:%.o=%.c) $(HEADERS)

scantod:	scanto.o scantod.o
	$(CC) scanto.o scantod.o -o scantod -lxml2 -lglib-2.0

