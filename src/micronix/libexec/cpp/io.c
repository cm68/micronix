/*
 * this i/o machinery is a unification of macro expansion, include files
 * and source file buffering.  these are stacked, so we can push
 * include files and save our position at each level
 *
 * we don't do any character processing at all at this level.
 * if there are nasty control characters, etc, we pass them up.
 * not our job; except nulls.  those are dirty; the first null is eof.
 */
#define _XOPEN_SOURCE 500   /* for mkstemp */
#include <stdlib.h>
#include <string.h>
#include "cpp.h"

#include <fcntl.h>
#include <unistd.h>

/*
 * the incoming character stream interface
 * a zero is EOF
 */
unsigned char curchar;               /* the current character */
unsigned char nextchar;              /* the next char - can change if macro */
int lineno;                 /* line number for error messages */
char *filename;             /* current file name */
char column;                /* 0=after newline, 1=first char, 2=other */
char nextcol = 0;
char namebuf[128];

/*
 * the formal definition of offset is the first unread character.
 * this is effectively the lookahead character, nextchar.
 * if we have not read anything from this buffer yet, it is zero.
 * advance() places this character into curchar.
 * if we do a macro insertion, it is after curchar
 */
/*
 * Join an include directory to a filename.
 *
 * A directory is usually a path and wants a '/' between the two.  On
 * CP/M it is a drive - "D:" - and a drive letter is already the
 * separator: "D:/lexeme.h" names nothing.  A path that ends in either
 * is left to speak for itself, so -ID: works there and -I/usr/include
 * works here, and a source can say plain "lexeme.h" on both.
 *
 * This is the whole of the difference between a system with
 * directories and one without, as far as an include path is
 * concerned.
 */
static void
joinpath(char *out, char *dir, char *name)
{
    int n = strlen(dir);

    if (n && (dir[n - 1] == '/' || dir[n - 1] == ':'))
        fmtstr(out, "%s%s", dir, name);
    else
        fmtstr(out, "%s/%s", dir, name);
}

struct textbuf *tbtop;

#ifdef DEBUG

/*
 * Dump current character state for debugging
 *
 * Outputs the current and next characters along with the top textbuf
 * state to stderr for I/O debugging. Non-printable characters are shown
 * in hex format.
 *
 * Parameters:
 *   tag - Descriptive label for this dump point (e.g., "advance", "before")
 */
void
cdump(char *tag)
{
    struct textbuf *t = tbtop;
    char cs[20];
    char ns[20];
    char tbuf[100];

    if (!(VERBOSE(V_IO))) {
        return;
    }

    if (tag) {
        fdprintf(2,"%s:\n", tag);
    }
    if (curchar <= ' ') {
        sprintf(cs, "0x%x", curchar);
    } else {
        sprintf(cs, "\'%c\'", curchar);
    }
    if (nextchar <= ' ') {
        sprintf(ns, "0x%x", nextchar);
    } else {
        sprintf(ns, "\'%c\'", nextchar);
    }
    if (t) {
    	sprintf(tbuf, "%s offset: 0x%x valid: 0x%x cs: %s ns: %s",
    			tag, tbtop->offset, tbtop->valid, cs, ns);
    	hexdump(tbuf, t->storage, t->valid);
    }
    
} 
#else
#define cdump(x)
#endif

#ifdef DEBUG
int tbdepth, tbpeak;	/* live textbufs, high-water */
int incstat(int *bytes);
#endif

struct include {
    char *path;
    struct include *next;
} *includes;

void
die(char *fmt, char *arg)
{
    char buf[140];
    fmtstr(buf, fmt, arg);
    write(2, buf, strlen(buf));
    exit(1);
}

/*
 * System include path for #include <foo.h>
 */
/*
 * Where <foo.h> is looked for when nothing on the command line says.
 *
 * On a target it is /usr/include, which is where the headers are on
 * both micronix and CP/M.  In the host build there is no target
 * filesystem to read, so it is the tree the headers are built from.
 * That is a question about which machine this cpp runs on, not about
 * which compiler built it, and it asks the host compiler now.
 */
#ifdef __GNUC__
char *sysIncPath = "libsrc/include";
#else
char *sysIncPath = "/usr/include";
#endif

/*
 * Add a path to the include file search list
 *
 * Appends a directory path to the end of the include search list.
 * When processing #include directives, these paths are searched in order
 * to locate header files.
 *
 * The include list is searched for:
 *   - #include "file.h" directives (quoted form)
 *   - #include <file.h> directives (after trying sysIncPath)
 *
 * Search order:
 *   1. System path (for <> includes only)
 *   2. Include list paths in order added (first match wins)
 *
 * Parameters:
 *   s - Directory path to add (string is duplicated)
 */
