//
// vadm.h - part of the Virtual AmigaDOS Machine (VADM)
//          global header file for VADM
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 
#ifndef VADM_H_INCLUDED
#define VADM_H_INCLUDED

// base address of Exec library, the only absolute address in the AmigaOS. We can't use 4
// (the value in the AmigaOS) because the address has to be a page boundary as we create a
// memory mapping there, and mmap(2) on Linux doesn't allow a mapping in the first 64KB anyway.
#define ABS_EXEC_BASE 0x00300000

#endif
