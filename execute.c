//
// execute.c - part of the Virtual AmigaDOS Machine (VADM)
//             contains the routines for executing the translated code
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "codegen.h"
#include "execute.h"
#include "translate.h"
#include "vadm.h"
#include "util.h"


static void log_func_name(const char *p_func_name)
{
    DEBUG("guest called library function %s()", p_func_name);
}


static uint8_t *emit_call_to_log_func_name(uint8_t *p_pos, const char *p_func_name)
{
    // move pointer to function name to register for first function argument = RDI
    p_pos = emit_move_imm_to_reg(p_pos, (uint64_t) p_func_name, REG_RDI, MODE_64);
    // call function to log the name
    p_pos = emit_abs_call_to_func(p_pos, (void (*)()) log_func_name);
    return p_pos;
}


static uint8_t *emit_thunk_for_func(uint8_t *p_pos, const char *p_func_name, void (*p_func)(), const char *p_arg_regs)
{
    // log the function name
    p_pos = emit_save_program_state(p_pos);
    p_pos = emit_call_to_log_func_name(p_pos, p_func_name);
    p_pos = emit_restore_program_state(p_pos);

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
        return false;
    }
    if ((p_abs_exec_base = mmap((void *) ABS_EXEC_BASE,
                           4,
                           PROT_READ | PROT_WRITE | PROT_EXEC,
                           MAP_FIXED | MAP_ANON | MAP_PRIVATE,
                           -1,
                           0)) == MAP_FAILED) {
        ERROR("could not create memory mapping for ABS_EXEC_BASE: %s", strerror(errno));
        return false;
    }
    #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    *p_abs_exec_base = (uint32_t) p_exec_base;
    #pragma GCC diagnostic pop

    // create separate process for the program
    switch ((pid = fork())) {
        case 0:     // child
            DEBUG("guest is starting...");
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
                if (WIFSIGNALED(status)) {
                    DEBUG("guest has been terminated by signal '%s'", strsignal(WTERMSIG(status)));

                    if (WTERMSIG(status) == SIGTRAP) {
                        ERROR("guest called unimplemented library function - terminating");
                        return false;
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
                    // shouldn't arrive here...
                    ERROR("unknown status of guest: %d", status);
                    return false;
                }
            }
    }
    // shouldn't arrive here...
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