#ifdef DEBUG
int
incstat(int *bytes)
{
    struct include *i;
    int c = 0;
    *bytes = 0;
    for (i = includes; i; i = i->next) {
        c++;
        *bytes += strlen(i->path) + 1;
    }
    return c;
}
#endif

void
addInclude(char *s)
{
    struct include *i, *ip;
    i = (struct include *)permalloc(sizeof(*i));
    i->path = permdup(s);
    i->next = 0;
    if (includes) {
        ip = includes;
        while (ip->next) {
            ip = ip->next;
        }
        ip->next = i;
    } else {
        includes = i;
    }
#ifdef DEBUG
    if (VERBOSE(V_CPP)) {
        fdprintf(2,"addInclude: %s\n", s);
    }
#endif
}

/*
 * Push a source file onto the input stack (main file, no path search)
 */
void
pushfile(char *name)
{
    struct textbuf *t;

    t = (struct textbuf *)xalloc(sizeof(*t));
    t->fd = open(name, 0);
    if (t->fd < 0)
        die("cannot open: %s\n", name);
    t->name = intern(name);
    t->offset = t->valid = 0;
    t->lineno = 1;
    t->storage = xalloc(TBSIZE);
    t->saved_column = column;
    t->prev = tbtop;
    tbtop = t;
    filename = t->name;
    lineno = 1;
}

/*
 * Push an include file onto the input stack
 *
 * Opens and pushes a new include file onto the textbuf stack, pausing
 * processing of the current file. The include file is searched using the
 * configured include paths.
 *
 * Search strategy:
 *   - sys=1 (<file.h>): Try sysIncPath first, then include list
 *   - sys=0 ("file.h"): Try include list paths only
 *   - Empty path in list means current directory
 *   - First file found is used (search stops)
 *
 * State preservation:
 *   - Current file position (offset) is saved in textbuf
 *   - Current column position is saved
 *   - Parent state restored when include file is exhausted
 *
 * New file initialization:
 *   - Line number starts at 1
 *   - Filename updated to resolved path
 *   - Empty buffer allocated (TBSIZE)
 *   - File descriptor opened for reading
 *
 * Error handling:
 *   - Fatal error if file not found in any search path
 *   - Sets filename and lineno for error message context
 *
 * Parameters:
 *   name - Include file name (relative or basename)
 *   sys  - 1 for <file.h> (system), 0 for "file.h" (user)
 */
void
insertfile(char *name, int sys)
{
	struct textbuf *t;
    struct include *i;


#ifdef DEBUG
    if (VERBOSE(V_IO)) {
        fdprintf(2,
            "insertfile: %s sys=%d curchar='%c'(0x%x) "
            "nextchar='%c'(0x%x) column=%d offset=%d\n",
            name, sys,
            curchar >= 32 ? curchar : '?', curchar,
            nextchar >= 32 ? nextchar : '?', nextchar,
            column,
            tbtop ? tbtop->offset : -1);
    }
#endif

	t = (struct textbuf *)xalloc(sizeof(*t));
    t->fd = -1;  /* Initialize to indicate "not opened yet" */

    /*
     * For system includes (<foo.h>), try system include path first
     */
    if (sys && sysIncPath) {
        joinpath(namebuf, sysIncPath, name);
        t->fd = open(namebuf, 0);
        if (t->fd > 0)
            goto found;
    }

    /*
     * try the filename in all the include path entries. first hit wins
     */
    for (i = includes; i; i = i->next) {
        if (i->path[0])
            joinpath(namebuf, i->path, name);
        else
            strcpy(namebuf, name);
        t->fd = open(namebuf, 0);
        if (t->fd > 0)
            break;
    }
    if (t->fd == -1)
        die("cannot find include file: %s\n", name);
found:
#ifdef DEBUG
    if (VERBOSE(V_IO))
        fdprintf(2, "resolved: %s\n", namebuf);
#endif
	t->name = intern(namebuf);
	t->offset = 0;
	t->lineno = 1;  /* New file starts at line 1 */
	t->storage = xalloc(TBSIZE);
	t->saved_column = column;  /* Save parent's column */
	t->prev = tbtop;
	tbtop = t;
#ifdef DEBUG
	if (++tbdepth > tbpeak) tbpeak = tbdepth;
#endif
    filename = t->name;  /* Use resolved path */
    lineno = 1;  /* Start at line 1 for new file */
    /* Read first chars from new file directly into curchar/nextchar */
    t->valid = read(t->fd, t->storage, TBSIZE);
    if (t->valid > 0) {
        curchar = t->storage[t->offset++];
        nextchar = (t->offset < t->valid) ? t->storage[t->offset] : 0;
        /* curchar is at column 0, next will be column 1 */
        column = 0;
        nextcol = 1;
    } else {
        curchar = nextchar = 0;
        column = nextcol = 0;
    }
}

