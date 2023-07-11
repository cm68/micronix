/*
 * z80 assembler guts
 *
 * substantially rewritten to remove stuff not needed for a compiler backend
 * or an assembler that is used in conjunction with a preprocessor
 * things removed:  the type machinery, and the odd defl, def syntax
 *
 * another messy feature removed is the local label stuff.
 *
 * /usr/src/cmd/asz/asm.c 
 *
 * Changed: <2023-07-09 17:36:53 curt>
 *
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
#include <stdio.h>
#ifdef linux
#include <stdlib.h>
#include <string.h>
#define INIT
#else
#define void int
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

#define TOKEN_BUF_SIZE 19
#define SYMBOL_NAME_SIZE 9

/*
 * symbols have a segment, emitted code does too.
 */
#define SEG_UNDEF   0       /* if index == 0xffff, not .globl */
#define SEG_TEXT    1
#define SEG_DATA    2
#define SEG_BSS     3
#define SEG_ABS     4
#define SEG_EXT     5

/*
 * expressions can be these
 */
struct expval {
	unsigned short num;
    struct symbol *sym;
};

/*
 * symbols come in a couple of flavors that are driven by
 * the assembler semantics:
 *
 * global symbols are exported to the object file, but can
 * have relocations referring to them.
 *
 * extern symbols are also found in the object file, and
 * are very likely to have relocations referring to them
 *
 * static symbols are not exported to the object file, but
 * are also likely to have relocations applied to them. these
 * relocations in the object file are implemented at segment
 * offsets.  they also are likely to start out unresolved
 * until they find definitions
 *
 * symbols are created when encountered, and usually it's a
 * forward reference without any information other than the
 * name.
 *
 * symbols that are intended to be in the object file get 
 * assigned an index in pass 1 of 0, otherwise 0xffff.
 *
 */
struct symbol {
    unsigned char seg;              /* SEG_* */
    unsigned short index;           /* object file ordinal */
    unsigned short value;           /* segment relative */
    char name[SYMBOL_NAME_SIZE];    /* zero padded */
    struct symbol *next;
};

/*
 * relocs are chained off of headers and need to stay
 * ordered.
 */
struct reloc {
    unsigned short addr;    /* where the fixup goes */
    struct symbol *sym;     /* what it contains */
    struct reloc *next;
};

struct rhead {
    char *segment;
    struct reloc *head;
    struct reloc *tail;
};

char *input_char = "";

char input_buffer[100];

/*
 * token buffer 
 */
char token_buf[TOKEN_BUF_SIZE] INIT;
char sym_name[TOKEN_BUF_SIZE] INIT;
unsigned short token_val;

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

struct rhead textr = { "text" };
struct rhead datar = { "data" };

struct symbol *symbols INIT;

void
fill_buf()
{
    if (!*input_char) {
        input_char = input_buffer;
        if (!fgets(input_buffer, sizeof(input_buffer), input_file)) {
            input_char[0] = -1;
            input_char[1] = 0;
        }
        line_num++;
    }
}

/*
 * returns what sio_next() would but does not move forward
 */
char
peek()
{
    if (!*input_char) 
        fill_buf();
    return (*input_char);
}

/*
 * returns the next character in the source, or -1 if complete
 */
char
get_next()
{
    char c;

    if (!*input_char)
        fill_buf();
    
    return (*input_char == -1 ? -1 : *input_char++);
}

/*
 * consumes to end of line
 */
void
consume()
{
    *input_char = '\0';
    fill_buf();
}

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
gripe(msg)
char *msg;
{
	printf("%s:%d %s\n%s", 
        infile, line_num, msg, input_buffer);
	exit(1);
}

void
gripe2(msg, arg)
char *msg;
char *arg;
{
	printf("%s:%d %s%s\n%s", 
        infile, line_num, msg, arg, input_buffer);
	exit(1);
}

void
save_symname()
{
	int i;

	for (i = 0; i < TOKEN_BUF_SIZE; i++)
		sym_name[i] = token_buf[i];
}

/*
 * is this an alphabetic or underscore
 */
char
alpha(in)
char in;
{
	return (in >= 'A' && in <= 'Z') || (in >= 'a' && in <= 'z')
		|| in == '_';
}

/*
 * converts an escaped char into its value
 *
 * \[bernetv] = c escape for control chars
 * \000 (octal) = 
 * \<anything else> = same
 */
char
escape()
{
    char c;
    int i = 0;

    c = get_next();
	switch (c) {
	case 'b':
		return '\b';
	case 'e':
		return 0x1B;
	case 'r':
		return '\r';
	case 'n':
		return '\n';
	case 't':
		return '\t';
	case 'v':
		return 0x0B;
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
        i = c - '0';
        while (1) {
            c = peek();
            if (c > '7' || c < '0') break;
            c = get_next();
            i = (i << 3) + c - '0';
        }
        return i;    
	default:
		return c;
	}
}

/*
 * parse number - handles quite a few formats:
 * decimal: 20, 78
 * hex:  0x0, 0X00, 000H, 00h
 * octal: 06, 003, 05o, 06O
 * binary: 0b0001010 000100B 01010b
 */
