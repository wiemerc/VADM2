#
#  Makefile - part of the Virtual AmigaDOS Machine (VADM)
#			  Makefile for the libraries
#
# Copyright(C) 2019, 2020 Constantin Wiemer
# 


CC      := gcc
CFLAGS  := -I/opt/m68k-amigaos//m68k-amigaos/ndk/include -Wall -Wextra -g -fPIC
LDFLAGS := -shared
LDLIBS  := -lbsd

.PHONY: all clean

all: libexec.so libdos.so

clean:
	rm -rf *.o *.dSYM *.so

%.so: %.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
