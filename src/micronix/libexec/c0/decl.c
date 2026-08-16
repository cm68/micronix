/*
 * declaration parsing and top-level parse driver
 */

#include <stdlib.h>
#include "p1core.h"
#include "p1expr.h"
#include "p1type.h"
#include "p1name.h"
#include "p1lex.h"
#include "p1pblk.h"

/* List of global declarations built during parsing */
static struct name *global = 0;

/*
 * Parse a function definition
 *
 * Processes the function body following a function declarator. This function:
 *   1. Sets up function context for static variable mangling
 *   2. Pushes a new scope for the function body
 *   3. Installs function parameters into level 2 scope
 *   4. Parses the function body statement tree
 *   5. Emits the AST for pass 2 (code generation)
 *   6. Pops the function scope and cleans up
 *
 * Two-phase operation:
 *   Phase 1: Parse body to discover locals, don't build expr trees, don't emit
 *   Phase 2: Parse body, build trees, emit AST
 *
 * Parameter handling:
 *   - Parameters are read from f->type->elem (function type signature)
 *   - New name entries are created at level 2 with funarg kind
 *   - This separates type signature (normalized, name-independent) from
 *     actual parameter symbols (visible in function body)
 *
 * Function-scoped static variables:
 *   - Mangled name format: <file>_<func>_<var>_<counter>
 *   - Allocated in global data section with proper scoping
 *
 * Debug assertions:
 *   - Verifies lexlevel returns to 1 (global) after parsing
 *   - Verifies no local names remain in symbol table
 *
 * Parameters:
 *   f - Function name entry with type signature in f->type
 */

void
parsefunc(struct name *f)
{
	struct name *param;
	struct name *phase1_func = NULL;
	struct name *gravemark;

	/* In phase 2, lookup the function definition from phase 1 to get
	 * its u.locals which contains locals for register allocation.
	 * The 'f' from declare() might be a new entry without u.locals. */
	if (phase == 2) {
		struct name *n;
		for (n = names; n; n = n->chain) {
			if (n->kind == kfdef && n->id == f->id) {
				phase1_func = n;
				break;
			}
		}
		if (phase1_func) {
			f = phase1_func;  /* Use the phase 1 version with u.locals */
		}
	}

#ifdef DEBUG
	if (phase == 2 && VERBOSE(V_PHASE2))
		fdprintf(2, "func %s: exprs=%d\n", nameOf(f->id), exprCurCnt);
#endif

	// Set current function context for static variable name mangling
	curFunc = f;
	// staticCtr is file-global, not reset per function
	shadowCtr = 0;

	/* Snapshot the graveyard: everything popScope parks above this
	 * point belongs to this function's scopes and dies with it. */
	gravemark = deadNames;

	// Push a new scope for the function body
	pushScope(nameOf(f->id));

	// Install function parameters into the scope at level 2
	// Read parameter info from f->type->elem but create NEW entries at level 2
	if (f->type->flags & TF_FUNC) {
		for (param = f->type->elem; param; param = param->next) {
			// Only add parameters with actual names (skip anonymous ones)
			if (param->id) {
				// Create a NEW name entry at level 2 (don't reuse type->elem)
				struct name *pn;

				pn = newName(param->id, kfunarg, param->type, 0);
				/*
				 * register survives the copy.  The declarator kept it
				 * on the prototype entry; this is the entry the
				 * allocator walks, and dropping it here is where the
				 * keyword used to silently die.
				 */
				if (pn && (param->sclass & SC_REGISTER))
					pn->sclass = SC_REGISTER;
			}
		}
	}

	// Consume the function body's opening brace
	expect(BEGIN, ER_S_CC);

	if (phase == 1) {
		/* Phase 1: Skip statement parsing, but capture locals.
		 * Locals have ref_count populated during parseExpr. */
#ifdef DEBUG
		if (VERBOSE(V_PHASE1))
			fdprintf(2, "parsefunc phase1: %s entering statement()\n",
			         nameOf(f->id));
#endif
		hlOpen();     /* nothing has taken HL from the first argument */
		statement();  /* Skips through function body */
#ifdef DEBUG
		if (VERBOSE(V_PHASE1))
			fdprintf(2, "parsefunc phase1: %s done\n", nameOf(f->id));
#endif
		f->kind = kfdef;
		f->u.locals = capLocals();  /* Capture before popScope */
	} else {
		/* Phase 2: Streaming emit - parse one statement at a time,
		 * emit immediately, free immediately.
		 * f->u.locals contains locals from phase 1 for register alloc. */
		emitFuncPre(f);     /* Emit header, params, locals, block prefix */
		statement();        /* Streams: parses, emits each stmt */
	}

	// Consume the function body's closing brace
	expect(END, ER_S_CC);

	// Pop the function scope
	popScope();
	/*
	 * The end of a span.  process() runs the two phases over a
	 * function at a time, so that phase 2 can free its locals before
	 * phase 1 discovers the next function's.
	 */
	spanStop = 1;

	/* Free this function's scope names now instead of at exit: the
	 * graveyard is LIFO, so entries above gravemark are all ours.
	 * Phase 1 already captured copies in f->u.locals; initializer
	 * exprs were freed when consumed; type->elem param entries are
	 * never on the lookup chain, so none of these are referenced. */
	while (deadNames != gravemark) {
		param = deadNames;
		deadNames = param->chain;
		free(param);
#ifdef DEBUG
		nameCurCnt--;
#endif
	}
	/* After phase 2 the function is fully emitted; the phase 1 local
	 * copies have served register allocation and can go too. */
	if (phase == 2 && f->kind == kfdef && f->u.locals) {
		freeLocals(f->u.locals);
		f->u.locals = NULL;
	}

	/*
	 * Debug assertion: verify we're back at global scope and all
	 * locals are cleaned up
	 */
#ifdef DEBUG
	if (lexlevel != 1) {
		fdprintf(2, "ASSERTION FAILED: lexlevel=%d after parsing "
		         "function %s (expected 1)\n", lexlevel, nameOf(f->id));
		fatal(ER_WTF);
	}
	/* Verify no local names remain in symbol table (phase 2 only) */
	/* In phase 1, names are preserved for phase 2 lookup */
	if (phase == 2) {
		struct name *n;
		for (n = names; n; n = n->chain) {
			if (n->level > 1) {
				fdprintf(2, "ASSERTION FAILED: found local name "
				         "'%s' at level %d after parsing "
				         "function %s\n",
				         nameOf(n->id), n->level, nameOf(f->id));
				fatal(ER_WTF);
			}
		}
	}
#endif

	// Clear current function context
	curFunc = NULL;
}