/*
 * Insert macro expansion text into the input stream
 *
 * Pushes macro expansion text onto the textbuf stack, effectively inserting
 * it BETWEEN curchar and nextchar. This allows nested macro expansion where
 * macros within the expansion text are recursively expanded.
 *
 * Optimization:
 *   If the macro text fits in the already-read portion of the current buffer
 *   (before offset), the text is copied there and offset is backed up. This
 *   avoids allocating a new textbuf for short macros.
 *
 * Optimization conditions:
 *   - Current textbuf has space before offset (offset > macro length)
 *   - Macro text is copied to [offset-length ... offset-1]
 *   - Offset backed up to start of macro text
 *   - curchar/nextchar updated to first characters of macro
 *
 * New textbuf allocation (if optimization doesn't apply):
 *   - Allocate new textbuf with fd=-1 (not a file)
 *   - Duplicate macro text as storage
 *   - Set valid to text length
 *   - Push onto textbuf stack
 *   - Save parent's column position
 *
 * Insertion point:
 *   - Macro text appears BETWEEN curchar and nextchar
 *   - curchar remains unchanged (already consumed)
 *   - nextchar becomes first character of macro
 *   - Parent's nextchar restored after macro exhausted
 *
 * Parameters:
 *   name   - Macro name (for debugging/error context)
 *   macbuf - Expanded macro text (parameter substitutions already done)
 */
void
insertmacro(char *name, char *macbuf)
{
	struct textbuf *t;
    int l;


    l = strlen(macbuf);         /* our macro without the terminating null */
#ifdef DEBUG
    if (VERBOSE(V_MACRO)) {
        fdprintf(2,"insert macro %s %d $%s$\n", name, l, macbuf);
    }
#endif
    /*
     * An empty expansion is exactly one advance: the name vanishes
     * and the stream continues.  The rewind path below did this by
     * hand - curchar = storage[offset++]; nextchar = storage[offset]
     * - without the refill guard, so an empty macro whose position
     * sat at the end of a 256-byte read block took nextchar from
     * storage[256]: a byte that was never part of the file.  What
     * that phantom byte broke depended on what happened to sit
     * there, which made the symptom move with every unrelated edit.
     */
    if (l == 0) {
        advance();
        return;
    }

    t = tbtop;

    /* does it fit */
    if (t->offset > l) {
        cdump("before");
        t->offset -= l;
        strncpy(&t->storage[t->offset], macbuf, l);
        curchar = t->storage[t->offset++];
        nextchar = t->storage[t->offset];
        cdump("after");
        return;
    }
 
    /* if it does not */
	t = (struct textbuf *)xalloc(sizeof(*t));
	t->storage = xalloc(l + 1);	/* loud, not a NULL strlen later */
	t->fd = -1;
	/*
	 * The buffer keeps the PARENT's name: macro text has no file of
	 * its own, and naming the buffer after the macro leaked markers
	 * like '# 178 "NSEEK"' into the output mid-statement whenever an
	 * expansion spilled here instead of fitting the rewind path.
	 */
	t->name = tbtop ? tbtop->name : filename;
	t->lineno = lineno;
	t->offset = 0;
	strcpy(t->storage, macbuf);
	t->valid = l;
	t->saved_column = column;  /* Save parent's column */
	t->prev = tbtop;
	tbtop = t;
#ifdef DEBUG
	if (++tbdepth > tbpeak) tbpeak = tbdepth;
#endif
    /* Set curchar/nextchar to first characters of macro text */
    curchar = t->storage[t->offset++];
    nextchar = t->storage[t->offset];
}

