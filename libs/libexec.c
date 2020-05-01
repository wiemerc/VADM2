//
// execute.c - part of the Virtual AmigaDOS Machine (VADM)
//             contains the implemented routines of the Exec library
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include <bsd/string.h>

#include "../execute.h"  // for load_library()


#define MAX_PATH_LEN 256


#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
void exec_open_library()
{
    register const char *p_lib_name asm("rcx");         // A1 => ECX
    register uint8_t *p_lib_base asm("r8");             // D0 => R8D, the library version is passed in D0 as well, but not used right now
    char lib_path_name[MAX_PATH_LEN];

    // build actual path name from library name
    asm("push %rcx");                                   // save RCX from being clobbered by strlcpy()
    strlcpy(lib_path_name, "libs/lib", MAX_PATH_LEN);   // prefix with directory where the libraries live and 'lib'
    asm("pop %rcx");
    if (strlcat(lib_path_name, p_lib_name, MAX_PATH_LEN) > MAX_PATH_LEN) {
        // library name is too long
        p_lib_base = NULL;
        return;
    }
    strtok(lib_path_name, ".");                         // remove extension .library (overwrite . with NUL)
    strlcat(lib_path_name, ".so", MAX_PATH_LEN);        // add extension .so

    p_lib_base = load_library(lib_path_name);
}

void exec_close_library()
{
    // TODO: call dlclose(), but how do we get the library handle?
}
#pragma GCC diagnostic pop


