//
// execute.c - part of the Virtual AmigaDOS Machine (VADM)
//             contains the routines for executing the translated code
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "execute.h"
#include "vadm.h"


// registers used for passing arguments to functions as specified by the x86-64 ABI
static uint8_t regs_for_func_args[] = {
    REG_EDI,
    REG_ESI,
    REG_EDX,
    REG_ECX,
    REG_R8D,
    REG_R9D
};


static uint8_t *encode_move_reg_to_reg(uint8_t *p_pos, uint8_t src, uint8_t dst)
{
    // As the source register number is the number of the 680x0 register, we have to differentiate
    // between data and address registers. If it's a data registers (number 0-7), we have to use
    // the REX.R prefix because we use the extended registers R8D..R15D. If it's a address register,
    // (numbers 8-15), we subtract 8 from the number because the address registers are numbered 8-15
    // in the libcall / syscall pragmas and swap the numbers of A4 and A7 in order to map A7 to ESP
    // (with all other registers, we can use the same numbers as the 680x0 uses).
    // If the destination register is an extended register (R8D or R9D for the 5th or 6th argument),
    // we have to use the REX.B prefix, otherwise we have to subtract 8 as well.
    uint8_t prefix = 0;
    if (src < 8) {
        prefix |= PREFIX_REXR;
    }
    else {
        src -= 8;
        switch (src) {
            case 4:
                src = 7;
                break;
            case 7:
                src = 4;
                break;
        }
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
    // MOD-REG-R/M byte with register numbers, mode = 11, source register goes into REG part,
    // destination register into R/M part
    WRITE_BYTE(p_pos, 0xc0 | (src << 3) | dst);
    return p_pos;
}


static uint8_t *create_absolute_call_to_func(uint8_t *p_pos, void (*p_func)())
{
    // push rbp, save old value because EBP = A5 needs to be preserved in AmigaOS
    WRITE_BYTE(p_pos, 0x55);
    // mov rbp, rsp, save old value of RSP before aligning it
    WRITE_BYTE(p_pos, PREFIX_REXW);
    WRITE_BYTE(p_pos, OPCODE_MOV_REG_REG);
    WRITE_BYTE(p_pos, 0xe5);
    // and rsp, 0xfffffffffffffff0, align RSP on a 16-byte boundary, required by the x86-64 ABI
    // (section 3.2.2) before calling any C function, see also and https://stackoverflow.com/a/48684316
    WRITE_BYTE(p_pos, PREFIX_REXW);
    WRITE_BYTE(p_pos, OPCODE_AND_IMM8);
    WRITE_BYTE(p_pos, 0xe4);
    WRITE_BYTE(p_pos, 0xf0);
    // mov rax, p_func
    WRITE_BYTE(p_pos, PREFIX_REXW);
    WRITE_BYTE(p_pos, OPCODE_MOV_IMM64_REG + 0);    // opcode + register
    WRITE_QWORD(p_pos, (uint64_t) p_func);
    // call rax
    WRITE_BYTE(p_pos, OPCODE_CALL_ABS64);
    WRITE_BYTE(p_pos, 0xd0);                        // MOD-REG-R/M byte
    // mov rsp, rbp
    WRITE_BYTE(p_pos, PREFIX_REXW);
    WRITE_BYTE(p_pos, OPCODE_MOV_REG_REG);
    WRITE_BYTE(p_pos, 0xec);
    // pop rbp
    WRITE_BYTE(p_pos, 0x5d);
    return p_pos;
}


static uint8_t *create_thunk_for_func(uint8_t *p_pos, const char *p_func_name, void (*p_func)(), const char *p_arg_regs)
{
    // move the arguments to the correct registers according to the ABI
    // p_arg_regs is the string taken from the libcall / syscall pragmas specifying the
    // registers which are used for function arguments and the return value (usually R8D = D0).
    // It contains the register number of the arguments in reverse order, with D0 = 0 and A0 = 8,
    // the register number of the return value and the number of arguments.
    uint8_t argnum, nargs, regnum;
    sscanf(p_arg_regs + strlen(p_arg_regs) - 1, "%1hhx", &nargs);
    for (argnum = 0; argnum < nargs; argnum++) {
        sscanf(p_arg_regs + argnum, "%1hhx", &regnum);
        p_pos = encode_move_reg_to_reg(p_pos, regnum, regs_for_func_args[nargs - argnum - 1]);
    }

    // TODO: raise interrupt to have the supervisor process log the function name
    // TODO: save all registers that need to be preserved in AmigaOS because they could be altered by the called function
    //       (see Amiga Guru book, page 45 for details)
    p_pos = create_absolute_call_to_func(p_pos, p_func);

    // move return value from RAX to the register specified by the libcall / syscall pragama (usually R8D = D0)
    sscanf(p_arg_regs + argnum, "%1hhx", &regnum);
    p_pos = encode_move_reg_to_reg(p_pos, REG_EAX, regnum);
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
    // and put relative jumps with 32-bit offsets to the second one into the first one (5 bytes in x86-64 code).
    // This second tables lives at the start of the memory block. For the functions that are not
    // implemented, the first table contains interrupt instructions to inform the supervisor process
    // that an unimplemented function has been called by the program.
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
            p_entry_in_2nd = create_thunk_for_func(p_entry_in_2nd, pfi->p_name, pfi->p_func, pfi->p_arg_regs);
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
            DEBUG("child is starting...");
            // allow parent to trace us
            ptrace(PTRACE_TRACEME, 0, 0, 0);
            p_code();
            DEBUG("child is terminating...");
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
                    DEBUG("child has been stopped by signal %s", strsignal(WSTOPSIG(status)));
                    struct user_regs_struct regs;
                    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
                        ERROR("reading registers failed: %s", strerror(errno));
                        return false;
                    }

                    if (WSTOPSIG(status) == SIGTRAP) {
                        ERROR("program called unimplemented library routine - terminating");
                        return false;
                    }
                    else {
                        ERROR("signal other than SIGTRAP received - terminating");
                        return false;
                    }
                }
                else if (WIFEXITED(status)) {
                    INFO("child has exited with status %d", WEXITSTATUS(status));
                    return true;
                }
                else {
                    // shouldn't reach here...
                    ERROR("unknown status of child: %d", status);
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