/*
 * Advance to the next character in the input stream
 *
 * This is the core I/O function that implements the unified character stream
 * from files, include files, and macro expansions. It manages the textbuf
 * stack, handles EOF/buffer exhaustion, and updates line/column tracking.
 *
 * Character flow:
 *   1. Move nextchar -> curchar
 *   2. Read new nextchar from current textbuf
 *   3. If buffer exhausted, refill from file or pop textbuf
 *   4. Update line/column counters
 *
 * Textbuf stack management:
 *   - If current buffer has more data: read next character
 *   - If buffer exhausted and file open: refill buffer from file
 *   - If file exhausted or macro empty: pop textbuf, restore parent state
 *   - If no parent textbuf: nextchar=0 (EOF)
 *
 * State restoration when popping:
 *   - Restore parent's column position
 *   - Restore parent's line number
 *   - Restore parent's filename
 *   - Read nextchar from parent's current position
 *   - Parent's curchar remains unchanged (important!)
 *
 * Line/column tracking:
 *   - column: Position of curchar (current character)
 *   - nextcol: Position where nextchar will be (future column)
 *   - Newline: Increments lineno, resets nextcol to 0
 *   - Tab: Converted to space in nextchar
 *
 * Special handling:
 *   - Tabs normalized to spaces
 *   - Null bytes treated as EOF
 *   - Line numbers synchronized with textbuf
 *
 * Side effects:
 *   - Updates curchar, nextchar
 *   - Updates column, nextcol, lineno
 *   - May pop textbufs and free memory
 *   - Updates filename for error context
 */
static void advslow();

/*
 * The common character, in assembly.
 *
 * advance() is called once per character - 279,245 times over rules.c,
 * the top entry in the profile - and takes no arguments, all of its
 * state being globals, so there is nothing to marshal and the block
 * below can stand at the head of the function.
 *
 * It handles only the case that is nearly always true: a textbuf with
 * another character already in its buffer, where that character needs
 * none of the bookkeeping at done: beyond the column.  Anything else -
 * end of buffer, end of file, a newline, a NUL, a backslash that might
 * begin a continuation, a tab to fold - falls through to advslow(),
 * which is the C this function used to be, unchanged.
 *
 * Nothing is written until every test has passed, so a fall-through
 * leaves the state exactly as it was found.
 *
 * The offsets are struct textbuf's, by hand - storage 3, offset 5,
 * valid 7 - which is the price of an asm block that cannot see a C
 * declaration.  The note over the struct in cpp.h is what keeps them
 * honest.  bc and ix are the compiler's to keep, so this uses only hl,
 * de and a.
 */
#ifdef CCC

void
advance()
{
    /* storage, offset and valid are adjacent: address once, walk it */
    asm {
	ld	hl,(_tbtop)
	ld	a,h
	or	l
	jr	z,advc9
	ld	de,3
	add	hl,de
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	push	de
	inc	hl
	push	hl
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	de
	inc	hl
	ld	a,(hl)
	inc	hl
	ld	h,(hl)
	ld	l,a
	or	a
	sbc	hl,de
	jp	m,advc8a
	ld	a,h
	or	l
	jr	z,advc8a
    }
    /* the character itself, and the four done: has work for */
    asm {
	pop	hl
	ex	(sp),hl
	add	hl,de
	ld	a,(hl)
	ld	l,a
	ld	a,(_nextchar)
	or	a
	jr	z,advc8b
	cp	10
	jr	z,advc8b
	cp	92
	jr	z,advc8b
	ld	h,a
	ld	a,l
	cp	9
	jr	z,advc8b
    }
    /* nothing has been written until here */
    asm {
	ld	(_nextchar),a
	ld	a,h
	ld	(_curchar),a
	pop	hl
	ld	(hl),e
	inc	hl
	ld	(hl),d
	ld	a,(_nextcol)
	ld	(_column),a
	cp	2
	jr	nc,advc7
	inc	a
	ld	(_nextcol),a
advc7:
	ret
advc8a:
	pop	hl
advc8b:
	pop	hl
advc9:
    }
    advslow();
}

#else

/*
 * The host build has no use for it - this binary is a cross tool, and
 * gcc does not read ccc's asm blocks.
 */
void
advance()
{
    advslow();
}

#endif

