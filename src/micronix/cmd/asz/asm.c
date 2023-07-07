/*
 * z80 assembler guts
 *
 * substantially rewritten to remove stuff not needed for a compiler backend
 * or an assembler that is used in conjunction with a preprocessor
 * things removed:  the type machinery, and the odd defl, def syntax
 *
 * /usr/src/cmd/asz/asm.c 
 *
 * Changed: <2023-07-06 17:56:36 curt>
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
#include <stdio.h>
#ifdef linux
#include <stdlib.h>
#define INIT
#else
#define INIT    = 0
#endif

/*
 * verbosity levels:
 * 1 = file 
 * 2 = pass progress
 * 3 = instructions
 * 4 = allocations/symbols/relocs
 * 5 - tokens
 */

#include "asm.h"
#include "isr.h"

#define EXP_STACK_DEPTH 16
#define TOKEN_BUF_SIZE 19
#define SYMBOL_NAME_SIZE 9

/*
 * special types 
 */
struct tval {
	unsigned short value;
	unsigned short type;
};

struct symbol {
	unsigned short type;
	char name[SYMBOL_NAME_SIZE];
	unsigned short size;
	unsigned short value;
	struct symbol *next;
};

struct local {
	unsigned short type;
	unsigned char label;
	unsigned short value;
	struct local *next;
};

struct global {
	struct symbol *symbol;
	struct global *next;
};

/*
 * symbols have a segment, emitted code does too.
 */
#define SEG_UNDEF   0
#define SEG_TEXT    1
#define SEG_DATA    2
#define SEG_BSS     3
#define SEG_ABS     4
#define SEG_EXT     5

/*
 * token buffer 
 */
char token_buf[TOKEN_BUF_SIZE] INIT;
char sym_name[TOKEN_BUF_SIZE] INIT;

/*
 * current assembly address 
 */
unsigned short cur_address INIT;

/*
 * segment tops 
 */
unsigned short text_top INIT;
unsigned short data_top INIT;
unsigned short bss_top INIT;

/*
 * sizes for header
 */
unsigned short text_size INIT;
unsigned short mem_size INIT;
unsigned short data_size INIT;
unsigned short bss_size INIT;

char pass INIT;

char segment INIT;

/*
 * the expression evaluator requires some larger data structures, lets
 * define them 
 */

struct tval exp_vstack[EXP_STACK_DEPTH] INIT;

char exp_estack[EXP_STACK_DEPTH] INIT;

struct symbol *sym_table INIT;

struct local *loc_table INIT;

int loc_cnt INIT;

struct global *glob_table INIT;

/*
 * relocs are chained off of headers and need to stay
 * ordered.
 */
struct reloc {
	unsigned short off;
	unsigned short type;
	struct reloc *next;
};

struct header {
	unsigned short last;
	unsigned char index;
	struct reloc *head;
	struct reloc *tail;
};

struct header textr INIT;
struct header datar INIT;

int glob_count INIT;

/*
 * extern number 
 */
unsigned short extn INIT;

struct symbol **sort INIT;

/*
 * checks if a string is equal
 * string a is read as lower case
 *
 * a = pointer to string a
 * b = pointer to string b
 */
char
asm_sequ(a, b)
char *a;
char *b;
{
	char lower;

	while (*b) {
		lower = *a;
		if (lower >= 'A' && lower <= 'Z')
			lower += 'a' - 'A';

		if (*a != *b)
			return 0;

		a++;
		b++;
	}

	return *a == *b;
}

/*
 * prints out an error message and exits
 *
 * msg = error message
 */
void
asm_error(msg)
char *msg;
{
	printf("%s:%d %s\n", infile, line_num, msg);
	exit(1);
}

/*
 * moves the contents of token_buf into token_cache
 */
void
asm_token_cache(token_cache)
char *token_cache;
{
	int i;

	for (i = 0; i < TOKEN_BUF_SIZE; i++)
		token_cache[i] = token_buf[i];
}

/*
 * skips past all of the white space to a token
 */
void
skipwhite()
{
	char comment;

	comment = 0;
	while ((peek() <= ' ' || peek() == ';' || comment)
		   && peek() != '\n' && peek() != -1)
		if (get_next() == ';')
			comment = 1;
}

/*
 * tests if a character is a alpha (aA - zZ or underscore)
 *
 * in = character to test
 * returns true (1) or false (0)
 */
char
asm_alpha(in)
char in;
{
	return (in >= 'A' && in <= 'Z') || (in >= 'a' && in <= 'z')
		|| in == '_';
}

/*
 * tests if a character is a alpha
 *
 * in = character to test
 * returns true (1) or false (0)
 */
char
asm_num(in)
char in;
{
	return (in >= '0' && in <= '9');
}

/*
 * reads the next token in from the source, buffers if needed, and returns type
 * white space will by cycled past, both in front and behind the token
 */
char
token_read()
{
	char c, out;
	int i;

	/*
	 * skip all leading white space 
	 */
	skipwhite();

	/*
	 * peek and check type 
	 */
	out = c = peek();
	if (asm_alpha(c))
		out = T_NAME;
	else if (asm_num(c))
		out = T_NUM;

	if (out == T_NAME || out == T_NUM) {
		/*
		 * scan in the buffer if needed 
		 */
		i = 0;
		while (asm_num(c) || asm_alpha(c)) {
			if (i < TOKEN_BUF_SIZE - 1)
				token_buf[i++] = c;

			get_next();
			c = peek();
		}
		token_buf[i] = 0;
	} else {
		get_next();
	}

	/*
	 * correct for new lines 
	 */
	if (out == '\n')
		out = T_NL;

	/*
	 * skip more whitespace 
	 */
	skipwhite();

	return out;
}

/*
 * expects a specific symbol
 * some symbols have special cases for ignoring trailing or leading line breaks
 *
 * c = symbol to expect
 */
void
asm_expect(c)
char c;
{
	char tok;

	if (c == '}') {
		while (peek() == '\n')
			token_read();
	}

	tok = token_read();

	if (tok != c) {
		asm_error("unexpected character");
	}

	if (c == '{' || c == ',') {
		while (peek() == '\n')
			token_read();
	}
}

/*
 * consumes an end of line
 */
void
asm_eol()
{
	char tok;

	tok = token_read();
	if (tok != T_NL && tok != T_EOF)
		asm_error("expected end of line");
}

/*
 * skips to and consumes an end of line
 */
void
asm_skip()
{
	char tok;

	do {
		tok = token_read();
	} while (tok != T_NL && tok != T_EOF);
}

/*
 * helper function for number parsing, returns radix type from character
 *
 * r = radix identifier
 * returns radix type, or 0 if not a radix
 */
char
asm_classify_radix(r)
char r;
{
	if (r == 'b' || r == 'B')
		return 2;
	if (r == 'o' || r == 'O')
		return 8;
	if (r == 'x' || r == 'X' || r == 'h' || r == 'H')
		return 16;
	return 0;
}

/*
 * another helper function, this time to take a decimal / hex char and convert it
 *
 * in = char to convert to number
 * returns number, of -1 if failed
 */
int
hexparse(in)
char in;
{
	if (in >= '0' && in <= '9')
		return in - '0';
	else if (in >= 'A' && in <= 'F')
		return (in - 'A') + 10;
	else if (in >= 'a' && in <= 'f')
		return (in - 'a') + 10;
	return -1;
}

/*
 * attempts to parse a number into an unsigned 16 bit integer
 *
 * in = pointer to string
 * returns actual value of number
 * 0 = ?, 2 = binary, 8 = octal, 10 = decimal, 16 = hex
 */
