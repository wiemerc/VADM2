//
// translate.h - part of the Virtual AmigaDOS Machine (VADM)
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include <stdint.h>
#include <stdlib.h>


#define MAX_CODE_SIZE   65536
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
static int m68k_bra_8(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_clr_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_jsr(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_lea(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_move_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_move_8(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_move_16(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_movea_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_moveq(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_rts(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_subq_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);
static int m68k_tst_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos);

static const OpcodeInfo opcode_info_tbl[] = {
//   opcode handler      mask    match   effective address mask
	{m68k_rts          , 0xffff, 0x4e75, 0x000},      // rts
	{m68k_tst_32       , 0xffc0, 0x4a80, 0xbf8},      // tst.l
	{m68k_clr_32       , 0xffc0, 0x4280, 0xbf8},      // clr.l
	{m68k_jsr          , 0xffc0, 0x4e80, 0x27b},      // jsr
	{m68k_bra_8        , 0xff00, 0x6000, 0x000},      // bra.s
	{m68k_subq_32      , 0xf1c0, 0x5180, 0xff8},      // subq.l
	{m68k_movea_32     , 0xf1c0, 0x2040, 0xfff},      // movea.l
	{m68k_lea          , 0xf1c0, 0x41c0, 0x27b},      // lea
	{m68k_moveq        , 0xf100, 0x7000, 0x000},      // moveq.l
	{m68k_bcc          , 0xf000, 0x6000, 0x000},      // beq.*
	{m68k_move_8       , 0xf000, 0x1000, 0xbff},      // move.b
	{m68k_move_16      , 0xf000, 0x3000, 0xfff},      // move.w
	{m68k_move_32      , 0xf000, 0x2000, 0xfff},      // move.l
	{NULL, 0, 0, 0}
};


//
// test case table, will be used if translate.c is compiled as standalone program
//
#if TEST
static const uint8_t testcase_tbl[][2][MAX_OPCODE_SIZE] = {
    // Motorola opcode, prefixed with number of bytes      // Intel opcode, prefixed with number of bytes
    {{2, 0x67, 0xfe},                                      {2, 0x74, 0xfe}},                                   // BEQ -> JE with 8-bit offset
    {{4, 0x67, 0x00, 0xde, 0xad},                          {6, 0x0f, 0x84, 0xad, 0xde, 0x00, 0x00}},           // BEQ => JE with 16-bit offset
    {{6, 0x67, 0xff, 0xde, 0xad, 0xbe, 0xef},              {6, 0x0f, 0x84, 0xef, 0xbe, 0xad, 0xde}},           // BEQ => JE with 32-bit offset
    {{2, 0x66, 0xfe},                                      {2, 0x75, 0xfe}},                                   // BNE -> JNE with 8-bit offset
    {{6, 0x66, 0xff, 0xde, 0xad, 0xbe, 0xef},              {6, 0x0f, 0x85, 0xef, 0xbe, 0xad, 0xde}},           // BNE => JNE with 32-bit offset
};
#endif
