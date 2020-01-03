//
// translate.c - part of the Virtual AmigaDOS Machine (VADM)
//               This file contains the routines for the binary translation from Motorola 680x0 to Intel x86-64 code.
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/mman.h>

#include "vadm.h"


//
// read one word from buffer and advance current position pointer
//
static uint16_t read_word(const uint8_t **pos)
{
    uint16_t val = ntohs(*((uint16_t *) *pos));
    *pos += 2;
    return val;
}

//
// read one dword from buffer and advance current position pointer
//
static uint32_t read_dword(const uint8_t **pos)
{
    uint32_t val = ntohl(*((uint32_t *) *pos));
    *pos += 4;
    return val;
}


//
// write opcode = series of bytes into buffer and advance current position pointer
//
static void write_opcode(const char *opcode, size_t len, uint8_t **pos)
{
    while (len-- > 0) {
        *((char *) *pos) = *opcode;
        ++opcode;
        ++(*pos);
    }
}


//
// write one byte into buffer and advance current position pointer
//
static void write_byte(uint8_t val, uint8_t **pos)
{
    *((uint8_t *) *pos) = val;
    *pos += 1;
}


//
// write one dword into buffer and advance current position pointer
//
static void write_dword(uint32_t val, uint8_t **pos)
{
    *((uint32_t *) *pos) = val;
    *pos += 4;
}


//
// opcode handlers
//
// All handlers have the following signature and return the number of bytes consumed, 
// which can be 0, or -1 in case of of an error.
// static int m68k_xxx(
//     uint16_t m68k_opcode,       // opcode to decode
//     void     **inpos,           // current position in the input stream, will be updated
//     void     **outpos           // current position in the output stream, will be updated
// )
typedef int (*opcode_handler_func_t)(uint16_t, const uint8_t **, uint8_t **);

// TODO: check all 32-bit immediate adresses if they need to be fixed

// Motorola M68000 Family Programmer’s Reference Manual, page 4-25
// Intel 64 and IA-32 Architectures Software Developer’s Manual, Volume 2, Instruction Set Reference, page 3-483
static int m68k_bcc(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    uint32_t offset;
    int      nbytes_used;

    DEBUG("decoding instruction BCC");
    offset = m68k_opcode & 0x00ff;
    switch (offset) {
        // TODO: check if we need to add / subtract the length of the current instruction
        case 0x0000:
            offset = read_word(inpos);
            DEBUG("16-bit offset = 0x%04x", offset);
            nbytes_used = 2;
            break;
        case 0x00ff:
            offset = read_dword(inpos);
            DEBUG("32-bit offset = 0x%08x", offset);
            nbytes_used = 4;
            break;
        default:
            DEBUG("8-bit offset = 0x%02x", offset);
            nbytes_used = 0;
    }
    switch (m68k_opcode & 0x0f00) {
        case 0x0600:
            DEBUG("BNE => JNE");
            if (offset <= 0xff)
                write_opcode("\x75", 1, outpos);
            else
                write_opcode("\x0f\x85", 2, outpos);
            break;
        case 0x0700:
            DEBUG("BEQ => JE");
            if (offset <= 0xff)
                write_opcode("\x74", 1, outpos);
            else
                write_opcode("\x0f\x84", 2, outpos);
            break;
        default:
            ERROR("condition 0x%x not supported", m68k_opcode & 0x0f00);
            return -1;
    }
    if (offset <= 0xff)
        write_byte(offset, outpos);
    else
        write_dword(offset, outpos);
    return nbytes_used;
}

static int m68k_bra_8(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	sprintf(g_dasm_str, "bra     $%x", temp_pc + make_int_8(g_cpu_ir));
}

static int m68k_clr_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	sprintf(g_dasm_str, "clr.l   %s", get_ea_mode_str_32(g_cpu_ir));
}

static int m68k_jsr(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	sprintf(g_dasm_str, "jsr     %s", get_ea_mode_str_32(g_cpu_ir));
//	SET_OPCODE_FLAGS(DASMFLAG_STEP_OVER);
}

