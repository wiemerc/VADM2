//
// codegen.c - part of the Virtual AmigaDOS Machine (VADM)
//             contains the routines for the code generation
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "codegen.h"
#include "util.h"


// registers that need to be preserved in AmigaOS, see Amiga Guru book, page 45, for details
// The frame (A5) and stack (A7) pointers don't need to be saved / restored by us because
// the prolog / epilog of the called function take care of that. Registers D4-D7 (R12D-R15D)
// and A3 (EBX) don't need to be saved / restored by us either because they're preserved by
// the called function according to the x86-64 ABI.
static uint8_t amigaos_regs_to_preserve[] = {
    REG_D2,
    REG_D3,
    REG_A2,
    REG_A4,
    REG_A6,
};


// mapping of 680x0 to x86 registers, the 680x0 registers are numbered from 0 to 15 consecutive (D0..D7 and A0..A7)
uint8_t x86_reg_for_m68k_reg[] = {
    REG_R8D,
    REG_R9D,
    REG_R10D,
    REG_R11D,
    REG_R12D,
    REG_R13D,
    REG_R14D,
    REG_R15D,
    REG_EAX,
    REG_ECX,
    REG_EDX,
    REG_EBX,
    REG_EDI,    // swapped with ESP
    REG_EBP,
    REG_ESI,
    REG_ESP     // swapped with EDI
};


// registers used for passing arguments to functions as specified by the x86-64 ABI
uint8_t x86_regs_for_func_args[] = {
    REG_EDI,
    REG_ESI,
    REG_EDX,
    REG_ECX,
    REG_R8D,
    REG_R9D
};


uint8_t *emit_move_reg_to_reg(uint8_t *p_pos, uint8_t src, uint8_t dst, uint8_t mode)
{
    uint8_t prefix = 0;
    if (mode == MODE_64) {
        prefix |= PREFIX_REXW;
    }
    if (src < 8) {
        // extended registers R8D..R15D
        prefix |= PREFIX_REXR;
    }
    else {
        // registers EAX..EDI, also encoded as number 0..7, but without a prefix
        src -= 8;
    }
    if (dst < 8) {
        prefix |= PREFIX_REXB;
    }
    else {
        dst -= 8;
    }
    if (prefix != 0) {
        WRITE_BYTE(p_pos, prefix);
    }
    WRITE_BYTE(p_pos, OPCODE_MOV_REG_REG);
    // MOD-REG-R/M byte with register numbers, mode = 11 (register only), source register goes into REG part,
    // destination register into R/M part
    WRITE_BYTE(p_pos, 0xc0 | (src << 3) | dst);
    return p_pos;
}


uint8_t *emit_move_imm_to_reg(uint8_t *p_pos, uint64_t value, uint8_t reg, uint8_t mode)
{
    uint8_t prefix = 0;
    if (mode == MODE_64) {
        prefix |= PREFIX_REXW;
    }
    if (reg < 8) {
        // extended registers R8D..R15D
        prefix |= PREFIX_REXR;
    }
    else {
        // registers EAX..EDI, also encoded as number 0..7, but without a prefix
        reg -= 8;
    }
    if (prefix != 0) {
        WRITE_BYTE(p_pos, prefix);
    }
    WRITE_BYTE(p_pos, OPCODE_MOV_IMM_REG + reg);
    if (mode == MODE_64) {
        WRITE_QWORD(p_pos, value);
    }
    else {
        WRITE_DWORD(p_pos, (uint32_t) value);
    }
    return p_pos;
}


uint8_t *emit_push_reg(uint8_t *p_pos, uint8_t reg)
{
    uint8_t prefix = 0;
    if (reg < 8) {
        prefix = PREFIX_REXB;
    }
    else {
        reg -= 8;
    }
    if (prefix != 0) {
        WRITE_BYTE(p_pos, prefix);
    }
    WRITE_BYTE(p_pos, OPCODE_PUSH_REG + reg);
    return p_pos;
}


uint8_t *emit_pop_reg(uint8_t *p_pos, uint8_t reg)
{
    uint8_t prefix = 0;
    if (reg < 8) {
        prefix = PREFIX_REXB;
    }
    else {
        reg -= 8;
    }
    if (prefix != 0) {
        WRITE_BYTE(p_pos, prefix);
    }
    WRITE_BYTE(p_pos, OPCODE_POP_REG + reg);
    return p_pos;
}


