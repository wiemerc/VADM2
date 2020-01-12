//
// loader.c - part of the Virtual AmigaDOS Machine (VADM)
//            This file contains the loader for executables in Amiga Hunk format.
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include <ctype.h>
#include <fcntl.h>
#include <netinet/in.h>         // for ntohs() and ntohl()
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// Amiga OS headers
#include <dos/doshunks.h>

#include "vadm.h"


//
// read one dword from the mapped executable and advance current position pointer
//
static uint32_t read_dword(void **ptr)
{
    uint32_t val = ntohl(*((uint32_t *) *ptr));
    *ptr += 4;
    return val;
}


//
// signal handler for SIGSEGV
//
static void sigsegv(int signum)
{
    CRIT("segmentation fault occurred while loading program image");
    exit(1);
}


//
// load the program image
//
bool load_program(
    const char *fname,              // IN: name of the program image
    void       **code_address,      // OUT: start address of the code (the HUNK_CODE block in the hunk)
    uint32_t   *code_size           // OUT: size of the code block
)
{
    // install our own signal handler for SIGSEGV
    DEBUG("installing signal handler for SIGSEGV");
    struct sigaction new_act, old_act;
    new_act.sa_handler = sigsegv;
    new_act.sa_flags   = 0;
    sigemptyset(&new_act.sa_mask);
    if (sigaction(SIGSEGV, &new_act, &old_act) == -1) {
        ERROR("failed to install signal handler: %s", strerror(errno));
        return false;
    }

    // map whole image into memory
    DEBUG("mapping file '%s' into memory", fname);
    int fd;
    if ((fd = open(fname, O_RDONLY)) == -1) {
        ERROR("could not open file: %s", strerror(errno));
        return false;
    }
    struct stat stat_info;
    if (fstat(fd, &stat_info) == -1) {
        ERROR("could not get file status: %s", strerror(errno));
        return false;
    }
    void  *sof, *eof;                   // start-of-file and end-of-file pointers
    if ((sof = mmap(NULL, stat_info.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
        ERROR("could not memory-map file: %s", strerror(errno));
        return false;
    }
    eof = ((uint8_t *) sof) + stat_info.st_size;
    DEBUG("file mapped at address %p", sof);

    DEBUG("reading individual hunks");
    void        *pos       = sof;               // current position in file
    void        *hunk_addresses[MAX_HUNKS];     // start addresses of the hunks
    uint32_t     hunk_num  = 0;                 // current hunk number
    uint32_t     block_type;                    // current block type
    uint32_t     ndwords;                       // number of dwords in current block
    while(pos < eof) {
        DEBUG("reading next block of hunk #%d", hunk_num);
        block_type = read_dword(&pos);
        switch(block_type) {
            case HUNK_HEADER:
                DEBUG("block type is HUNK_HEADER");
                if (read_dword(&pos) != 0) {
                    ERROR("executables that specify resident libraries in header are not supported");
                    return false;
                }
                read_dword(&pos);       // skip total number of hunks (including resident libraries and overlay hunks)
                uint32_t first_hnum = read_dword(&pos);
                uint32_t last_hnum = read_dword(&pos);
                if ((last_hnum - first_hnum + 1) > MAX_HUNKS) {
                    ERROR("executables with more than %d hunks are not supported", MAX_HUNKS);
                    return false;
                }

                // create memory mapping for all hunks with their maximum size
                // The naming is a bit confusing here. An executable usually contains at least 3 hunks, one
                // for code, one for data and one for BSS. A hunk consists of several blocks, starting with
                // a block containing the actual code or data and its size (HUNK_CODE, HUNK_DATA, HUNK_BSS),
                // optionally followed by blocks containing symbols (HUNK_SYMBOL) and relocations (HUNK_RELOC32)
                // and is ended by a HUNK_END block. The first hunk (usually the code hunk) starts with
                // the file header (HUNK_HEADER).
                // We use a fixed 32-bit address so that the loader can do the relocations = add the hunk
                // addresses to the offsets in the code. In additiion we don't need to deal with 
                // 64-bit addresses in the translation phase, which makes things a bit easier.
                DEBUG("creating memory mapping for hunks");
                void *hunk_addr;
                if ((hunk_addr = mmap((void *) HUNK_START_ADDRESS, MAX_HUNKS * MAX_HUNK_SIZE, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_ANON | MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
                    ERROR("could not create memory mapping for hunks: %s", strerror(errno));
                    return false;
                }

                // read sizes of the HUNK_CODE, HUNK_DATA and HUNK_BSS blocks and store pointers
                // to the memory regions where these blocks will be copied to
                for (uint32_t i = first_hnum; i <= last_hnum; i++) {
                    uint32_t hunk_size = read_dword(&pos);
                    DEBUG("size (in bytes) of hunk #%d = %d, will be stored at %p", i, hunk_size * 4, hunk_addr);
                    hunk_addresses[i] = hunk_addr;
                    hunk_addr += MAX_HUNK_SIZE;
                }
                break;

            case HUNK_CODE:
            case HUNK_DATA:
                DEBUG("block type is HUNK_CODE / HUNK_DATA");
                ndwords = read_dword(&pos);
                DEBUG("copying code / data (%d bytes) to mapped memory region at %p", ndwords * 4, hunk_addresses[hunk_num]);
                memcpy(hunk_addresses[hunk_num], pos, ndwords * 4);
                pos += ndwords * 4;

                if (block_type == HUNK_CODE) {
                    *code_address = hunk_addresses[hunk_num];
                    *code_size    = ndwords * 4;
                }
                break;

            case HUNK_BSS:
                DEBUG("block type is HUNK_BSS");
                ndwords = read_dword(&pos);
                DEBUG("zeroing mapped memory region at %p (%d bytes)", hunk_addresses[hunk_num], ndwords * 4);
                memset(hunk_addresses[hunk_num], 0, ndwords * 4);
                break;

            case HUNK_RELOC32:
                DEBUG("block type is HUNK_RELOC32");
                while (true) {
                    uint32_t npos_to_fix = read_dword(&pos);
                    if (npos_to_fix == 0)
                        break;

                    uint32_t ref_hnum = read_dword(&pos);
                    if (ref_hnum > last_hnum) {
                        ERROR("relocations referencing hunk #%d found while last hunk in executable is %d", ref_hnum, last_hnum);
                        return false;
                    }

                    for (uint32_t i = 0; i < npos_to_fix; i++) {
                        uint32_t pos_to_fix = read_dword(&pos);
                        DEBUG("applying reloc referencing hunk #%d at position %d", ref_hnum, pos_to_fix);
                        uint32_t offset = ntohl(*((uint32_t *) (hunk_addresses[hunk_num] + pos_to_fix)));
                        if (offset > (0xffffffff - HUNK_START_ADDRESS)) {
                            ERROR("offset at position %d is too large - cannot apply relocation", pos_to_fix);
                            return false;
                        }
                        offset += (uint32_t) hunk_addresses[ref_hnum];
                        *((uint32_t *) (hunk_addresses[hunk_num] + pos_to_fix)) = htonl(offset);
                    }
                }
                break;

            case HUNK_SYMBOL:
                DEBUG("block type is HUNK_SYMBOL");
                // just advance position to next block
                while ((ndwords = read_dword(&pos)) != 0)
                    pos += (ndwords + 1) * 4;
                break;

            case HUNK_DEBUG:
                DEBUG("block type is HUNK_DEBUG");
                // just advance position to next block
                ndwords = read_dword(&pos);
                pos += ndwords * 4;
                break;

            case HUNK_END:
                DEBUG("block type is HUNK_END");
                ++hunk_num;
                break;

            default:
                ERROR("unknown block type %d", block_type);
                return false;
        }
    }

    // restore previous signal handler
    if (sigaction(SIGSEGV, &old_act, NULL) == -1) {
        ERROR("failed to restore signal handler: %s", strerror(errno));
        return false;
    }
    return true;
}
