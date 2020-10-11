//
// vadm.h - part of the Virtual AmigaDOS Machine (VADM)
//          global header file for VADM
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 
#ifndef VADM_H_INCLUDED
#define VADM_H_INCLUDED

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// logging macros
void logmsg(const char *fname, int lineno, const char *func, const char *level, const char *fmtstr, ...);
#ifdef VERBOSE_LOGGING
    #define DEBUG(fmtstr, ...) {logmsg(__FILE__, __LINE__, __func__, "DEBUG", fmtstr, ##__VA_ARGS__);}
#else
    #define DEBUG(fmtstr, ...) {}
#endif
#define INFO(fmtstr, ...) {logmsg(__FILE__, __LINE__, __func__, "INFO", fmtstr, ##__VA_ARGS__);}
#define WARN(fmtstr, ...) {logmsg(__FILE__, __LINE__, __func__, "WARN", fmtstr, ##__VA_ARGS__);}
#define ERROR(fmtstr, ...) {logmsg(__FILE__, __LINE__, __func__, "ERROR", fmtstr, ##__VA_ARGS__);}
#define CRIT(fmtstr, ...) {logmsg(__FILE__, __LINE__, __func__, "CRIT", fmtstr, ##__VA_ARGS__);}

// base address of Exec library, the only absolute address in the AmigaOS, can't use 4 (the value
// in the AmigaOS) because it has to be a page boundary as we create a memory mapping there
#define ABS_EXEC_BASE 0x00300000

#endif
