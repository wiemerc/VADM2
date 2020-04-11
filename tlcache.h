//
// tlcache.h - part of the Virtual AmigaDOS Machine (VADM)
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 
#ifndef TLCACHE_H_INCLUDED
#define TLCACHE_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// constants
#define MAX_CODE_SIZE   65536
#define MAX_CODE_BLOCK_SIZE 256
#ifdef TEST
    #define NUM_SOURCE_ADDR_BITS 3
#else
    #define NUM_SOURCE_ADDR_BITS 21
#endif

// structures to implement the translation cache
struct TranslationCacheNode
{
    struct TranslationCacheNode *p_left_node;
    struct TranslationCacheNode *p_right_node;
};
typedef struct TranslationCacheNode TranslationCacheNode;
struct TranslationCache
{
    TranslationCacheNode *p_root_node;  // root node of the binary tree used to look up addresses
};
typedef struct TranslationCache TranslationCache;

// prototypes
TranslationCache *tc_init();
bool tc_put_addr(TranslationCache *p_tc, const uint8_t *p_src_addr, const uint8_t *p_dst_addr);
uint8_t *tc_get_addr(TranslationCache *p_tc, const uint8_t *p_src_addr);

#endif  // TLCACHE_H_INCLUDED
