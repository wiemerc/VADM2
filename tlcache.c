//
// tlcache.c - part of the Virtual AmigaDOS Machine (VADM)
//             contains the routines that implement the translation cache
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "tlcache.h"
#include "vadm.h"


// initialize cache = allocate memory for the cache + the TranslationCache object, backed by a file
TranslationCache *tc_init()
{
    int fd;
    if ((fd = open("/tmp/vadm-tc.bin", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
        ERROR("could not open output file: %s", strerror(errno));
        return NULL;
    }
    if (fallocate(fd, 0, 0, sizeof(TranslationCache) + MAX_CODE_SIZE) == -1) {
        ERROR("could not allocate disk space: %s", strerror(errno));
        return NULL;
    }
    TranslationCache *tc;
    if ((tc = mmap(NULL, sizeof(TranslationCache) + MAX_CODE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
        ERROR("could not create memory mapping for translated code: %s", strerror(errno));
        return NULL;
    }
    tc->tc_start_addr = ((uint8_t *) tc) + sizeof(TranslationCache);
    tc->tc_next_addr = tc->tc_start_addr;
    return tc;
}

// hand out next available block of size MAX_CODE_BLOCK_SIZE
uint8_t *tc_get_next_block(TranslationCache *tc)
{
    uint8_t *baddr = NULL;
    if (tc->tc_next_addr < (tc->tc_start_addr + MAX_CODE_SIZE)) {
        baddr = tc->tc_next_addr;
        tc->tc_next_addr += MAX_CODE_BLOCK_SIZE;
        DEBUG("handing out block from translation cache at address %p", baddr);
    }
    else {
        ERROR("no more free blocks available in translation cache");
    }
    return baddr;
}
