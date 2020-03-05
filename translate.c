//
// translate.c - part of the Virtual AmigaDOS Machine (VADM)
//               This file contains the routines for the binary translation from Motorola 680x0 to Intel x86-64 code.
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include <netinet/in.h>         // for ntohs() and ntohl()
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/mman.h>

#include "vadm.h"
#include "translate.h"


//
// utility functions
//

// read one word from buffer and advance current position pointer
static uint16_t read_word(const uint8_t **pos)
{
    uint16_t val = ntohs(*((uint16_t *) *pos));
    *pos += 2;
    return val;
}

// read one dword from buffer and advance current position pointer
static uint32_t read_dword(const uint8_t **pos)
{
    uint32_t val = ntohl(*((uint32_t *) *pos));
    *pos += 4;
    return val;
}

// write one byte into buffer and advance current position pointer
static void write_byte(uint8_t val, uint8_t **pos)
{
    *((uint8_t *) *pos) = val;
    *pos += 1;
}

// write one dword into buffer and advance current position pointer
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
//     uint16_t      m68k_opcode,       // opcode to decode
//     const uint8_t **inpos,           // current position in the input stream, will be updated
//     uint8_t       **outpos           // current position in the output stream, will be updated
// )

// TODO: handle address AbsExecBase (0x0000004)
// TODO: fix translation of branch / jump instructions by translating basic blocks
// TODO: decompose handlers into smaller functions:
//       - function(s) to extract operand(s) as structure with type, length and value
//       - functions to encode a specific opcode / operand combination, e. g. MOVE <register>, <memory address>

// Motorola M68000 Family Programmer’s Reference Manual, page 4-25
// Intel 64 and IA-32 Architectures Software Developer’s Manual, Volume 2, Instruction Set Reference, page 3-483
static int m68k_bcc(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    uint32_t offset = m68k_opcode & 0x00ff;
    int      nbytes_used;

    DEBUG("decoding instruction BCC");
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

    // opcode
    switch (m68k_opcode & 0x0f00) {
        case 0x0600:
            DEBUG("BNE => JNE");
            if (offset <= 0xff)
                write_byte(0x75, outpos);
            else {
                write_byte(0x0f, outpos);
                write_byte(0x85, outpos);
            }
            break;
        case 0x0700:
            DEBUG("BEQ => JE");
            if (offset <= 0xff)
                write_byte(0x74, outpos);
            else {
                write_byte(0x0f, outpos);
                write_byte(0x84, outpos);
            }
            break;
        default:
            ERROR("condition 0x%x not supported", m68k_opcode & 0x0f00);
            return -1;
    }
    // offset
    if (offset <= 0xff)
        write_byte(offset, outpos);
    else
        write_dword(offset, outpos);
    return nbytes_used;
}

// Motorola M68000 Family Programmer’s Reference Manual, page 4-119
// Intel 64 and IA-32 Architectures Software Developer’s Manual, Volume 2, Instruction Set Reference, page 4-35
// explanation of MOD-REG-R/M and SIB bytes: https://www-user.tu-chemnitz.de/~heha/viewchm.php/hs/x86.chm/x86.htm
// explanation of REX prefix: https://www-user.tu-chemnitz.de/~heha/viewchm.php/hs/x86.chm/x64.htm
static int m68k_movea(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    uint16_t mode_reg = m68k_opcode & 0x003f;
    uint8_t  reg_num  = (m68k_opcode & 0x0e00) >> 9;
    uint32_t addr;
    int      nbytes_used;

    DEBUG("decoding instruction MOVEA");
    if ((m68k_opcode & 0x3000) != 0x2000) {
        ERROR("only long operation supported");
        return -1;
    }
    DEBUG("destination register is A%d", reg_num);
    switch (mode_reg) {
        case 0x0038:
            DEBUG("MOVEA.L with 16-bit address");
            addr = read_word(inpos);
            nbytes_used = 2;
            break;
        case 0x0039:
            DEBUG("MOVEA.L with 32-bit address");
            addr = read_dword(inpos);
            nbytes_used = 4;
            break;
        default:
            ERROR("value 0x%02x for mode / register not supported", mode_reg);
            return -1;
    }

    // opcode
    write_byte(0x8b, outpos);
    // MOD-REG-R/M byte with register number
    switch (reg_num) {
        // In order to map A7 to ESP, we have to swap the register numbers of A4 and A7. With all
        // other registers, we can use the same numbers as on the 680x0.
        case 4:
            write_byte(0x3c, outpos);
            break;
        case 7:
            write_byte(0x24, outpos);
            break;
        default:
            write_byte(0x04 | (reg_num << 3), outpos);
    }
    // SIB byte (specifying displacement only as addressing mode) and address
    write_byte(0x25, outpos);
    write_dword(addr, outpos);

    return nbytes_used;
}