unsigned short
num_parse(in)
char *in;
{
	int num_start, num_end, i;
	unsigned short out;
	char radix;

	radix = 10;

	/*
	 * first skip through any leading zeros, and set octal maybe 
	 */
	for (num_start = 0; in[num_start] == '0'; num_start++)
		radix = 8;

	/*
	 * lets also find the end while we are at it 
	 */
	for (num_end = 0; in[num_end] != 0; num_end++);

	/*
	 * check and see if there is a radix identifier here 
	 */
	if ((i = asm_classify_radix(in[num_start]))) {
		radix = i;
		num_start++;
	} else {
		/*
		 * lets check at the end too 
		 */
		if ((i = asm_classify_radix(in[num_end - 1]))) {
			radix = i;
			num_end--;
		}
	}

	/*
	 * now to parse 
	 */
	out = 0;
	for (; num_start < num_end; num_start++) {
		i = hexparse(in[num_start]);

		/*
		 * error checking 
		 */
		if (i == -1)
			asm_error("unexpected character in numeric");
		if (i >= radix)
			asm_error("radix mismatch in numeric");

		out = (out * radix) + i;
	}

	return out;
}

/*
 * fetches the symbol
 *
 * parent = parent structure to search
 * sym = pointer to symbol name
 * returns pointer to found symbol, or null
 */
struct symbol *
sym_fetch(sym)
char *sym;
{
	struct symbol *entry;
	int i;
	char equal;

	for (entry = sym_table; entry; entry = entry->next) {

		equal = 1;
		for (i = 0; i < SYMBOL_NAME_SIZE; i++) {
			if (entry->name[i] != sym[i])
				equal = 0;
			if (!entry->name[i])
				break;
		}
		if (equal)
			return entry;
	}
	return NULL;
}

/*
 * defines or redefines a symbol
 *
 * sym = symbol name
 * type = symbol type (0 = undefined, 1 = text, 2 = data, 3 = bss, 4 = absolute, 5+ = external)
 * value = value of symbol
 */
struct symbol *
sym_update(sym, type, value)
char *sym;
short type;
unsigned short value;
{
	struct symbol *entry;
	int i;

	entry = sym_fetch(sym);

	if (!entry) {
		entry = (struct symbol *) malloc(sizeof(struct symbol));
		entry->next = sym_table;
		entry->size = 0;
		sym_table = entry;
		for (i = 0; i < SYMBOL_NAME_SIZE - 1 && sym[i] != 0; i++)
			entry->name[i] = sym[i];
		entry->name[i] = 0;
	}

	/*
	 * update the symbol 
	 */
	entry->type = type;
	entry->value = value;

	return entry;
}

/*
 * resets all allocation stuff
 */
void
asm_reset()
{
	sym_table = NULL;
	loc_table = NULL;
	glob_table = NULL;

	/*
	 * sym_update(sym_table, "sys", 1, NULL, 0x0005);
	 * sym_update(sym_table, "header", 1, NULL, 0x0000); 
	 */
	textr.last = 0;
	textr.index = 0;
	textr.head = 0;
	textr.tail = 0;

	datar.last = 0;
	datar.index = 0;
	datar.head = 0;
	datar.tail = 0;

	glob_count = 0;

	/*
	 * externs start at 5 
	 */
	extn = 5;
}

/*
 * appends a local symbol to the local table
 *
 * label = label # (0-9)
 * type = symbol segment (text, data, or bss)
 * value = value of symbol
 */
void
local_add(label, type, value)
unsigned char label;
unsigned char type;
unsigned short value;
{
	struct local *new, *curr;

	/*
	 * alloc the new local symbol 
	 */
	new = (struct local *) malloc(sizeof(struct local));
	new->label = label;
	new->type = type;
	new->value = value;

	/*
	 * append to local table 
	 */
	if (loc_table) {
		curr = loc_table;
		while (curr->next)
			curr = curr->next;

		curr->next = new;
	} else
		loc_table = new;
}

/*
 * fetches a local symbol
 *
 * index = how many local indicies have been counted during pass
 * label = label # (0-9)
 * dir = direction (0 = backwards, 1 = forwards)
 */
char
local_fetch(result, index, label, dir)
unsigned short *result;
int index;
unsigned char label;
char dir;
{
	struct local *curr, *last;

	curr = loc_table;

	/*
	 * iterate through list 
	 */
	last = NULL;
	while (curr) {

		if (curr->label == label) {
			if (index) {
				last = curr;
			} else
				break;
		}

		if (index)
			index--;
		curr = curr->next;
	}

	if (dir)
		last = curr;

	*result = 0;
	if (last) {
		*result = last->value;
		return last->type;
	}
	return 0;
}

/*
 * adds a symbol to the global table
 *
 * sym = symbol to add
 */
void
add_glob(sym)
struct symbol *sym;
{
	struct global *curr, *new;

	glob_count++;

	new = (struct global *) malloc(sizeof(struct global));
	new->symbol = sym;
	new->next = NULL;

	if (verbose > 3)
		printf("add_glob: %s\n", sym->name);

	curr = NULL;
	if (glob_table) {
		curr = glob_table;

		while (1) {
			/*
			 * if the symbol already is glob, just ignore it 
			 */
			if (curr->symbol == sym)
				return;

			if (curr->next) {
				curr = curr->next;
			} else {
				curr->next = new;
				return;
			}
		}
	} else {
		glob_table = new;
	}
}

/*
 * adds an address into a relocation table, extending it if needed
 *
 * tab = reloc table
 * target = address to add
 */
void
add_reloc(tab, addr, type)
struct header *tab;
unsigned short addr;
unsigned char type;
{
	unsigned short diff;
	unsigned char i, next;
	struct reloc *r;

	if (pass)
		return;

	if (addr < tab->last)
		asm_error("backwards reloc");

	r = (struct reloc *) malloc(sizeof(struct reloc *));

	r->off = addr - tab->last;
	r->next = 0;
	r->type = type;

	if (!tab->head) {
		tab->tail = tab->head = r;
	} else {
		tab->tail->next = r;
	}
	tab->tail = r;
	tab->last = addr;
}

/*
 * outputs a relocation table to whitesmith's object
 *
 * tab = relocation table
 */
void
reloc_out(r, base)
struct reloc *r;
unsigned short base;
{
	int last = base;
	int bump;
	int control;
	int s;

	while (r) {
		s = r->type;
		if (verbose > 3) {
			printf("reloc: base: %x off: %x(%x) type: %x %s\n",
				   base, r->off, base + r->off,
				   s, s < 5 ? "" : sort[s - 5]->name);
		}

		base += r->off;

		bump = base - last;
		if (verbose > 4) {
			printf("bump: %d\n", bump);
		}
		while (bump >= 8223) {
			outbyte(0x3f);
			outbyte(0xff);
			bump -= 8223;
		}
		if (bump >= 32) {
			bump -= 32;
			outbyte((bump >> 8) + 32);
			outbyte(bump & 0xff);
		} else if (bump) {
			outbyte(bump);
		}
		switch (r->type) {
		case SEG_UNDEF:
			printf("reloc for undef\n");
			break;
		case SEG_ABS:
			outbyte(0x40);
			break;
		case SEG_TEXT:
			outbyte(0x44);
			break;
		case SEG_DATA:
			outbyte(0x48);
			break;
		case SEG_BSS:
			outbyte(0x4c);
			break;
		default:
			control = (r->type - 5) + 4;
			if (control < 47) {
				outbyte((control + 16) << 2);
			} else if (control < 175) {
				outbyte(0xfc);
				outbyte(control - 47);
			} else {
				control -= 175;
				outbyte(0xfc);
				outbyte((control >> 8) + 0x80);
				outbyte(control);
			}
			break;
		}
		last = base + 2;
		r = r->next;
	}
	outbyte(0);
}

/*
 * converts an escaped char into its value
 *
 * c = char to escape
 * returns escaped value
 */
char
asm_escape_char(c)
char c;
{
	switch (c) {
	case 'a':
		return 0x07;

	case 'b':
		return 0x08;

	case 'e':
		return 0x1B;

	case 'r':
		return 0x0D;

	case 'f':
		return 0x0C;

	case 'n':
		return 0x0A;

	case 't':
		return 0x09;

	case 'v':
		return 0x0B;

	case '\\':
		return 0x5C;

	case '\'':
		return 0x27;

	case '\"':
		return 0x22;

	case '\?':
		return 0x3F;

	default:
		return 0;
	}
}

