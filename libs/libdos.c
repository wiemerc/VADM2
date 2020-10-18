//
// execute.c - part of the Virtual AmigaDOS Machine (VADM)
//             contains the implemented routines of the DOS library
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include <string.h>
#include <unistd.h>

#include "../execute.h"
#include "../vadm.h"


int32_t dos_put_str(const char *p_str)
{
    write(1, ">>> ", 4);
    write(1, p_str, strlen(p_str));
    return 0;
}


// lines below have been generated with the following command:
// grep libcall /opt/m68k-amigaos//m68k-amigaos/ndk/include/pragmas/dos_pragmas.h | perl -nale 'print "    {0x$F[4], \"$F[3]\", NULL},"'
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
FuncInfo g_func_info_tbl[] = {
    {0x1e, "Open", "2102", NULL},
    {0x24, "Close", "101", NULL},
    {0x2a, "Read", "32103", NULL},
    {0x30, "Write", "32103", NULL},
    {0x36, "Input", "00", NULL},
    {0x3c, "Output", "00", NULL},
    {0x42, "Seek", "32103", NULL},
    {0x48, "DeleteFile", "101", NULL},
    {0x4e, "Rename", "2102", NULL},
    {0x54, "Lock", "2102", NULL},
    {0x5a, "UnLock", "101", NULL},
    {0x60, "DupLock", "101", NULL},
    {0x66, "Examine", "2102", NULL},
    {0x6c, "ExNext", "2102", NULL},
    {0x72, "Info", "2102", NULL},
    {0x78, "CreateDir", "101", NULL},
    {0x7e, "CurrentDir", "101", NULL},
    {0x84, "IoErr", "00", NULL},
    {0x8a, "CreateProc", "432104", NULL},
    {0x90, "Exit", "101", NULL},
    {0x96, "LoadSeg", "101", NULL},
    {0x9c, "UnLoadSeg", "101", NULL},
    {0xae, "DeviceProc", "101", NULL},
    {0xb4, "SetComment", "2102", NULL},
    {0xba, "SetProtection", "2102", NULL},
    {0xc0, "DateStamp", "101", NULL},
    {0xc6, "Delay", "101", NULL},
    {0xcc, "WaitForChar", "2102", NULL},
    {0xd2, "ParentDir", "101", NULL},
    {0xd8, "IsInteractive", "101", NULL},
    {0xde, "Execute", "32103", NULL},
    {0xe4, "AllocDosObject", "2102", NULL},
    {0xe4, "AllocDosObjectTagList", "2102", NULL},
    {0xea, "FreeDosObject", "2102", NULL},
    {0xf0, "DoPkt", "765432107", NULL},
    {0xf0, "DoPkt0", "2102", NULL},
    {0xf0, "DoPkt1", "32103", NULL},
    {0xf0, "DoPkt2", "432104", NULL},
    {0xf0, "DoPkt3", "5432105", NULL},
    {0xf0, "DoPkt4", "65432106", NULL},
    {0xf6, "SendPkt", "32103", NULL},
    {0xfc, "WaitPkt", "00", NULL},
    {0x102, "ReplyPkt", "32103", NULL},
    {0x108, "AbortPkt", "2102", NULL},
    {0x10e, "LockRecord", "5432105", NULL},
    {0x114, "LockRecords", "2102", NULL},
    {0x11a, "UnLockRecord", "32103", NULL},
    {0x120, "UnLockRecords", "101", NULL},
    {0x126, "SelectInput", "101", NULL},
    {0x12c, "SelectOutput", "101", NULL},
    {0x132, "FGetC", "101", NULL},
    {0x138, "FPutC", "2102", NULL},
    {0x13e, "UnGetC", "2102", NULL},
    {0x144, "FRead", "432104", NULL},
    {0x14a, "FWrite", "432104", NULL},
    {0x150, "FGets", "32103", NULL},
    {0x156, "FPuts", "2102", NULL},
    {0x15c, "VFWritef", "32103", NULL},
    {0x162, "VFPrintf", "32103", NULL},
    {0x168, "Flush", "101", NULL},
    {0x16e, "SetVBuf", "432104", NULL},
    {0x174, "DupLockFromFH", "101", NULL},
    {0x17a, "OpenFromLock", "101", NULL},
    {0x180, "ParentOfFH", "101", NULL},
    {0x186, "ExamineFH", "2102", NULL},
    {0x18c, "SetFileDate", "2102", NULL},
    {0x192, "NameFromLock", "32103", NULL},
    {0x198, "NameFromFH", "32103", NULL},
    {0x19e, "SplitName", "5432105", NULL},
    {0x1a4, "SameLock", "2102", NULL},
    {0x1aa, "SetMode", "2102", NULL},
    {0x1b0, "ExAll", "5432105", NULL},
    {0x1b6, "ReadLink", "5432105", NULL},
    {0x1bc, "MakeLink", "32103", NULL},
    {0x1c2, "ChangeMode", "32103", NULL},
    {0x1c8, "SetFileSize", "32103", NULL},
    {0x1ce, "SetIoErr", "101", NULL},
    {0x1d4, "Fault", "432104", NULL},
    {0x1da, "PrintFault", "2102", NULL},
    {0x1e0, "ErrorReport", "432104", NULL},
    {0x1ec, "Cli", "00", NULL},
    {0x1f2, "CreateNewProc", "101", NULL},
    {0x1f2, "CreateNewProcTagList", "101", NULL},
    {0x1f8, "RunCommand", "432104", NULL},
    {0x1fe, "GetConsoleTask", "00", NULL},
    {0x204, "SetConsoleTask", "101", NULL},
    {0x20a, "GetFileSysTask", "00", NULL},
    {0x210, "SetFileSysTask", "101", NULL},
    {0x216, "GetArgStr", "00", NULL},
    {0x21c, "SetArgStr", "101", NULL},
    {0x222, "FindCliProc", "101", NULL},
    {0x228, "MaxCli", "00", NULL},
    {0x22e, "SetCurrentDirName", "101", NULL},
    {0x234, "GetCurrentDirName", "2102", NULL},
    {0x23a, "SetProgramName", "101", NULL},
    {0x240, "GetProgramName", "2102", NULL},
    {0x246, "SetPrompt", "101", NULL},
    {0x24c, "GetPrompt", "2102", NULL},
    {0x252, "SetProgramDir", "101", NULL},
    {0x258, "GetProgramDir", "00", NULL},
    {0x25e, "SystemTagList", "2102", NULL},
    {0x25e, "System", "2102", NULL},
    {0x264, "AssignLock", "2102", NULL},
    {0x26a, "AssignLate", "2102", NULL},
    {0x270, "AssignPath", "2102", NULL},
    {0x276, "AssignAdd", "2102", NULL},
    {0x27c, "RemAssignList", "2102", NULL},
    {0x282, "GetDeviceProc", "2102", NULL},
    {0x288, "FreeDeviceProc", "101", NULL},
    {0x28e, "LockDosList", "101", NULL},
    {0x294, "UnLockDosList", "101", NULL},
    {0x29a, "AttemptLockDosList", "101", NULL},
    {0x2a0, "RemDosEntry", "101", NULL},
    {0x2a6, "AddDosEntry", "101", NULL},
    {0x2ac, "FindDosEntry", "32103", NULL},
    {0x2b2, "NextDosEntry", "2102", NULL},
    {0x2b8, "MakeDosEntry", "2102", NULL},
    {0x2be, "FreeDosEntry", "101", NULL},
    {0x2c4, "IsFileSystem", "101", NULL},
    {0x2ca, "Format", "32103", NULL},
    {0x2d0, "Relabel", "2102", NULL},
    {0x2d6, "Inhibit", "2102", NULL},
    {0x2dc, "AddBuffers", "2102", NULL},
    {0x2e2, "CompareDates", "2102", NULL},
    {0x2e8, "DateToStr", "101", NULL},
    {0x2ee, "StrToDate", "101", NULL},
    {0x2f4, "InternalLoadSeg", "A98004", NULL},
    {0x2fa, "InternalUnLoadSeg", "9102", NULL},
    {0x300, "NewLoadSeg", "2102", NULL},
    {0x300, "NewLoadSegTagList", "2102", NULL},
    {0x306, "AddSegment", "32103", NULL},
    {0x30c, "FindSegment", "32103", NULL},
    {0x312, "RemSegment", "101", NULL},
    {0x318, "CheckSignal", "101", NULL},
    {0x31e, "ReadArgs", "32103", NULL},
    {0x324, "FindArg", "2102", NULL},
    {0x32a, "ReadItem", "32103", NULL},
    {0x330, "StrToLong", "2102", NULL},
    {0x336, "MatchFirst", "2102", NULL},
    {0x33c, "MatchNext", "101", NULL},
    {0x342, "MatchEnd", "101", NULL},
    {0x348, "ParsePattern", "32103", NULL},
    {0x34e, "MatchPattern", "2102", NULL},
    {0x35a, "FreeArgs", "101", NULL},
    {0x366, "FilePart", "101", NULL},
    {0x36c, "PathPart", "101", NULL},
    {0x372, "AddPart", "32103", NULL},
    {0x378, "StartNotify", "101", NULL},
    {0x37e, "EndNotify", "101", NULL},
    {0x384, "SetVar", "432104", NULL},
    {0x38a, "GetVar", "432104", NULL},
    {0x390, "DeleteVar", "2102", NULL},
    {0x396, "FindVar", "2102", NULL},
    {0x3a2, "CliInitNewcli", "801", NULL},
    {0x3a8, "CliInitRun", "801", NULL},
    {0x3ae, "WriteChars", "2102", NULL},
    {0x3b4, "PutStr", "101", dos_put_str},
    {0x3ba, "VPrintf", "2102", NULL},
    {0x3c6, "ParsePatternNoCase", "32103", NULL},
    {0x3cc, "MatchPatternNoCase", "2102", NULL},
    {0x3d8, "SameDevice", "2102", NULL},
    {0x3de, "ExAllEnd", "5432105", NULL},
    {0x3e4, "SetOwner", "2102", NULL},
    {0, NULL, NULL, NULL}
};
#pragma GCC diagnostic pop
