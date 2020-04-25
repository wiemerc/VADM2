//
// execute.h - part of the Virtual AmigaDOS Machine (VADM)
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 
#ifndef EXECUTE_H_INCLUDED
#define EXECUTE_H_INCLUDED

#include <dlfcn.h>
#include <stdint.h>
#include <sys/errno.h>
#include <sys/mman.h>

#define LIB_JUMP_TBL_SIZE   8192
#define OPCODE_INT_3        0xcc
#define OPCODE_JMP_REL32    0xe9
#define OPCODE_JMP_ABS64    0xff

typedef struct
{
    uint16_t offset;
    char     *p_name;
    void     (*p_func)();
} FuncInfo;

#endif