/*
 * pops a value off the estack and evaluates it in the vstack
 *
 * eindex = pointer to expression index
 * vindex = pointer to value index
 */
void
est_pop(eindex, vindex)
int *eindex;
int *vindex;
{
	unsigned short a, b, res;
	unsigned char at, bt, ot;
	char op;

	if (!(*eindex))
		asm_error("expression stack depletion");

	/*
	 * pop off estack 
	 */
	op = exp_estack[--*eindex];

	/*
	 * attempt to pop out two values from the value stack 
	 */
	if (*vindex < 2)
		asm_error("value stack depletion");

	/*
	 * grab values off the stack 
	 */
	b = exp_vstack[--*vindex].value;
	bt = exp_vstack[*vindex].type;
	a = exp_vstack[--*vindex].value;
	at = exp_vstack[*vindex].type;

	switch (op) {
	case '!':
		res = a | ~b;
		break;

	case '+':
		res = a + b;
		break;

	case '-':
		res = a - b;
		break;

	case '*':
		res = a * b;
		break;

	case '/':
		if (b == 0) {
			if (pass == 0)
				res = 0;
			else
				asm_error("zero divide");
		} else
			res = a / b;
		break;

	case '%':
		res = a % b;
		break;

	case '>':
		res = a >> b;
		break;

	case '<':
		res = a << b;
		break;

	case '&':
		res = a & b;
		break;

	case '^':
		res = a ^ b;
		break;

	case '|':
		res = a | b;
		break;

	case '(':
		asm_error("unexpected '('");

	default:
		res = 0;
		break;
	}

	/*
	 * calculate types 
	 */
	if (!at || !bt) {
		/*
		 * any operation with an undefined type will also be undefined 
		 */
		ot = 0;
	} else if (at != 4 && bt != 4) {
		/*
		 * operations between two non-absolute types are forbidden 
		 */
		asm_error("incompatable types");
	} else if (at == 4 && bt != 4) {
		/*
		 * a is absolute, b is not
		 * only addition is allowed here, ot becomes bt
		 */
		if (op != '+')
			asm_error("invalid type operation");
		ot = bt;
	} else if (at != 4 && bt == 4) {
		/*
		 * b is absolute, a is not
		 * either addition or subtraction is allowed here, ot becomes at
		 */
		if (op != '+' && op != '-')
			asm_error("invalid type operation");
		ot = at;
	} else {
		/*
		 * both are absolute 
		 */
		ot = 4;
	}

	/*
	 * push into stack 
	 */
	exp_vstack[*vindex].value = res;
	exp_vstack[(*vindex)++].type = ot;
}

/*
 * pushes a expression onto the estack
 *
 * eindex = pointer to expression idnex
 * op = expression to push
 */
void
est_push(eindex, op)
int *eindex;
char op;
{
	if (*eindex >= EXP_STACK_DEPTH)
		asm_error("expression stack overflow");
	exp_estack[(*eindex)++] = op;
}

/*
 * pushes a value onto the vstack
 *
 * vindex = pointer to value index
 * val = value to push
 */
void
vsta_push(vindex, type, value)
int *vindex;
unsigned char type;
unsigned short value;
{
	if (*vindex >= EXP_STACK_DEPTH)
		asm_error("value stack overflow");
	exp_vstack[*vindex].type = type;
	exp_vstack[(*vindex)++].value = value;
}

/*
 * returns the precedence of a specific token
 *
 * tok = token
 * returns precedence of token
 */
int
asm_precedence(tok)
char tok;
{
	switch (tok) {
	case '!':
		return 1;

	case '+':
	case '-':
		return 2;

	case '*':
	case '/':
	case '%':
		return 3;

	case '>':
	case '<':
		return 4;

	case '&':
		return 5;

	case '^':
		return 6;

	case '|':
		return 7;

	case '(':
		return 0;

	default:
		return 99;
	}
}

/*
 * checks to see if there is a left parenthesis in the expression stack
 *
 * size = size of stack
 * returns 1 if true, otherwise 0
 */
char
est_lpar(size)
int size;
{
	int i;

	for (i = 0; i < size; i++)
		if (exp_estack[i] == '(')
			return 1;

	return 0;
}

/*
 * evaluates an expression that is next in the token queue
 *
 * result = pointer where result will be placed in
 * tok = initial token, 0 if none
 * returns status 0 = unresolved, 1 = text, 2 = data, 3 = bss, 4 = absolute, 5+ = external types
 */
unsigned char
asm_evaluate(result, itok)
unsigned short *result;
char itok;
{
	char tok, op, type, dosz;
	unsigned short num;
	struct symbol *sym;
	int vindex, eindex;

	vindex = eindex = 0;

	while (1) {
		/*
		 * read token, or use inital token 
		 */
		if (itok) {
			tok = itok;
			itok = 0;
		} else
			tok = token_read();

		/*
		 * default is absolute 
		 */
		type = 4;

		/*
		 * it is a symbol 
		 */
		if (tok == T_NAME || tok == '$') {

			/*
			 * see if we are doing a size or value operation 
			 */
			dosz = 0;
			if (tok == '$') {
				dosz = 1;
				tok = token_read();
				if (tok != T_NAME)
					asm_error("unexpected token");
			}

			op = 0;
			sym = sym_fetch(token_buf);
			if (sym) {

				if (dosz) {
					/*
					 * all sizes are absolute 
					 */
					num = sym->size;
				} else {
					/*
					 * get type 
					 */
					type = sym->type;

					/*
					 * get value 
					 */
					num = sym->value;
				}
			} else {
				type = 0;
				num = 0;
			}
		} else if (tok == '0') {
			/*
			 * it is a numeric (maybe) 
			 */
			op = 0;

			if (asm_num(token_buf[0])
				&& (token_buf[1] == 'f' || token_buf[1] == 'b')
				&& token_buf[2] == 0) {
				/*
				 * nope, actually a local label 
				 */
				type =
					local_fetch(&num, loc_cnt,
								hexparse(token_buf[0]),
								token_buf[1] == 'f');
			} else {
				/*
				 * its a numeric (for realz) 
				 */
				num = num_parse(token_buf);
			}
		} else if (tok == '\'') {
			/*
			 * it is a char 
			 */
			op = 0;

			/*
			 * escape character 
			 */
			if (peek() == '\\') {
				get_next();
				num = asm_escape_char(get_next());

				if (!num)
					asm_error("unknown escape");
			} else {
				num = get_next();
			}

			if (token_read() != '\'')
				asm_error("expected \'");
		} else {
			/*
			 * it is a token (hopefully mathematic) 
			 */
			op = -1;

			if (tok == '+' || tok == '-' || tok == '*' || tok == '/' ||
				tok == '&' || tok == '|' || tok == '%' || tok == '!' ||
				tok == '^' || tok == '(' || tok == ')')
				op = tok;

			if (tok == '>' || tok == '<') {
				if (tok != peek())
					op = -1;
				else
					op = tok;

				token_read();
			}

			if (op == -1)
				asm_error("unknown token in expression");
		}

		/*
		 * now lets handle the token 
		 */
		if (op != ')' && op != '(' && op) {
			/*
			 * handle operators 
			 */

			/*
			 * pop off anything in the stack of higher precedence 
			 */
			while (eindex
				   && asm_precedence(op) <=
				   asm_precedence(exp_estack[eindex - 1]))
				est_pop(&eindex, &vindex);

			est_push(&eindex, op);
		} else if (op == '(') {
			/*
			 * handle left parenthesis 
			 */
			est_push(&eindex, '(');
		} else if (op == ')') {
			if (!est_lpar(eindex))
				asm_error("unexpected ')'");

			while (exp_estack[eindex - 1] != '(')
				est_pop(&eindex, &vindex);

			/*
			 * pop the '(' too 
			 */
			eindex--;
		} else {
			/*
			 * handle numbers 
			 */
			vsta_push(&vindex, type, num);
		}

		/*
		 * check for ending conditions 
		 */
		tok = peek();
		if (tok == ',' || tok == '\n' || tok == ']' || tok == '}'
			|| tok == -1)
			break;
		if (tok == ')' && !est_lpar(eindex))
			break;

	}

	while (eindex)
		est_pop(&eindex, &vindex);

	if (vindex != 1)
		asm_error("value stack overpopulation");

	*result = exp_vstack[0].value;

	/*
	 * return type 
	 */
	return exp_vstack[0].type;
}

