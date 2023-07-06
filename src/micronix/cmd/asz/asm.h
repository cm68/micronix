/*
 * includes
 *
 * /usr/src/cmd/asz/asm.h
 *
 * Changed: <2023-07-05 23:02:56 curt>
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
#ifndef ASM_H
#define ASM_H

#ifndef linux
#define void
#endif

#define EXP_STACK_DEPTH 16
#define TOKEN_BUF_SIZE 19
#define SYMBOL_NAME_SIZE 9
#define RELOC_SIZE 8

/* special types */
struct tval {
	unsigned short value;
	unsigned char type;
};

struct toff {
	unsigned char off;
	unsigned char type;
};

struct symbol {
	unsigned char type;
	char name[SYMBOL_NAME_SIZE];
	unsigned short size;
	unsigned short value;
	struct symbol *parent;
	struct symbol *next;
};

struct local {
	unsigned char type;
	unsigned char label;
	unsigned short value;
	struct local *next;
};

struct reloc {
	struct toff toff[RELOC_SIZE];
	struct reloc *next;
};

struct global {
	struct symbol *symbol;
	struct global *next;
};

/* headers for reloc tables */
struct header {
	unsigned short last;
	unsigned char index;
	struct reloc *head;
	struct reloc *tail;
};

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

#endif
