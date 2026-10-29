/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	test.c,v 1.9 1994/11/05 20:48:06 ache Exp
 */

/*
 * test expression
 * [ expression ]
 *
 * cmd/test/test.c
 *
 * 2.11BSD test (test.c 8.3.1), ported to micronix.  This is the
 * rewrite with the POSIX special cases and the full primary set; the
 * V7 grammar is smaller and is not what bsh's IF and WHILE expect a
 * POSIX script to have.
 *
 * The port changes are small and all of them are the system's:
 *
 *	- <types.h> and <sys/fs.h> replace <sys/types.h>; the tree's
 *	  headers are what the compiler and the rest of the tree use.
 *	- there is no st_size: the size is st_size0 (high byte) and
 *	  st_size1 (low word), and FSIZE() puts them together.
 *	- there is no geteuid, getgid or getgroups, and no group
 *	  membership list.  getuid() is the uid; -r, -w and -x test the
 *	  owner's bits if the caller owns the file and the other bits
 *	  if not.
 *	- there is no lstat and no symbolic link, FIFO or sticky bit.
 *	  -h runs stat (so it is false), and the type primaries for
 *	  links and FIFOs compare against a constant the mask can never
 *	  produce, so they are false.  -k is always false.
 *	- err() and errx() are two lines of fprintf here, not a library.
 *
 * The program is also [ - main() looks at argv[0] and a [ call must
 * end with a ]; install copies the binary to /bin/[.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <types.h>
#include <sys/fs.h>
#include <sys/stat.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "operators.h"

/*
 * Primaries this system has no file type for.  The type test masks
 * st_mode with S_IFMT before comparing, and 0100000 and 0120000 are
 * outside that mask, so neither can ever match.
 */
#define	S_IFLNK		0100000
#define	S_IFIFO		0120000
#define	S_ISVTX		0		/* no sticky bit: always false */

/*
 * Permission bits: sys/stat.h names these for the owner (IOWNER,
 * IGROUP, IOTHER) but not one by one the way 2.11 does.
 */
#define	S_IRUSR		0400
#define	S_IWUSR		0200
#define	S_IXUSR		0100
#define	S_IRGRP		0040
#define	S_IWGRP		0020
#define	S_IXGRP		0010
#define	S_IROTH		0004
#define	S_IWOTH		0002
#define	S_IXOTH		0001

/*
 * The 32-bit file size, from its two halves.
 */
#define	FSIZE(s)	((long)((s).st_size0) << 16 | (unsigned)(s).st_size1)

#define	STACKSIZE	12
#define	NESTINCR	16

/* data types */
#define	STRING	0
#define	INTEGER	1
#define	BOOLEAN	2

#define	IS_BANG(s) (s[0] == '!' && s[1] == '\0')

/*
 * This structure hold a value.  The type keyword specifies the type of
 * the value, and the union u holds the value.  The value of a boolean
 * is stored in u.num (1 = TRUE, 0 = FALSE).
 */
struct value {
	int type;
	union {
		char *string;
		long num;
	} u;
};

struct operator {
	short op;		/* Which operator. */
	short pri;		/* Priority of operator. */
};

struct filestat {
	char *name;		/* Name of file. */
	int rcode;		/* Return code from stat. */
	struct stat stat;	/* Status info on file. */
};

extern char *unary_op[];
extern char *binary_op[];
extern char *andor_op[];
extern int op_priority[];
extern int op_argflag[];

extern int isatty();

/* the two failure reports the library's err(3) would provide */
static void
fail(s)
	char *s;
{
	fprintf(stderr, "test: %s\n", s);
	exit(2);
}

static void
badint(s)
	char *s;
{
	fprintf(stderr, "test: %s: expected integer\n", s);
	exit(2);
}

int expr_is_false();
void expr_operator();
int lookup_op();
int pos_unary();
int pos_binary();
void get_int();
void syntax();
void overflow();

