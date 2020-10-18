//
// execute.h - part of the Virtual AmigaDOS Machine (VADM)
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 
#ifndef EXECUTE_H_INCLUDED
#define EXECUTE_H_INCLUDED

#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#define LIB_BASE_START_ADDRESS 0x00200000
#define LIB_JUMP_TBL_SIZE 0x10000

typedef struct
{
    uint16_t offset;
    char     *p_name;
    char     *p_arg_regs;
    void     (*p_func)();
} FuncInfo;

// see https://stackoverflow.com/questions/52719364/how-to-use-the-attribute-visibilitydefault and
// https://stackoverflow.com/questions/36692315/what-exactly-does-rdynamic-do-and-when-exactly-is-it-needed
// for details on how to export certain symbols only
__attribute__ ((visibility("default"))) uint8_t *load_library(const char *p_lib_name);
bool exec_program(int (*p_code)());

#endif  // EXECUTE_H_INCLUDED
