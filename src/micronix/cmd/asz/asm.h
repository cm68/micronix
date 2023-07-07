/*
 * includes
 *
 * /usr/src/cmd/asz/asm.h
 *
 * Changed: <2023-07-06 15:09:48 curt>
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

#define T_EOF   -1
#define T_NAME  'a'
#define T_NUM   '0'
#define T_NL    'n'

#endif