/*
 * Parse storage class specifiers in a declaration
 *
 * Storage class specifiers control visibility, lifetime, and storage
 * location of variables and functions. This function parses any combination
 * of storage class keywords and returns them as a bitmask.
 *
 * Recognized keywords:
 *   - extern:   External linkage (defined elsewhere)
 *   - static:   Internal linkage or persistent local storage
 *   - auto:     Automatic (stack) storage (default for locals)
 *   - register: Request register allocation (hint only)
 *   - typedef:  Type alias declaration
 *
 * Invalid combinations detected:
 *   - extern with static/auto/register (conflicting linkage)
 *   - register with static (conflicting storage)
 *   - static with auto (conflicting storage)
 *   - typedef with any storage class (typedef is not storage)
 *
 * Note: This function only handles parsing. The code generator determines
 * actual storage location based on context (global vs local) and storage
 * class.
 *
 * Returns:
 *   Bitmask of storage class flags (SC_EXTERN, SC_STATIC, etc.)
 */
/* token -> storage class bit mapping */
unsigned char
sclassBit(token_t t)
{
    static unsigned char sc_bit[] = {
        SC_TYPEDEF, SC_AUTO, SC_EXTERN, SC_STATIC, SC_REGISTER
    };
    if (t < SCLASS_MIN || t > SCLASS_MAX)
        return 0;

    t -= SCLASS_MIN;
    return sc_bit[t];
}

unsigned char
parseSclass()
{
	unsigned char ret = 0;
	unsigned char bit;

	while ((bit = sclassBit(cur.type)) != 0) {
		if (ret & bit)
			gripe(ER_P_SC);
		ret |= bit;
		gettoken();
	}
	/* bogosity checks: conflicting storage classes */
	if ((ret & SC_EXTERN) && (ret & (SC_STATIC|SC_AUTO|SC_REGISTER)))
		gripe(ER_P_SC);
	if ((ret & SC_REGISTER) && (ret & SC_STATIC))
		gripe(ER_P_SC);
	if ((ret & SC_STATIC) && (ret & SC_AUTO))
		gripe(ER_P_SC);
	return ret;
}