static int m68k_lea(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	sprintf(g_dasm_str, "lea     %s, A%d", get_ea_mode_str_32(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static int m68k_move_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	char* str = get_ea_mode_str_32(g_cpu_ir);
//	sprintf(g_dasm_str, "move.l  %s, %s", str, get_ea_mode_str_32(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

// move_8 and move_16 are not needed right now and are only here because they're referenced in the
// code below that builds the opcode handler table.
static int m68k_move_8(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	char* str = get_ea_mode_str_8(g_cpu_ir);
//	sprintf(g_dasm_str, "move.b  %s, %s", str, get_ea_mode_str_8(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static int m68k_move_16(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	char* str = get_ea_mode_str_16(g_cpu_ir);
//	sprintf(g_dasm_str, "move.w  %s, %s", str, get_ea_mode_str_16(((g_cpu_ir>>9) & 7) | ((g_cpu_ir>>3) & 0x38)));
}

static int m68k_movea_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	sprintf(g_dasm_str, "movea.l %s, A%d", get_ea_mode_str_32(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static int m68k_moveq(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	sprintf(g_dasm_str, "moveq   #%s, D%d", make_signed_hex_str_8(g_cpu_ir), (g_cpu_ir>>9)&7);
}

static int m68k_rts(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	sprintf(g_dasm_str, "rts");
//	SET_OPCODE_FLAGS(DASMFLAG_STEP_OUT);
}

static int m68k_subq_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	sprintf(g_dasm_str, "subq.l  #%d, %s", g_3bit_qdata_table[(g_cpu_ir>>9)&7], get_ea_mode_str_32(g_cpu_ir));
}

static int m68k_tst_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	sprintf(g_dasm_str, "tst.l   %s", get_ea_mode_str_32(g_cpu_ir));
}


// opcode info table for the binary translation (copied from Musashi), with just
// the 11 instructions which are used in the test program, needs to be sorted
// by the number of set bits in mask in descending order to ensure the longest
// match wins
typedef struct
{
	opcode_handler_func_t opc_handler;    // handler function
	uint16_t opc_mask;                    // mask on opcode
	uint16_t opc_match;                   // what to match after masking
	uint16_t opc_ea_mask;                 // allowed effective address modes
} OpcodeInfo;

static const OpcodeInfo opcode_info_tbl[] =
{
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
// check if opcode is using a valid effective address mode (code is copied straight from Musashi)
//
static bool valid_ea_mode(uint16_t opcode, uint16_t mask)
{
	if (mask == 0)
		return true;

	switch (opcode & 0x3f)
	{
		case 0x00: case 0x01: case 0x02: case 0x03:
		case 0x04: case 0x05: case 0x06: case 0x07:
			return (mask & 0x800) != 0;
		case 0x08: case 0x09: case 0x0a: case 0x0b:
		case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			return (mask & 0x400) != 0;
		case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x14: case 0x15: case 0x16: case 0x17:
			return (mask & 0x200) != 0;
		case 0x18: case 0x19: case 0x1a: case 0x1b:
		case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			return (mask & 0x100) != 0;
		case 0x20: case 0x21: case 0x22: case 0x23:
		case 0x24: case 0x25: case 0x26: case 0x27:
			return (mask & 0x080) != 0;
		case 0x28: case 0x29: case 0x2a: case 0x2b:
		case 0x2c: case 0x2d: case 0x2e: case 0x2f:
			return (mask & 0x040) != 0;
		case 0x30: case 0x31: case 0x32: case 0x33:
		case 0x34: case 0x35: case 0x36: case 0x37:
			return (mask & 0x020) != 0;
		case 0x38:
			return (mask & 0x010) != 0;
		case 0x39:
			return (mask & 0x008) != 0;
		case 0x3a:
			return (mask & 0x002) != 0;
		case 0x3b:
			return (mask & 0x001) != 0;
		case 0x3c:
			return (mask & 0x004) != 0;
	}
	return false;
}


//
// translate a block of code from Motorola 680x0 to Intel x86-64
//
bool translate_code_block(const uint8_t *inptr, uint8_t *outptr, int size)
{
    uint16_t opcode;
    int      nbytes_used;
    opcode_handler_func_t opcode_handler_tbl[0x10000];

    // build table with all 65536 possible opcodes and their handlers
    // (code is copied straight from Musashi)
    DEBUG("building opcode handler table");
    for(int i = 0; i < 0x10000; i++) {
        opcode = (uint16_t) i;
        // default is NULL
        opcode_handler_tbl[opcode] = NULL;
        // search through opcode info table for a match
        for (const OpcodeInfo *opcinfo = opcode_info_tbl; opcinfo->opc_handler != NULL; opcinfo++) {
            // match opcode mask and allowed effective address modes
            if ((opcode & opcinfo->opc_mask) == opcinfo->opc_match) {
                // handle destination effective address modes for move instructions
                if ((opcinfo->opc_handler == m68k_move_8 ||
                     opcinfo->opc_handler == m68k_move_16 ||
                     opcinfo->opc_handler == m68k_move_32) &&
                     !valid_ea_mode(((opcode >> 9) & 7) | ((opcode >> 3) & 0x38), 0xbf8))
                    continue;
                if (valid_ea_mode(opcode, opcinfo->opc_ea_mask)) {
                    opcode_handler_tbl[opcode] = opcinfo->opc_handler;
                    break;
                }
            }
        }
    }

    // translate instructions one by one
    while (size >= 2) {
        opcode = read_word(&inptr);
        size -= 2;

        DEBUG("looking up opcode 0x%04x in opcode handler table", opcode);
        if (opcode_handler_tbl[opcode])
            nbytes_used = opcode_handler_tbl[opcode](opcode, &inptr, &outptr);
        else {
            ERROR("no handler found for opcode 0x%04x", opcode);
            return false;
        }
        if (nbytes_used == -1) {
            ERROR("could not decode instruction at position %p", inptr - 2);
            return false;
        }
        else
            size -= nbytes_used;
    }
    return true;
}


#if TEST
int main()
{
    uint8_t inbuf[6] = {0x66, 0xff, 0xde, 0xad, 0xbe, 0xef};
    uint8_t *outbuf;

    // allocate buffer for translated code
    if ((outbuf = (uint8_t *) mmap(NULL, MAX_CODE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
        ERROR("could not create memory mapping for translated code: %s", strerror(errno));
        return -1;
    }

    // translate code
    if (translate_code_block(inbuf, outbuf, 2))
        return 0;
    else
        return -1;
}
#endif
