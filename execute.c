//
// execute.c - part of the Virtual AmigaDOS Machine (VADM)
//             contains the routines for executing the translated code
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "execute.h"
#include "translate.h"
#include "vadm.h"


// mapping of 680x0 to x86 registers, the 680x0 registers are numbered from 0 to 15 consecutive (D0..D7 and A0..A7)
static uint8_t x86_reg_for_m68k_reg[] = {
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

// registers used for passing arguments to functions as specified by the x86-64 ABI
static uint8_t x86_regs_for_func_args[] = {
    REG_EDI,
    REG_ESI,
    REG_EDX,
    REG_ECX,
    REG_R8D,
    REG_R9D
};


static uint8_t *emit_move_reg_to_reg(uint8_t *p_pos, uint8_t src, uint8_t dst, uint8_t mode)
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


uint8_t *emit_save_all_registers(uint8_t *p_pos)
{
    p_pos = emit_save_amigaos_registers(p_pos);
    p_pos = emit_push_reg(p_pos, x86_reg_for_m68k_reg[REG_D0]);
    p_pos = emit_push_reg(p_pos, x86_reg_for_m68k_reg[REG_D1]);
    p_pos = emit_push_reg(p_pos, x86_reg_for_m68k_reg[REG_A0]);
    p_pos = emit_push_reg(p_pos, x86_reg_for_m68k_reg[REG_A1]);
    return p_pos;
}


uint8_t *emit_restore_all_registers(uint8_t *p_pos)
{
    p_pos = emit_pop_reg(p_pos, x86_reg_for_m68k_reg[REG_A1]);
    p_pos = emit_pop_reg(p_pos, x86_reg_for_m68k_reg[REG_A0]);
    p_pos = emit_pop_reg(p_pos, x86_reg_for_m68k_reg[REG_D1]);
    p_pos = emit_pop_reg(p_pos, x86_reg_for_m68k_reg[REG_D0]);
    p_pos = emit_restore_amigaos_registers(p_pos);
    return p_pos;
}


static uint8_t *emit_func_name_int(uint8_t *p_pos, const char *p_func_name)
{
    // save registers used for parameters
    p_pos = emit_push_reg(p_pos, REG_RAX);      // type of interrupt
    p_pos = emit_push_reg(p_pos, REG_RBX);      // length of function name
    p_pos = emit_push_reg(p_pos, REG_RSI);      // pointer to function name
    // move parameters
    p_pos = emit_move_imm_to_reg(p_pos, INT_TYPE_FUNC_NAME, REG_EAX, MODE_32);
    p_pos = emit_move_imm_to_reg(p_pos, strlen(p_func_name) + 1, REG_EBX, MODE_32);
    p_pos = emit_move_imm_to_reg(p_pos, (uint64_t) p_func_name, REG_RSI, MODE_64);
    // raise interrupt
    WRITE_BYTE(p_pos, OPCODE_INT_3);
    // restore registers
    p_pos = emit_pop_reg(p_pos, REG_RSI);
    p_pos = emit_pop_reg(p_pos, REG_RBX);
    p_pos = emit_pop_reg(p_pos, REG_RAX);
    return p_pos;
}


static uint8_t *emit_thunk_for_func(uint8_t *p_pos, const char *p_func_name, void (*p_func)(), const char *p_arg_regs)
{
    // raise interrupt to have the supervisor process log the function name
//    p_pos = emit_func_name_int(p_pos, p_func_name);

    // save all registers that need to be preserved in AmigaOS because they could be altered by the called function
    p_pos = emit_save_amigaos_registers(p_pos);

    // move the arguments to the correct registers according to the x86-64 ABI
    // p_arg_regs is the string taken from the libcall / syscall pragmas specifying the
    // registers which are used for function arguments and the return value (usually R8D = D0).
    // It contains the register number of the arguments in reverse order, with D0 = 0 and A0 = 8,
    // the register number of the return value and the number of arguments.
    uint8_t argnum, nargs, regnum;
    sscanf(p_arg_regs + strlen(p_arg_regs) - 1, "%1hhx", &nargs);
    for (argnum = 0; argnum < nargs; argnum++) {
        sscanf(p_arg_regs + argnum, "%1hhx", &regnum);
        p_pos = emit_move_reg_to_reg(p_pos, x86_reg_for_m68k_reg[regnum], x86_regs_for_func_args[nargs - argnum - 1], MODE_32);
    }

    // call function
    p_pos = emit_abs_call_to_func(p_pos, p_func);

    // move return value from EAX to the register specified by the libcall / syscall pragama (usually R8D = D0)
    sscanf(p_arg_regs + argnum, "%1hhx", &regnum);
    p_pos = emit_move_reg_to_reg(p_pos, REG_EAX, regnum, MODE_32);

    // restore registers
    p_pos = emit_restore_amigaos_registers(p_pos);

    // return
    WRITE_BYTE(p_pos, OPCODE_RET);
    return p_pos;
}


static void setup_jump_tables(uint8_t *p_lib_base, const FuncInfo *p_func_info_tbl)
{
    // There are two jump tables to create. The first is the one that is used by the programs
    // that use the library to call the functions. The offsets in this table are specified in
    // the API documentation of the AmigaOS (in the FD files). They have to be subtracted from
    // the library base address as returned by OpenLibrary(). This means, we place this table
    // at the end of the memory block reserved for the jump tables and have OpenLibrary() return
    // this address.
    // In the AmigaOS, this table contained absolute jumps to the actual functions. However,
    // as the entries in this table are only 6 bytes apart each, there is not enough room to put
    // absolute jumps to the functions with 64-bit addresses there. Therefore, we create a
    // second table with the absolute jumps (and some additional code, so it's actually a thunk)
    // and put relative jumps with 32-bit offsets to the second one into the first one (5 bytes
    // in x86-64 code). This second tables lives at the start of the memory block. For the functions
    // that are not implemented, the first table contains interrupt instructions to inform the
    // supervisor process that an unimplemented function has been called by the program.
    uint8_t *p_entry_in_1st, *p_entry_in_2nd = p_lib_base;
    for (const FuncInfo *pfi = p_func_info_tbl; pfi->offset != 0; ++pfi) {
        p_entry_in_1st = p_lib_base + LIB_JUMP_TBL_SIZE - pfi->offset;
        if (pfi->p_func == NULL) {
            // function not implemented => interrupt
//            DEBUG("creating entry with interrupt for function %s()", pfi->p_name);
            *p_entry_in_1st = OPCODE_INT_3;
        }
        else {
            // function implemented => relative jump to 2nd table
            // offset = address of entry in 2nd table - address after JMP instruction including offset
            DEBUG("creating entry with jump and thunk for function %s()", pfi->p_name);
            *p_entry_in_1st = OPCODE_JMP_REL32;
            *((int32_t *) (p_entry_in_1st + 1)) = p_entry_in_2nd - (p_entry_in_1st + 5);
            p_entry_in_2nd = emit_thunk_for_func(p_entry_in_2nd, pfi->p_name, pfi->p_func, pfi->p_arg_regs);
        }
    }
}


uint8_t *load_library(const char *p_lib_name)
{
    DEBUG("dlopen()ing library '%s'", p_lib_name);
    void *lh;
    if ((lh = dlopen(p_lib_name, RTLD_NOW)) == NULL) {
        ERROR("dlopen() failed");
        return NULL;
    }
    if (dlsym(lh, "g_func_info_tbl") == NULL) {
        ERROR("library does not contain function info table");
        return NULL;
    }

    DEBUG("setting up library jump tables");
    static uint8_t *p_lib_base = (uint8_t *) LIB_BASE_START_ADDRESS;
    if ((p_lib_base = mmap(p_lib_base,
                           LIB_JUMP_TBL_SIZE,
                           PROT_READ | PROT_WRITE | PROT_EXEC,
                           MAP_FIXED | MAP_ANON | MAP_PRIVATE,
                           -1,
                           0)) == MAP_FAILED) {
        ERROR("could not create memory mapping for library jump tables: %s", strerror(errno));
        return NULL;
    }
    setup_jump_tables(p_lib_base, dlsym(lh, "g_func_info_tbl"));
    p_lib_base += LIB_JUMP_TBL_SIZE;
    return p_lib_base;
}


bool exec_program(int (*p_code)())
{
    int pid, status;

    // load Exec library and store base address at ABS_EXEC_BASE
    DEBUG("loading Exec library");
    uint8_t *p_exec_base;
    uint32_t *p_abs_exec_base;
    if ((p_exec_base = load_library("libs/libexec.so")) == NULL) {
        ERROR("could not load Exec library");
        return 1;
    }
    if ((p_abs_exec_base = mmap((void *) ABS_EXEC_BASE,
                           4,
                           PROT_READ | PROT_WRITE | PROT_EXEC,
                           MAP_FIXED | MAP_ANON | MAP_PRIVATE,
                           -1,
                           0)) == MAP_FAILED) {
        ERROR("could not create memory mapping for ABS_EXEC_BASE: %s", strerror(errno));
        return NULL;
    }
    #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    *p_abs_exec_base = (uint32_t) p_exec_base;
    #pragma GCC diagnostic pop

    // create separate process for the program
    switch ((pid = fork())) {
        case 0:     // child
            DEBUG("guest is starting...");
            // allow parent to trace us
//            ptrace(PTRACE_TRACEME, 0, 0, 0);
            p_code();
            DEBUG("guest is terminating...");
            // TODO: capture and return actual return value (in register R8D)
            exit(0);

        case -1:    // error
            ERROR("fork() failed: %s", strerror(errno));
            return false;

        default:    // parent
            while (true) {
                // wait for child
                pid = wait(&status);
                if (WIFSTOPPED(status)) {
                    DEBUG("guest has been stopped by signal '%s'", strsignal(WSTOPSIG(status)));
                    struct user_regs_struct regs;
                    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
                        ERROR("reading registers failed: %s", strerror(errno));
                        return false;
                    }
                    DEBUG("current RIP of guest = %p", (uint8_t *) regs.rip);

                    if (WSTOPSIG(status) == SIGTRAP) {
                        uint8_t nbytes_read = 0;
                        uint8_t *p_next_tu;
                        char func_name[64];
                        switch ((uint32_t) regs.rax) {
                            case INT_TYPE_FUNC_NAME:
                                // copy function name from address stored in RSI, length (including NUL byte) is stored in RBX
                                while (nbytes_read < regs.rbx) {
                                    *((uint64_t *) (func_name + nbytes_read)) = ptrace(PTRACE_PEEKDATA, pid, regs.rsi + nbytes_read, NULL);
                                    nbytes_read += 8;
                                }
                                DEBUG("guest called library function %s()", func_name);
                                ptrace(PTRACE_CONT, pid, 0, 0);
                                break;
                            case INT_TYPE_BRANCH_FAULT:
                                // translate TU and continue guest
                                DEBUG("branch fault occurred");
                                if ((p_next_tu = translate_tu(*((uint8_t **) regs.rip))) == NULL) {
                                    ERROR("translating next TU failed");
                                    return false;
                                }
                                regs.rip = (uint64_t) p_next_tu;
                                if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1) {
                                    ERROR("setting RIP failed: %s", strerror(errno));
                                    return false;
                                }
                                DEBUG("continuing guest with next TU at address %p", (uint8_t *) regs.rip);
                                ptrace(PTRACE_CONT, pid, 0, 0);
                                break;
                            default:
                                ERROR("guest called unimplemented library function - terminating");
                                return false;
                        }
                    }
                    else {
                        ERROR("signal other than SIGTRAP received - terminating");
                        return false;
                    }
                }
                else if (WIFEXITED(status)) {
                    INFO("guest has exited with status %d", WEXITSTATUS(status));
                    return true;
                }
                else {
                    // shouldn't reach here...
                    ERROR("unknown status of guest: %d", status);
                    return false;
                }
            }
    }
    // shouldn't reach here...
    return false;
}


//
// unit tests
//
#ifdef TEST
int main()
{

    // call library functions via the jump tables
    register uint8_t *p_lib_base asm("rsi");    // library base address
    register char *p_lib_name asm("rcx");       // argument for OpenLibrary()
    register char *p_str asm("r9");             // argument for PutStr()
    p_lib_base = load_library("libs/libexec.so");
    p_lib_name = "dos.library";
    asm(
        "add    $-552, %esi\n"
        "call   *%rsi\n"
        "mov    %r8d, %esi"
    );
    p_str = "So a scheener Dog\n";
    asm(
        "add    $-948, %esi\n"
        "call   *%rsi\n"
    );
    return 0;
}
#endif
