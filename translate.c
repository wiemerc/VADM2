//
// translate.c - part of the Virtual AmigaDOS Machine (VADM)
//               contains the routines for the binary translation from Motorola 680x0 to Intel x86-64 code
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "execute.h"
#include "translate.h"
#include "tlcache.h"
#include "vadm.h"
#include "util.h"


// TODO: adapt to naming convention

//
// utility routines
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

// extract operand from instruction stream and fill Operand structure, return number of bytes used
static int extract_operand(uint8_t mode_reg, const uint8_t **pos, Operand *op)
{
    if ((mode_reg & 0xf8) == 0) {
        op->op_type = OP_DREG;
        op->op_length = 4;
        op->op_value = mode_reg & 0x07;
        DEBUG("operand is register D%d", mode_reg & 0x07);
        return 0;
    }
    else if ((mode_reg & 0xf8) == 8) {
        op->op_type = OP_AREG;
        op->op_length = 4;
        op->op_value = mode_reg & 0x07;
        DEBUG("operand is register A%d", mode_reg & 0x07);
        return 0;
    }
    else if ((mode_reg & 0xf8) == 0x28) {
        op->op_type = OP_AREG_OFFSET;
        op->op_length = 4;
        op->op_value = mode_reg & 0x07;
        DEBUG("operand is register A%d with offset", mode_reg & 0x07);
        return 0;
    }
    else if (mode_reg == 0x38) {
        op->op_type = OP_MEM;
        op->op_length = 2;
        op->op_value = (uint32_t) read_word(pos);
        DEBUG("operand is 16-bit address 0x%04x", (uint16_t) op->op_value);
        return 2;
    }
    else if (mode_reg == 0x39) {
        op->op_type = OP_MEM;
        op->op_length = 4;
        op->op_value = read_dword(pos);
        DEBUG("operand is 32-bit address 0x%08x", op->op_value);
        return 4;
    }
    else if (mode_reg == 0x3c) {
        op->op_type = OP_IMM;
        // TODO: always 4 bytes?
        op->op_length = 4;
        op->op_value = read_dword(pos);
        DEBUG("operand is immediate value 0x%08x", op->op_value);
        return 4;
    }
    else {
        ERROR("only data / address register, memory and immediate value supported as operand");
        return -1;
    }

}


//
// routines to encode a specific opcode / operand combination, e. g. MOV <register>, <address>
//
// explanation of MOD-REG-R/M and SIB bytes: https://www-user.tu-chemnitz.de/~heha/viewchm.php/hs/x86.chm/x86.htm
// explanation of REX prefix: https://www-user.tu-chemnitz.de/~heha/viewchm.php/hs/x86.chm/x64.htm
// Intel 64 and IA-32 Architectures Software Developer’s Manual, Volume 2, Instruction Set Reference, Appendix B, Instruction Formats And Encodings
//

// move memory to address register (EAX..EDX, ESI, EDI, EPP, ESP)
static void x86_encode_move_mem_to_areg(uint32_t addr, uint8_t reg, uint8_t **pos)
{
    // opcode
    write_byte(0x8b, pos);
    // MOD-REG-R/M byte with register number
    switch (reg) {
        // In order to map A7 to ESP, we have to swap the register numbers of A4 and A7. With all
        // other registers, we can use the same numbers as the 680x0 uses.
        case 4:
            reg = 7;
            break;
        case 7:
            reg = 4;
            break;
    }
    write_byte(0x04 | (reg << 3), pos);
    // SIB byte (specifying displacement only as addressing mode) and address
    write_byte(0x25, pos);
    write_dword(addr, pos);
}

// move memory to data register (R8D..R15D)
static void x86_encode_move_mem_to_dreg(uint32_t addr, uint8_t reg, uint8_t **pos)
{
    // prefix byte indicating extension of register field in MOD-REG-R/M byte (because we use registers R8D..R15D)
    write_byte(0x44, pos);
    // opcode
    write_byte(0x8b, pos);
    // MOD-REG-R/M byte with register number
    write_byte(0x04 | (reg << 3), pos);
    // SIB byte (specifying displacement only as addressing mode) and address
    write_byte(0x25, pos);
    write_dword(addr, pos);
}