/*
 * emits a byte into assembly output
 * no bytes emitted on first pass, only update addresses
 *
 * b = byte to emit
 */
void
asm_emit(b)
unsigned char b;
{
	if (pass == 1) {
		switch (segment) {
		case SEG_TEXT:
			outbyte((char) b);
			break;
		case SEG_DATA:
			outtmp((char) b);
			break;
		case SEG_BSS:
			if (b)
				asm_error("data in bss");
			break;
		default:
			break;
		}
	}

	cur_address++;
}

/*
 * emits a little endian word to the binary
 *
 * w = word to emit
 */
void
emitword(w)
unsigned short w;
{
	asm_emit(w & 0xFF);
	asm_emit(w >> 8);
}

void
outword(word)
unsigned short word;
{
	outbyte(word & 0xFF);
	outbyte(word >> 8);
}

/*
 * emits a string found in the char stream
 */
void
emit_str()
{
	char c, state;
	int radix, length;
	unsigned char decode, num;

	/*
	 * zero state, just accept raw characters 
	 */
	state = 0;

	get_next();
	while (1) {
		c = get_next();

		/*
		 * we are done (maybe) 
		 */
		if (c == -1)
			break;
		if (c == '"') {
			if (state != 1) {
				if (state == 3) {
					asm_emit(decode);
				}

				break;
			}
		}
		/*
		 * just emit the char outright 
		 */
		if (!state) {
			/*
			 * sets the state to 1 
			 */
			if (c == '\\')
				state = 1;
			else
				asm_emit(c);

		} else if (state == 1) {
			/*
			 * escape character 
			 */
			decode = asm_escape_char(c);

			/*
			 * simple escape 
			 */
			if (decode) {
				asm_emit(decode);
				state = 0;
			} else if (asm_num(c)) {
				state = 3;
				radix = 8;
				length = 3;
			} else if (c == 'x') {
				state = 2;
				radix = 16;
				length = 2;
			} else {
				asm_error("unknown escape");
			}
		}

		if (state == 3) {
			/*
			 * numeric parsing 
			 */
			num = hexparse(c);

			if (num == -1)
				asm_error("unexpected character in numeric");
			if (num >= radix)
				asm_error("radix mismatch in numeric");

			decode = (decode * radix) + num;

			num = asm_classify_radix(peek());
			length--;

			/*
			 * end the parsing 
			 */
			if (length < 1 || num == -1 || num >= radix) {
				state = 0;
				asm_emit(decode);
			}
		}
		/*
		 * this is to consume the 'x' identifier 
		 */
		if (state == 2)
			state = 3;
	}

	/*
	 * make sure we don't land in whitespace 
	 */
	skipwhite();
}

/*
 * fills a region with either zeros or undefined allocated space
 *
 * size = number of bytes to fill
 */
void
fill(size)
unsigned short size;
{
	if (verbose > 3)
		printf("fill segment: %d for %d\n", segment, size);
	while (size--)
		asm_emit(0);
}

/*
 * emits up to two bytes, and handels relocation tracking
 *
 * size = number of bytes to emit
 * value = value to emit
 * type = segment to emit into
 */
void
emit_addr(size, value, type)
unsigned short size;
unsigned short value;
unsigned char type;
{
	unsigned short rel;

	if (type == SEG_UNDEF) {
		/*
		 * if we are on the second pass, error out 
		 */
		if (pass == 1)
			asm_error("undefined symbol");

		value = 0;
	}

	if (!size)
		asm_error("not a type");

	if (size == 1) {
		/*
		 * here we output only a byte 
		 */
		if ((type >= SEG_EXT) && (pass == 1))
			asm_error("cannot extern byte");

		if ((type >= SEG_TEXT) && (type <= SEG_BSS)) {
			/*
			 * emit a relative address 
			 */
			rel = (value - cur_address) - 1;
			if ((rel < 0x80) || (rel > 0xFF7F))
				asm_emit(rel);
			else
				asm_error("relative out of bounds");
		} else {
			asm_emit(value);
		}

	} else {

		if (((type >= SEG_TEXT) && (type <= SEG_BSS)) ||
			((type > 4) && (pass == 1))) {

			switch (segment) {
			case SEG_TEXT:
				add_reloc(&textr, cur_address, type);
				break;

			case SEG_DATA:
				add_reloc(&datar, cur_address - text_top, type);
				break;

			default:
				asm_error("invalid segment");
			}
		}
		emitword(value);
	}
}

/*
 * helper function to emit an immediate and do type checking
 * only absolute resolutions will be allowed
 */
void
emit_imm(value, type)
unsigned short value;
unsigned char type;
{
	if (type != SEG_ABS && (pass == 1))
		asm_error("must be absolute");

	asm_emit(value);
}

/*
 * helper function to evaluate an expression and emit the results
 * will handle relocation tracking and pass related stuff
 *
 * size = maximum size of space
 */
void
emit_exp(size, tok)
unsigned short size;
char tok;
{
	unsigned short value;
	unsigned char type;

	type = asm_evaluate(&value, tok);

	emit_addr(size, value, type);
}

void
db()
{
	char tok;

	while (peek() != '\n' && peek() != -1) {
		tok = peek();
		if (tok == '"') {
			emit_str();
		} else {
			emit_exp(1, 0);
		}
		if (peek() != ',')
			break;
		else
			asm_expect(',');
	}
}

void
dw()
{
	char tok;

	while (peek() != '\n' && peek() != -1) {
		tok = peek();
		emit_exp(2, 0);
		if (peek() != ',')
			break;
		else
			asm_expect(',');
	}
}

void
ds()
{
	short value;
	char type;

	type = asm_evaluate(&value, 0);
	cur_address += value;
}

/*
 * parses an operand, extracting the type of operation and/or constant
 *
 * con = pointer to constant return
 * eval = automatically evaluate expressions into con
 *        it also converts 'c' from a register to a flag is eval is not on
 * returns type of operand
 */
unsigned char
asm_arg(con, eval)
unsigned short *con;
unsigned char eval;
{
	int i;
	char tok;
	unsigned char ret, type;

	/*
	 * check if there is anything next 
	 */
	if (peek() == '\n' || peek() == -1)
		return 255;

	/*
	 * assume at plain expression at first 
	 */
	ret = 31;

	/*
	 * read the token 
	 */
	tok = token_read();

	/*
	 * maybe a register symbol? 
	 */
	if (tok == 'a') {
		i = 0;
		while (op_table[i].type != 255) {
			if (asm_sequ(token_buf, op_table[i].mnem)) {
				if (!eval && op_table[i].type == 1)
					return 16;
				return op_table[i].type;
			}
			i++;
		}
	}
	/*
	 * maybe in parenthesis? 
	 */
	if (tok == '(') {
		tok = token_read();

		if (asm_sequ(token_buf, "hl")) {
			asm_expect(')');
			return 6;
		} else if (asm_sequ(token_buf, "c")) {
			asm_expect(')');
			return 33;
		} else if (asm_sequ(token_buf, "sp")) {
			asm_expect(')');
			return 34;
		} else if (asm_sequ(token_buf, "bc")) {
			asm_expect(')');
			return 35;
		} else if (asm_sequ(token_buf, "de")) {
			asm_expect(')');
			return 36;
		}

		else if (asm_sequ(token_buf, "ix")) {
			if (peek() == '+') {
				/*
				 * its got a constant 
				 */
				token_read();
				tok = 0;
				ret = 25;
			} else {
				asm_expect(')');
				return 29;
			}
		} else if (asm_sequ(token_buf, "iy")) {
			if (peek() == '+') {
				/*
				 * its got a constant 
				 */
				token_read();
				tok = 0;
				ret = 28;
			} else {
				asm_expect(')');
				return 30;
			}
		}
		/*
		 * evaluate as deferred expression 
		 */
		else {
			ret = 32;
		}
	}
	/*
	 * ok, its an expression 
	 */
	if (eval) {
		type = asm_evaluate(con, tok);
		if (type == 0) {
			*con = 0;
			if (pass == 1)
				asm_error("undefined symbol");
		} else if (type != 4)
			asm_error("must be absolute");

		/*
		 * if not 29, needs a trailing ')' 
		 */
		if (ret != 31)
			asm_expect(')');
	} else {
		/*
		 * hack to return tok so the caller can run asm_emit_exp 
		 */
		*con = tok;
	}
	return ret;
}