/*
 * Parse a complete declaration statement
 *
 * Handles variable and function declarations at global or local scope.
 * A declaration consists of a storage class (optional), base type, and
 * one or more declarators separated by commas.
 *
 * Declaration forms:
 *   - Variables:    int x, *p, arr[10];
 *   - Functions:    int foo(int x);
 *   - Typedefs:     typedef int* intptr;
 *   - Initializers: int x = 10, arr[] = {1, 2, 3};
 *
 * Processing:
 *   1. Parse storage class keywords (extern, static, typedef, etc.)
 *   2. Parse base type once (shared across all declarators)
 *   3. For each declarator:
 *      - Parse declarator (name, pointers, arrays, functions)
 *      - Handle initializers (= expr or = { ... })
 *      - Apply storage class
 *      - Emit to AST if global or static
 *      - If function body present ({ ... }), parse function
 *
 * Typedef handling:
 *   - Changes name kind from var to tdef
 *   - Typedefs cannot have initializers or function bodies
 *   - Creates type alias in current scope
 *
 * Function definitions:
 *   - Detected by BEGIN token after function declarator
 *   - Calls parsefunc() to parse body
 *   - Statement tree freed after emission
 *
 * Local variable initializers:
 *   - Added to declInits list for conversion to assignments
 *   - Static locals are initialized in data section, not converted
 *
 * Array size inference:
 *   - char[] = "string" infers size from string length
 *   - int[] = {1, 2, 3} infers size from initializer count
 */
