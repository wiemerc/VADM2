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


TranslationCache *gp_tlcache;


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
    INFO("initializing translation cache and translating first TU...");
    if ((gp_tlcache = tc_init()) == NULL) {
        ERROR("initializing translation cache failed")
    }
    if ((p_x86_code_addr = setup_tu(p_m68k_code_addr)) == NULL) {
        ERROR("setting up TU failed");
        return 1;
    }
    if ((translate_tu(p_m68k_code_addr, UINT32_MAX, false)) == NULL) {
        ERROR("translating first TU failed");
        return 1;
    }
    INFO("executing program...");
    if (!exec_program((int (*)()) p_x86_code_addr)) {
        ERROR("executing program failed");
        return 1;
    }
    return 0;
}
