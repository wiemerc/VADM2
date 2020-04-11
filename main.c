//
// main.c - part of the Virtual AmigaDOS Machine (VADM)
//          contains the main routine
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "loader.h"
#include "tlcache.h"
#include "translate.h"
#include "vadm.h"


int main(int argc, char **argv)
{
    uint8_t *m68k_code_addr, *x86_code_addr;
    uint32_t m68k_code_size;

    INFO("loading program");
    if (!load_program(argv[1], &m68k_code_addr, &m68k_code_size)) {
        ERROR("loading program failed");
        return 1;
    }
    INFO("translating code");
    if ((g_p_tlcache = tc_init()) == NULL) {
        ERROR("initializing translation cache failed")
    }
    if ((x86_code_addr = tc_alloc_block_for_addr(g_p_tlcache, m68k_code_addr)) == NULL) {
        ERROR("translating code failed");
        return 1;
    }
    if (!translate_code_block(m68k_code_addr, x86_code_addr, UINT32_MAX)) {
        ERROR("translating code failed");
        return 1;
    }
    return 0;
}
