//
// execute.c - part of the Virtual AmigaDOS Machine (VADM)
//             contains the routines for executing the translated code
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "execute.h"
#include "vadm.h"


static void log_func_name(const char *p_func_name)
{
    DEBUG("calling function %s()", p_func_name);
}


static uint8_t create_call_to_log_func_name(uint8_t *p_code, const char *p_func_name)
{
    // mov rdi, p_func_name
    // RDI holds the 1st function argument in the x86-64 ABI
    p_code[0] = 0x48;  // REX.W prefix
    p_code[1] = 0xbf;  // MOV opcode + register
    *((const char **) (p_code + 2)) = p_func_name;
    // mov rax, log_func_name
    p_code[10] = 0x48;  // REX.W prefix
    p_code[11] = 0xb8;  // MOV opcode + register
    *((void (**)()) (p_code + 12)) = log_func_name;
    // call rax
    p_code[20] = 0xff;  // CALL opcode
    p_code[21] = 0xd0;  // MOD-REG-R/M byte
    return 22;  // number of bytes written
}


static uint8_t create_move_args_thunk(uint8_t *p_code, uint8_t *p_arg_regs)
{
    // TODO: move the arguments to the correct registers according to the ABI
}


static uint8_t create_absolute_jmp_to_func(uint8_t *p_code, void (*p_func)())
{
    p_code[0] = OPCODE_JMP_ABS64;
    p_code[1] = 0x25;  // MOD-REG-R/M byte
    p_code[2] = 0x00;  // relative address where absolute address is stored => immediately after instruction
    p_code[3] = 0x00;
    p_code[4] = 0x00;
    p_code[5] = 0x00;
    *((void (**)()) &p_code[6]) = p_func;
    return 14;  // number of bytes written
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
    // second table with the absolute jumps and put relative jumps with 32-bit offsets to the
    // second one into the first one (5 bytes in x86-64 code). This second tables lives at the
    // start of the memory block. For the functions that are not implemented, the first table
    // contains interrupt instructions to inform the supervisor process that an unimplemented
    // function has been called by the program.
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
            DEBUG("creating entry with jump for function %s()", pfi->p_name);
            *p_entry_in_1st = OPCODE_JMP_REL32;
            *((int32_t *) (p_entry_in_1st + 1)) = p_entry_in_2nd - (p_entry_in_1st + 5);
            p_entry_in_2nd += create_call_to_log_func_name(p_entry_in_2nd, pfi->p_name);
//            p_entry_in_2nd += create_move_args_thunk(p_entry_in_2nd, pfi->arg_regs);
            p_entry_in_2nd += create_absolute_jmp_to_func(p_entry_in_2nd, pfi->p_func);
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