uint8_t *emit_abs_call_to_func(uint8_t *p_pos, void (*p_func)())
{
    // save old value of RBP because EBP = A5 needs to be preserved in AmigaOS
    p_pos = emit_push_reg(p_pos, REG_RBP);
    // save old value of RSP before aligning it
    p_pos = emit_move_reg_to_reg(p_pos, REG_RSP, REG_RBP, MODE_64);
    // and rsp, 0xfffffffffffffff0, align RSP on a 16-byte boundary, required by the x86-64 ABI
    // (section 3.2.2) before calling any C function, see also https://stackoverflow.com/a/48684316
    WRITE_BYTE(p_pos, PREFIX_REXW);
    WRITE_BYTE(p_pos, OPCODE_AND_IMM8);
    WRITE_BYTE(p_pos, 0xe4);                        // MOD-REG-R/M byte with opcode extension and register
    WRITE_BYTE(p_pos, 0xf0);                        // immediate value, gets sign-extended to 64 bits
    // mov rax, p_func
    p_pos = emit_move_imm_to_reg(p_pos, (uint64_t) p_func, REG_RAX, MODE_64);
    // call rax
    WRITE_BYTE(p_pos, OPCODE_CALL_ABS64);
    WRITE_BYTE(p_pos, 0xd0);                        // MOD-REG-R/M byte with register
    p_pos = emit_move_reg_to_reg(p_pos, REG_RBP, REG_RSP, MODE_64);
    p_pos = emit_pop_reg(p_pos, REG_RBP);
    return p_pos;
}


//
// The following two functions save / restore all registers that needed to be preserved
// across a function call in AmigaOS (see amigaos_regs_to_preserve for the list).
//
uint8_t *emit_save_amigaos_registers(uint8_t *p_pos)
{
    int8_t i;
    for (i = 0; i <= (int8_t) (sizeof(amigaos_regs_to_preserve) / sizeof(amigaos_regs_to_preserve[0]) - 1); i++) {
        p_pos = emit_push_reg(p_pos, x86_reg_for_m68k_reg[amigaos_regs_to_preserve[i]]);
    }
    return p_pos;
}


uint8_t *emit_restore_amigaos_registers(uint8_t *p_pos)
{
    int8_t i;
    for (i = (int8_t) (sizeof(amigaos_regs_to_preserve) / sizeof(amigaos_regs_to_preserve[0]) - 1); i >= 0; i--) {
        p_pos = emit_pop_reg(p_pos, x86_reg_for_m68k_reg[amigaos_regs_to_preserve[i]]);
    }
    return p_pos;
}


//
// The following two functions make a call to a function completely transparent to the Amiga
// program by saving / restoring all registers that needed to be preserved across a function
// call in AmigaOS, and in addition D0/D1, A0/A1 and RFLAGS.
//
uint8_t *emit_save_program_state(uint8_t *p_pos)
{
    p_pos = emit_save_amigaos_registers(p_pos);
    p_pos = emit_push_reg(p_pos, x86_reg_for_m68k_reg[REG_D0]);
    p_pos = emit_push_reg(p_pos, x86_reg_for_m68k_reg[REG_D1]);
    p_pos = emit_push_reg(p_pos, x86_reg_for_m68k_reg[REG_A0]);
    p_pos = emit_push_reg(p_pos, x86_reg_for_m68k_reg[REG_A1]);
    WRITE_BYTE(p_pos, OPCODE_PUSHFQ);
    return p_pos;
}


uint8_t *emit_restore_program_state(uint8_t *p_pos)
{
    WRITE_BYTE(p_pos, OPCODE_POPFQ);
    p_pos = emit_pop_reg(p_pos, x86_reg_for_m68k_reg[REG_A1]);
    p_pos = emit_pop_reg(p_pos, x86_reg_for_m68k_reg[REG_A0]);
    p_pos = emit_pop_reg(p_pos, x86_reg_for_m68k_reg[REG_D1]);
    p_pos = emit_pop_reg(p_pos, x86_reg_for_m68k_reg[REG_D0]);
    p_pos = emit_restore_amigaos_registers(p_pos);
    return p_pos;
}
