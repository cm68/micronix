/*
 * lexdata.c - Token classification data
 *
 * Sized to KW_LAST+1 (161 entries).  Filters only see lexer-produced
 * tokens (0..160); synthetic AST tokens (200+) never reach token_props.
 *
 * Traditional positional initializer (no C99 designated init).
 * Layout: indices match the token numbers defined in lexeme.h.
 *   0      E_O_F
 *   1..9   delimiters (SEMI BEGIN END LBRACK RBRACK LPAR RPAR COLON COMMA)
 *   10..127 operators / terminals / line tracking (all zero)
 *   128..160 keywords (TYPE/COND/DO/ELSE flags)
 */
#include "lexeme.h"

unsigned char token_props[KW_LAST + 1] = {
	0,                    /*   0 E_O_F   */
	TF_TERM,              /*   1 SEMI    */
	TF_OPEN,              /*   2 BEGIN   */
	TF_TERM | TF_CLOSE,   /*   3 END     */
	TF_OPEN,              /*   4 LBRACK  */
	TF_CLOSE,             /*   5 RBRACK  */
	TF_OPEN,              /*   6 LPAR    */
	TF_CLOSE,             /*   7 RPAR    */
	0, 0,                 /*   8 COLON,  9 COMMA */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /*  10- 19 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /*  20- 29 (SYM..LNUMBER + ops) */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /*  30- 39 (unary/binary ops)   */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /*  40- 49 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /*  50- 59 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /*  60- 69 (relops)             */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /*  70- 79 (assign ops)         */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /*  80- 89 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /*  90- 99 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 100-109 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 110-119 (LABEL/LINENO/etc)   */
	0, 0, 0, 0, 0, 0, 0, 0,          /* 120-127 */
	TF_TYPE,              /* 128 INT     */
	TF_TYPE,              /* 129 CHAR    */
	0, 0,                 /* 130,131 reserved - see lexeme.h */
	TF_TYPE,              /* 132 STRUCT  */
	TF_TYPE,              /* 133 SIGNED  */
	TF_TYPE,              /* 134 LONG    */
	TF_TYPE,              /* 135 UNSIGNED*/
	TF_TYPE,              /* 136 UNION   */
	TF_TYPE,              /* 137 SHORT   */
	TF_TYPE,              /* 138 VOID    */
	TF_TYPE,              /* 139 ENUM    */
	TF_TYPE,              /* 140 TYPEDEF */
	TF_TYPE,              /* 141 AUTO    */
	TF_TYPE,              /* 142 EXTERN  */
	TF_TYPE,              /* 143 STATIC  */
	TF_TYPE,              /* 144 REGISTER*/
	0,                    /* 145 GOTO    */
	0,                    /* 146 RETURN  */
	TF_COND,              /* 147 IF      */
	TF_COND,              /* 148 WHILE   */
	TF_ELSE,              /* 149 ELSE    */
	TF_COND,              /* 150 SWITCH  */
	0,                    /* 151 CASE    */
	0,                    /* 152 BREAK   */
	0,                    /* 153 CONTINUE*/
	TF_DO,                /* 154 DO      */
	0,                    /* 155 DEFAULT */
	TF_COND,              /* 156 FOR     */
	0,                    /* 157 ASM     */
	TF_TYPE,              /* 158 CONST   */
	TF_TYPE               /* 159 VOLATILE*/
	/* 160 SIZEOF_KW omitted - trailing zero */
};
