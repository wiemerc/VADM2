//
// tlcache.c - part of the Virtual AmigaDOS Machine (VADM)
//             contains the routines that implement the translation cache
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "tlcache.h"
#include "vadm.h"


// The cache is implemented as a binary search tree and stores the mapping of source addresses
// (original code) to destination addresses (translated code). The bits of the source address
// are encoded as path through the tree, from the root node to the final node. The final node
// stores the destination address, either as left or right "successor", depending on the value
// of the LSB.

// initialize TranslationCache object
TranslationCache *tc_init()
{
    TranslationCache *p_tc;
    if ((p_tc = malloc(sizeof(TranslationCache))) == NULL) {
        ERROR("could not allocate memory");
        return NULL;
    }
    if ((p_tc->p_root_node = malloc(sizeof(TranslationCacheNode))) == NULL) {
        ERROR("could not allocate memory");
        return NULL;
    }
    return p_tc;
}

// allocate block of size MAX_CODE_BLOCK_SIZE and store the corresponding source address in the cache
uint8_t *tc_alloc_block_for_addr(TranslationCache *p_tc, const uint8_t *p_src_addr)
{
    // allocate block of memory
    uint8_t *p_dst_addr;
    if ((p_dst_addr = malloc(MAX_CODE_BLOCK_SIZE)) == NULL) {
        ERROR("could not allocate memory");
        return NULL;
    }

    // store source address in cache
    uint32_t curr_bit = 1 << (NUM_SOURCE_ADDR_BITS - 1);
    TranslationCacheNode **pp_curr_node = &(p_tc->p_root_node);
    while (curr_bit > 1) {
        pp_curr_node = ((uint32_t) p_src_addr & curr_bit) ? &((*pp_curr_node)->p_left_node) : &((*pp_curr_node)->p_right_node);
        if (*pp_curr_node == NULL) {
            if ((*pp_curr_node = malloc(sizeof(TranslationCacheNode))) == NULL) {
                ERROR("could not allocate memory");
                return NULL;
            }
            (*pp_curr_node)->p_left_node = NULL;
            (*pp_curr_node)->p_right_node = NULL;
        }
        curr_bit >>= 1;
    }
    if ((uint32_t) p_src_addr & 1)
        (*pp_curr_node)->p_left_node = (TranslationCacheNode *) p_dst_addr;
    else
        (*pp_curr_node)->p_right_node = (TranslationCacheNode *) p_dst_addr;
    return p_dst_addr;
}

// lookup source address in the cache and return the corresponding block if found
uint8_t *tc_get_block_by_addr(TranslationCache *p_tc, const uint8_t *p_src_addr)
{
    uint32_t curr_bit = 1 << (NUM_SOURCE_ADDR_BITS - 1);
    TranslationCacheNode **pp_curr_node = &(p_tc->p_root_node);
    while (curr_bit) {
        pp_curr_node = ((uint32_t) p_src_addr & curr_bit) ? &((*pp_curr_node)->p_left_node) : &((*pp_curr_node)->p_right_node);
        if (*pp_curr_node == NULL)
                return NULL;
        curr_bit >>= 1;
    }
    return (uint8_t *) *pp_curr_node;
}


//
// unit tests
//
#ifdef TEST
int main()
{
    int retval = 0;
    TranslationCache *p_tc = tc_init();
    uint8_t *p1, *p2;

    if ((p1 = tc_alloc_block_for_addr(p_tc, (const uint8_t *) 0x5)) == NULL) {
        ERROR("storing address 0x5 failed");
        ++retval;
    }
    if ((p2 = tc_alloc_block_for_addr(p_tc, (const uint8_t *) 0x6)) == NULL) {
        ERROR("storing address 0x5 failed");
        ++retval;
    }
    if (tc_get_block_by_addr(p_tc, (const uint8_t *) 0x5) != p1) {
        ERROR("looking up address 0x5 failed");
        ++retval;
    }
    if (tc_get_block_by_addr(p_tc, (const uint8_t *) 0x6) != p2) {
        ERROR("looking up address 0x6 failed");
        ++retval;
    }
    if (tc_get_block_by_addr(p_tc, (const uint8_t *) 0x7) != NULL) {
        ERROR("looking up address 0x7 succeeded");
        ++retval;
    }
    return retval;
}
#endif
