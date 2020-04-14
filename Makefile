#
#  Makefile for the Virtual AmigaDOS Machine (VADM)
#
# Copyright(C) 2019, 2020 Constantin Wiemer
# 


CC      := gcc
AS      := as
CFLAGS  := -I/opt/m68k-amigaos//m68k-amigaos/ndk/include -Wall -Wextra -g
LDLIBS  := -lcapstone

.PHONY: all clean

all: vadm ptrace-test hello.bin loop

clean:
	rm -rf *.o *.dSYM vadm ptrace-test translate tlcache hello.bin loop

ptrace-test: ptrace-test.c
	$(CC) $(CFLAGS) -o $@ $^ -lcapstone

vadm.o: vadm.c vadm.h

loader.o: loader.c loader.h vadm.h

translate.o: translate.c translate.h tlcache.h vadm.h

translate: translate.c translate.h tlcache.h vadm.h
	$(CC) $(CFLAGS) -DTEST -o $@ translate.c

tlcache.o: tlcache.c tlcache.h vadm.h

tlcache: tlcache.c tlcache.h vadm.h
	$(CC) $(CFLAGS) -DTEST -o $@ tlcache.c

vadm: vadm.o loader.o translate.o tlcache.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

loop.o: loop.s
	/opt/m68k-amigaos/bin/m68k-amigaos-as -o $@ $^

loop: loop.o
	/opt/m68k-amigaos/bin/m68k-amigaos-gcc -noixemul -nostdlib -o $@ $^

%.o: %.s
	$(AS) -o $@ $^

%.bin: %.o
	objcopy -O binary $< $@

history:
	git log --format="format:%h %ci %s"

# TODO: fix tests
tests: translate tlcache
	./translate
	./tlcache