/*
 * load indirect
 */
int
do_stax()
{
	unsigned char prim, arg, reg, type;
	unsigned short con, value;

	type = asm_evaluate(&value, con);
	asm_expect(')');
	asm_expect(',');
	arg = asm_arg(&con, 1);

	switch (arg) {
	case 10:					/* ld (nn), hl */
		asm_emit(0x22);
		break;

	case 7:					/* ld (nn), a */
		asm_emit(0x32);
		break;

	case 21:					/* ld (nn), ix */
		asm_emit(0xDD);
		asm_emit(0x22);
		break;

	case 22:					/* ld (nn), iy */
		asm_emit(0xFD);
		asm_emit(0x22);
		break;

	case 8:					/* ld (nn), bc */
	case 9:					/* ld (nn), de */
	case 11:					/* ld (nn), sp */
		asm_emit(0xED);
		asm_emit(0x43 + ((arg - 8) << 4));
		break;

	default:
		return 1;
	}
	emit_addr(2, value, type);
	return 0;
}

/*
 * 16 bit load
 */
int
do_16i(arg)
char arg;
{
	unsigned char reg;
	unsigned short con;

	/*
	 * correct for ix,iy into hl 
	 */
	if (arg == 21) {
		asm_emit(0xDD);
		arg = 10;
	} else if (arg == 22) {
		asm_emit(0xFD);
		arg = 10;
	}
	/*
	 * grab a direct or deferred word 
	 */
	asm_expect(',');
	reg = asm_arg(&con, 0);

	if (reg == 31) {
		/*
		 * ld bc|de|hl|sp, nn 
		 */
		asm_emit(0x01 + ((arg - 8) << 4));
		emit_exp(2, con);
	} else if (reg == 32) {
		if (arg == 10) {
			/*
			 * ld hl, (nn) 
			 */
			asm_emit(0x2A);
		} else {
			/*
			 * ld bc|de|sp, (nn) 
			 */
			asm_emit(0xED);
			asm_emit(0x4B + ((arg - 8) << 4));
		}
		emit_exp(2, con);
		asm_expect(')');
	} else if (arg == 11) {
		/*
		 * ld sp,hl|ix|iy specials 
		 */
		switch (reg) {
		case 10:
			break;
		case 21:
			asm_emit(0xDD);
			break;
		case 22:
			asm_emit(0xFD);
			break;
		default:
			return 1;
		}
		asm_emit(0xF9);
	} else
		return 1;
	return 0;
}

int
do_ldr8(arg)
char arg;
{
	unsigned char prim, reg, type;
	unsigned short con, value;

	prim = 0;

	/*
	 * grab any constants if they exist 
	 */
	if (arg == 25 || arg == 28) {
		type = asm_evaluate(&value, con);
		asm_expect(')');
		prim++;
	}
	asm_expect(',');

	/*
	 * correct for carry flag 
	 */
	reg = asm_arg(&con, 0);
	if (reg == 16)
		reg = 1;

	/*
	 * i* class dest? 
	 */
	if (arg >= 23 && arg <= 28) {
		/*
		 * check for ix or iy, correct iy 
		 */
		if (arg <= 25) {
			/*
			 * ix 
			 */
			asm_emit(0xDD);
		} else {
			/*
			 * iy 
			 */
			asm_emit(0xFD);

			/*
			 * iy should now act like ix 
			 */
			if (reg >= 23 && reg <= 28) {
				if (reg < 26)
					return 1;
				reg = reg - 3;
			}
			arg = arg - 3;
		}

		/*
		 * check if arg is (ix+*) or not 
		 */
		if (arg == 25) {
			/*
			 * no (hl) 
			 */
			if (reg == 6)
				return 1;
		} else {
			/*
			 * no h-(hl) 
			 */
			if (reg >= 4 && reg <= 6)
				return 1;

			/*
			 * downconvert ix* 
			 */
			if (reg >= 23 && reg <= 25) {
				if (reg == 25)
					return 1;
				reg = reg - 19;
			}
		}

		arg = arg - 19;

	}
	/*
	 * i* class src? 
	 */
	else if (reg >= 23 && reg <= 28) {
		/*
		 * no (hl) 
		 */
		if (arg == 6)
			return 1;

		/*
		 * check for ix or iy, correct iy 
		 */
		if (reg <= 25) {
			/*
			 * ix 
			 */
			asm_emit(0xDD);
		} else {
			/*
			 * iy 
			 */
			asm_emit(0xFD);

			/*
			 * iy should now act like ix 
			 */
			if (reg >= 23 && reg <= 28) {
				if (reg < 26)
					return 1;
				reg = reg - 3;
			}
		}

		/*
		 * grab ix/iy offset
		 * h/l only allowed for ix+*
		 */
		if (reg == 25) {
			type = asm_evaluate(&value, con);
			asm_expect(')');
			prim++;
		} else if (arg == 4 || arg == 5)
			return 1;

		reg = reg - 19;
	}

	/*
	 * no (hl),(hl) 
	 */
	if (arg == 6 && reg == 6)
		return 1;

	if (arg < 8 && reg < 8) {
		/*
		 * reg8->reg8 
		 */
		asm_emit(0x40 + (arg << 3) + reg);
		if (prim)
			emit_imm(value, type);
	} else if (arg < 8 && reg == 31) {
		/*
		 *->reg8 */
		asm_emit(0x06 + (arg << 3));
		if (prim)
			emit_imm(value, type);
		type = asm_evaluate(&value, con);
		emit_imm(value, type);
	} else if (arg == 7) {
		/*
		 * special a loads 
		 */
		switch (reg) {
		case 35:
			asm_emit(0x0A);
			break;

		case 36:
			asm_emit(0x1A);
			break;

		case 32:
			asm_emit(0x3A);
			emit_exp(2, con);
			asm_expect(')');
			break;

		case 37:
			asm_emit(0xED);
			asm_emit(0x57);
			break;

		case 38:
			asm_emit(0xED);
			asm_emit(0x5F);
			break;

		default:
			return 1;
		}
	} else
		return 1;
	return 0;
}

/*
 * assembles an instructions
 * if/elses that would make yandev blush
 *
 * isr = pointer to instruct
 * returns 0 if successful
 */
char
asm_doisr(isr)
struct instruct *isr;
{
	unsigned char prim, arg, reg, type;
	unsigned short con, value;

	/*
	 * primary select to 0 
	 */
	prim = 0;
	if (isr->type == BASIC) {
		/*
		 * basic ops 
		 */
		asm_emit(isr->opcode);
		return 0;
	}

	if (isr->type == BASIC_EXT) {
		/*
		 * basic extended ops 
		 */
		asm_emit(isr->arg);
		asm_emit(isr->opcode);
		return 0;
	}