unsigned short
parsenum(s, i)
char *s;
int i;
{
    unsigned short val = 0;
    int base;
    char c = s[i-1] | 0x20;

    /* detect and consume our radix markers */
    if (c == 'h') {
        base = 16;
        s[i-1] = '\0';
    } else if (c == 'o') {
        base = 8;
        s[i-1] = '\0';
    } else if (c == 'b') {
        base = 2;
        s[i-1] = '\0';
    } else if (*s == 0) {
        c = *s++ | 0x20;
        if (c == 'h') {
            base = 16;
            s++;
        } else if (c == 'b') {
            base = 2;
            s++;
        } else {
            base = 8;
        }
    }

    while (*s) {
        val *= base;
        c = *s | 0x20; 
        c -= '0';
        if ((base == 16) && (c > 9)) 
            c -= ('a' - '9') + 1;
        if ((c > base) || (c < 0)) {
            gripe("numeric digit out of range");
        }
        val += c;
        s++;
    }
    return val;
}

char
delimiter(c)
char c;
{
    switch (c) {
    case ',': case ')': case '+': case '-':
    case '|': case '&': case ' ': case '\t':
        return 1;
    default:
        return 0;
    }
}

/*
 * the lexer.
 * reads the next token in from the source, 
 * white space and comments are ignored
 * special tokens are advanced over
 * [a-zA-Z_]+ -> T_NAME, token_buf filled
 * '\escape' 'c' -> T_NUM, token_val filled
 * "string" -> T_STRING, token_buf filled
 * anything else passes as the character

 * NB: ambiguity: how is ABBAH parsed?  we call it a NAME.
 * to make it a number, prefix it with 0. 0ABBAH.
 */
char
get_token()
{
    int i = 0;
    unsigned short val = 0;
    char *s;
    char c;

    c = get_next();

    if (alpha(c)) {
        token_buf[i++] = c;
        while (alpha(c = peek())) {
            token_buf[i++] = c;
            get_next();
        }
        token_buf[i++] = '\0';
        return T_NAME;
    }

    /*
     * a little tricky.  we need to scan to the next delimiter
     * into the token_buf, because we might have a postfix base
     */
    if (c >= '0' & c <= '9') {
        s = token_buf;
        token_buf[i++];
        while (1) {
            c = peek();
            if (delimiter(c))
                break;
            token_buf[i++] = c;
            get_next();
        }
        token_buf[i++] = '\0';
        token_val = parsenum(token_buf);
        return T_NUM;
    }

    if (c == '\'') {
        token_val = get_next();
        if (token_val == '\\') {
            token_val = escape();
        }
        if (get_next() != '\'') {
            gripe("unterminated char literal");
        }
        return T_NUM;
    }

    if (c == '\"') {
        while (1) {
            c = get_next();
            if (c == '\n') {
                gripe("unterminated string");
            }
            if (c == '\"') {
                break;
            }
            if (c == '\\') {
                c = escape();
            }
            token_buf[i++] = c;
        }
        token_buf[i++] = '\0';
        return T_STR;
    }
	return c;
}

/*
 * require a specific symbol
 * c = symbol to expect
 */
void
need(c)
char c;
{
	char tok;

	if (c == '}') {
		while (peek() == '\n')
			get_token();
	}

	tok = get_token();

	if (tok != c) {
		gripe("unexpected character");
	}

	if (c == '{' || c == ',') {
		while (peek() == '\n')
			get_token();
	}
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
			gripe("unexpected character in numeric");
		if (i >= radix)
			gripe("radix mismatch in numeric");

		out = (out * radix) + i;
	}

	return out;
}

/*
 * fetches the symbol
 * returns pointer to found symbol, or null
 */
struct symbol *
sym_fetch(name)
char *name;
{
	struct symbol *sym;
	int i;
	char equal;

	for (sym = symbols; sym; sym = sym->next) {

		equal = 1;
		for (i = 0; i < SYMBOL_NAME_SIZE; i++) {
			if (sym->name[i] != name[i])
				equal = 0;
			if (!sym->name[i])
				break;
		}
		if (equal)
			return sym;
	}
	return NULL;
}

/*
 * defines or redefines a symbol
 */
struct symbol *
sym_update(name, seg, value, visible)
char *name;
short seg;
unsigned short value;
int visible;
{
	struct symbol *sym;
	int i;

	sym = sym_fetch(name);

	if (!sym) {
		sym = (struct symbol *) malloc(sizeof(struct symbol));
		sym->next = symbols;
		symbols = sym;
        sym->seg = SEG_UNDEF;
        sym->index = 0xffff;
		for (i = 0; i < SYMBOL_NAME_SIZE - 1 && name[i] != 0; i++)
			sym->name[i] = name[i];
		sym->name[i] = 0;
	}

	/*
	 * update the symbol 
	 */
    if ((sym->seg != SEG_UNDEF) && 
        (sym->seg != seg)) {
        gripe2("segment for symbol changed", name);            
    }
	sym->seg = seg;
	sym->value = value;
    if (visible) sym->index = 0;
	return sym;
}

void
freerelocs(rh)
struct rhead *rh;
{
    struct reloc *r, *n;

    for (r = rh->head; r;) {
        n = r->next;
        free(r);
        r = n;
    }

    rh->tail = 0;
    rh->head = 0;
}

/*
 * resets all allocation stuff
 * this is what we run between assemblies.
 * it should clean out everything.
 */
void
asm_reset()
{
    struct symbol *s, *n;
    struct reloc *r;

    for (s = symbols; s;) {
        n = s->next;
        free(s);
        s = n;
    }
    freerelocs(&textr);
    freerelocs(&datar);
}

