#
#  Makefile for the Virtual AmigaDOS Machine (VADM)
#
# Copyright(C) 2019, 2020 Constantin Wiemer
# 


CC      := gcc
AS      := as
CFLAGS  := -I/opt/m68k-amigaos//m68k-amigaos/ndk/include -Wall -Wextra -g

.PHONY: all clean

all: vadm ptrace-test translate hello.bin

clean:
	rm -rf *.o *.dSYM vadm ptrace-test hello.bin

main.o: main.c vadm.h

loader.o: loader.c vadm.h

translate.o: translate.c translate.h

translate: translate.c translate.h
	$(CC) $(CFLAGS) -DTEST -o $@ translate.c

vadm: main.o loader.o translate.o
	$(CC) $(LDFLAGS) -o $@ $^

loop.o: loop.s
	/opt/m68k-amigaos/bin/m68k-amigaos-as -o $@ $^

loop: loop.o
	/opt/m68k-amigaos/bin/m68k-amigaos-gcc -noixemul -nostdlib -o $@.bad $^ && python3 -c 'import sys; data = open(sys.argv[1] + ".bad", "rb").read(); fh = open(sys.argv[1], "wb"); pos = data.find(b"possible\x0a"); fh.write(data[0:pos + 12]); fh.write(data[pos + 16:]); fh.close()' $@ && rm $@.bad

%.o: %.s
	$(AS) -o $@ $^

%.bin: %.o
	objcopy -O binary $< $@

history:
	git log --format="format:%h %ci %s"
