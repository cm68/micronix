/*
 * includes
 *
 * /usr/src/cmd/asz/asm.h
 *
 * Changed: <2023-08-02 08:56:38 curt>
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
#ifndef ASM_H
#define ASM_H

extern FILE *input_file;
extern int line_num;
extern char *infile;
extern char verbose;
extern char g_flag;

/* interface functions */

void appendtmp();
void asm_reset();
void assemble();
char peek();
char get_next();
void outbyte();
void outtmp();

#define T_B     0
#define T_C     1
#define T_D     2
#define T_E     3
#define T_H     4
#define T_L     5
#define T_HL_I  6
#define T_A     7

#define T_BC    8
#define T_DE    9
#define T_HL    10
#define T_SP    11
#define T_AF    12
#define T_IX    21
#define T_IY    22

#define T_NZ    13
#define T_Z     14
#define T_NC    15
#define T_CR    16
#define T_PO    17
#define T_PE    18
#define T_P     19
#define T_M     20

#define T_IXH   23
#define T_IXL   24
#define T_IX_DISP 25
#define T_IYH   26
#define T_IYL   27
#define T_IY_DISP 28

#define T_PLAIN 31
#define T_INDIR 32

#define T_SP_I  34
#define T_BC_I  35
#define T_DE_I  36
#define T_IX_I  29
#define T_IY_I  30

#define T_C_I   33
#define T_I     37
#define T_R     38

#define T_NAME  39
#define T_NUM   40
#define T_STR   41
#define T_EOL   42
#define T_EOF   43

#endif