/*
 * adds an reference into a relocation table
 * we only do this in the second pass, since that's when
 * all symbols and segment addresses are resolved
 */
void
add_reloc(tab, addr, sym)
struct rhead *tab;
unsigned short addr;
struct symbol *sym;
{
	unsigned short diff;
	unsigned char i, next;
	struct reloc *r;

	if (!pass)
		return;

    if (verbose > 2)
        printf("add_reloc: %s %x %s\n", 
            tab->segment, addr, sym ? sym->name : "nosym");

    if (sym->seg == SEG_ABS)
        return;

    if (sym->seg == SEG_UNDEF)
        return;

	r = (struct reloc *) malloc(sizeof(struct reloc *));

	r->addr = addr;
    r->sym = sym;
	r->next = 0;

	if (!tab->head) {
		tab->tail = tab->head = r;
	} else {
		tab->tail->next = r;
	}
	tab->tail = r;
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
    int seg;

	while (r) {
		seg = r->sym->seg;
		if (verbose > 3) {
			printf("reloc: base: %x addr: %x seg: %d %s\n",
				   base, r->addr, seg, r->sym->name);
		}

		bump = r->addr - last;
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
		switch (seg) {
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
			control = r->sym->index;
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
 * evaluates an expression that is next in the token queue
 *
 * result = pointer where result will be placed in
 * returns segment of expression
 *
 * in pass 1, we really only need to parse the expression
 * and create symbol references.
 *
 * in pass 2, we actually need to do the expression eval
 * with all the interesting segment and arithmetic rules:
 * ABS = RELOC - RELOC if same segment
 * RELOC = RELOC +- ABS
 * ABS = ABS op ABS
 * and the usual precedence rules
 *
 * nb: the original author seems like he never heard of
 * recursion, so he built a stack structure to deal with
 * expressions.  C has a perfectly good stack.
 */
unsigned char
evaluate(result)
struct expval *result;
{
    char ret = SEG_ABS;
    char tok;
    struct symbol *sym;

    result->sym = 0;
    result->num = 0;

	while (1) {
		tok = get_token();

		/* it is a symbol */
		if (tok == T_NAME) {
			sym = sym_fetch(token_buf);
			if (sym) {
			    result->sym = sym;
				result->num = sym->value;
			} else {
                result->sym = sym_update(token_buf, SEG_UNDEF, 0, 0);
                result->num = 0;
			}
            ret = result->sym->seg;
		} else if (tok == T_NUM) {
			result->num = num_parse(token_buf);
		} else {
            gripe("neither fish nor fowl");
        }

        /*
         * here is where we look ahead and handle parenthesis and
         * operator precedence
         */

		/*
		 * check for ending conditions 
		 */
		tok = peek();
		if (tok == ',' || tok == '\n' || tok == -1)
			break;
	}
    return ret;
}

/*
 * emits a byte into assembly output
 * no bytes emitted on first pass, only update addresses
 *
 * b = byte to emit
 */
void
emitbyte(b)
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
				gripe("data in bss");
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
	emitbyte(w & 0xFF);
	emitbyte(w >> 8);
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
					emitbyte(decode);
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
				emitbyte(c);

		} else if (state == 1) {
			/*
			 * escape character 
			 */
			decode = c_escape(c);

			/*
			 * simple escape 
			 */
			if (decode) {
				emitbyte(decode);
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
				gripe("unknown escape");
			}
		}

		if (state == 3) {
			/*
			 * numeric parsing 
			 */
			num = hexparse(c);

			if (num == -1)
				gripe("unexpected character in numeric");
			if (num >= radix)
				gripe("radix mismatch in numeric");

			decode = (decode * radix) + num;

			num = asm_classify_radix(peek());
			length--;

			/*
			 * end the parsing 
			 */
			if (length < 1 || num == -1 || num >= radix) {
				state = 0;
				emitbyte(decode);
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
		emitbyte(0);
}

/*
 * emits up to two bytes, and handles relocation tracking
 *
 * size = number of bytes to emit
 * vp = value to push out
 */
void
emit_addr(size, vp)
unsigned short size;
struct expval *vp;
{
	unsigned short rel;
    unsigned char seg;
    unsigned short num;

    if (vp->sym) {
        seg = vp->sym->seg;
    } else {
        seg = SEG_ABS;
    }
	if (seg == SEG_UNDEF) {
		/* if we are on the second pass, error out */
		if (pass == 1)
			gripe2("undefined symbol", vp->sym->name);
		num = 0;
	}

	if (size == 1) {
		/*
		 * here we output only a byte 
		 */
		if ((seg >= SEG_EXT) && (pass == 1))
			gripe("cannot extern byte");

		if (seg == SEG_TEXT) {
			rel = (vp->sym->value - cur_address) - 1;
			if ((rel < 0x80) || (rel > 0xFF7F))
				emitbyte(rel);
			else
				gripe("relative out of bounds");
		} else {
			emitbyte(vp->num);
		}

	} else {

		if (vp->sym && pass) {
			switch (segment) {
			case SEG_TEXT:
				add_reloc(&textr, cur_address, vp->sym);
				break;
			case SEG_DATA:
				add_reloc(&datar, cur_address, vp->sym);
				break;
			default:
				gripe("invalid segment");
			}
		}
		emitword(num);
	}
}

/*
 * helper function to emit an immediate and do type checking
 * only absolute resolutions will be allowed
 */
void
emit_imm(vp)
struct expval *vp;
{
	if (vp->sym && vp->sym->seg != SEG_ABS && (pass == 1))
		gripe("must be absolute");

	emitbyte(vp->num);
}

/*
 * helper function to evaluate an expression and emit the results
 * will handle relocation tracking and pass related stuff
 *
 * size = maximum size of space
 */
void
emit_exp(size)
unsigned short size;
{
    struct expval value;

	evaluate(&value);

	emit_addr(size, &value);
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
			emit_exp(1);
		}
		if (peek() != ',')
			break;
		else
			need(',');
	}
}

void
dw()
{
	char tok;

	while (peek() != '\n' && peek() != -1) {
		tok = peek();
		emit_exp(2);
		if (peek() != ',')
			break;
		else
			need(',');
	}
}

void
ds()
{
    unsigned char seg;
    struct expval value;

	seg = evaluate(&value);
    if (seg != SEG_ABS) {
        gripe("ds requires absolute argument");
    }
    fill(value.num);
}

/*
 * parses an operand, 
 * returns token describing the argument,
 * populate vp if it's passed in.
 */
unsigned char
operand(vp)
struct expval *vp;
{
	int i;
	char tok;
	unsigned char ret, seg;
    struct symbol *sym;

