//
// vadm.h - part of the Virtual AmigaDOS Machine (VADM)
//          This file is the global header file for VADM.
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include <stdio.h>


// logging macros
#define DEBUG(fmt, ...) {fputs("DEBUG | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define INFO(fmt, ...)  {fputs("INFO  | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define WARN(fmt, ...)  {fputs("WARN  | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define ERROR(fmt, ...) {fputs("ERROR | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define CRIT(fmt, ...)  {fputs("CRIT  | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}

// function prototypes
int load_program(const char *fname, void **hunk_addresses);

// constants for the loader
#define MAX_HUNKS     4         // HUNK_CODE, HUNK_DATA, HUNK_BSS and one hunk just in case...
#define MAX_HUNK_SIZE 65536     // 64KB should be more than enough for any example program
