/*
 * mkkw.c - generate skip-optimized keyword table for kw.c
 *
 * Builds a trie-like structure with skip pointers that allows
 * fast keyword lookup by skipping entire subtrees on mismatch.
 *
 * Usage: ./mkkw
 * Output: C code for optimized ckw[] table
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexeme.h"

#define HI 0x80
#define MAXKW 64
#define MAXLEN 16

struct keyword {
	char *name;
	char *tokname;
	int value;
};

/*
 * C keywords - ordered by frequency for fast lookup
 * Most common keywords first: if, int, char, return, etc.
 */
struct keyword keywords[] = {
	{ "if",       "IF",       IF },
	{ "int",      "INT",      INT },
	{ "char",     "CHAR",     CHAR },
	{ "return",   "RETURN",   RETURN },
	{ "for",      "FOR",      FOR },
	{ "while",    "WHILE",    WHILE },
	{ "else",     "ELSE",     ELSE },
	{ "void",     "VOID",     VOID },
	{ "static",   "STATIC",   STATIC },
	{ "struct",   "STRUCT",   STRUCT },
	{ "unsigned", "UNSIGNED", UNSIGNED },
	{ "signed",   "SIGNED",   SIGNED },
	{ "const",    "CONST",    CONST },
	{ "volatile", "VOLATILE", VOLATILE },
	{ "long",     "LONG",     LONG },
	{ "short",    "SHORT",    SHORT },
	{ "switch",   "SWITCH",   SWITCH },
	{ "case",     "CASE",     CASE },
	{ "break",    "BREAK",    BREAK },
	{ "continue", "CONTINUE", CONTINUE },
	{ "default",  "DEFAULT",  DEFAULT },
	{ "do",       "DO",       DO },
	{ "enum",     "ENUM",     ENUM },
	{ "extern",   "EXTERN",   EXTERN },
	{ "goto",     "GOTO",     GOTO },
	{ "register", "REGISTER", REGISTER },
	{ "sizeof",   "SIZEOF_KW", SIZEOF_KW },
	{ "typedef",  "TYPEDEF",  TYPEDEF },
	{ "union",    "UNION",    UNION },
	{ "auto",     "AUTO",     AUTO },
	{ "asm",      "ASM",      ASM },
	{ NULL, NULL, 0 }
};

/*
 * Trie node for building the keyword tree
 */
struct tnode {
	char c;              /* character at this node */
	int token;           /* -1 if not a terminal, else token value */
	char *tokname;       /* token name for output */
	struct tnode *child; /* first child */
	struct tnode *sib;   /* next sibling */
};

struct tnode *root;

/* output buffer for building table */
unsigned char outbuf[1024];
char *outnames[1024];  /* parallel array for token names */
char istoken[1024];    /* 1 if this byte is a token value */
int outdepth[1024];    /* depth at each position */
int outpos;
int emitdepth;         /* current depth during emit */

struct tnode *
newnode(char c)
{
	struct tnode *n = calloc(1, sizeof(*n));
	n->c = c;
	n->token = -1;
	return n;
}

/*
 * Insert a keyword into the trie
 */
void
insert(char *word, char *tokname, int token)
{
	struct tnode **pp = &root;
	struct tnode *p;

	while (*word) {
		/* find or create node for this character */
		while ((p = *pp) != NULL) {
			if (p->c == *word)
				break;
			pp = &p->sib;
		}
		if (!p) {
			p = newnode(*word);
			*pp = p;
		}
		pp = &p->child;
		word++;
	}
	/* mark terminal */
	p->token = token;
	p->tokname = tokname;
}

/*
 * Count siblings (including self)
 */
int
countsib(struct tnode *n)
{
	int count = 0;
	while (n) {
		count++;
		n = n->sib;
	}
	return count;
}

/*
 * Calculate size of subtree in bytes
 * This is used to compute skip distances
 */