	/*
	 * check if there is anything next 
	 */
    tok = peek();
	if (tok == '\n' || tok == -1)
		return 255;

	/*
	 * assume at plain expression at first 
	 */
	ret = T_PLAIN;

	/*
	 * read the token 
	 */
	tok = get_token();

	/*
	 * maybe a register symbol? sometimes 'c' means carry
	 */
	if (tok == T_NAME) {
		for (i = 0; op_table[i].token != 255; i++) {
			if (asm_sequ(token_buf, op_table[i].mnem)) {
				return op_table[i].token;
			}
		}
	}

	/*
	 * maybe in parenthesis? 
	 */
	if (tok == '(') {
		tok = get_token();

		if (asm_sequ(token_buf, "hl")) {
			need(')');
			return T_HL_I;
		} else if (asm_sequ(token_buf, "c")) {
			need(')');
			return T_C_I;
		} else if (asm_sequ(token_buf, "sp")) {
			need(')');
			return T_SP_I;
		} else if (asm_sequ(token_buf, "bc")) {
			need(')');
			return T_BC_I;
		} else if (asm_sequ(token_buf, "de")) {
			need(')');
			return T_DE_I;
		}

		else if (asm_sequ(token_buf, "ix")) {
			if (peek() == '+') {
				get_token();
				tok = 0;
				ret = T_IX_DISP;
			} else {
				need(')');
				return T_IX_I;
			}
		} else if (asm_sequ(token_buf, "iy")) {
			if (peek() == '+') {
				get_token();
				tok = 0;
				ret = T_IY_DISP;
			} else {
				need(')');
				return T_IY_I;
			}
		} else {
			ret = T_INDIR;
		}
	}

	/*
	 * ok, its an expression 
	 */
	seg = evaluate(vp);
	if (seg == SEG_UNDEF) {
		if (pass == 1)
			gripe2("undefined symbol", vp->sym->name);
	}

    /* all others have already returned */
    if (ret != T_PLAIN) {
	    need(')');
	}
	return ret;
}

/*
 * load indirect
 */
int
do_stax(vp)
struct expval *vp;
{
	unsigned char prim, arg, reg, type;
	unsigned short value;
    
	need(',');
	arg = operand(value);

