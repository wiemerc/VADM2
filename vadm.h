//
// vadm.h - part of the Virtual AmigaDOS Machine (VADM)
//          global header file for VADM
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 
#ifndef VADM_H_INCLUDED
#define VADM_H_INCLUDED

#include <stdio.h>
#include <string.h>

// logging macros
// TODO: add file name / line number
#define DEBUG(fmt, ...) {fputs("DEBUG | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define INFO(fmt, ...)  {fputs("INFO  | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define WARN(fmt, ...)  {fputs("WARN  | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define ERROR(fmt, ...) {fputs("ERROR | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define CRIT(fmt, ...)  {fputs("CRIT  | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}

// base address of Exec library, the only absolute address in the AmigaOS, can't use 4 (the value
// in the AmigaOS) because it has to be a page boundary as we create a memory mapping there
#define ABS_EXEC_BASE 0x00300000

#endif
