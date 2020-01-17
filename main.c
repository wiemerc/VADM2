//
// loader.c - part of the Virtual AmigaDOS Machine (VADM)
//            This file contains the main routine.
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#define _GNU_SOURCE
#include <fcntl.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <unistd.h>

#include "vadm.h"


int main(int argc, char **argv)
{
    void *m68k_code_addr, *x86_code_addr;
    uint32_t m68k_code_size;

    INFO("loading program");
    if (!load_program(argv[1], &m68k_code_addr, &m68k_code_size)) {
        ERROR("loading program failed");
        return 1;
    }
    INFO("translating code");
    int fd;
    if ((fd = open("/tmp/vadm-output.bin", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
        ERROR("could not open output file: %s", strerror(errno));
        return -1;
    }
    if (fallocate(fd, 0, 0, MAX_CODE_SIZE) == -1) {
        ERROR("could not allocate disk space: %s", strerror(errno));
        return -1;
    }
    if ((x86_code_addr = mmap(NULL, MAX_CODE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        ERROR("could not create memory mapping for translated code: %s", strerror(errno));
        return 1;
    }
    if (!translate_code_block(m68k_code_addr, x86_code_addr, m68k_code_size)) {
        ERROR("translating code failed");
        return 1;
    }
    return 0;
}