main(argc, argv)
	int argc;
	char *argv[];
{

	struct operator opstack[STACKSIZE];
	register struct operator *opsp;
	struct value valstack[STACKSIZE + 1];
	register struct value *valsp;
	struct filestat fs;
	char  c, **ap, *opname, *p;
	int binary, nest, op, pri, ret_val, skipping;

	if ((p = argv[0]) == NULL)
		fail("argc is zero");

	if (*p != '\0' && p[strlen(p) - 1] == '[') {
		if (strcmp(argv[--argc], "]"))
			fail("missing ]");
		argv[argc] = NULL;
	}
	ap = argv + 1;
	fs.name = NULL;

	/*
	 * Test(1) implements an inherently ambiguous grammer.  In order to
	 * assure some degree of consistency, we special case the POSIX 1003.2
	 * requirements to assure correct evaluation for POSIX scripts.  The
	 * following special cases comply with POSIX P1003.2/D11.2 Section
	 * 4.62.4.
	 */
	switch(argc - 1) {
	case 0:				/* % test */
		return (1);
		break;
	case 1:				/* % test arg */
		return (argv[1] == NULL || *argv[1] == '\0') ? 1 : 0;
		break;
	case 2:				/* % test op arg */
		opname = argv[1];
		if (IS_BANG(opname))
			return (*argv[2] == '\0') ? 0 : 1;
		else {
			ret_val = pos_unary(&argv[1]);
			if (ret_val >= 0)
				return (ret_val);
		}
		break;
	case 3:				/* % test arg1 op arg2 */
		if (IS_BANG(argv[1])) {
			ret_val = pos_unary(&argv[1]);
			if (ret_val >= 0)
				return (!ret_val);
		} else {
			ret_val = pos_binary(&argv[1]);
			if (ret_val >= 0)
				return (ret_val);
		}
		break;
	case 4:				/* % test ! arg1 op arg2 */
		if (IS_BANG(argv[1]) && lookup_op(argv[3], andor_op) < 0 ) {
			ret_val = pos_binary(&argv[2]);
			if (ret_val >= 0)
				return (!ret_val);
		}
		break;
	default:
		break;
	}

	/*
	 * We use operator precedence parsing, evaluating the expression as
	 * we parse it.  Parentheses are handled by bumping up the priority
	 * of operators using the variable "nest."  We use the variable
	 * "skipping" to turn off evaluation temporarily for the short
	 * circuit boolean operators.  (It is important do the short circuit
	 * evaluation because under NFS a stat operation can take infinitely
	 * long.)
	 */
	opsp = opstack + STACKSIZE;
	valsp = valstack;
	nest = skipping = 0;
	if (*ap == NULL) {
		valstack[0].type = BOOLEAN;
		valstack[0].u.num = 0;
		goto done;
	}
	for (;;) {
		opname = *ap++;
		if (opname == NULL)
			syntax();
		if (opname[0] == '(' && opname[1] == '\0') {
			nest += NESTINCR;
			continue;
		} else if (*ap && (op = lookup_op(opname, unary_op)) >= 0) {
			if (opsp == &opstack[0])
				overflow();
			--opsp;
			opsp->op = op;
			opsp->pri = op_priority[op] + nest;
			continue;
		} else {
			valsp->type = STRING;
			valsp->u.string = opname;
			valsp++;
		}
		for (;;) {
			opname = *ap++;
			if (opname == NULL) {
				if (nest != 0)
					syntax();
				pri = 0;
				break;
			}
			if (opname[0] != ')' || opname[1] != '\0') {
				if ((op = lookup_op(opname, binary_op)) < 0)
					syntax();
				op += FIRST_BINARY_OP;
				pri = op_priority[op] + nest;
				break;
			}
			if ((nest -= NESTINCR) < 0)
				syntax();
		}
		while (opsp < &opstack[STACKSIZE] && opsp->pri >= pri) {
			binary = opsp->op;
			for (;;) {
				valsp--;
				c = op_argflag[opsp->op];
				if (c == OP_INT) {
					if (valsp->type == STRING)
						get_int(valsp->u.string,
						    &valsp->u.num);
					valsp->type = INTEGER;
				} else if (c >= OP_STRING) {
					            /* OP_STRING or OP_FILE */
					if (valsp->type == INTEGER) {
						if ((p = (char *)malloc(32)) == NULL)
							fail("out of memory");
						(void)sprintf(p,
						    "%d", valsp->u.num);
						valsp->u.string = p;
					} else if (valsp->type == BOOLEAN) {
						if (valsp->u.num)
							valsp->u.string =
						            "true";
						else
							valsp->u.string = "";
					}
					valsp->type = STRING;
					if (c == OP_FILE && (fs.name == NULL ||
					    strcmp(fs.name, valsp->u.string))) {
						fs.name = valsp->u.string;
						fs.rcode =
						    stat(valsp->u.string,
                                                    (char *)&fs.stat);
					}
				}
				if (binary < FIRST_BINARY_OP)
					break;
				binary = 0;
			}
			if (!skipping)
				expr_operator(opsp->op, valsp, &fs);
			else if (opsp->op == AND1 || opsp->op == OR1)
				skipping--;
			valsp++;		/* push value */
			opsp++;			/* pop operator */
		}
		if (opname == NULL)
			break;
		if (opsp == &opstack[0])
			overflow();
		if (op == AND1 || op == AND2) {
			op = AND1;
			if (skipping || expr_is_false(valsp - 1))
				skipping++;
		}
		if (op == OR1 || op == OR2) {
			op = OR1;
			if (skipping || !expr_is_false(valsp - 1))
				skipping++;
		}
		opsp--;
		opsp->op = op;
		opsp->pri = pri;
	}
done:	return (expr_is_false(&valstack[0]));
}

