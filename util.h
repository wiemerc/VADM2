//
// util.h - part of the Virtual AmigaDOS Machine (VADM)
//          header file for the utility functions
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 
#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

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

#endif
