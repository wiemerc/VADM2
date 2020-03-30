//
// loader.c - part of the Virtual AmigaDOS Machine (VADM)
//            This file contains the main routine.
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include <limits.h>

#include "vadm.h"


int main(int argc, char **argv)
{
    uint8_t *m68k_code_addr, *x86_code_addr;
    uint32_t m68k_code_size;
    TranslationCache *tc;

    INFO("loading program");
    if (!load_program(argv[1], &m68k_code_addr, &m68k_code_size)) {
        ERROR("loading program failed");
        return 1;
    }
    INFO("translating code");
    if ((tc = tc_init()) == NULL) {
        ERROR("initializing translation cache failed")
    }
    if ((x86_code_addr = tc_get_next_block(tc)) == NULL) {
        ERROR("translating code failed");
        return 1;
    }
    if (!translate_code_block(m68k_code_addr, x86_code_addr, UINT32_MAX)) {
        ERROR("translating code failed");
        return 1;
    }
    return 0;
}
