//
// vadm.h - part of the Virtual AmigaDOS Machine (VADM)
//          This file is the global header file for VADM.
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


// logging macros
#define DEBUG(fmt, ...) {fputs("DEBUG | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define INFO(fmt, ...)  {fputs("INFO  | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define WARN(fmt, ...)  {fputs("WARN  | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define ERROR(fmt, ...) {fputs("ERROR | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define CRIT(fmt, ...)  {fputs("CRIT  | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}

// function prototypes
bool load_program(const char *fname, void **code_address, uint32_t *code_size);
bool translate_code_block(const uint8_t *inptr, uint8_t *outptr, int size);

// constants for the loader
#define HUNK_START_ADDRESS  0x00100000
#define MAX_HUNKS           4         // HUNK_CODE, HUNK_DATA, HUNK_BSS and one hunk just in case...
#define MAX_HUNK_SIZE       65536     // 64KB should be more than enough for any example program

// constants for the code translation
#define MAX_CODE_SIZE   65536