void
declaration()
{
	unsigned char sclass;
	struct type *basetype;
	struct name *v;

#ifdef DEBUG
	if (VERBOSE(V_PHASE1) && phase == 1)
		fdprintf(2, "P1 decl start: cur.type=%d\n", cur.type);
#endif
	/* Parse storage class and base type once at the beginning */
	sclass = parseSclass();
	/* Initialize once, then shared across comma-separated declarators */
	basetype = 0;

	/*
	 * K&R implicit int, after a storage class.
	 *
	 *	extern _exit();		extern foo;		static bar;
	 *
	 * all mean int, and the Bell Labs headers are written that way.
	 * Our own stdio.h declares _exit and exit like this, so every
	 * program that included it failed - silently, with nothing said,
	 * because the parse gave up before anything could report.
	 *
	 * Without a storage class the same spelling never comes here: a
	 * leading SYM is dispatched as an expression statement, so
	 * "foo();" parses as a call and always worked.  It is the storage
	 * class that brings it down this path with no type to work from.
	 *
	 * The test is what follows rather than what is missing.  A
	 * declarator starts with a name, a star or a parenthesis, and any
	 * of those means the type was left out; anything else - struct,
	 * union, unsigned, a type keyword - is left to getbasetype(),
	 * which is the one place that should know what a type looks like.
	 *
	 * Not a typedef name, which this used to claim.  getbasetype()
	 * cannot resolve one and never could: parsebasic() has no case
	 * for SYM and there is no name-to-type lookup in this pass at
	 * all.  It does not need one - cpp dissolves every typedef before
	 * the stream gets here, so a name arriving where a type belongs
	 * is a declarator and nothing else.  That is what makes the test
	 * above safe, and reading it the other way is what makes the
	 * SYM case look dangerous when it is not.  A typedef that does
	 * reach this pass is a pipeline fault and is reported as one -
	 * see the SC_TYPEDEF gripe below.
	 */
	if (sclass && (cur.type == SYM || cur.type == STAR ||
	    cur.type == LPAR))
		basetype = inttype;

	while (1) {
        v = declare(&basetype, 0);
#ifdef DEBUG
	if (VERBOSE(V_PHASE1) && phase == 1 && v)
		fdprintf(2, "P1 decl after declare(%s): cur.type=%d\n",
		         nameOf(v->id), cur.type);
#endif

        /* error recovery: if declare failed, skip to next ; or , */
        if (!v) {
            while (cur.type != SEMI && cur.type != COMMA && cur.type != E_O_F) {
                gettoken();
            }
            if (cur.type == SEMI) {
                gettoken();
                break;
            }
            if (cur.type == COMMA) {
                gettoken();
                continue;
            }
            /* E_O_F */
            break;
        }

        /*
         * A typedef reaching this pass is a pipeline fault: cpp
         * dissolves every one.  Loud, and the declarator carries
         * on as a variable so the error does not cascade.
         */
        if (sclass & SC_TYPEDEF)
            gripe(ER_T_TD);

        if (v->type->flags & TF_FUNC) {
#ifdef DEBUG
            if ((VERBOSE(V_PHASE1) && phase == 1) || (VERBOSE(V_PHASE2) && phase == 2))
                fdprintf(2, "P%d func %s: cur.type=%d (BEGIN=%d)\n",
                         phase, nameOf(v->id), cur.type, BEGIN);
#endif
            if (cur.type == BEGIN) {
                /*
                 * A static definition of a name already declared
                 * extern, which a call above it will have done
                 * implicitly - K&R says an undeclared name called as
                 * a function is extern int, and pfx.c obliges.
                 *
                 * The two halves of the file then disagree about what
                 * the name means, and the calls are not revisited:
                 * phase 1 and phase 2 run per function, so everything
                 * above here has been emitted already, calling _g,
                 * while everything below calls the local label.  One
                 * name, one file, two functions.
                 *
                 * It is only a link error when nothing else in the
                 * program defines that global - and the names that
                 * get made static are the ordinary ones, expand,
                 * compare, lookup, getline.  Let another file define
                 * one of those and it links, quietly, wrong.  So say
                 * so here, where the file has not been written yet;
                 * the source wants the forward declaration that K&R
                 * requires, and then both halves agree.
                 */
                if ((sclass & SC_STATIC) && (v->sclass & SC_EXTERN))
                    gripe(ER_D_RD);
                /* Assign storage class BEFORE parsing function body
                 * so it's available when emitting the AST */
                if (sclass & SC_STATIC) {
                    v->sclass = SC_STATIC;
                    /* Static functions get static_id for S<id> naming */
                    if (!v->static_id)
                        v->static_id = ++staticCtr;
                } else if (sclass & SC_EXTERN) {
                    v->sclass = SC_EXTERN;
                }

                parsefunc(v);

                v->next = global;
                global = v;
                break;
            }
        }

        /*
         * Not a function definition: any function type in this
         * declarator keeps only parameter types, not names.
         * Never slim a type owned by a function definition, though:
         * in phase 2 a prototype resolves to the phase 1 fdef entry,
         * and its type still carries the parameter names the body
         * needs (parsefunc/emitPrmDecls/regalloc bind by name).
         */
        if (v->kind != kfdef)
            slimFnArgs(v->type);

        /*
         * Assign storage class for variables (non-functions or
         * function prototypes)
         */
        if (sclass & SC_STATIC) {
            v->sclass = SC_STATIC;
            /*
             * All statics get static_id for S<id> naming, and it has
             * to be minted HERE, from staticCtr.
             *
             * symDecl hands any name declared in a nested block an id
             * of its own out of shadowCtr, so that two sibling blocks
             * each declaring "b" do not arrive in the hoisted locals
             * list as one name.  Those are spelled L<n>.  But a
             * static is spelled S<n>, and shadowCtr restarts at zero
             * for every function while staticCtr runs the length of
             * the file - so a static declared inside a block took a
             * per-function number into a file-wide namespace:
             *
             *	S1:	.ds 2		counter()'s  static int n
             *	S1:	.ds 16		nested()'s   static char ibuf[16]
             *
             * One label, two variables, sharing storage and writing
             * over each other.  Nothing said a word.
             *
             * Only at block scope: a file-scope static that already
             * carries an id is a redeclaration merged onto the
             * existing name, and it has to keep the number it was
             * emitted against.
             */
            if (!v->static_id || v->level > 1)
                v->static_id = ++staticCtr;
        } else if (sclass & SC_EXTERN) {
            v->sclass = SC_EXTERN;
        } else if (sclass & SC_REGISTER) {
            v->sclass = SC_REGISTER;
        } else if (v->type->flags & TF_FUNC) {
            /*
             * A function declared and not defined has external
             * linkage, whatever scope it was written in - C says so,
             * and there is nowhere else it could live.
             *
             * Falling into the clear below instead left a block-scope
             * one looking like an ordinary local: emitted as a bare
             * name rather than _name, it addressed a frame slot, and
             * the call became "push iy / pop hl / add hl,de / call
             * tramp" - an indirect jump through whatever the frame
             * happened to hold.  "char *file, *s_getmsg(), msg[80];"
             * is how K&R names a routine returning other than int,
             * and the call to it went nowhere.
             */
            v->sclass |= SC_EXTERN;
        } else {
            /* Clear extern flag if this is a definition (not extern decl) */
            v->sclass &= ~SC_EXTERN;
        }

        if (cur.type == ASSIGN) {
            /* Auto initializers only for scalars - cpp splits them into decl + assign.
             * Aggregate auto init (arrays, structs) is not supported. */
            if (lexlevel > 1 && !(sclass & SC_STATIC) &&
                (v->type->flags & (TF_AGGREGATE | TF_ARRAY))) {
                gripe(ER_D_AI);
                /* skip the initializer */
                while (cur.type != SEMI && cur.type != COMMA && cur.type != E_O_F)
                    gettoken();
                goto next_decl;
            }
            doInitlzr(v);
            v->emitted = 1;
            goto next_decl;
        }

		/* Emit global variables and static locals immediately */
		/*
		 * Emit if: (global scope OR static storage) AND not
		 * typedef AND not function def AND not extern declaration
		 * Extern declarations don't define storage - they just
		 * reference symbols defined elsewhere.
		 * Skip if already emitted via streaming (v->emitted).
		 */
		if ((lexlevel == 1 || (sclass & SC_STATIC)) &&
		    v->kind != kfdef &&
		    !(sclass & SC_EXTERN) && !v->emitted) {
			/* Skip function declarations - only emit actual variables */
			if (!(v->type->flags & TF_FUNC)) {
				emitGv(v);
				/* Free initializer after emission to avoid memory buildup */
				if (v->u.init) {
					FreeExpr(v->u.init);
					v->u.init = NULL;
				}
			}
		}

		/*
		 * A declaration nothing refers to.
		 *
		 * cpp scores every name by how often the stream mentions
		 * it, and one mention means the only one is this
		 * declaration: nothing in the file can refer to it, now or
		 * later, so remembering it buys nothing.  Headers are
		 * almost entirely this - pass1's outast.c reaches here
		 * with 258 file-scope names and refers to 36 - and at 37
		 * bytes each they were the bulk of what c0 ran out of.
		 *
		 * Only for declarations that put nothing in the object: a
		 * prototype, or an extern that names storage defined
		 * elsewhere.  A definition is reached by another path (a
		 * function body breaks out of this loop above) or has
		 * already emitted by the time we are here, and its name
		 * has to stay to be emitted against.  Statics stay too,
		 * whatever their score: their S<n> numbering is minted as
		 * they are seen, and a name dropped in one phase and
		 * re-minted in the other would number them differently.
		 */
		if (lexlevel == 1 && !(sclass & SC_STATIC) && !v->static_id &&
		    !v->emitted && v->kind != kfdef && idOnce(v->id) &&
		    ((v->type->flags & TF_FUNC) || (sclass & SC_EXTERN))) {
			dropName(v);
			v = 0;
		}

next_decl:
		if (cur.type == COMMA) {
			gettoken();
			continue;
		}
		if (cur.type == SEMI) {
			gettoken();
			break;
		}
		/* Error recovery: unexpected token, break out of loop */
		break;
	}
}