	if (isr->type == ARITH) {
		/*
		 * arithmetic operations 
		 */
		arg = asm_arg(&con, 1);

		/*
		 * detect type of operation 
		 */
		if (isr->arg == CARRY) {
			if (arg == 10) {
				/*
				 * hl adc/sbc 
				 */
				prim = 1;
			} else if (arg != 7)
				return 1;

			/*
			 * grab next arg 
			 */
			asm_expect(',');
			arg = asm_arg(&con, 1);
		} else if (isr->arg == ADD) {
			if (arg == 10) {
				/*
				 * hl add 
				 */
				prim = 2;

			} else if (arg == 21 || arg == 22) {
				/*
				 * ix/iy add 
				 */
				prim = 3;
				reg = arg;
			} else if (arg != 7)
				return 1;

			/*
			 * grab next arg 
			 */
			asm_expect(',');
			arg = asm_arg(&con, 1);

			/*
			 * no add i*,hl 
			 */
			if (prim == 3 && arg == 10)
				return 1;

			/*
			 * add i*,i* 
			 */
			if (prim == 3 && arg == reg)
				arg = 10;
		}

		if (prim == 0) {
			if (arg < 8) {
				/*
				 * basic from a-(hl) 
				 */
				asm_emit(isr->opcode + arg);
			} else if (arg >= 23 && arg <= 25) {
				/*
				 * ix class 
				 */
				asm_emit(0xDD);
				asm_emit(isr->opcode + (arg - 23) + 4);
				if (arg == 25)
					asm_emit(con & 0xFF);
			} else if (arg >= 26 && arg <= 28) {
				/*
				 * iy class 
				 */
				asm_emit(0xFD);
				asm_emit(isr->opcode + (arg - 26) + 4);
				if (arg == 28)
					asm_emit(con & 0xFF);
			} else if (arg == 31) {
				/*
				 * constant 
				 */
				asm_emit(isr->opcode + 0x46);
				asm_emit(con);
			} else
				return 1;
		} else if (prim == 1) {
			/*
			 * 16 bit carry ops bc-sp 
			 */
			if (arg >= 8 && arg <= 11) {
				asm_emit(0xED);
				asm_emit((0x42 + (isr->opcode == 0x88 ? 8 : 0)) +
						 ((arg - 8) << 4));
			} else
				return 1;
		} else if (prim == 2) {
			/*
			 * 16 bit add ops bc-sp 
			 */
			if (arg >= 8 && arg <= 11) {
				asm_emit(0x09 + ((arg - 8) << 4));
			} else
				return 1;
		} else if (prim == 3) {
			/*
			 * correct for hl -> ix,iy 
			 */
			if (arg == 10)
				arg = reg;
			if (arg == reg)
				arg = 10;

			/*
			 * pick ext block 
			 */
			if (reg == 21)
				asm_emit(0xDD);
			else
				asm_emit(0xFD);

			/*
			 * 16 bit add ops bc-sp 
			 */
			if (arg >= 8 && arg <= 11) {
				asm_emit(0x09 + ((arg - 8) << 4));
			} else
				return 1;
		}
		return 0;
	}

	if (isr->type == INCR) {
		arg = asm_arg(&con, 1);

		if (arg < 8) {
			/*
			 * basic from a-(hl) 
			 */
			asm_emit(isr->opcode + ((arg) << 3));
		} else if (arg < 12) {
			/*
			 * words bc-sp 
			 */
			asm_emit(isr->arg + ((arg - 8) << 4));
		} else if (arg == 21) {
			/*
			 * ix 
			 */
			asm_emit(0xDD);
			asm_emit(isr->arg + 0x20);
		} else if (arg == 22) {
			/*
			 * iy 
			 */
			asm_emit(0xFD);
			asm_emit(isr->arg + 0x20);
		} else if (arg >= 23 && arg <= 25) {
			/*
			 * ixh-(ix+*) 
			 */
			asm_emit(0xDD);
			asm_emit(isr->opcode + ((arg - 19) << 3));
			if (arg == 25)
				asm_emit(con);
		} else if (arg >= 26 && arg <= 28) {
			/*
			 * iyh-(iy+*) 
			 */
			asm_emit(0xFD);
			asm_emit(isr->opcode + ((arg - 22) << 3));
			if (arg == 28)
				asm_emit(con);
		} else
			return 1;
		return 0;
	}

	if (isr->type == BITSH) {
		arg = asm_arg(&con, 1);

		/*
		 * bit instructions have a bit indicator that must be parsed 
		 */
		reg = 0;
		if (isr->arg) {
			if (arg != 31)
				return 1;

			if (con > 7)
				return 1;

			reg = con;

			/*
			 * grab next 
			 */
			asm_expect(',');
			arg = asm_arg(&con, 1);
		}
		/*
		 * check for (ix+*) / (iy+*) 
		 */
		if (arg == 25 || arg == 28) {

			if (arg == 25)
				asm_emit(0xDD);
			else
				asm_emit(0xFD);

			asm_emit(0xCB);

			/*
			 * write offset 
			 */
			asm_emit(con);

			arg = 6;
			/*
			 * its an undefined operation 
			 */
			if (peek() == ',') {
				asm_expect(',');
				arg = asm_arg(&con, 1);

				/*
				 * short out for (hl) 
				 */
				if (arg == 6)
					arg = 8;
			}
		} else
			asm_emit(0xCB);

		if (arg > 7)
			return 1;

		asm_emit(isr->opcode + arg + (reg << 3));
		return 0;
	}

	if (isr->type == STACK) {
		arg = asm_arg(&con, 1);

		/*
		 * swap af for sp 
		 */
		if (arg == 11)
			arg = 12;
		else if (arg == 12)
			arg = 11;

		if (arg >= 8 && arg <= 11) {
			/*
			 * bc-af 
			 */
			asm_emit(isr->opcode + ((arg - 8) << 4));
		} else if (arg == 21) {
			asm_emit(0xDD);
			asm_emit(isr->opcode + 0x20);
		} else if (arg == 22) {
			asm_emit(0xFD);
			asm_emit(isr->opcode + 0x20);
		} else
			return 1;
		return 0;
	}

	if (isr->type == RETFLO) {
		/*
		 * return 
		 */
		arg = asm_arg(&con, 0);

		if (arg >= 13 && arg <= 20) {
			asm_emit(isr->opcode + ((arg - 13) << 3));
		} else if (arg == 255) {
			asm_emit(isr->arg);
		} else
			return 1;
		return 0;
	}

	if (isr->type == JMPFLO) {
		/*
		 * jump (absolute) 
		 */
		arg = asm_arg(&con, 0);

		if (arg >= 13 && arg <= 20) {
			asm_emit(isr->opcode + ((arg - 13) << 3));
			asm_expect(',');
			emit_exp(2, 0);
		} else if (arg == 31) {
			asm_emit(isr->opcode + 1);
			emit_exp(2, con);
		} else if (arg == 6) {
			asm_emit(isr->arg);
		} else if (arg == 29) {
			asm_emit(0xDD);
			asm_emit(isr->arg);
		} else if (arg == 30) {
			asm_emit(0xFD);
			asm_emit(isr->arg);
		} else
			return 1;
		return 0;
	}

	if (isr->type == JRLFLO) {
		/*
		 * jump (relative) 
		 */
		arg = asm_arg(&con, 0);

		/*
		 * jr allows for 4 conditional modes 
		 */
		reg = 0;
		if (isr->arg) {
			if (arg >= 13 && arg <= 16) {
				reg = (arg - 12) << 3;
				asm_expect(',');
				arg = asm_arg(&con, 0);
			} else if (arg != 31)
				return 1;
		}

		if (arg != 31)
			return 1;

		asm_emit(isr->opcode + reg);
		emit_exp(1, con);
		return 0;
	}

	if (isr->type == CALFLO) {
		/*
		 * call 
		 */
		arg = asm_arg(&con, 0);

		if (arg >= 13 && arg <= 20) {
			asm_emit(isr->opcode + ((arg - 13) << 3));
			asm_expect(',');
			emit_exp(2, 0);
		} else if (arg == 31) {
			asm_emit(isr->arg);
			emit_exp(2, con);
		} else
			return 1;
		return 0;
	}