// lines below have been generated with the following command:
// grep syscall /opt/m68k-amigaos//m68k-amigaos/ndk/include/pragmas/exec_pragmas.h | perl -nale 'print "    {0x$F[3], \"$F[2]\", NULL},"'
FuncInfo g_func_info_tbl[] = {
    {0x1e, "Supervisor", NULL},
    {0x48, "InitCode", NULL},
    {0x4e, "InitStruct", NULL},
    {0x54, "MakeLibrary", NULL},
    {0x5a, "MakeFunctions", NULL},
    {0x60, "FindResident", NULL},
    {0x66, "InitResident", NULL},
    {0x6c, "Alert", NULL},
    {0x72, "Debug", NULL},
    {0x78, "Disable", NULL},
    {0x7e, "Enable", NULL},
    {0x84, "Forbid", NULL},
    {0x8a, "Permit", NULL},
    {0x90, "SetSR", NULL},
    {0x96, "SuperState", NULL},
    {0x9c, "UserState", NULL},
    {0xa2, "SetIntVector", NULL},
    {0xa8, "AddIntServer", NULL},
    {0xae, "RemIntServer", NULL},
    {0xb4, "Cause", NULL},
    {0xba, "Allocate", NULL},
    {0xc0, "Deallocate", NULL},
    {0xc6, "AllocMem", NULL},
    {0xcc, "AllocAbs", NULL},
    {0xd2, "FreeMem", NULL},
    {0xd8, "AvailMem", NULL},
    {0xde, "AllocEntry", NULL},
    {0xe4, "FreeEntry", NULL},
    {0xea, "Insert", NULL},
    {0xf0, "AddHead", NULL},
    {0xf6, "AddTail", NULL},
    {0xfc, "Remove", NULL},
    {0x102, "RemHead", NULL},
    {0x108, "RemTail", NULL},
    {0x10e, "Enqueue", NULL},
    {0x114, "FindName", NULL},
    {0x11a, "AddTask", NULL},
    {0x120, "RemTask", NULL},
    {0x126, "FindTask", NULL},
    {0x12c, "SetTaskPri", NULL},
    {0x132, "SetSignal", NULL},
    {0x138, "SetExcept", NULL},
    {0x13e, "Wait", NULL},
    {0x144, "Signal", NULL},
    {0x14a, "AllocSignal", NULL},
    {0x150, "FreeSignal", NULL},
    {0x156, "AllocTrap", NULL},
    {0x15c, "FreeTrap", NULL},
    {0x162, "AddPort", NULL},
    {0x168, "RemPort", NULL},
    {0x16e, "PutMsg", NULL},
    {0x174, "GetMsg", NULL},
    {0x17a, "ReplyMsg", NULL},
    {0x180, "WaitPort", NULL},
    {0x186, "FindPort", NULL},
    {0x18c, "AddLibrary", NULL},
    {0x192, "RemLibrary", NULL},
    {0x198, "OldOpenLibrary", NULL},
    {0x19e, "CloseLibrary", exec_close_library},
    {0x1a4, "SetFunction", NULL},
    {0x1aa, "SumLibrary", NULL},
    {0x1b0, "AddDevice", NULL},
    {0x1b6, "RemDevice", NULL},
    {0x1bc, "OpenDevice", NULL},
    {0x1c2, "CloseDevice", NULL},
    {0x1c8, "DoIO", NULL},
    {0x1ce, "SendIO", NULL},
    {0x1d4, "CheckIO", NULL},
    {0x1da, "WaitIO", NULL},
    {0x1e0, "AbortIO", NULL},
    {0x1e6, "AddResource", NULL},
    {0x1ec, "RemResource", NULL},
    {0x1f2, "OpenResource", NULL},
    {0x20a, "RawDoFmt", NULL},
    {0x210, "GetCC", NULL},
    {0x216, "TypeOfMem", NULL},
    {0x21c, "Procure", NULL},
    {0x222, "Vacate", NULL},
    {0x228, "OpenLibrary", exec_open_library},
    {0x22e, "InitSemaphore", NULL},
    {0x234, "ObtainSemaphore", NULL},
    {0x23a, "ReleaseSemaphore", NULL},
    {0x240, "AttemptSemaphore", NULL},
    {0x246, "ObtainSemaphoreList", NULL},
    {0x24c, "ReleaseSemaphoreList", NULL},
    {0x252, "FindSemaphore", NULL},
    {0x258, "AddSemaphore", NULL},
    {0x25e, "RemSemaphore", NULL},
    {0x264, "SumKickData", NULL},
    {0x26a, "AddMemList", NULL},
    {0x270, "CopyMem", NULL},
    {0x276, "CopyMemQuick", NULL},
    {0x27c, "CacheClearU", NULL},
    {0x282, "CacheClearE", NULL},
    {0x288, "CacheControl", NULL},
    {0x28e, "CreateIORequest", NULL},
    {0x294, "DeleteIORequest", NULL},
    {0x29a, "CreateMsgPort", NULL},
    {0x2a0, "DeleteMsgPort", NULL},
    {0x2a6, "ObtainSemaphoreShared", NULL},
    {0x2ac, "AllocVec", NULL},
    {0x2b2, "FreeVec", NULL},
    {0x2b8, "CreatePool", NULL},
    {0x2be, "DeletePool", NULL},
    {0x2c4, "AllocPooled", NULL},
    {0x2ca, "FreePooled", NULL},
    {0x2d0, "AttemptSemaphoreShared", NULL},
    {0x2d6, "ColdReboot", NULL},
    {0x2dc, "StackSwap", NULL},
    {0x2fa, "CachePreDMA", NULL},
    {0x300, "CachePostDMA", NULL},
    {0x306, "AddMemHandler", NULL},
    {0x30c, "RemMemHandler", NULL},
    {0x312, "ObtainQuickVector", NULL},
    {0x33c, "NewMinList", NULL},
    {0x354, "AVL_AddNode", NULL},
    {0x35a, "AVL_RemNodeByAddress", NULL},
    {0x360, "AVL_RemNodeByKey", NULL},
    {0x366, "AVL_FindNode", NULL},
    {0x36c, "AVL_FindPrevNodeByAddress", NULL},
    {0x372, "AVL_FindPrevNodeByKey", NULL},
    {0x378, "AVL_FindNextNodeByAddress", NULL},
    {0x37e, "AVL_FindNextNodeByKey", NULL},
    {0x384, "AVL_FindFirstNode", NULL},
    {0x38a, "AVL_FindLastNode", NULL},
    {0, NULL, NULL}
};