// Motorola M68000 Family Programmer’s Reference Manual, page 4-134
// Intel 64 and IA-32 Architectures Software Developer’s Manual, Volume 2, Instruction Set Reference, page 4-35
static int m68k_moveq(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    int8_t   data_byte = m68k_opcode & 0x00ff;
    uint8_t  reg_num  = (m68k_opcode & 0x0e00) >> 9;
    int      nbytes_used = 0;

    DEBUG("decoding instruction MOVEQ");
    DEBUG("destination register is D%d", reg_num);

    // prefix byte indicating extension of opcode register field (because we use registers R8D..R15D)
    write_byte(0x41, outpos);
    // opcode + register number as one byte
    write_byte(0xb8 + reg_num, outpos);
    // immediate value as sign-extended 32-bit value
    write_dword((int32_t) data_byte, outpos);

    return nbytes_used;
}

static int m68k_move(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    uint8_t  src_mode_reg = m68k_opcode & 0x003f;
    uint8_t  dst_mode_reg = (m68k_opcode & 0x0fc0) >> 6;
    uint8_t  x86_opcode;
    uint8_t  x86_mod_reg_rm = 0;
    uint8_t  x86_prefix_byte;
    uint32_t addr;
    uint32_t val;
    int      nbytes_used = 0;

    DEBUG("decoding instruction MOVE");
    if ((m68k_opcode & 0x3000) != 0x2000) {
        ERROR("only long operation supported");
        return -1;
    }

    // opcode depending on operand type and source part of the MOD-REG-R/M byte
    if ((src_mode_reg & 0xf8) == 0) {
        DEBUG("source operand is register D%d", src_mode_reg & 0x07);
        x86_opcode = 0x89;
        x86_mod_reg_rm = (src_mode_reg & 0x0007) << 3;
        x86_prefix_byte = 0x44;
    }
    else if (src_mode_reg == 0x39) {
        DEBUG("source operand is memory address");
        addr = read_dword(inpos);
        nbytes_used = 4;
        x86_opcode = 0x8b;
        x86_mod_reg_rm = 0x04;
        x86_prefix_byte = 0x44;
    }
    else if (src_mode_reg == 0x3c) {
        DEBUG("source operand is immediate value");
        val = read_dword(inpos);
        nbytes_used = 4;
        x86_opcode = 0xb8;
        x86_prefix_byte = 0x41;
    }
    else {
        ERROR("only data register, memory address and immediate value supported as source operand");
        return -1;
    }

    // destination part of the MOD-REG-R/M byte
    if ((dst_mode_reg & 0xc7) == 0) {
        DEBUG("destination operand is register D%d", (dst_mode_reg & 0x38) >> 3);
        if (x86_opcode == 0x89)
            // source operand is also a register => put register in R/M part and set MOD to 11
            x86_mod_reg_rm |= ((dst_mode_reg & 0x38) >> 3) | 0xc0;
        else if (x86_opcode == 0xb8)
            // source operand is immediate value => add register number to opcode (no separate MOD-REG-R/M byte)
            x86_opcode |= ((dst_mode_reg & 0x38) >> 3);
        else
            // source operand is memory address => put register in REG part
            x86_mod_reg_rm |= (dst_mode_reg & 0x38);
    }
    else if (dst_mode_reg == 0x0f) {
        DEBUG("destination operand is memory address");
        addr = read_dword(inpos);
        nbytes_used = 4;
        x86_mod_reg_rm |= 0x04;
    }
    else {
        ERROR("only data register or memory address supported as destination operand");
        return -1;
    }

    // prefix byte indicating extension of register field in opcode or MOD-REG-RM byte (because we use registers R8D..R15D)
    write_byte(x86_prefix_byte, outpos);
    write_byte(x86_opcode, outpos);
    // immediate value if there is one
    if (x86_prefix_byte == 0x41)
        write_dword(val, outpos);
    else {
        write_byte(x86_mod_reg_rm, outpos);
        // SIB byte (specifying displacement only as addressing mode) and address, only if one operand is a memory address
        if (nbytes_used > 0) {
            write_byte(0x25, outpos);
            write_dword(addr, outpos);
        }
    }

    return nbytes_used;
}

static int m68k_bra(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	sprintf(g_dasm_str, "bra     $%x", temp_pc + make_int_8(g_cpu_ir));
}

static int m68k_jsr(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
//	sprintf(g_dasm_str, "jsr     %s", get_ea_mode_str_32(g_cpu_ir));
//	SET_OPCODE_FLAGS(DASMFLAG_STEP_OVER);
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
                if ((opcinfo->opc_handler == m68k_move) &&
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
    uint8_t buffer[16];

    // test case table consists of one row per test case with two colums (Motorola and Intel opcodes) each
    for (unsigned int i = 0; i < sizeof(testcase_tbl) / (MAX_OPCODE_SIZE + 1) / 2; i++) {
        // start with the outpuf buffer set to a fixed value
        memset(buffer, 0x55, 16);
        translate_code_block(&testcase_tbl[i][0][1], buffer, testcase_tbl[i][0][0]);
        if (memcmp(&testcase_tbl[i][1][1], buffer, testcase_tbl[i][1][0]) == 0) {
            INFO("test case #%d passed", i);
        }
        else {
            ERROR("test case #%d failed", i);
        }
    }
}
#endif
