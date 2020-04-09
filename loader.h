//
// loader.h - part of the Virtual AmigaDOS Machine (VADM)
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 
#ifndef LOADER_H_INCLUDED
#define LOADER_H_INCLUDED

#include <ctype.h>
#include <fcntl.h>
#include <netinet/in.h>         // for ntohs() and ntohl()
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dos/doshunks.h>       // from Amiga OS

// constants
#define HUNK_START_ADDRESS  0x00100000
#define MAX_HUNKS           4         // HUNK_CODE, HUNK_DATA, HUNK_BSS and one hunk just in case...
#define MAX_HUNK_SIZE       65536     // 64KB should be more than enough for any example program

// prototypes
bool load_program(const char *fname,  uint8_t **code_address, uint32_t *code_size);

#endif
