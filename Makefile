#
#  Makefile - part of the Virtual AmigaDOS Machine (VADM)
#			  global Makefile
#
# Copyright(C) 2019, 2020 Constantin Wiemer
# 


CC      := gcc
AS      := as
CFLAGS  := -I/opt/m68k-amigaos//m68k-amigaos/ndk/include -Wall -Wextra -g -fvisibility=hidden -DVERBOSE_LOGGING
LDFLAGS := -rdynamic
LDLIBS  := -ldl

.PHONY: all clean libs tests history

all: vadm loop libs

clean:
	rm -rf *.o *.dSYM vadm translate tlcache execute loop
	$(MAKE) --directory=libs clean

vadm.o: vadm.c vadm.h

loader.o: loader.c loader.h vadm.h

translate.o: translate.c translate.h tlcache.h vadm.h

translate: translate.c translate.h vadm.h tlcache.h tlcache.o
	$(CC) $(CFLAGS) -DTEST -o translate.test.o -c translate.c
	$(CC) $(CFLAGS) -o $@ translate.test.o tlcache.o

tlcache.o: tlcache.c tlcache.h vadm.h

tlcache: tlcache.c tlcache.h vadm.h
	$(CC) $(CFLAGS) -DTEST -o $@ tlcache.c

execute.o: execute.c execute.h vadm.h

execute: execute.c execute.h vadm.h
	$(CC) $(CFLAGS) -DTEST -o $@ execute.c $(LDLIBS)

vadm: vadm.o loader.o translate.o tlcache.o execute.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

loop.o: loop.s
	/opt/m68k-amigaos/bin/m68k-amigaos-as -o $@ $^

loop: loop.o
	/opt/m68k-amigaos/bin/m68k-amigaos-gcc -noixemul -nostdlib -o $@ $^

%.o: %.s
	$(AS) -o $@ $^

libs:
	$(MAKE) --directory=$@

history:
	git log --format="format:%h %ci %s"

tests: translate tlcache execute
	./translate
	./tlcache
	./execute
