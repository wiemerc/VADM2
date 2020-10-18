//
// vadm.c - part of the Virtual AmigaDOS Machine (VADM)
//          contains the main routine
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "execute.h"
#include "loader.h"
#include "tlcache.h"
#include "translate.h"
#include "vadm.h"
#include "util.h"


int main(int argc, char **argv)
{
    uint8_t *p_m68k_code_addr, *p_x86_code_addr;
    uint32_t m68k_code_size;

    if (argc != 2) {
        ERROR("usage: vadm <program to execute>");
        return 1;
    }
    INFO("loading program...");
    if (!load_program(argv[1], &p_m68k_code_addr, &m68k_code_size)) {
        ERROR("loading program failed");
        return 1;
    }
    INFO("initializing translation cache and setting up first TU...");
    if ((gp_tlcache = tc_init()) == NULL) {
        ERROR("initializing translation cache failed")
    }
    if ((p_x86_code_addr = setup_tu(p_m68k_code_addr)) == NULL) {
        ERROR("setting up TU failed");
        return 1;
    }
    INFO("executing program...");
    if (!exec_program((int (*)()) p_x86_code_addr)) {
        ERROR("executing program failed");
        return 1;
    }
    return 0;
}