/*
 * Parse a C source file at global scope
 *
 * This is the top-level entry point for parsing a translation unit.
 * It processes all global declarations (variables, functions, typedefs)
 * until EOF is reached.
 *
 * Initialization:
 *   1. Push global scope (level 1)
 *   2. Initialize basic types (char, int, long, void, etc.)
 *   3. Process declarations until EOF
 *
 * Declaration recognition:
 *   - Type keywords: int, char, struct, etc.
 *   - Storage class: extern, static, typedef, etc.
 *   - Typedef names: Previously declared type aliases
 *
 * Incremental emission:
 *   - Global variables are emitted to AST as they're parsed
 *   - Functions are emitted after their body is parsed
 *   - This avoids buffering entire program in memory
 *
 * Cleanup:
 *   - Pops global scope on completion
 *   - Debug builds verify all names properly cleaned up
 *   - Only basic types (level 0) should remain after parsing
 *
 * Error recovery:
 *   - Unrecognized tokens are skipped to prevent infinite loops
 *   - TOK_NONE tokens (from lexer errors) are consumed and ignored
 */
/*
 * One span: declarations until the end of a function definition, or
 * the end of the file.  The scope belongs to the caller - a span is
 * part of a file, not a file, and the globals it declares have to
 * still be there for the next one.
 */
