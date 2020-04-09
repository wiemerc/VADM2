//
// tlcache.h - part of the Virtual AmigaDOS Machine (VADM)
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 
#ifndef TLCACHE_H_INCLUDED
#define TLCACHE_H_INCLUDED

#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <unistd.h>

// constants
#define MAX_CODE_SIZE   65536
#define MAX_CODE_BLOCK_SIZE 256

// structure to implement the translation cache
typedef struct
{
    uint8_t *tc_start_addr;             // address of the allocated memory block
    uint8_t *tc_next_addr;              // address of the next code block (to be handed out)
} TranslationCache;

// global TranslationCache object
TranslationCache *g_tlcache;

// prototypes
TranslationCache *tc_init();
uint8_t *tc_get_next_block(TranslationCache *tc);

#endif