int
subtreesize(struct tnode *n)
{
	int size = 0;

	if (!n)
		return 0;

	/* this node's contribution */
	if (countsib(n) > 1 || n->sib) {
		/* has siblings - needs skip byte */
		size = 2;
	} else {
		size = 1;
	}

	/* terminal marker if this is end of word */
	if (n->token >= 0) {
		size += 2;  /* 0xff/0xfe + token */
	}

	/* children */
	size += subtreesize(n->child);

	/* siblings */
	size += subtreesize(n->sib);

	return size;
}

/*
 * Complete words ending in this subtree: the node's own terminal
 * plus everything below and beside its child.
 */
int
subterms(struct tnode *n)
{
	if (!n)
		return 0;
	return (n->token >= 0) + subterms(n->child) + subterms(n->sib);
}

/*
 * Emit a subtree, calculating skip distances
 * Returns number of bytes emitted
 */
int
emit(struct tnode *n)
{
	int start = outpos;
	int hassib;
	int mydepth = emitdepth;
	int skipstart, skipend, skippos, skip;

	if (!n)
		return 0;

	/*
	 * The skip byte is not only for siblings.  The matcher recovers
	 * from a plain-literal mismatch by scanning to the first pattern
	 * terminator and resuming with the string reset - which is only
	 * sound if that terminator ends the whole entry.  A branching
	 * subtree has terminators inside it, so a mismatch on its head
	 * has to skip all of it at once or the recovery resumes in the
	 * middle, reading a branch arm as a fresh entry.  auto/asm share
	 * an 'a' that was emitted plain because nothing followed it, and
	 * the identifier "sm" walked into the tail of "asm" and came
	 * back a keyword.  With no sibling the skip just lands past the
	 * subtree, on the next entry or the terminating zero.
	 */
	hassib = (n->sib != NULL) ||
	    (n->token >= 0) + subterms(n->child) > 1;

	/* emit character, with HI bit if has siblings */
	if (hassib) {
		/* calculate skip distance to sibling */
		skipstart = outpos + 2;  /* after char and skip byte */

		/* emit char with HI bit */
		outdepth[outpos] = mydepth;
		outbuf[outpos++] = n->c | HI;

		/* placeholder for skip - fill in later */
		skippos = outpos++;

		/* emit terminal if this is end of word */
		if (n->token >= 0) {
			if (n->child) {
				outbuf[outpos] = 0xfe;  /* more patterns follow */
			} else {
				outbuf[outpos] = 0xff;  /* end of pattern */
			}
			outpos++;
			outbuf[outpos] = n->token;
			outnames[outpos] = n->tokname;
			istoken[outpos] = 1;
			outdepth[outpos] = mydepth;
			outpos++;
		}

		/* emit children */
		emitdepth = mydepth + 1;
		emit(n->child);

		skipend = outpos;

		/* fill in skip distance */
		skip = skipend - skipstart;
		if (skip > 127) {
			fprintf(stderr, "skip too large: %d\n", skip);
			exit(1);
		}
		outbuf[skippos] = skip;

		/* emit siblings at same depth */
		emitdepth = mydepth;
		emit(n->sib);
	} else {
		/* no siblings - simple literal match */
		outdepth[outpos] = mydepth;
		outbuf[outpos++] = n->c;

		/* emit terminal if this is end of word */
		if (n->token >= 0) {
			if (n->child) {
				outbuf[outpos] = 0xfe;
			} else {
				outbuf[outpos] = 0xff;
			}
			outpos++;
			outbuf[outpos] = n->token;
			outnames[outpos] = n->tokname;
			istoken[outpos] = 1;
			outdepth[outpos] = mydepth;
			outpos++;
		}

		/* emit children */
		emitdepth = mydepth + 1;
		emit(n->child);
	}

	return outpos - start;
}

/*
 * Print the table as C code with structure
 */