	if (isr->type == RSTFLO) {
		/*
		 * rst 
		 */
		arg = asm_arg(&con, 1);

		if (arg != 31 || con & 0x7 || con > 0x38)
			return 1;

		asm_emit(isr->opcode + con);
		return 0;
	}

	if (isr->type == IOIN) {
		/*
		 * in 
		 */
		arg = asm_arg(&con, 1);

		/*
		 * special case for (c) only 
		 */
		if (arg == 33) {
			asm_emit(0xED);
			asm_emit(isr->arg + 0x30);
			return 0;
		}
		/*
		 * throw out (hl) 
		 */
		if (arg == 6 || arg > 7)
			return 1;

		/*
		 * grab next argument 
		 */
		reg = arg;
		asm_expect(',');
		arg = asm_arg(&con, 1);

		/*
		 * decode 
		 */
		if (reg == 7 && arg == 32) {
			asm_emit(isr->opcode);
			asm_emit(con);
		} else if (arg == 33) {
			asm_emit(0xED);
			asm_emit(isr->arg + (reg << 3));
		} else
			return 1;
		return 0;
	}

	if (isr->type == IOOUT) {
		/*
		 * out 
		 */
		arg = asm_arg(&con, 1);

		if (arg == 32) {
			/*
			 * immediate 
			 */
			reg = con;
			asm_expect(',');
			arg = asm_arg(&con, 1);

			/*
			 * only 'a' is supported 
			 */
			if (arg != 7)
				return 1;

			asm_emit(isr->opcode);
			asm_emit(reg);
		} else if (arg == 33) {
			/*
			 * (c) 
			 */
			asm_expect(',');
			arg = asm_arg(&con, 1);

			/*
			 * no (hl), but we can do '0' instead 
			 */
			if (arg == 6)
				return 1;
			if (arg == 31 && !con)
				arg = 6;

			if (arg > 7)
				return 1;

			asm_emit(0xED);
			asm_emit(isr->arg + (arg << 3));
		} else
			return 1;
		return 0;
	}

	if (isr->type == EXCH) {
		reg = asm_arg(&con, 1);
		asm_expect(',');
		arg = asm_arg(&con, 1);

		/*
		 * af 
		 */
		if (reg == 12) {
			if (arg == 12) {
				asm_expect('\'');
				asm_emit(isr->arg);
			} else
				return 1;
		}
		/*
		 * de 
		 */
		else if (reg == 9) {
			if (arg == 10) {
				asm_emit(isr->opcode + 0x08);
			} else
				return 1;
		}
		/*
		 * (sp) 
		 */
		else if (reg == 34) {
			switch (arg) {
			case 10:
				break;

			case 21:
				asm_emit(0xDD);
				break;

			case 22:
				asm_emit(0xFD);
				break;

			default:
				return 1;
			}

			asm_emit(isr->opcode);
		}
		return 0;
	}

	if (isr->type == INTMODE) {
		arg = asm_arg(&con, 1);

		/*
		 * only 0-2 
		 */
		if (arg != 31)
			return 1;

		asm_emit(0xED);
		switch (con) {
		case 0:
		case 1:
			asm_emit(isr->opcode + (con << 4));
			break;

		case 2:
			asm_emit(isr->arg);
			break;

		default:
			return 1;
		}
		return 0;
	}

	if (isr->type == LOAD) {
		/*
		 * i will never forgive you zilog 
		 */

		/*
		 * correct for carry flag 
		 */
		arg = asm_arg(&con, 0);
		if (arg == 16)
			arg = 1;

		/*
		 * special case for deferred constant 
		 */
		if (arg == 32) {
			return do_stax();
		}

		/*
		 * standard a-(hl) and ixh-(iy+*) 
		 */
		if (arg < 8 || (arg >= 23 && arg <= 28)) {
			return do_ldr8(arg);
		}

		/*
		 * bc-sp, and ix/iy 
		 */
		if ((arg >= 8 && arg <= 11) || (arg == 21 || arg == 22)) {
			return do_16i(arg);
		}

		/*
		 * ld (bc)|(de)|i|r, a 
		 */
		if (arg >= 35 && arg <= 38) {
			asm_expect(',');
			reg = asm_arg(&con, 1);
			if (reg != 7)
				return 1;

			switch (arg) {
			case 35:			/* ld (bc),a */
				asm_emit(0x02);
				break;

			case 36:			/* ld (de),a */
				asm_emit(0x12);
				break;

			case 37:			/* ld i,a */
				asm_emit(0xED);
				asm_emit(0x47);
				break;

			case 38:			/* ld r,a */
				asm_emit(0xED);
				asm_emit(0x4F);
				break;
			}
		} else
			return 1;
	}
	return 1;
}

/*
 * attempts to assemble an instruction assuming a symbol has just been tokenized
 *
 * in = pointer to string
 * returns 0 if an instruction is not matched, 1 if it is
 */
char
asm_instr(in)
char *in;
{
	int i;

	/*
	 * search for and assemble instruction 
	 */
	i = 0;
	while (isr_table[i].type) {
		if (asm_sequ(in, isr_table[i].mnem)) {
			if (verbose > 2)
				printf("instruction: %s\n", in);
			if (asm_doisr(&isr_table[i]))
				asm_error("invalid operand");;
			return 1;
		}
		i++;
	}

	return 0;
}

/*
 * changes segments for first pass segment top tracking
 * save our place
 * next = next segment
 */
void
change_seg(next)
char next;
{
	switch (segment) {
	case SEG_TEXT:
		text_top = cur_address;
		break;
	case SEG_DATA:
		data_top = cur_address;
		break;
	case SEG_BSS:
		bss_top = cur_address;
		break;
	default:
		break;
	}

	switch (next) {
	case SEG_TEXT:
		cur_address = text_top;
		break;
	case SEG_DATA:
		cur_address = data_top;
		break;
	case SEG_BSS:
		cur_address = bss_top;
		break;
	default:
		break;
	}
	segment = next;
}

/*
 * iterates through and fixes all segments for the second pass
 */
void
fix_seg()
{
	struct symbol *sym;
	struct local *loc;

	for (sym = sym_table; sym; sym = sym->next) {

		/*
		 * data starts at text end - XXX 
		 */
		if (sym->type == SEG_DATA) {
			sym->value += text_size;
		}

		/*
		 * bss -> text 
		 */
		if (sym->type == SEG_BSS) {
			sym->value += text_top + data_top;
		}
	}

	for (loc = loc_table; loc; loc = loc->next) {

		/*
		 * data -> text 
		 */
		if (loc->type == SEG_DATA) {
			loc->value += text_top;
		}
		/*
		 * bss -> text 
		 */
		if (loc->type == SEG_BSS) {
			loc->value += text_top + data_top;
		}
	}
}

char translate[6] = {
	0x00,						/* undefined */
	0x05 | 0x08,				/* text */
	0x06 | 0x08,				/* data */
	0x07 | 0x08,				/* bss */
	0x04 | 0x08,				/* absolute */
	0x00						/* external */
};

void
out_symbol(s)
struct symbol *s;
{
	int i;
	int t;

	if (verbose > 3) {
		printf("symbol: %s\t type %d value %x\n", s->name, s->type,
			   s->value);
	}
	t = s->type;
	if (t > 5)
		t = 5;
	outword(s->value);
	outbyte(translate[t]);
	for (i = 0; i < 9; i++) {
		outbyte(s->name[i]);
	}
}

#ifdef notdef
int
gcomp(a, b)
struct symbol **a;
struct symbol **b;
{
	return (strcmp((*a)->name, (*b)->name));
}
#endif

/*
 * outputs the metadata blocks after assembly is done
 */