// move immediate value to address register (EAX..EDX, ESI, EDI, EPP, ESP)
static void x86_encode_move_imm_to_areg(uint32_t value, uint8_t reg, uint8_t **pos)
{
    // opcode + register number as one byte
    switch (reg) {
        // In order to map A7 to ESP, we have to swap the register numbers of A4 and A7. With all
        // other registers, we can use the same numbers as the 680x0 uses.
        case 4:
            reg = 7;
            break;
        case 7:
            reg = 4;
            break;
    }
    write_byte(0xb8 + reg, pos);
    // immediate value
    write_dword(value, pos);
}

// move immediate value to data register (R8D..R15D)
static void x86_encode_move_imm_to_dreg(uint32_t value, uint8_t reg, uint8_t **pos)
{
    // prefix byte indicating extension of opcode register field (because we use registers R8D..R15D)
    write_byte(0x41, pos);
    // opcode + register number as one byte
    write_byte(0xb8 + reg, pos);
    // immediate value
    write_dword(value, pos);
}

// move data register (R8D..R15D) to memory
static void x86_encode_move_dreg_to_mem(uint8_t reg, uint32_t addr, uint8_t **pos)
{
    // prefix byte indicating extension of register field in MOD-REG-R/M byte (because we use registers R8D..R15D)
    write_byte(0x44, pos);
    // opcode
    write_byte(0x89, pos);
    // MOD-REG-R/M byte with register number
    write_byte(0x04 | (reg << 3), pos);
    // SIB byte (specifying displacement only as addressing mode) and address
    write_byte(0x25, pos);
    write_dword(addr, pos);
}

