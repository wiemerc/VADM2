//
// main.c - part of the Virtual AmigaDOS Machine (VADM)
//          contains the main routine
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "loader.h"
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
    if ((x86_code_addr = translate_unit(m68k_code_addr, UINT32_MAX)) == NULL) {
        ERROR("translating code failed");
        return 1;
    }
    return 0;
}