static void
advslow()
{
	struct textbuf *t;

again:
    t = tbtop;

    curchar = nextchar;

    /* if no textbuf, are at eof */
    if (!t) {
        nextchar = 0;
        goto done;
    }

    /* do we have a valid nextchar? */
	if (t->offset + 1 < t->valid) {
            nextchar = t->storage[++t->offset];
#ifdef DEBUG
            if (VERBOSE(V_IO)) {
                fdprintf(2,
                    "Read nextchar from buffer: '%c'(0x%x) at "
                    "offset %d\n",
                    nextchar >= 32 ? nextchar : '?', nextchar,
                    t->offset);
            }
#endif
            goto done;
	}

    /* if we have a file open, read some more of it */
    if (t->fd != -1) {
        t->valid = read(t->fd, t->storage, TBSIZE);
        t->offset = 0;
        if (t->valid > 0) { /* read worked */
            nextchar = t->storage[0];
            goto done;
        }
        close(t->fd);
        t->fd = -1;  /* Mark as closed, will pop on next advance */
        nextchar = 0;  /* No more chars after current */
        goto done;  /* Return current char, pop on next call */
    }
    /* closed file or empty macro buffer - pop */
    tbtop = t->prev;
    if (tbtop) {
#ifdef DEBUG
        if (VERBOSE(V_IO)) {
            fdprintf(2,"Popping from %s, restoring column from %d to %d\n",
                   t->name, column, t->saved_column);
        }
#endif
        /* Restore parent's state - reset column to 0 so next # is recognized */
        column = 0;
        nextcol = 0;
        lineno = tbtop->lineno;
        filename = tbtop->name;

        /*
         * Read nextchar from parent buffer WITHOUT doing
         * curchar=nextchar again.
         */
        if (tbtop->offset < tbtop->valid) {
            nextchar = tbtop->storage[tbtop->offset];
        } else if (tbtop->fd != -1) {
            /* Need to read more from parent file */
            tbtop->valid = read(tbtop->fd, tbtop->storage, TBSIZE);
            tbtop->offset = 0;
            if (tbtop->valid > 0) {
                nextchar = tbtop->storage[0];
            } else {
                nextchar = 0;
            }
        } else {
            nextchar = 0;
        }
    }
    free(t->storage);
    /* t->name is interned (pool-owned) - tokens may still reference it. */
    free(t);
#ifdef DEBUG
    tbdepth--;
#endif
    if (!tbtop) {
        /* No parent textbuf, we're at EOF */
        nextchar = 0;
    } else if (curchar == 0) {
        /*
         * curchar became 0 from end of macro buffer.
         * Re-run advance with parent buffer to get proper curchar.
         */
        goto again;
    }
done:
    column = nextcol;
    if (curchar == 0) {
        nextcol = 0;
    } else if (curchar == '\n') {
        nextcol = 0;
        lineno++;
        if (tbtop) {
            tbtop->lineno = lineno;  /* Keep textbuf lineno in sync */
        }
    } else {
        if (nextcol < 2) nextcol++;
    }
    if (nextchar == '\t') nextchar = ' ';

    /* Handle backslash-newline line continuation (C translation phase 2).
     * When we see '\' followed by '\n', skip both and get the next char.
     * This makes line continuations invisible to the rest of the lexer.
     * First advance() skips the backslash and processes the newline (updating
     * lineno), second advance() gets the real next character. */
    if (curchar == '\\' && nextchar == '\n') {
        advance();  /* Skip '\', get '\n' as curchar (lineno++ in done:) */
        advance();  /* Skip '\n', get next real char as curchar */
        return;
    }

#ifdef DEBUG
    if (VERBOSE(V_IO)) {
        fdprintf(2,
            "After done: curchar='%c'(0x%x) nextchar='%c'(0x%x) "
            "column=%d nextcol=%d\n",
            curchar >= 32 ? curchar : '?', curchar,
            nextchar >= 32 ? nextchar : '?', nextchar,
            column, nextcol);
    }
#endif
    cdump("advance");
}

/*
 * Initialize the I/O system for a new source file
 *
 * "Primes the pump" by calling advance() twice to load curchar and
 * nextchar with the first two characters from the input. This establishes
 * the two-character lookahead used throughout the lexer.
 *
 * Initialization sequence:
 *   1. Set line number to 1
 *   2. First advance(): loads curchar with first character
 *   3. Second advance(): loads nextchar with second character
 *   4. Reset column to 0 for start of file
 *
 * Called after insertfile() to begin reading the first (or included) file.
 */
void
ioinit()
{
    lineno = 1;
    advance();
    advance();
    column = 0;
    nextcol = 1;
}

/*
 * Write straight to the lexeme stream.  An output-buffer stack
 * (outbufPush/Pop/Replay + spill-to-tempfile) used to live here for
 * deferred/reordered emission, but nothing ever pushed.  All token
 * reordering is now done in the filter pipeline (pendbuf), so direct
 * write is sufficient.
 */
void
outbufWrite(void *data, int len)
{
    extern char lexFd;
    write(lexFd, data, len);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
