//
// vadm.h - part of the Virtual AmigaDOS Machine (VADM)
//          global header file for VADM
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 
#ifndef VADM_H_INCLUDED
#define VADM_H_INCLUDED

#include <capstone/capstone.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>


// logging macros
// TODO: add file name / line number
#define DEBUG(fmt, ...) {fputs("DEBUG | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define INFO(fmt, ...)  {fputs("INFO  | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define WARN(fmt, ...)  {fputs("WARN  | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define ERROR(fmt, ...) {fputs("ERROR | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}
#define CRIT(fmt, ...)  {fputs("CRIT  | ", stdout); printf(fmt, ##__VA_ARGS__); fputs("\n", stdout);}

#endif