void
asm_meta()
{
	int i;
	unsigned char lextn;
	struct global *glob;

	sort = (struct symbol **) malloc(sizeof(struct symbol *) * glob_count);
	i = 0;
	for (glob = glob_table; glob; glob = glob->next) {
		sort[i++] = glob->symbol;
	}
	/*
	 * qsort(sort, i, sizeof(struct symbol *), gcomp); 
	 */

	for (i = 0; i < glob_count; i++) {
		out_symbol(sort[i]);
	}

	reloc_out(textr.head, 0);
	reloc_out(datar.head, text_top);
}

/*
 * perform assembly functions
 * we do some passes over the source code, 
 * pass 1: locate symbols and relocs in relative segments
 *      all segments start at zero, and overlay
 *      fix up sizes and emit header
 * pass 2: now we know segment sizes, we can assign addresses
 *      and actually emit code and data
 */
void
assemble()
{
	char tok, type, next;
	int ifdepth, trdepth;
	unsigned short result;
	struct symbol *sym;

	asm_reset();

	pass = 0;

	segment = SEG_TEXT;
	text_top = data_top = bss_top = 0;
	cur_address = 0;

	loc_cnt = 0;

	ifdepth = trdepth = 0;

	/*
	 * run passes 
	 */
	while (1) {

		change_seg(SEG_TEXT);
		cur_address = 0;
		text_top = 0;

		if (verbose) {
			printf("start of pass %d\n", pass);
			printf
				("text_top: %d data_top: %d bss_top: %d mem_size: %d syms: %d\n",
				 text_top, data_top, bss_top, mem_size, glob_count);
		}

		while ((tok = token_read()) != -1) {
			if (verbose > 4)
				printf("token %d %c\n", tok, tok);

			/*
			 * command read 
			 */
			if (tok == '.') {
				tok = token_read();

				if (tok != T_NAME)
					asm_error("expected directive");

				if (verbose > 2)
					printf("directive: %s\n", token_buf);

				if (asm_sequ(token_buf, "if")) {
					ifdepth++;

					/*
					 * evaluate the expression 
					 */
					type = asm_evaluate(&result, 0);

					if (type != 4)
						asm_error("must be absolute");

					if (result)
						trdepth++;

					asm_eol();
					continue;
				}

				else if (asm_sequ(token_buf, "endif")) {
					if (!ifdepth)
						asm_error("unpaired .endif");

					if (ifdepth == trdepth)
						trdepth--;
					ifdepth--;

					asm_eol();
					continue;
				}

				/*
				 * skip if in an untrue if segment 
				 */
				if (ifdepth > trdepth) {
					asm_skip();
					continue;
				}

				next = 0;
				if (asm_sequ(token_buf, "text")) {
					next = 1;
				} else if (asm_sequ(token_buf, "data")) {
					next = 2;
				} else if (asm_sequ(token_buf, "bss")) {
					next = 3;
				}

				/*
				 * change segment 
				 */
				if (next != 0) {
					change_seg(next);
					asm_eol();
					continue;
				}

				if (asm_sequ(token_buf, "globl")) {
					while (1) {
						tok = token_read();
						if (tok != T_NAME)
							asm_error("expected symbol");
						if (pass == 0) {
							sym = sym_update(token_buf, SEG_UNDEF, 0);
							if (!sym)
								asm_error("undefined symbol");
							if (sym->type >= SEG_EXT)
								asm_error("symbol is external");
							add_glob(sym);
						}
						/*
						 * see if there is another 
						 */
						if (peek() == ',')
							asm_expect(',');
						else
							break;
					}
					asm_eol();
					continue;
				}

				if (asm_sequ(token_buf, "extern")) {
					while (1) {
						tok = token_read();
						if (tok != T_NAME)
							asm_error("expected symbol");
						if (pass == 0) {
							/*
							 * create external symbol 
							 */
							sym = sym_update(token_buf, extn++, 0);

							/*
							 * output extern to the global table
							 * this will ensure that it is outputted so the
							 * linker can see it
							 */
							add_glob(sym);
						}
						/*
						 * see if there is another 
						 */
						if (peek() == ',')
							asm_expect(',');
						else
							break;
					}
					asm_eol();
					continue;
				}

				/*
				 * .ds <byte count> 
				 */
				if (asm_sequ(token_buf, "ds")) {
					ds();
					asm_eol();
					continue;
				}

				/*
				 * .defb <byte>|<string>[,...] 
				 */
				if (asm_sequ(token_buf, "defb") ||
					asm_sequ(token_buf, "db")) {
					db();
					asm_eol();
					continue;
				}

				/*
				 * .defw <word>[,...]
				 */
				if (asm_sequ(token_buf, "defw") ||
					asm_sequ(token_buf, "dw")) {
					dw();
					asm_eol();
					continue;
				}

				printf("%s\n", token_buf);
				asm_error("unkown directive");
				continue;
			}
			/*
			 * skip if in an untrue if segment 
			 */
			if (ifdepth > trdepth && tok != 'n') {
				asm_skip();
			}

			/*
			 * symbol read 
			 */
			else if (tok == 'a') {

				/*
				 * try to get the type of the symbol 
				 */
				if (asm_instr(token_buf)) {
					/*
					 * it's an instruction 
					 */
					asm_eol();
				} else if (peek() == '=') {
					/*
					 * it's a symbol definition 
					 */
					asm_token_cache(sym_name);
					token_read();

					/*
					 * evaluate the expression 
					 */
					type = asm_evaluate(&result, 0);

					/*
					 * set the new symbol 
					 */
					sym_update(sym_name, type, result);
					asm_eol();
				} else if (peek() == ':') {
					/*
					 * it's a label 
					 */

					/*
					 * set the new symbol (if it is the first pass) 
					 */
					if (pass == 0) {
						sym_update(token_buf, segment, cur_address);

						/*
						 * auto globals? 
						 */
						if (g_flag) {
							sym = sym_fetch(token_buf);
							add_glob(sym);
						}
					}

					token_read();
				} else {
					asm_error("unexpected symbol");
				}
			}

			else if (tok == '0') {
				/*
				 * numeric read 
				 */
				result = num_parse(token_buf);

				if (result > 9)
					asm_error("local too large");

				asm_expect(':');

				loc_cnt++;
				if (pass == 0)
					local_add(result, segment, cur_address);

			} else if (tok != 'n') {
				asm_error("unexpected token");
			}
		}

		if (ifdepth)
			asm_error("unpaired .if");

		change_seg(SEG_TEXT);
        
		if (verbose) {
			printf("end of pass %d\n", pass);
			printf
				("text_top: %d data_top: %d bss_top: %d mem_size: %d syms: %d\n\n",
				 text_top, data_top, bss_top, mem_size, glob_count);
		}

		pass++;

		/*
		 * pass 1, so we know our text + data segment sizes
		 */
		if (pass == 1) {

			mem_size = text_top + data_top + bss_top;
			text_size = text_top;
			data_size = data_top;
			bss_size = bss_top;

			outbyte(0x99);
			outbyte(0x14);
			outword(glob_count * 12); /* symbol table size */
			outword(text_size);	/* text */
			outword(data_size);	/* data */
			outword(bss_size);	/* bss */
			outword(0);			/* stack+heap */
			outword(0);			/* textoff */
			outword(text_size);	/* dataoff */

			printf
				("magic %x text:%d data:%d bss:%d heap:%d symbols:%d textoff:%x dataoff:%x\n",
				 0x9914, text_size, data_size, bss_size, 0,
				 glob_count * 12, 0, text_size);

			loc_cnt = 0;

			/*
			 * fix segment symbols 
			 */
			change_seg(SEG_TEXT);
			fix_seg();

			/*
			 * reset segment addresses to their final addresses
			 */
			text_top = 0;
			data_top = text_size;
			bss_top = data_top + data_size;
			cur_address = 0;

			rewind(input_file);

			continue;
		}

		/*
		 * pass 2 is to output the code, tmp, and metadata
		 * and then we're done.
		 */
		if (pass == 2) {
			appendtmp();
			asm_meta();
			return;
		}
	}
}