	switch (arg) {
	case T_HL:					/* ld (nn), hl */
		emitbyte(0x22);
		break;

	case T_A:					/* ld (nn), a */
		emitbyte(0x32);
		break;

	case T_IX:					/* ld (nn), ix */
		emitbyte(0xDD);
		emitbyte(0x22);
		break;

	case T_IY:					/* ld (nn), iy */
		emitbyte(0xFD);
		emitbyte(0x22);
		break;

	case T_BC:					/* ld (nn), bc */
	case T_DE:					/* ld (nn), de */
	case T_SP:					/* ld (nn), sp */
		emitbyte(0xED);
		emitbyte(0x43 + ((arg - T_BC) << 4));
		break;

	default:
		return 1;
	}
	emit_addr(2, vp);
	return 0;
}

/*
 * 16 bit load
 */
int
do_16i(reg)
char reg;
{
	unsigned char arg;
	struct expval value;

	/*
	 * correct for ix,iy into hl 
	 */
	if (reg == T_IX) {
		emitbyte(0xDD);
		reg = T_HL;
	} else if (reg == T_IY) {
		emitbyte(0xFD);
		reg = T_HL;
	}

	/*
	 * grab a direct or deferred word 
	 */
	need(',');
	arg = operand(&value);

	if (arg == T_PLAIN) {
		/*
		 * ld bc|de|hl|sp, nn 
		 */
		emitbyte(0x01 + ((reg - T_BC) << 4));
		emit_exp(2);
	} else if (arg == T_INDIR) {
		if (reg == T_HL) {
			emitbyte(0x2A);
		} else {
			/*
			 * ld bc|de|sp, (nn) 
			 */
			emitbyte(0xED);
			emitbyte(0x4B + ((reg - T_BC) << 4));
		}
		emit_exp(2);
		need(')');
	} else if (reg == T_SP) {
		/*
		 * ld sp,hl|ix|iy specials 
		 */
		switch (arg) {
		case T_HL:
			break;
		case T_IX:
			emitbyte(0xDD);
			break;
		case T_IY:
			emitbyte(0xFD);
			break;
		default:
			return 1;
		}
		emitbyte(0xF9);
	} else
		return 1;
	return 0;
}

int
do_ldr8(arg)
char arg;
{
	unsigned char prim, reg, type;
    struct symbol *sym;
    struct expval value, disp;
    value.sym = 0;
    disp.sym = 0;

	prim = 0;

	/*
	 * grab any constants if they exist 
	 */
	if (arg == T_IX_DISP || arg == T_IY_DISP) {
		if (evaluate(&disp) != SEG_ABS) {
            gripe("nonconstant displacement");
        }
        prim++;
		need(')');
	}
	need(',');

	reg = operand(&value);

	if (arg >= T_IXH && arg <= T_IY_DISP) {
		if (arg <= T_IX_DISP) {
			emitbyte(0xDD);
            /* lose on ld ix*, iy* or ld ix[hl], (ix+d) */
            if (reg >= T_IYH)
                return 1;
            if (arg != T_IX_DISP && reg == T_IX_DISP)
                return 1;
		} else {
			emitbyte(0xFD);
            /* lose on ld iy*, ix* or ld iy[hl], (iy+d) */
            if (reg >= T_IXH && reg <= T_IX_DISP)
                return 1;
            if (arg != T_IY_DISP && reg == T_IY_DISP)
                return 1;
            arg -= 3;
		}
        arg = arg - (T_IXH + T_H);
	} else if (reg >= T_IXH && reg <= T_IY_DISP) {
		if (arg == T_HL_I)
			return 1;

		if (reg <= T_IX_DISP) {
			emitbyte(0xDD);
		} else {
			emitbyte(0xFD);
            reg -= 3;
		}
		if (reg == T_IX_DISP) {
            if (evaluate(&disp) != SEG_ABS) {
                gripe("nonconstant displacement");
            }
            prim++;
			need(')');
		} else if (arg == 4 || arg == 5)
            /* lose on ld [hl], ix[hl] */
			return 1;
        reg = reg - (T_IXH + T_H);
	}

	/*
	 * no (hl),(hl) 
	 */
	if (arg == T_HL_I && reg == T_HL_I)
		return 1;

	if (arg <= T_A && reg <= T_A) {
		/* reg8->reg8 */
		emitbyte(0x40 + (arg << 3) + reg);
		if (prim)
			emit_imm(&disp);
	} else if (arg <= T_A && reg == T_PLAIN) {
		/* ld reg8, n */
		emitbyte(0x06 + (arg << 3));
		if (prim)
			emit_imm(&disp);
		if (evaluate(&value) != SEG_ABS) {
            gripe("non constant immediate");
        }
		emit_imm(&value);
	} else if (arg == T_A) {
		/*
		 * special a loads 
		 */
		switch (reg) {
		case T_BC_I:
			emitbyte(0x0A);
			break;

		case T_DE_I:
			emitbyte(0x1A);
			break;

		case T_INDIR:
			emitbyte(0x3A);
			emit_exp(2);
			need(')');
			break;

		case T_I:
			emitbyte(0xED);
			emitbyte(0x57);
			break;

		case T_R:
			emitbyte(0xED);
			emitbyte(0x5F);
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
    struct expval value;

	/*
	 * primary select to 0 
	 */
	prim = 0;
	if (isr->type == BASIC) {
		/*
		 * basic ops 
		 */
		emitbyte(isr->opcode);
		return 0;
	}

	if (isr->type == BASIC_EXT) {
		/*
		 * basic extended ops 
		 */
		emitbyte(isr->arg);
		emitbyte(isr->opcode);
		return 0;
	}

	if (isr->type == ARITH) {
		/*
		 * arithmetic operations 
		 */
		arg = operand(&value);

		/*
		 * detect type of operation 
		 */
		if (isr->arg == CARRY) {
			if (arg == T_HL) {
				/*
				 * hl adc/sbc 
				 */
				prim = 1;
			} else if (arg != T_A)
				return 1;

			/*
			 * grab next arg 
			 */
			need(',');
			arg = operand(&value);
		} else if (isr->arg == ADD) {
			if (arg == T_HL) {
				/*
				 * hl add 
				 */
				prim = 2;

			} else if (arg == T_IX || arg == T_IY) {
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
			need(',');
			arg = operand(&value);

			/*
			 * no add i*,hl 
			 */
			if (prim == 3 && arg == T_HL)
				return 1;

			/*
			 * add i*,i* 
			 */
			if (prim == 3 && arg == reg)
				arg = T_HL;
		}

		if (prim == 0) {
			if (arg <= T_A) {
				/*
				 * basic from a-(hl) 
				 */
				emitbyte(isr->opcode + arg);
			} else if (arg >= T_IXH && arg <= T_IX_DISP) {
				emitbyte(0xDD);
				emitbyte(isr->opcode + (arg - T_IXH) + 4);
				if (arg == T_IX_DISP)
					emitbyte(value.num & 0xFF);
			} else if (arg >= T_IYH && arg <= T_IY_DISP) {
				emitbyte(0xFD);
				emitbyte(isr->opcode + (arg - T_IYH) + 4);
				if (arg == T_IY_DISP)
					emitbyte(value.num & 0xFF);
			} else if (arg == T_PLAIN) {
				emitbyte(isr->opcode + 0x46);
				emitbyte(value.num);
			} else
				return 1;
		} else if (prim == 1) {
			/*
			 * 16 bit carry ops bc-sp 
			 */
			if (arg >= T_BC && arg <= T_SP) {
				emitbyte(0xED);
				emitbyte((0x42 + (isr->opcode == 0x88 ? 8 : 0)) +
						 ((arg - 8) << 4));
			} else
				return 1;
		} else if (prim == 2) {
			/*
			 * 16 bit add ops bc-sp 
			 */
			if (arg >= T_BC && arg <= T_SP) {
				emitbyte(0x09 + ((arg - 8) << 4));
			} else
				return 1;
		} else if (prim == 3) {
			/*
			 * correct for hl -> ix,iy 
			 */
			if (arg == T_HL)
				arg = reg;
			if (arg == reg)
				arg = T_HL;

			/*
			 * pick ext block 
			 */
			if (reg == T_IX)
				emitbyte(0xDD);
			else
				emitbyte(0xFD);

			/*
			 * 16 bit add ops bc-sp 
			 */
			if (arg >= T_BC && arg <= T_SP) {
				emitbyte(0x09 + ((arg - 8) << 4));
			} else
				return 1;
		}
		return 0;
	}

	if (isr->type == INCR) {
		arg = operand(&value);

		if (arg <= T_A) {
			/* b,c,d,e,h,l,a */
			emitbyte(isr->opcode + ((arg) << 3));
		} else if (arg <= T_SP) {
			/* words bc,de,hl,sp */
			emitbyte(isr->arg + ((arg - T_BC) << 4));
		} else if (arg == T_IX) {
			emitbyte(0xDD);
			emitbyte(isr->arg + 0x20);
		} else if (arg == T_IY) {
			emitbyte(0xFD);
			emitbyte(isr->arg + 0x20);
		} else if (arg >= T_IXH && arg <= T_IX_DISP) {
			/*
			 * ixh-(ix+*) 
			 */
			emitbyte(0xDD);
			emitbyte(isr->opcode + ((arg - 19) << 3));
			if (arg == T_IX_DISP)
				emitbyte(&value);
		} else if (arg >= T_IYH && arg <= T_IY_DISP) {
			/*
			 * iyh-(iy+*) 
			 */
			emitbyte(0xFD);
			emitbyte(isr->opcode + ((arg - T_IY) << 3));
			if (arg == T_IY_DISP)
				emitbyte(&value);
		} else
			return 1;
		return 0;
	}

	if (isr->type == BITSH) {
		arg = operand(&value);

		/*
		 * bit instructions have a bit indicator that must be parsed 
		 */
		reg = 0;
		if (isr->arg) {
			if (arg != T_PLAIN)
				return 1;

			if (value.num > 7)
				return 1;

			reg = value.num;

			need(',');
			arg = operand(&value);
		}
		/*
		 * check for (ix+*) / (iy+*) 
		 */
		if (arg == T_IX_DISP || arg == T_IY_DISP) {

			if (arg == T_IX_DISP)
				emitbyte(0xDD);
			else
				emitbyte(0xFD);

			emitbyte(0xCB);

			/*
			 * write offset 
			 */
			emitbyte(&value);

			arg = T_HL_I;
			/*
			 * its an undefined operation 
			 */
			if (peek() == ',') {
				need(',');
				arg = operand(&value);

				/*
				 * short out for (hl) 
				 */
				if (arg == 6)
					arg = 8;
			}
		} else
			emitbyte(0xCB);

		if (arg > 7)
			return 1;

		emitbyte(isr->opcode + arg + (reg << 3));
		return 0;
	}

	if (isr->type == STACK) {
		arg = operand(&value);
		/* hack - swap af for sp */
		if (arg == T_SP)
			arg = 12;
		else if (arg == 12)
			arg = T_SP;

		if (arg >= T_BC && arg <= T_SP) {
			emitbyte(isr->opcode + ((arg - T_BC) << 4));
		} else if (arg == T_IX) {
			emitbyte(0xDD);
			emitbyte(isr->opcode + 0x20);
		} else if (arg == T_IY) {
			emitbyte(0xFD);
			emitbyte(isr->opcode + 0x20);
		} else
			return 1;
		return 0;
	}

	if (isr->type == RET) {
		arg = operand(&value);

		if (arg >= T_NZ && arg <= T_M) {
			emitbyte(isr->opcode + ((arg - T_NZ) << 3));
		} else if (arg == 255) {
			emitbyte(isr->arg);
		} else
			return 1;
		return 0;
	}

	if (isr->type == JMP) {
		arg = operand(&value);

        if (arg == 1) arg = T_CR;
		if (arg >= T_NZ && arg <= T_M) {
			emitbyte(isr->opcode + ((arg - T_NZ) << 3));
			need(',');
			emit_exp(2);
		} else if (arg == T_PLAIN) {
			emitbyte(isr->opcode + 1);
			emit_exp(2);
		} else if (arg == T_HL_I) {
			emitbyte(isr->arg);
		} else if (arg == T_IX_I) {
			emitbyte(0xDD);
			emitbyte(isr->arg);
		} else if (arg == T_IY_I) {
			emitbyte(0xFD);
			emitbyte(isr->arg);
		} else
			return 1;
		return 0;
	}

	if (isr->type == JRL) {
		arg = operand(&value);

		reg = 0;
		if (isr->arg) { /* not djnz */
            if (arg == 1) arg = T_CR;
			if (arg >= T_NZ && arg <= T_CR) {
				reg = (arg - T_NZ) << 3;
				need(',');
				arg = operand(&value);
			} else if (arg != T_PLAIN)
				return 1;
		}

		if (arg != T_PLAIN)
			return 1;

		emitbyte(isr->opcode + reg);
		emit_exp(1);
		return 0;
	}

	if (isr->type == CALL) {
		arg = operand(&value);

        if (arg == 1) arg = T_CR;
		if (arg >= T_NZ && arg <= T_M) {
			emitbyte(isr->opcode + ((arg - T_NZ) << 3));
			need(',');
			emit_exp(2);
		} else if (arg == T_PLAIN) {
			emitbyte(isr->arg);
			emit_exp(2);
		} else
			return 1;
		return 0;
	}

	if (isr->type == RST) {
		arg = operand(&value);

		if (arg != T_PLAIN || value.num & 0x7 || value.num > 0x38)
			return 1;

		emitbyte(isr->opcode + value.num);
		return 0;
	}

	if (isr->type == IN) {
		arg = operand(&value);

		/* in (c) */
		if (arg == T_C_I) {
			emitbyte(0xED);
			emitbyte(0x70);
			return 0;
		}

		if (arg == T_HL_I || arg > T_A)
			return 1;

		reg = arg;
		need(',');
		arg = operand(&value);

		if (reg == T_A && arg == T_INDIR) {
			emitbyte(isr->opcode);
			emitbyte(&value);
		} else if (arg == T_C_I) {
			emitbyte(0xED);
			emitbyte(0x40 + (reg << 3));
		} else
			return 1;
		return 0;
	}

	if (isr->type == OUT) {
		arg = operand(&value);

		if (arg == T_INDIR) {
			reg = value.num;
			need(',');
			arg = operand(&value);

			if (arg != T_A)
				return 1;

			emitbyte(isr->opcode);
			emitbyte(reg);
		} else if (arg == T_C_I) {
			need(',');
			arg = operand(&value);

			/* special dispensation for out (c),0 */
			if (arg == T_HL_I)
				return 1;
			if (arg == T_PLAIN && !value.num)
				arg = T_HL_I;

			if (arg > T_A)
				return 1;

			emitbyte(0xED);
			emitbyte(0x41 + (arg << 3));
		} else
			return 1;
		return 0;
	}

	if (isr->type == EXCH) {
		reg = operand(&value);
		need(',');
		arg = operand(&value);

		if (reg == T_AF) {
			if (arg == T_AF) {
				need('\'');
				emitbyte(isr->arg);
			} else
				return 1;
		}
		else if (reg == T_DE) {
			if (arg == T_HL) {
				emitbyte(isr->opcode + 0x08);
			} else
				return 1;
		}
		else if (reg == T_SP_I) {
			switch (arg) {
			case T_HL:
				break;
			case T_IX:
				emitbyte(0xDD);
				break;
			case T_IY:
				emitbyte(0xFD);
				break;
			default:
				return 1;
			}
			emitbyte(isr->opcode);
		}
		return 0;
	}

	if (isr->type == INTMODE) {
		arg = operand(&value);

		/*
		 * only 0-2 
		 */
		if (arg != T_PLAIN)
			return 1;

		emitbyte(0xED);
		switch (value.num) {
		case 0:
		case 1:
			emitbyte(isr->opcode + (value.num << 4));
			break;

		case 2:
			emitbyte(isr->arg);
			break;

		default:
			return 1;
		}
		return 0;
	}

	if (isr->type == LOAD) {
		arg = operand(&value);

		/*
		 * special case for deferred constant 
		 */
		if (arg == T_INDIR) {
			return do_stax(&value);
		}

		/*
		 * standard a-(hl) and ixh-(iy+*) 
		 */
		if (arg <= T_A || (arg >= T_IXH && arg <= T_IY_DISP)) {
			return do_ldr8(arg);
		}

		/*
		 * bc-sp, and ix/iy 
		 */
		if ((arg >= T_BC && arg <= T_SP) || (arg == T_IX || arg == T_IY)) {
			return do_16i(arg);
		}

		/*
		 * ld (bc)|(de)|i|r, a 
		 */
		if (arg >= 35 && arg <= 38) {
			need(',');
			reg = operand(&value);
			if (reg != 7)
				return 1;

			switch (arg) {
			case 35:			/* ld (bc),a */
				emitbyte(0x02);
				break;

			case 36:			/* ld (de),a */
				emitbyte(0x12);
				break;

			case 37:			/* ld i,a */
				emitbyte(0xED);
				emitbyte(0x47);
				break;

			case 38:			/* ld r,a */
				emitbyte(0xED);
				emitbyte(0x4F);
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
			if (asm_doisr(&isr_table[i]))
				gripe("invalid operand");;
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
	char tok;
    unsigned short type;
	unsigned short result;
	struct symbol *sym;
    unsigned short next;

	asm_reset();

	pass = 0;

	segment = SEG_TEXT;
	text_top = data_top = bss_top = 0;
	cur_address = 0;

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
				("text_top: %d data_top: %d bss_top: %d mem_size: %d\n",
				 text_top, data_top, bss_top, mem_size);
		}

		while ((tok = get_token()) != -1) {
			if (verbose > 4)
				printf("line %d: %s", line_num, input_buffer);

			/*
			 * command read 
			 */
			if (tok == '.') {
				tok = get_token();

				if (tok != T_NAME)
					gripe2("expected directive", token_buf);

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
					consume();
					continue;
				}

				if (asm_sequ(token_buf, "globl")) {
					while (1) {
						tok = get_token();
						if (tok != T_NAME)
							gripe("expected symbol");
						if (pass == 0) {
							sym = sym_update(token_buf, SEG_UNDEF, 0, 1);
						}
						/* see if there is another */
						if (peek() == ',')
							need(',');
						else
							break;
					}
					consume();
					continue;
				}

				if (asm_sequ(token_buf, "extern")) {
					while (1) {
						tok = get_token();
						if (tok != T_NAME)
							gripe("expected symbol");
						if (pass == 0) {
							sym = sym_update(token_buf, SEG_EXT, 0, 1);
						}
						/* see if there is another */
						if (peek() == ',')
							need(',');
						else
							break;
					}
					consume();
					continue;
				}

				/*
				 * .ds <byte count> 
				 */
				if (asm_sequ(token_buf, "ds")) {
					ds();
					consume();
					continue;
				}

				/*
				 * .defb <byte>|<string>[,...] 
				 */
				if (asm_sequ(token_buf, "defb") ||
					asm_sequ(token_buf, "db")) {
					db();
					consume();
					continue;
				}

				/*
				 * .defw <word>[,...]
				 */
				if (asm_sequ(token_buf, "defw") ||
					asm_sequ(token_buf, "dw")) {
					dw();
					consume();
					continue;
				}

				printf("%s\n", token_buf);
				gripe("unkown directive");
				continue;
			}

			/*
			 * symbol read 
			 */
			else if (tok == T_NAME) {
				/*
				 * try to get the type of the symbol 
				 */
				if (asm_instr(token_buf)) {
					/*
					 * it's an instruction 
					 */
					consume();
				} else if (peek() == '=') {
					/*
					 * it's a symbol definition 
					 */
					save_symname();
					get_token();

					type = evaluate(&result, &sym, 0);

					sym_update(sym_name, type, result, 0);
					consume();
				} else if (peek() == ':') {
					/*
					 * set the new symbol (if it is the first pass) 
					 */
					if (pass == 0) {
						sym_update(token_buf, segment, cur_address, 0);
					}

					get_token();
				} else {
					gripe("unexpected symbol");
				}
			} else if (tok != 'n') {
				gripe("unexpected token");
			}
		}

		change_seg(SEG_TEXT);
        
		if (verbose) {
			printf("end of pass %d\n", pass);
			printf
				("text_top: %d data_top: %d bss_top: %d mem_size: %d\n\n",
				 text_top, data_top, bss_top, mem_size);
		}

		pass++;

		/*
		 * pass 1, so we know our text + data segment sizes
		 */
		if (pass == 1) {

			change_seg(SEG_TEXT);

			mem_size = text_top + data_top + bss_top;
			text_size = text_top;
			data_size = data_top;
			bss_size = bss_top;

            next = 0;

            /* we've seen everything, so we can assign indexes */
	        for (sym = symbols; sym; sym = sym->next) {

                if (sym->seg == SEG_UNDEF) {
                    gripe2("undefined symbol", sym->name);
                    continue;
                }
                if (sym->index == 0) {
                    sym->index = next++;
                }
		        if (sym->seg == SEG_DATA) {
                    sym->value += text_size;
                }
                if (sym->seg == SEG_BSS) {
                    sym->value += text_size + data_size;
                }
            }

			outbyte(0x99);
			outbyte(0x14);
			outword(next * 12); /* symbol table size */
			outword(text_size);	/* text */
			outword(data_size);	/* data */
			outword(bss_size);	/* bss */
			outword(0);			/* stack+heap */
			outword(0);			/* textoff */
			outword(text_size);	/* dataoff */

            if (verbose)
                printf("magic %x text:%d data:%d bss:%d heap:%d symbols:%d textoff:%x dataoff:%x\n",
                     0x9914, text_size, data_size, bss_size, 0,
                     next * 12, 0, text_size);

			/*
			 * reset segment addresses to their final addresses
			 */
			text_top = 0;
			data_top = text_size;
			bss_top = data_top + data_size;
			cur_address = 0;

            line_num = 0;
			rewind(input_file);

			continue;
		}

		/*
		 * pass 2 is to output the code, data, symbols and reloc
		 */
		if (pass == 2) {
			appendtmp();

            for (sym = symbols; sym; sym = sym->next) {
                switch (sym->seg) {
                case SEG_UNDEF:
                    type = 0x08;
                    break;
                case SEG_TEXT:
                    type = 0x05 | 0x08;
                    break;
                case SEG_DATA:
                    type = 0x06 | 0x08;
                    break;
                case SEG_BSS:
                    type = 0x07 | 0x08;
                    break;
                case SEG_ABS:
                    type = 0x04 | 0x08;
                    break;
                case SEG_EXT:
                    type = 0x08;
                    break;
                default:
                    break;
                }
                if (verbose > 3) {
                    printf("sym: %9s index: %5d seg: %d type: %x\n",
                        sym->name, sym->index, sym->seg, type);
                }
                if (sym->index == 0xffff)
                    continue;
	            outword(sym->value);
                outbyte(type);
                for (next = 0; next < 9; next++) {
                    outbyte(sym->name[next]);
                }
            }

            reloc_out(textr.head, 0);
            reloc_out(datar.head, text_top);

			return;
		}
	}
}