// move data register (R8D..R15D) to data register
static void x86_encode_move_dreg_to_dreg(uint8_t src, uint8_t dst, uint8_t **pos)
{
    // prefix byte indicating extension of register fields (REG and R/M) in MOD-REG-R/M byte (because we use registers R8D..R15D)
    write_byte(0x45, pos);
    // opcode
    write_byte(0x89, pos);
    // MOD-REG-R/M byte with register numbers, mode = 11, source register goes into REG part,
    // destination register into R/M part
    write_byte(0xc0 | (src << 3) | dst, pos);
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

// Motorola M68000 Family Programmer’s Reference Manual, page 4-25
// Intel 64 and IA-32 Architectures Software Developer’s Manual, Volume 2, Instruction Set Reference, page 3-483
#ifdef TEST
#pragma GCC diagnostic ignored "-Wunused-parameter"
static int m68k_bcc(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    return -1;
}
#pragma GCC diagnostic pop
#else
static int m68k_bcc(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    int32_t offset;
    int      nbytes_used;

    DEBUG("translating instruction BCC");
    switch (m68k_opcode & 0x00ff) {
        case 0x0000:
            offset = (int16_t) read_word(inpos);
            DEBUG("16-bit offset = %d", offset);
            nbytes_used = 2;
            break;
        case 0x00ff:
            offset = (int32_t) read_dword(inpos);
            DEBUG("32-bit offset = %d", offset);
            nbytes_used = 4;
            break;
        default:
            offset = (int8_t) (m68k_opcode & 0x00ff);
            DEBUG("8-bit offset = %d", offset);
            nbytes_used = 0;
    }

    // write opcode
    write_byte(0x0f, outpos);
    switch (m68k_opcode & 0x0f00) {
        case 0x0600:
            DEBUG("BNE => JNE");
            write_byte(0x85, outpos);
            break;
        case 0x0700:
            DEBUG("BEQ => JE");
            write_byte(0x84, outpos);
            break;
        default:
            ERROR("condition 0x%x not supported", m68k_opcode & 0x0f00);
            return -1;
    }

    // recursively call setup_tu() twice, once with the branch target as start address,
    // once with the address of the following instruction
    // The offset of the branch target is calculated from the position after the *opcode*,
    // so we need to subtract the number of bytes used for the offset itself.
    // This method was inspired by a paper describing how VMware does binary translation:
    // https://www.vmware.com/pdf/asplos235_adams.pdf
    uint8_t *branch_taken_addr, *branch_not_taken_addr;
    DEBUG("setting up TU of branch taken");
    if ((branch_taken_addr = setup_tu(*inpos + offset - nbytes_used)) == NULL) {
        ERROR("failed to set up next TU (branch taken)")
        return -1;
    }
    DEBUG("setting up TU of branch not taken");
    if ((branch_not_taken_addr = setup_tu(*inpos)) == NULL) {
        ERROR("failed to set up next TU (branch not taken)")
        return -1;
    }

    // write offset
    // offset in translated code = address of TU of branch - value of IP after branch instruction,
    // not counting the opcode itself (so it's + 4 instead of +6)
    // To make things easier, we always use the less compact 2-byte encoding with a 32-bit offset.
    offset = branch_taken_addr - (*outpos + 4);
    write_dword(offset, outpos);

    // add jump to the corresponding TU if branch is not taken, here the one byte for the opcode is included
    offset = branch_not_taken_addr - (*outpos + 5);
    write_byte(0xe9, outpos);
    write_dword(offset, outpos);

    return nbytes_used;
}
#endif

// Motorola M68000 Family Programmer’s Reference Manual, page 4-109
// Intel 64 and IA-32 Architectures Software Developer’s Manual, Volume 2, Instruction Set Reference, page 3-122
static int m68k_jsr(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    uint16_t mode_reg = m68k_opcode & 0x003f;
    int16_t offset;
    Operand  op;
    int      nbytes_used;

    DEBUG("translating instruction JSR");
    nbytes_used = extract_operand(mode_reg, inpos, &op);
    if (op.op_type != OP_AREG_OFFSET) {
        ERROR("only address register with offset supported as operand type");
        return -1;
    }
    offset = (int16_t) read_word(inpos);
    nbytes_used = 2;
    if (op.op_value == 6) {
        // special case: register is A6 => we assume this is a call of a library routine
        // As the x86 doesn't support register + offset as operand for CALL, we need to
        // insert an additional ADD instruction before the CALL, but of course we have
        // to save the old value and restore it after the call.
        write_byte(0x56, outpos);  // push rsi
        write_byte(0x81, outpos);  // add esi, <offset>
        write_byte(0xc6, outpos);
        write_dword(offset, outpos);
        write_byte(0xff, outpos);  // call rsi
        write_byte(0xd6, outpos);
        write_byte(0x5e, outpos);  // pop rsi
    }
    else {
        ERROR("generic JSR instruction not supported");
        return -1;
    }

    return nbytes_used;
}

// Motorola M68000 Family Programmer’s Reference Manual, page 4-119
// Intel 64 and IA-32 Architectures Software Developer’s Manual, Volume 2, Instruction Set Reference, page 4-35
static int m68k_movea(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    uint16_t mode_reg = m68k_opcode & 0x003f;
    uint8_t  reg = (m68k_opcode & 0x0e00) >> 9;
    Operand  op;
    int      nbytes_used;

    DEBUG("translating instruction MOVEA");
    if ((m68k_opcode & 0x3000) != 0x2000) {
        ERROR("only long operation supported");
        return -1;
    }
    DEBUG("destination register is A%d", reg);
    nbytes_used = extract_operand(mode_reg, inpos, &op);
    if (op.op_type == OP_MEM) {
        // replace the original value of AbsExecBase (0x0000004) with the address where the base address of Exec library is stored
#ifndef TEST
        if (op.op_value == 0x4)
            op.op_value = ABS_EXEC_BASE;
#endif
        x86_encode_move_mem_to_areg(op.op_value, reg, outpos);
    }
    else if (op.op_type == OP_IMM)
        x86_encode_move_imm_to_areg(op.op_value, reg, outpos);
    else {
        ERROR("invalid operand type %d for MOVEA", op.op_type);
        return -1;
    }
    return nbytes_used;
}

// Motorola M68000 Family Programmer’s Reference Manual, page 4-134
// Intel 64 and IA-32 Architectures Software Developer’s Manual, Volume 2, Instruction Set Reference, page 4-35
#pragma GCC diagnostic ignored "-Wunused-parameter"
static int m68k_moveq(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    // immediate value as sign-extended 32-bit value
    int32_t  value = (int8_t) (m68k_opcode & 0x00ff);
    uint8_t  reg  = (m68k_opcode & 0x0e00) >> 9;

    DEBUG("translating instruction MOVEQ");
    DEBUG("destination register is D%d", reg);
    DEBUG("immediate value = %d", value);
    x86_encode_move_imm_to_dreg(value, reg, outpos);
    return 0;
}
#pragma GCC diagnostic pop

// Motorola M68000 Family Programmer’s Reference Manual, page 4-116
// Intel 64 and IA-32 Architectures Software Developer’s Manual, Volume 2, Instruction Set Reference, page 4-35
static int m68k_move(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    uint8_t  src_mode_reg = m68k_opcode & 0x003f;
    uint8_t  dst_mode_reg = (m68k_opcode & 0x0fc0) >> 6;
    Operand  srcop, dstop;
    int      nbytes_used = 0;

    DEBUG("translating instruction MOVE");
    if ((m68k_opcode & 0x3000) != 0x2000) {
        ERROR("only long operation supported");
        return -1;
    }

    nbytes_used += extract_operand(src_mode_reg, inpos, &srcop);
    // destination operand has mode and register parts swapped
    dst_mode_reg = ((dst_mode_reg & 0x07) << 3) | ((dst_mode_reg & 0x38) >> 3);
    nbytes_used += extract_operand(dst_mode_reg, inpos, &dstop);

    // call the appropriate function depending on the combination of source / destination operand type
    if ((srcop.op_type      == OP_MEM)  && (dstop.op_type == OP_DREG))
        x86_encode_move_mem_to_dreg(srcop.op_value, dstop.op_value, outpos);
    else if ((srcop.op_type == OP_IMM)  && (dstop.op_type == OP_DREG))
        x86_encode_move_imm_to_dreg(srcop.op_value, dstop.op_value, outpos);
    else if ((srcop.op_type == OP_DREG) && (dstop.op_type == OP_MEM))
        x86_encode_move_dreg_to_mem(srcop.op_value, dstop.op_value, outpos);
    else if ((srcop.op_type == OP_DREG) && (dstop.op_type == OP_DREG))
        x86_encode_move_dreg_to_dreg(srcop.op_value, dstop.op_value, outpos);
    else {
        ERROR("combination of source / destination operand types %d / %d not supported", srcop.op_type, dstop.op_type);
        return -1;
    }
    return nbytes_used;
}

// Motorola M68000 Family Programmer’s Reference Manual, page 4-169
// Intel 64 and IA-32 Architectures Software Developer’s Manual, Volume 2, Instruction Set Reference, page 4-553
#pragma GCC diagnostic ignored "-Wunused-parameter"
static int m68k_rts(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    DEBUG("translating instruction RTS");
    write_byte(0xc3, outpos);
    return 0;
}
#pragma GCC diagnostic pop

// Motorola M68000 Family Programmer’s Reference Manual, page 4-181
// Intel 64 and IA-32 Architectures Software Developer’s Manual, Volume 2, Instruction Set Reference, page 4-654
static int m68k_subq_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    uint16_t mode_reg = m68k_opcode & 0x003f;
    uint8_t  value = (m68k_opcode & 0x0e00) >> 9;
    Operand  op;
    int      nbytes_used;

    DEBUG("translating instruction SUBQ");
    if ((m68k_opcode & 0x00c0) != 0x0080) {
        ERROR("only long operation supported");
        return -1;
    }
    DEBUG("immediate value = %d", value);
    nbytes_used = extract_operand(mode_reg, inpos, &op);
    if (op.op_type != OP_DREG) {
        ERROR("only data register supported as destination operand");
        return -1;
    }
    // prefix byte indicating extension of opcode register field (because we use registers R8D..R15D)
    write_byte(0x41, outpos);
    // opcode
    write_byte(0x83, outpos);
    // register number
    write_byte(0xe8 + op.op_value, outpos);
    // value
    write_byte(value, outpos);
    return nbytes_used;
}

