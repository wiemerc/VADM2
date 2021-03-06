/*
 * some constants
 */
.set AbsExecBase, 4
.set OpenLibrary, -552 
.set CloseLibrary, -414
.set PutStr, -948


.text
    /* open DOS library */
    movea.l     AbsExecBase, a6
    movea.l     #libname, a1
    moveq.l     #0, d0
    jsr         OpenLibrary(a6)
    tst.l       d0
    beq.w       error_no_dos
    move.l      d0, DOSBase

    /* print text 3 times */
    movea.l     DOSBase, a6
    moveq.l     #3, d2                  /* loop counter */
loop_start:
    move.l      #msg, d1                /* text address, we can't use lea because it only works with address registers */
    jsr         PutStr(a6)
    subq.l      #1, d2                  /* decrement loop counter */
    bne.s       loop_start

normal_exit:
    /* close DOS library */
    movea.l     AbsExecBase, a6
    movea.l     DOSBase, a1
    jsr         CloseLibrary(a6)
    moveq.l     #0, d0                  /* exit code */
    rts

error_no_dos:
    moveq.l     #1, d0                  /* exit code */
    rts


.data
    .comm DOSBase, 4

    libname:    .asciz "dos.library"
    msg:        .asciz "Only Amiga made it possible\n"