int
expr_is_false(val)
	register struct value *val;
{

	if (val->type == STRING) {
		if (val->u.string[0] == '\0')
			return (1);
	} else {		/* INTEGER or BOOLEAN */
		if (val->u.num == 0)
			return (1);
	}
	return (0);
}

/*
 * Set a value to a boolean, so the cases below can say it in one
 * statement rather than sharing labels between cases.
 */
void
setval(sp, n)
	register struct value *sp;
	long n;
{
	sp->u.num = n;
	sp->type = BOOLEAN;
}

/*
 * The permission bits -r, -w and -x look at.  A caller who owns the
 * file is tested on the owner's bits; there is no group list here, so
 * anyone else is tested as other.
 */
int
permbits(st, own, grp, oth)
	struct stat *st;
	int own, grp, oth;
{
	if (st->st_uid == getuid())
		return (own);
	return (oth);
}


/*
 * Execute an operator.  Op is the operator.  Sp is the stack pointer;
 * sp[0] refers to the first operand, sp[1] refers to the second operand
 * (if any), and the result is placed in sp[0].  The operands are converted
 * to the type expected by the operator before expr_operator is called.
 * Fs is a pointer to a structure which holds the value of the last call
 * to stat, to avoid repeated stat calls on the same file.
 */
void
expr_operator(op, sp, fs)
	int op;
	register struct value *sp;
	struct filestat *fs;
{
	register int i;

