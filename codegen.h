//
// codegen.h - part of the Virtual AmigaDOS Machine (VADM)
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 
#ifndef CODEGEN_H_INCLUDED
#define CODEGEN_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

// constants for encoding the instructions
#define MODE_32 0
#define MODE_64 1
#define OPCODE_INT_3            0xcc
#define OPCODE_JMP_REL8         0xeb
#define OPCODE_JMP_REL32        0xe9
#define OPCODE_JMP_ABS64        0xff
#define OPCODE_CALL_ABS64       0xff
#define OPCODE_MOV_REG_REG      0x89
#define OPCODE_MOV_IMM_REG      0xb8
#define OPCODE_RET              0xc3
#define OPCODE_AND_IMM8         0x83
#define OPCODE_PUSH_REG         0x50
#define OPCODE_POP_REG          0x58
#define OPCODE_PUSHFQ           0x9c
#define OPCODE_POPFQ            0x9d
#define OPCODE_NOP              0x90
#define PREFIX_REXB             0x41
#define PREFIX_REXR             0x44
#define PREFIX_REXW             0x48

// helper macros
#define WRITE_BYTE(p_pos, val) {*p_pos++ = (val);}
#define WRITE_DWORD(p_pos, val) {*((uint32_t *) p_pos) = (val); p_pos += 4;}
#define WRITE_QWORD(p_pos, val) {*((uint64_t *) p_pos) = (val); p_pos += 8;}

// register numbers as used by the 680x0
enum m68k_registers {
    REG_D0, REG_D1, REG_D2, REG_D3, REG_D4, REG_D5, REG_D6, REG_D7,
    REG_A0, REG_A1, REG_A2, REG_A3, REG_A4, REG_A5, REG_A6, REG_A7
};

// register numbers as used in the instruction encodings for the x86, 32 and 64 bit registers
enum x86_32_registers {
    REG_R8D, REG_R9D, REG_R10D, REG_R11D, REG_R12D, REG_R13D, REG_R14D, REG_R15D,
    REG_EAX, REG_ECX, REG_EDX, REG_EBX, REG_ESP, REG_EBP, REG_ESI, REG_EDI
};
enum x86_64_registers {
    REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15,
    REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI
};

extern uint8_t x86_reg_for_m68k_reg[];
extern uint8_t x86_regs_for_func_args[];

// prototypes
uint8_t *emit_push_reg(uint8_t *p_pos, uint8_t reg);
uint8_t *emit_pop_reg(uint8_t *p_pos, uint8_t reg);
uint8_t *emit_move_imm_to_reg(uint8_t *p_pos, uint64_t value, uint8_t reg, uint8_t mode);
uint8_t *emit_move_reg_to_reg(uint8_t *p_pos, uint8_t src, uint8_t dst, uint8_t mode);
uint8_t *emit_abs_call_to_func(uint8_t *p_pos, void (*p_func)());
uint8_t *emit_save_amigaos_registers(uint8_t *p_pos);
uint8_t *emit_restore_amigaos_registers(uint8_t *p_pos);
uint8_t *emit_save_program_state(uint8_t *p_pos);
uint8_t *emit_restore_program_state(uint8_t *p_pos);

#endif  // EXECUTE_H_INCLUDED

