//
// main.c - part of the Virtual AmigaDOS Machine (VADM)
//          contains the main routine
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "loader.h"
#include "translate.h"
#include "vadm.h"


// TODO: fix all warnings

static bool exec_program(int (*p_code)())
{
    int pid, status;
    csh handle;
    cs_insn *insn;

    // initialize Capstone
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) {
        printf("could not initialize Capstone");
        return 1;
    }

    // create separate process for the program
    switch ((pid = fork())) {
        case 0:     // child
            DEBUG("child is starting...");
            ptrace(PTRACE_TRACEME, 0, 0, 0);  // allow parent to trace us

            pid = getpid();
            kill(pid, SIGTRAP);  // tell parent to single-step us
            p_code();            // call program
            kill(pid, SIGCONT);  // tell parent to continue normally

            DEBUG("child is terminating...");
            // TODO: capture and return actual return value (in register R8D)
            return true;

        case -1:    // error
            ERROR("fork() failed: %s", strerror(errno));
            return false;

        default:    // parent
            while (true) {
                // wait for child
                pid = wait(&status);
                if (WIFSTOPPED(status)) {
                    DEBUG("child has been stopped by signal %s", strsignal(WSTOPSIG(status)));
                    if (WSTOPSIG(status) == SIGTRAP) {
                        // read registers (only RIP is relevant)
                        struct user_regs_struct regs;
                        if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
                            ERROR("reading registers failed: %s", strerror(errno));
                            return false;
                        }

                        // disassemble next instruction (the one pointed to by RIP)
                        if (cs_disasm(handle, regs.rip, 16, regs.rip, 1, &insn) > 0) {
                            DEBUG("RIP=%p:\t%s\t\t%s", insn[0].address, insn[0].mnemonic, insn[0].op_str);
                            cs_free(insn, 1);
                        }
                        else {
                            ERROR("could not disassemble instruction");
                            return false;
                        }

                        // execute next instruction
                        ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
                    }
                    else if (WSTOPSIG(status) == SIGCONT) {
                        DEBUG("continuing program normally");
                        ptrace(PTRACE_CONT, pid, 0, 0);
                    }
                    else {
                        ERROR("signal other than SIGTRAP or SIGCONT received - terminating");
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


int main(int argc, char **argv)
{
    uint8_t *p_m68k_code_addr, *p_x86_code_addr;
    uint32_t m68k_code_size;

    INFO("loading program...");
    if (!load_program(argv[1], &p_m68k_code_addr, &m68k_code_size)) {
        ERROR("loading program failed");
        return 1;
    }
    INFO("translating code...");
    if ((p_x86_code_addr = translate_unit(p_m68k_code_addr, UINT32_MAX)) == NULL) {
        ERROR("translating code failed");
        return 1;
    }
    INFO("executing program...");
    if (!exec_program(p_x86_code_addr)) {
        ERROR("executing program failed");
        return 1;
    }
    return 0;
}
