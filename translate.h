//
// translate.h - part of the Virtual AmigaDOS Machine (VADM)
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include <stdint.h>
#include <stdlib.h>


#define MAX_OPCODE_SIZE 8


//
// opcode info table for the binary translation (copied from Musashi), with just
// the 11 instructions which are used in the test program, needs to be sorted by the
// number of set bits in mask in descending order to ensure the longest match wins
//
typedef int (*opcode_handler_func_t)(uint16_t, const uint8_t **, uint8_t **);
typedef struct
{
    opcode_handler_func_t opc_handler;    // handler function
    uint16_t opc_mask;                    // mask on opcode
    uint16_t opc_match;                   // what to match after masking
    uint16_t opc_ea_mask;                 // allowed effective address modes
} OpcodeInfo;

static int m68k_bcc(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_bra(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_jsr(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_lea(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_move(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_movea(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_moveq(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_rts(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_subq_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_tst_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);

static const OpcodeInfo opcode_info_tbl[] = {
//   opcode handler      mask    match   effective address mask
    {m68k_rts          , 0xffff, 0x4e75, 0x000},      // rts
    {m68k_tst_32       , 0xffc0, 0x4a80, 0xbf8},      // tst.l
    {m68k_jsr          , 0xffc0, 0x4e80, 0x27b},      // jsr
    {m68k_bra          , 0xff00, 0x6000, 0x000},      // bra.*
    {m68k_subq_32      , 0xf1c0, 0x5180, 0xff8},      // subq.l
    {m68k_movea        , 0xf1c0, 0x2040, 0xfff},      // movea.*
    {m68k_lea          , 0xf1c0, 0x41c0, 0x27b},      // lea
    {m68k_moveq        , 0xf100, 0x7000, 0x000},      // moveq.l
    {m68k_bcc          , 0xf000, 0x6000, 0x000},      // bcc.*
    {m68k_move         , 0xf000, 0x1000, 0xbff},      // move.b
    {m68k_move         , 0xf000, 0x3000, 0xfff},      // move.w
    {m68k_move         , 0xf000, 0x2000, 0xfff},      // move.l
    {NULL, 0, 0, 0}
};


//
// test case table, will be used if translate.c is compiled as standalone program
//
#if TEST
static const uint8_t testcase_tbl[][2][MAX_OPCODE_SIZE + 1] = {
    // Motorola opcode, prefixed with number of bytes      // Intel opcode, prefixed with number of bytes
    {{2, 0x67, 0xfe},                                      {2, 0x74, 0xfe}},                                   		// beq -> je with 8-bit offset
    {{4, 0x67, 0x00, 0xde, 0xad},                          {6, 0x0f, 0x84, 0xad, 0xde, 0x00, 0x00}},           		// beq => je with 16-bit offset
    {{6, 0x67, 0xff, 0xde, 0xad, 0xbe, 0xef},              {6, 0x0f, 0x84, 0xef, 0xbe, 0xad, 0xde}},           		// beq => je with 32-bit offset
    {{2, 0x66, 0xfe},                                      {2, 0x75, 0xfe}},                                   		// bne -> jne with 8-bit offset
    {{6, 0x66, 0xff, 0xde, 0xad, 0xbe, 0xef},              {6, 0x0f, 0x85, 0xef, 0xbe, 0xad, 0xde}},           		// bne => jne with 32-bit offset
    {{4, 0x2c, 0x78, 0x00, 0x04},						   {7, 0x8b, 0x34, 0x25, 0x04, 0x00, 0x00, 0x00}},	        // movea.l 0x0004, a6 => mov esi, [0x00000004]
    {{6, 0x28, 0x79, 0xde, 0xad, 0xbe, 0xef},			   {7, 0x8b, 0x3c, 0x25, 0xef, 0xbe, 0xad, 0xde}},	        // movea.l 0xdeadbeef, a4 => mov edi, [0xdeadbeef]
    {{6, 0x2e, 0x79, 0xde, 0xad, 0xbe, 0xef},			   {7, 0x8b, 0x24, 0x25, 0xef, 0xbe, 0xad, 0xde}},	        // movea.l 0xdeadbeef, a7 => mov esp, [0xdeadbeef]
    {{6, 0x4f, 0xf9, 0xde, 0xad, 0xbe, 0xef},			   {7, 0x8d, 0x24, 0x25, 0xef, 0xbe, 0xad, 0xde}},	        // lea 0xdeadbeef, a7 => lea esp, [0xdeadbeef]
    {{2, 0x70, 0x80},                                      {6, 0x41, 0xb8, 0x80, 0xff, 0xff, 0xff}},                // moveq.l 0x80, d0 => mov r8d, 0x80
    {{2, 0x72, 0x7f},                                      {6, 0x41, 0xb9, 0x7f, 0x00, 0x00, 0x00}},                // moveq.l 0x7f, d1 => mov r9d, 0x7f
};
#endif