// Motorola M68000 Family Programmer’s Reference Manual, page 4-193
// Intel 64 and IA-32 Architectures Software Developer’s Manual, Volume 2, Instruction Set Reference, page 4-679
static int m68k_tst_32(uint16_t m68k_opcode, const uint8_t **inpos, uint8_t **outpos)
{
    uint16_t mode_reg = m68k_opcode & 0x003f;
    Operand  op;
    int      nbytes_used;

    DEBUG("translating instruction TST");
    if ((m68k_opcode & 0x00c0) != 0x0080) {
        ERROR("only long operation supported");
        return -1;
    }
    nbytes_used = extract_operand(mode_reg, inpos, &op);
    if (op.op_type != OP_DREG) {
        ERROR("only data register supported as destination operand");
        return -1;
    }
    // prefix byte indicating extension of opcode register field (because we use registers R8D..R15D)
    write_byte(0x45, outpos);
    // opcode
    write_byte(0x85, outpos);
    // register number
    // With the Motorola TST instruction, the value to test against is implicitly 0, this has
    // to be encoded as TEST <register>, <register> for Intel.
    write_byte(0xc0 | (op.op_value << 3) | op.op_value, outpos);
    return nbytes_used;
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
// opcode info table (copied from Musashi), with just the 8 instructions which are used
// in the test program, needs to be sorted by the number of set bits in mask in descending
// order to ensure the longest match wins, lives here instead of in the header because
// we don't want to export the handler functions
//
static const OpcodeInfo opcode_info_tbl[] = {
//   opcode handler      mask    match   effective address mask     terminal y/n?
    {m68k_rts          , 0xffff, 0x4e75, 0x000,                     true},       // rts
    {m68k_tst_32       , 0xffc0, 0x4a80, 0xbf8,                     false},      // tst.l
    {m68k_jsr          , 0xffc0, 0x4e80, 0x27b,                     false},      // jsr
    {m68k_subq_32      , 0xf1c0, 0x5180, 0xff8,                     false},      // subq.l
    {m68k_movea        , 0xf1c0, 0x2040, 0xfff,                     false},      // movea.*
    {m68k_moveq        , 0xf100, 0x7000, 0x000,                     false},      // moveq.l
    {m68k_bcc          , 0xf000, 0x6000, 0x000,                     true},       // bcc.*
    {m68k_move         , 0xf000, 0x1000, 0xbff,                     false},      // move.b
    {m68k_move         , 0xf000, 0x3000, 0xfff,                     false},      // move.w
    {m68k_move         , 0xf000, 0x2000, 0xfff,                     false},      // move.l
    {NULL, 0, 0, 0, false}
};


//
// initialize opcode info lookup table
//
static void init_opc_info_lookup_tbl (const OpcodeInfo **pp_opc_info_lookup_tbl)
{
    // build table with all 65536 possible opcodes and their handlers on first run
    // (code is copied straight from Musashi)
    uint16_t opcode;
    for(int i = 0; i < 0x10000; i++) {
        opcode = (uint16_t) i;
        // default is NULL
        pp_opc_info_lookup_tbl[opcode] = NULL;
        // search through opcode info table for a match
        for (const OpcodeInfo *opcinfo = opcode_info_tbl; opcinfo->opc_handler != NULL; opcinfo++) {
            // match opcode mask and allowed effective address modes
            if ((opcode & opcinfo->opc_mask) == opcinfo->opc_match) {
                // handle destination effective address modes for move instructions
                if ((opcinfo->opc_handler == m68k_move) &&
                    !valid_ea_mode(((opcode >> 9) & 7) | ((opcode >> 3) & 0x38), 0xbf8))
                    continue;
                if (valid_ea_mode(opcode, opcinfo->opc_ea_mask)) {
                    pp_opc_info_lookup_tbl[opcode] = opcinfo;
                    break;
                }
            }
        }
    }
}


//
// set up a translation unit for later translation when it is about to execute
// (basically a stub for the actual TU that calls translate_tu() upon execution)
//
uint8_t *setup_tu(const uint8_t *p_m68k_code)
{
    uint8_t *p_x86_code;

    // check if TU is already in the cache
    if ((p_x86_code = tc_get_addr(gp_tlcache, p_m68k_code)) != NULL) {
        DEBUG("TU with source address %p is already in the cache - nothing to do", p_m68k_code);
        return p_x86_code;
    }

    // get memory block for the translated code and put mapping of source to destination address into cache
    if ((p_x86_code = tc_get_code_block(gp_tlcache)) == NULL) {
        ERROR("could not get memory block for translated code");
        return NULL;
    }
    if (!tc_put_addr(gp_tlcache, p_m68k_code, p_x86_code)) {
        ERROR("could not put mapping of source to destination address into cache");
        return NULL;
    }

    // generate code to call translate_tu()
    // We fill the memory block with NOPs which get overwritten by the generated code below,
    // starting at position 0 in the block, and the translated code, starting at position
    // START_OF_TRANSLATED_CODE (128). This way, we don't need to jump to the translated code
    // but we create a NOP sled instead. We just need to make sure the code below needed
    // to call translate_tu() never exceeds 128 bytes (currently 56 bytes).
    memset(p_x86_code, OPCODE_NOP, MAX_CODE_BLOCK_SIZE);
    uint8_t *p_pos = p_x86_code;
    // Amiga programs of course don't expect a function call to happen upon the execution
    // of a branch instruction and thus expect registers and flags to be preserved across
    // branch instructions (the call to translate_tu() needs to be completely transparent
    // to the Amiga program). emit_save_program_state() ensures just that by saving all
    // registers that needed to be preserved in AmigaOS, and in addition also A0/A1, D0/D1
    // and RFLAGS.
    p_pos = emit_save_program_state(p_pos);
    // call translate_tu() with address of this TU as argument
    // TODO: check return value
    p_pos = emit_move_imm_to_reg(p_pos, (uint64_t) p_m68k_code, REG_RDI, MODE_64);
    p_pos = emit_abs_call_to_func(p_pos, (void (*)()) translate_tu);
    p_pos = emit_restore_program_state(p_pos);
    return p_x86_code;
}


//
// translate a translation unit from Motorola 680x0 to Intel x86-64 code
//
uint8_t *translate_tu(const uint8_t *p_m68k_code)
{
    uint8_t *p_x86_code;
    uint16_t opcode;
    int nbytes_used;
    static const OpcodeInfo *p_opc_info_lookup_tbl[0x10000];
    static bool initialized = false;

    if (!initialized) {
        DEBUG("building opcode handler table");
        init_opc_info_lookup_tbl(p_opc_info_lookup_tbl);
        initialized = true;
    }

    // get address of memory block for the translated code
    if ((p_x86_code = tc_get_addr(gp_tlcache, p_m68k_code)) == NULL) {
        ERROR("translate_tu() called on a TU with source address %p that is not in the cache", p_m68k_code);
        return NULL;
    }

    DEBUG("translating TU with source address %p and destination address %p", p_m68k_code, p_x86_code);
    // translate instructions one by one until we hit a terminal instruction
    // We need to put the translated code after the stub that called us, that is
    // at position START_OF_TRANSLATED_CODE in the memory block.
    // TODO: check if there is still enough space in the memory block
    // TODO: store name of instruction in table and print it here instead of in the handlers
    // TODO: store position of mode / register byte in table and extract operand here
    const uint8_t *p = p_m68k_code;
    uint8_t *q = p_x86_code + START_OF_TRANSLATED_CODE;
    while (true) {
        opcode = read_word(&p);

        DEBUG("looking up opcode 0x%04x in opcode handler table", opcode);
        if (p_opc_info_lookup_tbl[opcode])
            nbytes_used = p_opc_info_lookup_tbl[opcode]->opc_handler(opcode, &p, &q);
        else {
            ERROR("no handler found for opcode 0x%04x", opcode);
            return NULL;
        }
        if (nbytes_used == -1) {
            ERROR("could not decode instruction at position %p", p - 2);
            return NULL;
        }
        if (p_opc_info_lookup_tbl[opcode]->opc_terminal) {
            DEBUG("instruction is the terminal instruction in this TU - continuing execution of guest");
            // insert jump to the translated code at the beginning of the memory block to
            // keep us from being called again if this TU gets executed more than once
            q = p_x86_code;
            WRITE_BYTE(q, OPCODE_JMP_REL8);
            WRITE_BYTE(q, 0x7e);
            return p_x86_code;
        }
    }
}


//
// unit tests
//
#ifdef TEST
int main()
{
    int retval = 0;
    uint8_t *p_code;

    // test case table consists of one row per test case with two colums (Motorola and Intel opcodes) each
    // TODO: rewrite without using translate_tu
    for (unsigned int i = 0; i < sizeof(testcase_tbl) / (MAX_OPCODE_SIZE + 1) / 2; i++) {
        p_code = translate_tu(&testcase_tbl[i][0][1], 1);
        if (memcmp(&testcase_tbl[i][1][1], p_code, testcase_tbl[i][1][0]) == 0) {
            INFO("test case #%d passed", i);
        }
        else {
            ERROR("test case #%d failed", i);
            ++retval;
        }
    }
    return retval;
}
#endif
