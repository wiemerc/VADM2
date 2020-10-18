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

execute.o: execute.c execute.h codegen.h vadm.h util.h

execute: execute.c execute.h codegen.h codegen.o vadm.h util.h util.o
	$(CC) $(CFLAGS) -DTEST -o execute.test.o -c execute.c
	$(CC) $(CFLAGS) -o $@ execute.test.o util.o $(LDLIBS)

loader.o: loader.c loader.h vadm.h util.h

tlcache.o: tlcache.c tlcache.h vadm.h util.h

tlcache: tlcache.c tlcache.h vadm.h util.h
	$(CC) $(CFLAGS) -DTEST -o tlcache.test.o -c tlcache.c
	$(CC) $(CFLAGS) -o $@ tlcache.test.o util.o

translate.o: translate.c translate.h codegen.h tlcache.h vadm.h util.h

translate: translate.c translate.h codegen.h codegen.o tlcache.h tlcache.o vadm.h util.h util.o
	$(CC) $(CFLAGS) -DTEST -o translate.test.o -c translate.c
	$(CC) $(CFLAGS) -o $@ translate.test.o codegen.o tlcache.o util.o

vadm.o: vadm.c vadm.h

vadm: codegen.o execute.o loader.o tlcache.o translate.o vadm.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

util.o: util.c util.h

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
