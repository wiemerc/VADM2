//
// translate.h - part of the Virtual AmigaDOS Machine (VADM)
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 
#ifndef TRANSLATE_H_INCLUDED
#define TRANSLATE_H_INCLUDED

#include <netinet/in.h>         // for ntohs() and ntohl()
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <stdlib.h> 
#include <sys/errno.h>
#include <sys/mman.h>

// constants
#define MAX_CODE_SIZE   65536
#define MAX_CODE_BLOCK_SIZE 256
#define MAX_OPCODE_SIZE 8

// structure describing an opcode
// TODO: adapt to naming convention
typedef int (*OpcodeHandlerFunc)(uint16_t, const uint8_t **, uint8_t **);
typedef struct
{
    OpcodeHandlerFunc  opc_handler;     // handler function
    uint16_t opc_mask;                  // mask on opcode
    uint16_t opc_match;                 // what to match after masking
    uint16_t opc_ea_mask;               // allowed effective address modes
    bool     opc_terminal;              // terminal instruction in a translation unit
} OpcodeInfo;

// structure describing an operand as returned by extract_operand()
typedef struct
{
    uint8_t  op_type;                   // operand type: register, address, immediate value
    uint8_t  op_length;                 // operand length: 1, 2 or 4 bytes
    uint32_t op_value;                  // operand value
} Operand;

#define OP_AREG         0
#define OP_DREG         1
#define OP_MEM          2
#define OP_IMM          3
#define OP_AREG_OFFSET  4

// prototypes
uint8_t *setup_tu(const uint8_t *p_m68k_code);
uint8_t *translate_tu(const uint8_t *p_m68k_code, uint32_t ninstr_to_translate);

// test case table, will be used if translate.c is compiled as standalone program
#if TEST
static const uint8_t testcase_tbl[][2][MAX_OPCODE_SIZE + 1] = {
    // Motorola opcode, prefixed with number of bytes      // Intel opcode, prefixed with number of bytes
    {{4, 0x2c, 0x78, 0x00, 0x04},						   {7, 0x8b, 0x34, 0x25, 0x04, 0x00, 0x00, 0x00}},	        // movea.l 0x0004, a6 => mov esi, [0x00000004]
    {{6, 0x28, 0x7c, 0xde, 0xad, 0xbe, 0xef},			   {5, 0xbf, 0xef, 0xbe, 0xad, 0xde}},	                    // movea.l #0xdeadbeef, a4 => mov edi, 0xdeadbeef
    {{6, 0x2e, 0x79, 0xde, 0xad, 0xbe, 0xef},			   {7, 0x8b, 0x24, 0x25, 0xef, 0xbe, 0xad, 0xde}},	        // movea.l 0xdeadbeef, a7 => mov esp, [0xdeadbeef]
    {{2, 0x70, 0x80},                                      {6, 0x41, 0xb8, 0x80, 0xff, 0xff, 0xff}},                // moveq.l 0x80, d0 => mov r8d, 0x80
    {{2, 0x72, 0x7f},                                      {6, 0x41, 0xb9, 0x7f, 0x00, 0x00, 0x00}},                // moveq.l 0x7f, d1 => mov r9d, 0x7f
    {{6, 0x20, 0x39, 0x55, 0x55, 0xaa, 0xaa},              {8, 0x44, 0x8b, 0x04, 0x25, 0xaa, 0xaa, 0x55, 0x55}},    // move.l 0x5555aaaa, d0 => mov r8d, [0x5555aaaa]
    {{6, 0x22, 0x3c, 0x55, 0x55, 0xaa, 0xaa},              {6, 0x41, 0xb9, 0xaa, 0xaa, 0x55, 0x55}},                // move.l #0x5555aaaa, d1 => mov r9d, 0x5555aaaa
    {{6, 0x23, 0xc1, 0x55, 0x55, 0xaa, 0xaa},              {8, 0x44, 0x89, 0x0c, 0x25, 0xaa, 0xaa, 0x55, 0x55}},    // move.l d1, 0x5555aaaa => mov [0x5555aaaa], r9d
    {{2, 0x26, 0x02},                                      {3, 0x45, 0x89, 0xd3}},                                  // move.l d2, d3 => mov r11d, r10d
    {{2, 0x53, 0x82},                                      {4, 0x41, 0x83, 0xea, 0x01}},                            // subq.l #1, d2 => sub, r10d, 1
    {{2, 0x4a, 0x80},                                      {3, 0x45, 0x85, 0xc0}},                                  // tst.l d0 => test r8d, r8d
    {{4, 0x4e, 0xae, 0xfc, 0x4c},                          {8, 0x81, 0xc6, 0x4c, 0xfc, 0xff, 0xff, 0xff, 0xd6}},    // jsr -948(a6) => add esi, -948; call rsi
};
#endif

#endif
