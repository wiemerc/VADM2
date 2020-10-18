//
// util.c - part of the Virtual AmigaDOS Machine (VADM)
//          contains the utility routines
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "util.h"


void logmsg(const char *fname, int lineno, const char *func, const char *level, const char *fmtstr, ...)
{
    char location[32];
    snprintf(location, 32, "%s:%d", fname, lineno);
    printf("%-20s | %-20s | %-5s | ", location, func, level);

    va_list args;
    va_start(args, fmtstr);
    vprintf(fmtstr, args);
    va_end(args);
    printf("\n");
}
