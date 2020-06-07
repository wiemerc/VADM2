//
// execute.c - part of the Virtual AmigaDOS Machine (VADM)
//             contains the implemented routines of the Exec library
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include <bsd/string.h>

#include "../execute.h"  // for load_library()


#define MAX_PATH_LEN 256


#pragma GCC diagnostic ignored "-Wunused-parameter"
uint8_t *exec_open_library(const char *p_lib_name, uint32_t lib_version)
{
    char lib_path_name[MAX_PATH_LEN];

    // build actual path name from library name
    strlcpy(lib_path_name, "libs/lib", MAX_PATH_LEN);   // prefix with directory where the libraries live and 'lib'
    if (strlcat(lib_path_name, p_lib_name, MAX_PATH_LEN) > MAX_PATH_LEN) {
        // library name is too long
        return NULL;
    }
    strtok(lib_path_name, ".");                         // remove extension .library (overwrite . with NUL)
    strlcat(lib_path_name, ".so", MAX_PATH_LEN);        // add extension .so

    return load_library(lib_path_name);
}

void exec_close_library(uint8_t *p_lib_base)
{
    // TODO: call dlclose(), but how do we get the library handle?
}
#pragma GCC diagnostic pop


// lines below have been generated with the following command:
// grep syscall /opt/m68k-amigaos//m68k-amigaos/ndk/include/pragmas/exec_pragmas.h | perl -nale 'print "    {0x$F[3], \"$F[2]\", \"$F[4]\", NULL},"'
FuncInfo g_func_info_tbl[] = {
    {0x1e, "Supervisor", "D01", NULL},
    {0x48, "InitCode", "1002", NULL},
    {0x4e, "InitStruct", "0A903", NULL},
    {0x54, "MakeLibrary", "10A9805", NULL},
    {0x5a, "MakeFunctions", "A9803", NULL},
    {0x60, "FindResident", "901", NULL},
    {0x66, "InitResident", "1902", NULL},
    {0x6c, "Alert", "701", NULL},
    {0x72, "Debug", "001", NULL},
    {0x78, "Disable", "00", NULL},
    {0x7e, "Enable", "00", NULL},
    {0x84, "Forbid", "00", NULL},
    {0x8a, "Permit", "00", NULL},
    {0x90, "SetSR", "1002", NULL},
    {0x96, "SuperState", "00", NULL},
    {0x9c, "UserState", "001", NULL},
    {0xa2, "SetIntVector", "9002", NULL},
    {0xa8, "AddIntServer", "9002", NULL},
    {0xae, "RemIntServer", "9002", NULL},
    {0xb4, "Cause", "901", NULL},
    {0xba, "Allocate", "0802", NULL},
    {0xc0, "Deallocate", "09803", NULL},
    {0xc6, "AllocMem", "1002", NULL},
    {0xcc, "AllocAbs", "9002", NULL},
    {0xd2, "FreeMem", "0902", NULL},
    {0xd8, "AvailMem", "101", NULL},
    {0xde, "AllocEntry", "801", NULL},
    {0xe4, "FreeEntry", "801", NULL},
    {0xea, "Insert", "A9803", NULL},
    {0xf0, "AddHead", "9802", NULL},
    {0xf6, "AddTail", "9802", NULL},
    {0xfc, "Remove", "901", NULL},
    {0x102, "RemHead", "801", NULL},
    {0x108, "RemTail", "801", NULL},
    {0x10e, "Enqueue", "9802", NULL},
    {0x114, "FindName", "9802", NULL},
    {0x11a, "AddTask", "BA903", NULL},
    {0x120, "RemTask", "901", NULL},
    {0x126, "FindTask", "901", NULL},
    {0x12c, "SetTaskPri", "0902", NULL},
    {0x132, "SetSignal", "1002", NULL},
    {0x138, "SetExcept", "1002", NULL},
    {0x13e, "Wait", "001", NULL},
    {0x144, "Signal", "0902", NULL},
    {0x14a, "AllocSignal", "001", NULL},
    {0x150, "FreeSignal", "001", NULL},
    {0x156, "AllocTrap", "001", NULL},
    {0x15c, "FreeTrap", "001", NULL},
    {0x162, "AddPort", "901", NULL},
    {0x168, "RemPort", "901", NULL},
    {0x16e, "PutMsg", "9802", NULL},
    {0x174, "GetMsg", "801", NULL},
    {0x17a, "ReplyMsg", "901", NULL},
    {0x180, "WaitPort", "801", NULL},
    {0x186, "FindPort", "901", NULL},
    {0x18c, "AddLibrary", "901", NULL},
    {0x192, "RemLibrary", "901", NULL},
    {0x198, "OldOpenLibrary", "901", NULL},
    {0x19e, "CloseLibrary", "901", exec_close_library},
    {0x1a4, "SetFunction", "08903", NULL},
    {0x1aa, "SumLibrary", "901", NULL},
    {0x1b0, "AddDevice", "901", NULL},
    {0x1b6, "RemDevice", "901", NULL},
    {0x1bc, "OpenDevice", "190804", NULL},
    {0x1c2, "CloseDevice", "901", NULL},
    {0x1c8, "DoIO", "901", NULL},
    {0x1ce, "SendIO", "901", NULL},
    {0x1d4, "CheckIO", "901", NULL},
    {0x1da, "WaitIO", "901", NULL},
    {0x1e0, "AbortIO", "901", NULL},
    {0x1e6, "AddResource", "901", NULL},
    {0x1ec, "RemResource", "901", NULL},
    {0x1f2, "OpenResource", "901", NULL},
    {0x20a, "RawDoFmt", "BA9804", NULL},
    {0x210, "GetCC", "00", NULL},
    {0x216, "TypeOfMem", "901", NULL},
    {0x21c, "Procure", "9802", NULL},
    {0x222, "Vacate", "9802", NULL},
    {0x228, "OpenLibrary", "0902", exec_open_library},
    {0x22e, "InitSemaphore", "801", NULL},
    {0x234, "ObtainSemaphore", "801", NULL},
    {0x23a, "ReleaseSemaphore", "801", NULL},
    {0x240, "AttemptSemaphore", "801", NULL},
    {0x246, "ObtainSemaphoreList", "801", NULL},
    {0x24c, "ReleaseSemaphoreList", "801", NULL},
    {0x252, "FindSemaphore", "901", NULL},
    {0x258, "AddSemaphore", "901", NULL},
    {0x25e, "RemSemaphore", "901", NULL},
    {0x264, "SumKickData", "00", NULL},
    {0x26a, "AddMemList", "9821005", NULL},
    {0x270, "CopyMem", "09803", NULL},
    {0x276, "CopyMemQuick", "09803", NULL},
    {0x27c, "CacheClearU", "00", NULL},
    {0x282, "CacheClearE", "10803", NULL},
    {0x288, "CacheControl", "1002", NULL},
    {0x28e, "CreateIORequest", "0802", NULL},
    {0x294, "DeleteIORequest", "801", NULL},
    {0x29a, "CreateMsgPort", "00", NULL},
    {0x2a0, "DeleteMsgPort", "801", NULL},
    {0x2a6, "ObtainSemaphoreShared", "801", NULL},
    {0x2ac, "AllocVec", "1002", NULL},
    {0x2b2, "FreeVec", "901", NULL},
    {0x2b8, "CreatePool", "21003", NULL},
    {0x2be, "DeletePool", "801", NULL},
    {0x2c4, "AllocPooled", "0802", NULL},
    {0x2ca, "FreePooled", "09803", NULL},
    {0x2d0, "AttemptSemaphoreShared", "801", NULL},
    {0x2d6, "ColdReboot", "00", NULL},
    {0x2dc, "StackSwap", "801", NULL},
    {0x2fa, "CachePreDMA", "09803", NULL},
    {0x300, "CachePostDMA", "09803", NULL},
    {0x306, "AddMemHandler", "901", NULL},
    {0x30c, "RemMemHandler", "901", NULL},
    {0x312, "ObtainQuickVector", "801", NULL},
    {0x33c, "NewMinList", "801", NULL},
    {0x354, "AVL_AddNode", "A9803", NULL},
    {0x35a, "AVL_RemNodeByAddress", "9802", NULL},
    {0x360, "AVL_RemNodeByKey", "A9803", NULL},
    {0x366, "AVL_FindNode", "A9803", NULL},
    {0x36c, "AVL_FindPrevNodeByAddress", "801", NULL},
    {0x372, "AVL_FindPrevNodeByKey", "A9803", NULL},
    {0x378, "AVL_FindNextNodeByAddress", "801", NULL},
    {0x37e, "AVL_FindNextNodeByKey", "A9803", NULL},
    {0x384, "AVL_FindFirstNode", "801", NULL},
    {0x38a, "AVL_FindLastNode", "801", NULL},
    {0, NULL, NULL, NULL}
};