void
printtable(void)
{
	int i, j;
	int needindent = 1;
	int curdepth = 0;
	unsigned char c;

	printf("#include \"lexeme.h\"\n");
	printf("#define HI 0x80\n\n");
	printf("unsigned char ckw[] = {\n");

	for (i = 0; i < outpos; i++) {
		c = outbuf[i];

		/* update depth from any char position (HI or literal) */
		if ((c & HI) || (c != 0xff && c != 0xfe && !istoken[i])) {
			curdepth = outdepth[i];
		}

		if (needindent) {
			printf("\t");
			for (j = 0; j < curdepth; j++)
				printf("  ");
			needindent = 0;
		}

		if (c == 0xff) {
			printf("0xff, ");
		} else if (c == 0xfe) {
			printf("0xfe, ");
		} else if (istoken[i]) {
			/* token value - print symbolic name */
			printf("%s,\n", outnames[i]);
			needindent = 1;
		} else if (c & HI) {
			printf("'%c'|HI, %d, ", c & 0x7f, outbuf[i+1]);
			i++;  /* skip the skip byte */
		} else {
			/* literal character */
			printf("'%c', ", c);
		}
	}

	if (!needindent)
		printf("\n");
	printf("\t0\n};\n");
}

/*
 * Debug: print the trie
 */
void
dumptrie(struct tnode *n, int depth)
{
	int i;

	if (!n)
		return;

	for (i = 0; i < depth; i++)
		printf("  ");
	printf("%c", n->c);
	if (n->token >= 0)
		printf(" [%s=%d]", n->tokname, n->token);
	printf("\n");

	dumptrie(n->child, depth + 1);
	dumptrie(n->sib, depth);
}

/*
 * Test the generated table with kwlook algorithm
 */
unsigned char
kwlook(unsigned char *str, unsigned char *table)
{
	unsigned char c;
	unsigned char *s = str;

	while (1) {
		c = *table;
		if (c == 0)
			return 0xff;
		if (c == 0xff || c == 0xfe) {
			if (*s == 0)
				return table[1];
			table += 2;
			if (c == 0xff)
				s = str;
			continue;
		}
		if (c & 0x80) {
			if (*s == (c & 0x7f)) {
				s++;
				table += 2;
			} else {
				table += table[1] + 2;
			}
			continue;
		}
		if (c != *s) {
			while (*table != 0xff && *table != 0xfe && *table != 0) {
				if (*table & 0x80)
					table += 2;
				else
					table++;
			}
			if (*table == 0)
				return 0xff;
			table += 2;
			s = str;
			continue;
		}
		s++;
		table++;
	}
}

int
testtable(void)
{
	struct keyword *kw;
	int errors = 0;
	unsigned char result;
	char **p;
	static char *nonkw[] = { "foo", "iffy", "integer", "i", "in", "ret", "", NULL };

	/* add terminator */
	outbuf[outpos] = 0;

	/* test all keywords */
	for (kw = keywords; kw->name; kw++) {
		result = kwlook((unsigned char *)kw->name, outbuf);
		if (result != kw->value) {
			fprintf(stderr, "FAIL: %s expected %d got %d\n",
				kw->name, kw->value, result);
			errors++;
		}
	}

	/* test non-keywords */
	for (p = nonkw; *p; p++) {
		result = kwlook((unsigned char *)*p, outbuf);
		if (result != 0xff) {
			fprintf(stderr, "FAIL: '%s' should not match, got %d\n",
				*p, result);
			errors++;
		}
	}

	if (errors == 0)
		fprintf(stderr, "All tests passed\n");
	return errors;
}

void
usage(void)
{
	fprintf(stderr, "Usage: mkkw [-d] [-t]\n");
	fprintf(stderr, "  -d  Debug: show trie structure\n");
	fprintf(stderr, "  -t  Test: verify generated table\n");
	fprintf(stderr, "Output: C code for skip-optimized ckw[] table\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	struct keyword *kw;
	int debug = 0;
	int test = 0;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-d") == 0)
			debug = 1;
		else if (strcmp(argv[i], "-t") == 0)
			test = 1;
		else if (strcmp(argv[i], "-h") == 0)
			usage();
	}

	/* build trie */
	for (kw = keywords; kw->name; kw++) {
		insert(kw->name, kw->tokname, kw->value);
	}

	if (debug) {
		printf("/* Trie structure:\n");
		dumptrie(root, 0);
		printf("*/\n\n");
	}

	/* emit table */
	emit(root);

	if (test) {
		return testtable();
	}

	/* output */
	printtable();

	fprintf(stderr, "Generated %d bytes\n", outpos + 1);

	return 0;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
