//
// loader.c - part of the Virtual AmigaDOS Machine (VADM)
//            This file contains the loader for executables in Amiga Hunk format.
//
// Copyright(C) 2019, 2020 Constantin Wiemer
// 


#include "vadm.h"


int main(int argc, char **argv)
{
    void *hunks[MAX_HUNKS];

    load_program(argv[1], hunks);
    return 0;
}
