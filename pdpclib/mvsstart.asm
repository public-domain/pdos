**********************************************************************
*                                                                    *
*  THIS PROGRAM WRITTEN BY PAUL EDWARDS.                             *
*  RELEASED TO THE PUBLIC DOMAIN                                     *
*                                                                    *
**********************************************************************
**********************************************************************
*                                                                    *
*  MVSSTART - startup routines for MVS.                              *
*  It is currently coded to work with GCC. To activate the C/370     *
*  version change the "&COMP" switch.                                *
*                                                                    *
**********************************************************************
         LCLC &COMP               Declare compiler switch
&COMP    SETC 'GCC'               Indicate that this is for GCC
* &COMP    SETC 'C370'            Indicate that this is for C/370
         AIF ('&SYSPARM' EQ 'IFOX00').NOMODE
         AMODE ANY
         RMODE ANY
.NOMODE ANOP
         PRINT NOGEN
* YREGS IS NOT AVAILABLE WITH IFOX
*         YREGS
R0       EQU   0
R1       EQU   1
R2       EQU   2
R3       EQU   3
R4       EQU   4
R5       EQU   5
R6       EQU   6
R7       EQU   7
R8       EQU   8
R9       EQU   9
R10      EQU   10
R11      EQU   11
R12      EQU   12
R13      EQU   13
R14      EQU   14
R15      EQU   15
SUBPOOL  EQU   0
         CSECT
         ENTRY @@CRT0
@@CRT0   EQU   *
         AIF ('&COMP' NE 'C370').NOCEES
         ENTRY CEESTART 
CEESTART EQU   *
.NOCEES  ANOP
         SAVE  (14,12),,@@CRT0
         LR    R10,R15
         USING @@CRT0,R10
         LR    R11,R1
         GETMAIN RU,LV=STACKLEN,SP=SUBPOOL
         ST    R13,4(R1)
         ST    R1,8(R13)
         LR    R13,R1
         LR    R1,R11
         USING STACK,R13
*
* SAVE STACK POINTER FOR SETJMP/LONGJMP          
         EXTRN @@MANSTK                             
         L     R3,=V(@@MANSTK)                      
         ST    R13,0(R3)                            
         L     R2,=A(STACKLEN)                      
         ST    R2,4(R3)                             
*
         LA    R2,0
         ST    R2,DUMMYPTR       WHO KNOWS WHAT THIS IS USED FOR
         LA    R2,MAINSTK
         ST    R2,THEIRSTK       NEXT AVAILABLE SPOT IN STACK
         LA    R12,ANCHOR
         ST    R14,EXITADDR
         L     R3,=A(MAINLEN)
         AR    R2,R3
         ST    R2,12(R12)        TOP OF STACK POINTER
         LA    R2,0
         ST    R2,116(R12)       ADDR OF MEMORY ALLOCATION ROUTINE
         ST    R2,ARGPTR
*
         USING PSA,R0
         L     R2,PSATOLD
         USING TCB,R2
         L     R7,TCBRBP
         USING RBBASIC,R7
         LA    R8,0
         ICM   R8,B'0111',RBCDE1
         USING CDENTRY,R8
         MVC   PGMNAME,CDNAME
*
         L     R2,TCBJSCB
         USING IEZJSCB,R2
         LH    R2,JSCBTJID
         ST    R2,TYPE
         L     R2,0(R1)
         LA    R2,0(R2)
         ST    R2,ARGPTR
         LA    R2,PGMNAME
         ST    R2,PGMNPTR
*
* FOR GCC WE NEED TO BE ABLE TO RESTORE R13
         L     R5,SAVEAREA+4
         ST    R5,SAVER13
*
         LA    R1,PARMLIST
         CALL  @@START
*
RETURNMS DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         LR    R14,R15
         FREEMAIN RU,LV=STACKLEN,A=(R1),SP=SUBPOOL
         LR    R15,R14
         RETURN (14,12),RC=(15)
SAVER13  DS    F
         LTORG
*         ENTRY CEESG003
*CEESG003 EQU   *
         ENTRY @@EXITA
@@EXITA  EQU   *
* THIS WAS THE C/370 CODE BUT I DON'T KNOW WHETHER
* THE GCC REPLACEMENT CODE IS GOOD ENOUGH.
*         L     R14,0(R12)
*         L     R15,0(R1)
*         BR    R14
* FOR GCC, WE HAVE TO USE OUR SAVED R13
         DROP  R10
         USING @@EXITA,R15
         L     R13,=A(SAVER13)
         L     R13,0(R13)
         L     R15,0(R1)
         RETURN (14,12),RC=(15)
         LTORG
*
         CVT   DSECT=YES
         IKJTCB
         IEZJSCB
         IHAPSA
         IHARB
         IHACDE
STACK    DSECT
SAVEAREA DS    18F
DUMMYPTR DS    F
THEIRSTK DS    F
PARMLIST DS    0F
ARGPTR   DS    F
PGMNPTR  DS    F
TYPE     DS    F
PGMNAME  DS    CL8
PGMNAMEN DS    C                 NUL BYTE FOR C
ANCHOR   DS    0F
EXITADDR DS    F
         DS    49F
MAINSTK  DS    32000F
MAINLEN  EQU   *-MAINSTK
STACKLEN EQU   *-STACK
         END
