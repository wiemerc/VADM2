#include <capstone/capstone.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>


int main(int argc, char **argv)
{
    struct user_regs_struct     regs;           // buffer for processor registers
    struct stat                 sb;             // buffer for fstat
    int fd, pid, status, i = 0;
    uint8_t *shmem;
    csh handle;
    cs_insn *insn;

    // map whole image into memory
    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        perror("could not open file");
        return 1;
    }
    if (fstat(fd, &sb) == -1) {
        perror("could not get file status");
        return 1;
    }
    if ((shmem = mmap(NULL, sb.st_size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
        perror("could not memory-map file");
        return 1;
    }
    printf("shared memory mapped at %p\n", shmem);

    // initialize Capstone
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) {
        printf("could not initialize Capstone");
        return 1;
    }

    switch ((pid = fork())) {
        case 0:
            //
            // child
            //

            // allow parent to trace us
            printf("child is starting...\n");
            ptrace(PTRACE_TRACEME, 0, 0, 0);

            // send signal to ourselves to give control back to parent
            kill(getpid(), SIGTRAP);

            // should normally not be reached...
            printf("child is terminating...\n");
            return 0;

        case -1:
            perror("fork() failed");
            return 1;

        default:
            //
            // parent
            //

            // wait for child
            while(i < 20) {
                ++i;
                pid = wait(&status);
//                printf("child gave control back to us\n");
                if (WIFSTOPPED(status)) {
//                    printf("child has been stopped by signal %s\n", strsignal(WSTOPSIG(status)));
                    // if child is stopped, get registers and continue
                    if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
                        perror("ptrace(PTRACE_GETREGS, ...) failed");
                        return 1;
                    }
                    if (cs_disasm(handle, regs.rip, 16, regs.rip, 1, &insn) > 0) {
                        printf("RIP=%p:\t%s\t\t%s\n", insn[0].address, insn[0].mnemonic, insn[0].op_str);
                        cs_free(insn, 1);
                    }
                    else {
                        printf("could not disassemble instruction");
                        return 1;
                    }

                    if (i == 1) {
                        regs.rip = (unsigned long) shmem;
                        if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1) {
                            perror("ptrace(PTRACE_SETREGS, ...) failed");
                            return 1;
                        }
                    }
                    ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
                }
                else if (WIFEXITED(status)) {
                    printf("child has exited with status %d\n", WEXITSTATUS(status));
                    return WEXITSTATUS(status);
                }
                else
                    return 0;
            }
    }

    munmap(shmem, 16);
    return 0;
}