	switch (op) {
	case NOT:
		sp->u.num = expr_is_false(sp);
		sp->type = BOOLEAN;
		break;
	case ISEXIST:
		setval(sp, fs != NULL && fs->rcode >= 0);
		break;
	case ISREAD:
		if (getuid() == 0) {
			setval(sp, fs->rcode >= 0);
			break;
		}
		i = permbits(&fs->stat, S_IRUSR, S_IRGRP, S_IROTH);
		goto filebit;
	case ISWRITE:
		if (getuid() == 0) {
			i = S_IWOTH|S_IWGRP|S_IWUSR;
			goto filebit;
		}
		i = permbits(&fs->stat, S_IWUSR, S_IWGRP, S_IWOTH);
		goto filebit;
	case ISEXEC:
		if (getuid() == 0) {
			i = S_IXOTH|S_IXGRP|S_IXUSR;
			goto filebit;
		}
		i = permbits(&fs->stat, S_IXUSR, S_IXGRP, S_IXOTH);
		goto filebit;	/* true if (stat.st_mode & i) != 0 */
	case ISFILE:
		i = S_IFREG;
		goto filetype;
	case ISDIR:
		i = S_IFDIR;
		goto filetype;
	case ISCHAR:
		i = S_IFCHR;
		goto filetype;
	case ISBLOCK:
		i = S_IFBLK;
		goto filetype;
	case ISSYMLINK:
		i = S_IFLNK;
		(void)stat(sp->u.string, (char *)&fs->stat);
		goto filetype;
	case ISFIFO:
		i = S_IFIFO;
		goto filetype;
filetype:
		setval(sp, (fs->stat.st_mode & S_IFMT) == i &&
		    fs->rcode >= 0);
		break;
	case ISSETUID:
		i = S_ISUID;
		goto filebit;
	case ISSETGID:
		i = S_ISGID;
		goto filebit;
	case ISSTICKY:
		i = S_ISVTX;
filebit:
		setval(sp, (fs->stat.st_mode & i) != 0 && fs->rcode >= 0);
		break;
	case ISSIZE:
		sp->u.num = fs->rcode >= 0 ? FSIZE(fs->stat) : 0L;
		sp->type = INTEGER;
		break;
	case ISTTY:
		sp->u.num = isatty(sp->u.num);
		sp->type = BOOLEAN;
		break;
	case NULSTR:
		setval(sp, sp->u.string[0] == '\0');
		break;
	case STRLEN:
		sp->u.num = strlen(sp->u.string);
		sp->type = INTEGER;
		break;
	case OR1:
	case AND1:
		/*
		 * These operators are mostly handled by the parser.  If we
		 * get here it means that both operands were evaluated, so
		 * the value is the value of the second operand.
		 */
		sp->type = (sp + 1)->type;
		if (sp->type == STRING)
			sp->u.string = (sp + 1)->u.string;
		else
			sp->u.num = (sp + 1)->u.num;
		break;
	case STREQ:
	case STRNE:
		i = 0;
		if (!strcmp(sp->u.string, (sp + 1)->u.string))
			i++;
		if (op == STRNE)
			i = 1 - i;
		sp->u.num = i;
		sp->type = BOOLEAN;
		break;
	case EQ:
		setval(sp, sp->u.num == (sp + 1)->u.num);
		break;
	case NE:
		setval(sp, sp->u.num != (sp + 1)->u.num);
		break;
	case GT:
		setval(sp, sp->u.num > (sp + 1)->u.num);
		break;
	case LT:
		setval(sp, sp->u.num < (sp + 1)->u.num);
		break;
	case LE:
		setval(sp, sp->u.num <= (sp + 1)->u.num);
		break;
	case GE:
		setval(sp, sp->u.num >= (sp + 1)->u.num);
		break;

	}
}

int
lookup_op(name, table)
	char *name;
	char **table;
{
	int i;

	for (i = 0; strcmp(table[i], "ZZZ"); i++)
		if (table[i][1] == name[1] && !strcmp(table[i], name))
			return (i);
	return (-1);
}

int
pos_unary(argv)
	char **argv;
{
	struct filestat fs;
	struct value valp;
	int op, c;
	char *opname;

	opname = *argv;
	if ((op = lookup_op(opname, unary_op)) < 0)
		return (-1);
	c = op_argflag[op];
	opname = argv[1];
	valp.u.string = opname;
	if (c == OP_FILE) {
		fs.name = opname;
		fs.rcode = stat(opname, (char *)&fs.stat);
	} else if (c != OP_STRING)
		return (-1);

	expr_operator(op, &valp, &fs);
	return (valp.u.num == 0);
}

int
pos_binary(argv)
	char  **argv;
{
	struct value v[2];
	int op, c;
	char *opname;

	opname = argv[1];
	if ((op = lookup_op(opname, binary_op)) < 0)
		return (-1);
	op += FIRST_BINARY_OP;
	c = op_argflag[op];

	if (c == OP_INT) {
		get_int(argv[0], &v[0].u.num);
		get_int(argv[2], &v[1].u.num);
	} else {
		v[0].u.string = argv[0];
		v[1].u.string = argv[2];
	}
	expr_operator(op, v, NULL);
	return (v[0].u.num == 0);
}

/*
 * Integer type checking.
 */
void
get_int(v, lp)
	register char *v;
	long *lp;
{

	for (; *v && isspace(*v); ++v);

	if(!*v) {
		long z;

		z = 0;
		*lp = z;
		return;
	}

	if (isdigit(*v) || ((*v == '-' || *v == '+') && isdigit(*(v+1)))) {
		*lp = atol(v);
		return;
	}
	badint(v);
}

void
syntax()
{

	fail("syntax error");
}

void
overflow()
{

	fail("expression is too complex");
}
