# makefile for Magnetic

CC = gcc
CFLAGS = -O3 -ffast-math -I../generic
LIBS = -lamp -lalleg -lpng -lz -lm
OBJS = allegro.o emu.o

default : magnetic.exe
.PHONY  : clean

clean :
	del *.o

allegro.o : allegro.c
	$(CC) $(CFLAGS) -s -o $@ -c $<

emu.o : ../generic/emu.c
	$(CC) $(CFLAGS) -s -o $@ -c $<

magnetic.exe : $(OBJS)
	$(CC) -s -o $@ $(OBJS) $(LIBS)

