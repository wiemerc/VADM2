//
// execute.h - part of the Virtual AmigaDOS Machine (VADM)
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 
#ifndef EXECUTE_H_INCLUDED
#define EXECUTE_H_INCLUDED

#include <capstone/capstone.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#define LIB_BASE_START_ADDRESS 0x00200000
#define LIB_JUMP_TBL_SIZE 8192
#define OPCODE_INT_3        0xcc
#define OPCODE_JMP_REL32    0xe9
#define OPCODE_JMP_ABS64    0xff

typedef struct
{
    uint16_t offset;
    char     *p_name;
    void     (*p_func)();
} FuncInfo;

// TODO: exporting load_library() only doesn't work
__attribute__ ((visibility("default"))) uint8_t *load_library(const char *p_lib_name);
bool exec_program(int (*p_code)());

#endif  // EXECUTE_H_INCLUDED