void
parseSpan()
{

	spanStop = 0;
	while (cur.type != E_O_F) {
		while (cur.type == TOK_NONE) {
			gettoken();
		}
#ifdef DEBUG
		if (VERBOSE(V_PHASE1) && phase == 1 && cur.type == BEGIN)
			fdprintf(2, "P1 parse() found BEGIN at global\n");
#endif
		/* Check if current token looks like start of a declaration */
		/* Also check if it's a typedef name (SYM that's a typedef) */
		if (isTypeToken(cur.type) ||
			cur.type == STATIC || cur.type == REGISTER ||
			cur.type == AUTO || cur.type == EXTERN) {
			declaration();
		} else if (cur.type == ASM) {
			/* Global asm block - get text and emit directly */
			char *text = getAsmText();
			emitGlobalAsm(text);
			free(text);
		} else {
			/* Not a declaration - skip this token to avoid getting stuck */
#ifdef DEBUG
			if (VERBOSE(V_PHASE1) && phase == 1)
				fdprintf(2, "P1 parse() skip: cur.type=%d\n", cur.type);
#endif
			gettoken();
		}
		if (spanStop)
			return;
	}
}

/*
 * The whole file, as one span after another.
 */
void
parse()
{
	pushScope("global");
	while (cur.type != E_O_F)
		parseSpan();
	popScope();

	/*
	 * Debug assertion: verify all allocations have been freed
	 * after parsing file
	 */
#ifdef DEBUG
	if (lexlevel != 0) {
		fdprintf(2, "ASSERTION FAILED: lexlevel=%d after parsing "
		         "file (expected 0)\n", lexlevel);
		fatal(ER_WTF);
	}
	/* Verify only basic types remain in symbol table (level 0) */
	/* Skip in phase 1: names are preserved for phase 2 lookup */
	if (phase == 2) {
		struct name *n;
		int nonBasicCnt = 0;
		for (n = names; n; n = n->chain) {
			if (n->level > 0) {
				fdprintf(2, "WARNING: name '%s' at level %d "
				         "still in symbol table after "
				         "file parse\n",
				         nameOf(n->id), n->level);
				nonBasicCnt++;
			}
		}
		if (nonBasicCnt > 0) {
			fdprintf(2, "ASSERTION FAILED: found %d non-basic "
			         "names after parsing file\n", nonBasicCnt);
			fatal(ER_WTF);
		}
	}
#endif
}


/*
 * Free the graveyard.
 *
 * popScope cannot free a name on the spot because pending AST may
 * still point at it, so it parks it here instead.  At the end of a
 * span that is no longer true: the function has been emitted and its
 * exprs freed, and nothing outside it ever held a name below file
 * scope - capLocals keeps copies, by value, of everything a later
 * pass needs.  So the graveyard only has to hold one function's
 * worth, not the file's.
 *
 * fdef entries own their u.locals copies; initializer exprs were
 * freed when they were emitted.
 */
void
drainGraves()
{
	struct name *n;

	while (deadNames) {
		n = deadNames;
		deadNames = n->chain;
		if (n->kind == kfdef && n->u.locals)
			freeLocals(n->u.locals);
		free(n);
#ifdef DEBUG
		nameCurCnt--;
#endif
	}
}

/*
 * Clean up allocated memory after parsing, preserving basic types
 * Traverse chain and free non-basic (level > 0) names
 */
void
cleanupParse()
{
	struct name *n;

	/* Free non-basic names by traversing until we hit level 0 */
	while (names->level > 0) {
		n = names;
		names = n->chain;
		/* u.locals (for fdef) and u.init (for var) share a union */
		if (n->kind == kfdef) {
			if (n->u.locals)
				freeLocals(n->u.locals);
		} else {
			if (n->u.init)
				FreeExpr(n->u.init);
		}
		if (n->kind != kfunarg) {
			free(n);
#ifdef DEBUG
			nameCurCnt--;
#endif
		}
	}
	/* names now points to first basic type (level 0) */

	/* Whatever the last span left behind. */
	drainGraves();

	/* Types live in the permalloc arena and are never freed. */
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
