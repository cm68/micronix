/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOK_BracketOnly = 258,
     TypeUndef = 259,
     TypeEndLine = 260,
     TypeComment = 261,
     TypeNumber = 262,
     TypeLiteral = 263,
     TypeIdentifier = 264,
     TypeMacroName = 265,
     TypeMacroArg = 266,
     TypeEndOfINCLUDE = 267,
     TypeNoFileINCLUDE = 268,
     TypeWhiteSpace = 269,
     TypeMOD = 270,
     TypeSHL = 271,
     TypeSHR = 272,
     TypeNOT = 273,
     TypeEQ = 274,
     TypeLT = 275,
     TypeLE = 276,
     TypeGT = 277,
     TypeGE = 278,
     TypeNE = 279,
     TypeNUL = 280,
     TypeDEFINED = 281,
     TypeHIGH = 282,
     TypeLOW = 283,
     TypeShlOp = 284,
     TypeShrOp = 285,
     TypeLeOp = 286,
     TypeGeOp = 287,
     TypeNeOp = 288,
     TypeBoolAnd = 289,
     TypeBoolOr = 290,
     TypeSharpSharp = 291,
     TypeADC = 292,
     TypeADD = 293,
     TypeAND = 294,
     TypeBIT = 295,
     TypeCALL = 296,
     TypeCCF = 297,
     TypeCP = 298,
     TypeCPD = 299,
     TypeCPDR = 300,
     TypeCPI = 301,
     TypeCPIR = 302,
     TypeCPL = 303,
     TypeDAA = 304,
     TypeDEC = 305,
     TypeDI = 306,
     TypeDJNZ = 307,
     TypeEI = 308,
     TypeEX = 309,
     TypeEXX = 310,
     TypeHALT = 311,
     TypeIM = 312,
     TypeIN = 313,
     TypeINC = 314,
     TypeIND = 315,
     TypeINDR = 316,
     TypeINI = 317,
     TypeINIR = 318,
     TypeJP = 319,
     TypeJR = 320,
     TypeLD = 321,
     TypeLDD = 322,
     TypeLDDR = 323,
     TypeLDI = 324,
     TypeLDIR = 325,
     TypeNEG = 326,
     TypeNOP = 327,
     TypeOR = 328,
     TypeOTDR = 329,
     TypeOTIR = 330,
     TypeOUT = 331,
     TypeOUTD = 332,
     TypeOUTI = 333,
     TypePOP = 334,
     TypePUSH = 335,
     TypeRES = 336,
     TypeRET = 337,
     TypeRETI = 338,
     TypeRETN = 339,
     TypeRL = 340,
     TypeRLA = 341,
     TypeRLC = 342,
     TypeRLCA = 343,
     TypeRLD = 344,
     TypeRR = 345,
     TypeRRA = 346,
     TypeRRC = 347,
     TypeRRCA = 348,
     TypeRRD = 349,
     TypeRST = 350,
     TypeSBC = 351,
     TypeSCF = 352,
     TypeSET = 353,
     TypeSLA = 354,
     TypeSLL = 355,
     TypeSRA = 356,
     TypeSRL = 357,
     TypeSUB = 358,
     TypeXOR = 359,
     TypeA = 360,
     TypeAF = 361,
     TypeAFp = 362,
     TypeB = 363,
     TypeBC = 364,
     TypeD = 365,
     TypeE = 366,
     TypeDE = 367,
     TypeH = 368,
     TypeL = 369,
     TypeHL = 370,
     TypeSP = 371,
     TypeIX = 372,
     TypeIXH = 373,
     TypeIXL = 374,
     TypeIY = 375,
     TypeIYH = 376,
     TypeIYL = 377,
     TypeI = 378,
     TypeR = 379,
     TypeNZ = 380,
     TypeZ = 381,
     TypeNC = 382,
     TypeC = 383,
     TypePO = 384,
     TypePE = 385,
     TypeP = 386,
     TypeM = 387,
     TypeASEG = 388,
     TypeCOMMON = 389,
     TypeCSEG = 390,
     TypeDB = 391,
     TypeDEFB = 392,
     TypeDEFL = 393,
     TypeDEFM = 394,
     TypeDEFS = 395,
     TypeDEFW = 396,
     TypeDS = 397,
     TypeDSEG = 398,
     TypeDW = 399,
     TypeELSE = 400,
     TypeEND = 401,
     TypeENDIF = 402,
     TypeENDM = 403,
     TypeENDP = 404,
     TypeEQU = 405,
     TypeEXITM = 406,
     TypeEXTRN = 407,
     TypeIF = 408,
     TypeIF1 = 409,
     TypeIF2 = 410,
     TypeIFDEF = 411,
     TypeIFNDEF = 412,
     TypeINCBIN = 413,
     TypeINCLUDE = 414,
     TypeIRP = 415,
     TypeIRPC = 416,
     TypeLOCAL = 417,
     TypeMACRO = 418,
     TypeORG = 419,
     TypePROC = 420,
     TypePUBLIC = 421,
     TypeREPT = 422,
     Type_8080 = 423,
     Type_8086 = 424,
     Type_DEPHASE = 425,
     Type_ERROR = 426,
     Type_PHASE = 427,
     Type_SHIFT = 428,
     Type_WARNING = 429,
     Type_Z80 = 430,
     TypeAND_8080 = 431,
     TypeOR_8080 = 432,
     TypeXOR_8080 = 433,
     TypeM_8080 = 434,
     TypePSW_8080 = 435,
     TypeSET_8080 = 436,
     TypeADC_8080 = 437,
     TypeADD_8080 = 438,
     TypeACI_8080 = 439,
     TypeADI_8080 = 440,
     TypeANA_8080 = 441,
     TypeANI_8080 = 442,
     TypeCALL_8080 = 443,
     TypeCC_8080 = 444,
     TypeCM_8080 = 445,
     TypeCMA_8080 = 446,
     TypeCMC_8080 = 447,
     TypeCMP_8080 = 448,
     TypeCNC_8080 = 449,
     TypeCNZ_8080 = 450,
     TypeCP_8080 = 451,
     TypeCPE_8080 = 452,
     TypeCPI_8080 = 453,
     TypeCPO_8080 = 454,
     TypeCZ_8080 = 455,
     TypeDAD_8080 = 456,
     TypeDCR_8080 = 457,
     TypeDCX_8080 = 458,
     TypeHLT_8080 = 459,
     TypeIN_8080 = 460,
     TypeINR_8080 = 461,
     TypeINX_8080 = 462,
     TypeJC_8080 = 463,
     TypeJM_8080 = 464,
     TypeJMP_8080 = 465,
     TypeJNC_8080 = 466,
     TypeJNZ_8080 = 467,
     TypeJP_8080 = 468,
     TypeJPE_8080 = 469,
     TypeJPO_8080 = 470,
     TypeJZ_8080 = 471,
     TypeLDA_8080 = 472,
     TypeLDAX_8080 = 473,
     TypeLHLD_8080 = 474,
     TypeLXI_8080 = 475,
     TypeMVI_8080 = 476,
     TypeMOV_8080 = 477,
     TypeORA_8080 = 478,
     TypeORI_8080 = 479,
     TypeOUT_8080 = 480,
     TypePCHL_8080 = 481,
     TypePOP_8080 = 482,
     TypePUSH_8080 = 483,
     TypeRAL_8080 = 484,
     TypeRAR_8080 = 485,
     TypeRLC_8080 = 486,
     TypeRC_8080 = 487,
     TypeRRC_8080 = 488,
     TypeRET_8080 = 489,
     TypeRM_8080 = 490,
     TypeRNC_8080 = 491,
     TypeRNZ_8080 = 492,
     TypeRP_8080 = 493,
     TypeRPE_8080 = 494,
     TypeRPO_8080 = 495,
     TypeRST_8080 = 496,
     TypeRZ_8080 = 497,
     TypeSBB_8080 = 498,
     TypeSBI_8080 = 499,
     TypeSHLD_8080 = 500,
     TypeSPHL_8080 = 501,
     TypeSTA_8080 = 502,
     TypeSTAX_8080 = 503,
     TypeSTC_8080 = 504,
     TypeSUB_8080 = 505,
     TypeSUI_8080 = 506,
     TypeXCHG_8080 = 507,
     TypeXRA_8080 = 508,
     TypeXRI_8080 = 509,
     TypeXTHL_8080 = 510
   };
#endif
/* Tokens.  */
#define TOK_BracketOnly 258
#define TypeUndef 259
#define TypeEndLine 260
#define TypeComment 261
#define TypeNumber 262
#define TypeLiteral 263
#define TypeIdentifier 264
#define TypeMacroName 265
#define TypeMacroArg 266
#define TypeEndOfINCLUDE 267
#define TypeNoFileINCLUDE 268
#define TypeWhiteSpace 269
#define TypeMOD 270
#define TypeSHL 271
#define TypeSHR 272
#define TypeNOT 273
#define TypeEQ 274
#define TypeLT 275
#define TypeLE 276
#define TypeGT 277
#define TypeGE 278
#define TypeNE 279
#define TypeNUL 280
#define TypeDEFINED 281
#define TypeHIGH 282
#define TypeLOW 283
#define TypeShlOp 284
#define TypeShrOp 285
#define TypeLeOp 286
#define TypeGeOp 287
#define TypeNeOp 288
#define TypeBoolAnd 289
#define TypeBoolOr 290
#define TypeSharpSharp 291
#define TypeADC 292
#define TypeADD 293
#define TypeAND 294
#define TypeBIT 295
#define TypeCALL 296
#define TypeCCF 297
#define TypeCP 298
#define TypeCPD 299
#define TypeCPDR 300
#define TypeCPI 301
#define TypeCPIR 302
#define TypeCPL 303
#define TypeDAA 304
#define TypeDEC 305
#define TypeDI 306
#define TypeDJNZ 307
#define TypeEI 308
#define TypeEX 309
#define TypeEXX 310
#define TypeHALT 311
#define TypeIM 312
#define TypeIN 313
#define TypeINC 314
#define TypeIND 315
#define TypeINDR 316
#define TypeINI 317
#define TypeINIR 318
#define TypeJP 319
#define TypeJR 320
#define TypeLD 321
#define TypeLDD 322
#define TypeLDDR 323
#define TypeLDI 324
#define TypeLDIR 325
#define TypeNEG 326
#define TypeNOP 327
#define TypeOR 328
#define TypeOTDR 329
#define TypeOTIR 330
#define TypeOUT 331
#define TypeOUTD 332
#define TypeOUTI 333
#define TypePOP 334
#define TypePUSH 335
#define TypeRES 336
#define TypeRET 337
#define TypeRETI 338
#define TypeRETN 339
#define TypeRL 340
#define TypeRLA 341
#define TypeRLC 342
#define TypeRLCA 343
#define TypeRLD 344
#define TypeRR 345
#define TypeRRA 346
#define TypeRRC 347
#define TypeRRCA 348
#define TypeRRD 349
#define TypeRST 350
#define TypeSBC 351
#define TypeSCF 352
#define TypeSET 353
#define TypeSLA 354
#define TypeSLL 355
#define TypeSRA 356
#define TypeSRL 357
#define TypeSUB 358
#define TypeXOR 359
#define TypeA 360
#define TypeAF 361
#define TypeAFp 362
#define TypeB 363
#define TypeBC 364
#define TypeD 365
#define TypeE 366
#define TypeDE 367
#define TypeH 368
#define TypeL 369
#define TypeHL 370
#define TypeSP 371
#define TypeIX 372
#define TypeIXH 373
#define TypeIXL 374
#define TypeIY 375
#define TypeIYH 376
#define TypeIYL 377
#define TypeI 378
#define TypeR 379
#define TypeNZ 380
#define TypeZ 381
#define TypeNC 382
#define TypeC 383
#define TypePO 384
#define TypePE 385
#define TypeP 386
#define TypeM 387
#define TypeASEG 388
#define TypeCOMMON 389
#define TypeCSEG 390
#define TypeDB 391
#define TypeDEFB 392
#define TypeDEFL 393
#define TypeDEFM 394
#define TypeDEFS 395
#define TypeDEFW 396
#define TypeDS 397
#define TypeDSEG 398
#define TypeDW 399
#define TypeELSE 400
#define TypeEND 401
#define TypeENDIF 402
#define TypeENDM 403
#define TypeENDP 404
#define TypeEQU 405
#define TypeEXITM 406
#define TypeEXTRN 407
#define TypeIF 408
#define TypeIF1 409
#define TypeIF2 410
#define TypeIFDEF 411
#define TypeIFNDEF 412
#define TypeINCBIN 413
#define TypeINCLUDE 414
#define TypeIRP 415
#define TypeIRPC 416
#define TypeLOCAL 417
#define TypeMACRO 418
#define TypeORG 419
#define TypePROC 420
#define TypePUBLIC 421
#define TypeREPT 422
#define Type_8080 423
#define Type_8086 424
#define Type_DEPHASE 425
#define Type_ERROR 426
#define Type_PHASE 427
#define Type_SHIFT 428
#define Type_WARNING 429
#define Type_Z80 430
#define TypeAND_8080 431
#define TypeOR_8080 432
#define TypeXOR_8080 433
#define TypeM_8080 434
#define TypePSW_8080 435
#define TypeSET_8080 436
#define TypeADC_8080 437
#define TypeADD_8080 438
#define TypeACI_8080 439
#define TypeADI_8080 440
#define TypeANA_8080 441
#define TypeANI_8080 442
#define TypeCALL_8080 443
#define TypeCC_8080 444
#define TypeCM_8080 445
#define TypeCMA_8080 446
#define TypeCMC_8080 447
#define TypeCMP_8080 448
#define TypeCNC_8080 449
#define TypeCNZ_8080 450
#define TypeCP_8080 451
#define TypeCPE_8080 452
#define TypeCPI_8080 453
#define TypeCPO_8080 454
#define TypeCZ_8080 455
#define TypeDAD_8080 456
#define TypeDCR_8080 457
#define TypeDCX_8080 458
#define TypeHLT_8080 459
#define TypeIN_8080 460
#define TypeINR_8080 461
#define TypeINX_8080 462
#define TypeJC_8080 463
#define TypeJM_8080 464
#define TypeJMP_8080 465
#define TypeJNC_8080 466
#define TypeJNZ_8080 467
#define TypeJP_8080 468
#define TypeJPE_8080 469
#define TypeJPO_8080 470
#define TypeJZ_8080 471
#define TypeLDA_8080 472
#define TypeLDAX_8080 473
#define TypeLHLD_8080 474
#define TypeLXI_8080 475
#define TypeMVI_8080 476
#define TypeMOV_8080 477
#define TypeORA_8080 478
#define TypeORI_8080 479
#define TypeOUT_8080 480
#define TypePCHL_8080 481
#define TypePOP_8080 482
#define TypePUSH_8080 483
#define TypeRAL_8080 484
#define TypeRAR_8080 485
#define TypeRLC_8080 486
#define TypeRC_8080 487
#define TypeRRC_8080 488
#define TypeRET_8080 489
#define TypeRM_8080 490
#define TypeRNC_8080 491
#define TypeRNZ_8080 492
#define TypeRP_8080 493
#define TypeRPE_8080 494
#define TypeRPO_8080 495
#define TypeRST_8080 496
#define TypeRZ_8080 497
#define TypeSBB_8080 498
#define TypeSBI_8080 499
#define TypeSHLD_8080 500
#define TypeSPHL_8080 501
#define TypeSTA_8080 502
#define TypeSTAX_8080 503
#define TypeSTC_8080 504
#define TypeSUB_8080 505
#define TypeSUI_8080 506
#define TypeXCHG_8080 507
#define TypeXRA_8080 508
#define TypeXRI_8080 509
#define TypeXTHL_8080 510




/* Copy the first part of user declarations.  */
#line 6 "../parser.yxx"


#include "pasmoimpl.h"
#include "machine.h"
#include "parseraux.h"

namespace pasmo {
namespace impl {

namespace parser {

inline Oper getop (const Token & tok)
{
	return static_cast <Oper> (tok.num () );
}

inline bool isparenexp (const Token & tok)
{
	return tok.num () != 0;
}

inline void checkugly (Machine & mach, const Token & tok)
{
	if (isparenexp (tok) )
		mach.ugly ();
}

} /* namespace parser */

#line 340 "../parser.yxx"


namespace parser {

const address value_0= address (0);
const address value_1= address (1);

#define GENVALADDR(r) \
	const address val ## r= static_cast <address> (r)

GENVALADDR (TypeA);
GENVALADDR (TypeC);
GENVALADDR (TypeBC);
GENVALADDR (TypeDE);
GENVALADDR (TypeHL);
GENVALADDR (TypeM);
GENVALADDR (TypeNC);
GENVALADDR (TypeNZ);
GENVALADDR (TypeP);
GENVALADDR (TypePE);
GENVALADDR (TypePO);
GENVALADDR (TypeZ);
GENVALADDR (regA);
GENVALADDR (regB);
GENVALADDR (regC);
GENVALADDR (regD);
GENVALADDR (regE);
GENVALADDR (regH);
GENVALADDR (regL);
GENVALADDR (reg_HL_);
GENVALADDR (regAF);
GENVALADDR (regBC);
GENVALADDR (regDE);
GENVALADDR (regHL);
GENVALADDR (regSP);
GENVALADDR (flagNZ);
GENVALADDR (flagZ);
GENVALADDR (flagNC);
GENVALADDR (flagC);
GENVALADDR (flagPO);
GENVALADDR (flagPE);
GENVALADDR (flagP);
GENVALADDR (flagM);
GENVALADDR (prefixNone);
GENVALADDR (prefixIX);
GENVALADDR (prefixIY);
GENVALADDR (tiADCA);
GENVALADDR (tiADDA);
GENVALADDR (tiAND);
GENVALADDR (tiCP);
GENVALADDR (tiOR);
GENVALADDR (tiSBCA);
GENVALADDR (tiSUB);
GENVALADDR (tiXOR);
GENVALADDR (codeBIT);
GENVALADDR (codeLD_A_I);
GENVALADDR (codeLD_A_R);
GENVALADDR (codeLD_I_A);
GENVALADDR (codeLD_R_A);
GENVALADDR (codeRES);
GENVALADDR (codeRL);
GENVALADDR (codeRLC);
GENVALADDR (codeRR);
GENVALADDR (codeRRC);
GENVALADDR (codeSET);
GENVALADDR (codeSLA);
GENVALADDR (codeSLL);
GENVALADDR (codeSRA);
GENVALADDR (codeSRL);

#undef GENVALADDR

inline void addregIXH (Machine & mach)
{
	mach.addcode (valregH);
	mach.addcode (valprefixIX);
}

inline void addregIXL (Machine & mach)
{
	mach.addcode (valregL);
	mach.addcode (valprefixIX);
}

inline void addregIYH (Machine & mach)
{
	mach.addcode (valregH);
	mach.addcode (valprefixIY);
}

inline void addregIYL (Machine & mach)
{
	mach.addcode (valregL);
	mach.addcode (valprefixIY);
}

} /* namespace parser */

using namespace parser;



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 748 "parser.cxx"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  716
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   9425

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  276
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  613
/* YYNRULES -- Number of rules.  */
#define YYNRULES  1444
/* YYNRULES -- Number of states.  */
#define YYNSTATES  2025

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   510

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint16 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   259,     2,     2,   275,   267,   257,     2,
     270,   272,   265,   264,   269,   263,     2,   266,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   268,     2,
     261,   260,   262,   274,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   271,     2,   273,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   256,     2,   258,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,   253,   254,
     255
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      19,    21,    23,    25,    27,    29,    31,    33,    35,    37,
      39,    41,    43,    45,    47,    49,    51,    53,    55,    57,
      59,    60,    62,    64,    66,    68,    70,    73,    75,    77,
      79,    81,    83,    85,    87,    89,    91,    93,    95,    97,
      99,   101,   103,   105,   107,   109,   111,   113,   115,   117,
     119,   121,   123,   125,   127,   129,   131,   133,   135,   137,
     139,   141,   143,   145,   147,   149,   151,   153,   155,   157,
     159,   161,   163,   165,   167,   169,   171,   173,   175,   177,
     179,   181,   183,   185,   187,   189,   191,   193,   195,   197,
     199,   201,   203,   205,   207,   209,   211,   213,   215,   217,
     219,   221,   223,   225,   227,   229,   231,   233,   235,   237,
     239,   241,   243,   245,   247,   249,   251,   253,   255,   257,
     259,   261,   263,   265,   267,   269,   271,   273,   275,   277,
     279,   281,   283,   285,   287,   289,   291,   293,   295,   297,
     299,   301,   303,   305,   307,   309,   311,   313,   315,   317,
     319,   321,   323,   325,   327,   329,   331,   333,   335,   337,
     339,   341,   343,   345,   347,   349,   351,   353,   355,   357,
     359,   361,   363,   365,   367,   369,   371,   373,   375,   377,
     379,   381,   383,   385,   387,   389,   391,   393,   395,   397,
     399,   401,   403,   404,   408,   409,   413,   415,   417,   419,
     421,   422,   426,   428,   430,   432,   434,   436,   437,   441,
     443,   445,   447,   449,   450,   453,   455,   457,   459,   460,
     463,   464,   468,   470,   472,   473,   477,   479,   481,   482,
     485,   486,   489,   490,   491,   495,   497,   499,   500,   504,
     506,   508,   509,   513,   514,   518,   519,   523,   524,   528,
     530,   531,   535,   536,   540,   542,   544,   546,   547,   551,
     552,   556,   557,   561,   562,   566,   568,   569,   573,   574,
     578,   579,   583,   585,   587,   588,   592,   594,   597,   600,
     603,   606,   608,   611,   614,   615,   620,   623,   625,   628,
     631,   632,   635,   637,   641,   644,   646,   648,   651,   654,
     656,   659,   662,   664,   667,   669,   671,   674,   677,   679,
     682,   684,   687,   690,   693,   696,   697,   700,   702,   705,
     708,   712,   714,   716,   719,   722,   726,   730,   734,   738,
     742,   745,   748,   752,   754,   755,   759,   761,   762,   766,
     767,   771,   773,   774,   778,   780,   781,   785,   786,   788,
     790,   791,   795,   796,   800,   801,   805,   808,   810,   812,
     815,   816,   821,   823,   825,   827,   829,   831,   834,   835,
     839,   842,   843,   847,   849,   852,   854,   857,   859,   863,
     865,   867,   869,   873,   875,   877,   880,   883,   886,   889,
     891,   893,   896,   899,   901,   903,   906,   909,   911,   913,
     916,   919,   921,   924,   926,   929,   932,   935,   937,   940,
     943,   945,   948,   951,   954,   956,   959,   962,   963,   967,
     970,   973,   974,   979,   984,   986,   987,   992,   997,   999,
    1001,  1003,  1006,  1009,  1010,  1015,  1017,  1019,  1020,  1025,
    1027,  1029,  1034,  1039,  1044,  1046,  1048,  1050,  1052,  1054,
    1056,  1058,  1060,  1061,  1065,  1066,  1070,  1071,  1075,  1076,
    1080,  1081,  1085,  1086,  1090,  1091,  1095,  1096,  1101,  1102,
    1107,  1111,  1115,  1119,  1121,  1123,  1127,  1129,  1131,  1133,
    1135,  1137,  1139,  1143,  1145,  1147,  1148,  1152,  1153,  1157,
    1158,  1162,  1163,  1167,  1168,  1172,  1173,  1177,  1178,  1182,
    1183,  1187,  1190,  1192,  1193,  1197,  1198,  1202,  1203,  1207,
    1208,  1212,  1213,  1218,  1220,  1222,  1223,  1227,  1228,  1232,
    1233,  1237,  1238,  1242,  1245,  1248,  1250,  1254,  1258,  1260,
    1263,  1266,  1268,  1270,  1272,  1276,  1280,  1283,  1286,  1288,
    1290,  1292,  1294,  1297,  1299,  1301,  1303,  1305,  1309,  1311,
    1313,  1317,  1321,  1324,  1328,  1330,  1333,  1337,  1339,  1342,
    1345,  1348,  1350,  1353,  1355,  1359,  1363,  1365,  1368,  1371,
    1373,  1376,  1378,  1382,  1386,  1388,  1392,  1396,  1398,  1400,
    1402,  1403,  1407,  1408,  1412,  1416,  1418,  1420,  1421,  1425,
    1426,  1430,  1434,  1436,  1438,  1439,  1443,  1444,  1448,  1449,
    1453,  1454,  1458,  1459,  1463,  1464,  1468,  1469,  1473,  1474,
    1478,  1481,  1483,  1487,  1491,  1493,  1496,  1498,  1500,  1502,
    1504,  1507,  1509,  1511,  1514,  1516,  1518,  1520,  1522,  1524,
    1526,  1530,  1534,  1536,  1538,  1540,  1542,  1546,  1550,  1552,
    1554,  1556,  1558,  1561,  1564,  1566,  1570,  1575,  1580,  1583,
    1585,  1589,  1593,  1596,  1599,  1604,  1610,  1612,  1614,  1618,
    1623,  1628,  1631,  1633,  1637,  1639,  1640,  1645,  1646,  1651,
    1655,  1658,  1661,  1666,  1670,  1673,  1675,  1677,  1679,  1683,
    1687,  1691,  1695,  1697,  1699,  1701,  1703,  1705,  1707,  1709,
    1711,  1713,  1715,  1717,  1719,  1721,  1723,  1725,  1727,  1729,
    1731,  1733,  1735,  1737,  1739,  1741,  1743,  1745,  1747,  1749,
    1751,  1753,  1755,  1757,  1759,  1761,  1763,  1766,  1768,  1770,
    1772,  1775,  1778,  1781,  1784,  1786,  1788,  1791,  1794,  1797,
    1800,  1802,  1805,  1808,  1811,  1816,  1819,  1821,  1823,  1826,
    1829,  1833,  1837,  1841,  1845,  1849,  1853,  1857,  1860,  1863,
    1865,  1868,  1871,  1873,  1876,  1879,  1882,  1885,  1889,  1891,
    1895,  1897,  1900,  1903,  1905,  1908,  1911,  1915,  1917,  1920,
    1923,  1926,  1929,  1931,  1933,  1935,  1937,  1939,  1941,  1943,
    1945,  1947,  1949,  1951,  1955,  1957,  1959,  1963,  1965,  1967,
    1969,  1973,  1975,  1977,  1981,  1985,  1989,  1991,  1993,  1995,
    1997,  1999,  2003,  2007,  2011,  2013,  2015,  2017,  2019,  2021,
    2025,  2029,  2031,  2033,  2035,  2036,  2040,  2041,  2045,  2046,
    2050,  2051,  2055,  2059,  2063,  2066,  2069,  2071,  2074,  2076,
    2081,  2085,  2087,  2092,  2096,  2098,  2100,  2102,  2105,  2109,
    2111,  2113,  2114,  2118,  2119,  2123,  2124,  2128,  2129,  2133,
    2136,  2138,  2141,  2143,  2144,  2149,  2150,  2155,  2156,  2160,
    2161,  2165,  2166,  2170,  2171,  2175,  2176,  2180,  2181,  2185,
    2186,  2190,  2191,  2195,  2198,  2201,  2203,  2206,  2209,  2211,
    2212,  2216,  2217,  2221,  2224,  2227,  2228,  2233,  2237,  2239,
    2240,  2245,  2249,  2251,  2254,  2256,  2257,  2262,  2263,  2268,
    2272,  2274,  2276,  2277,  2281,  2282,  2286,  2287,  2291,  2292,
    2296,  2297,  2301,  2302,  2306,  2307,  2311,  2312,  2316,  2320,
    2322,  2324,  2328,  2330,  2332,  2334,  2335,  2339,  2340,  2344,
    2345,  2349,  2350,  2354,  2355,  2359,  2360,  2364,  2365,  2369,
    2370,  2374,  2377,  2380,  2382,  2385,  2387,  2389,  2392,  2394,
    2397,  2399,  2401,  2404,  2407,  2410,  2412,  2414,  2417,  2419,
    2421,  2424,  2427,  2430,  2432,  2434,  2437,  2439,  2441,  2444,
    2447,  2449,  2452,  2455,  2458,  2461,  2464,  2467,  2470,  2472,
    2475,  2477,  2479,  2481,  2483,  2485,  2487,  2489,  2491,  2493,
    2495,  2498,  2500,  2502,  2504,  2507,  2509,  2511,  2513,  2516,
    2518,  2521,  2523,  2526,  2528,  2530,  2532,  2534,  2537,  2539,
    2542,  2545,  2548,  2550,  2553,  2556,  2558,  2561,  2564,  2567,
    2569,  2572,  2575,  2577,  2580,  2582,  2585,  2588,  2591,  2593,
    2596,  2599,  2601,  2603,  2605,  2607,  2609,  2611,  2613,  2615,
    2617,  2619,  2621,  2623,  2625,  2627,  2629,  2631,  2633,  2635,
    2637,  2639,  2641,  2643,  2645,  2647,  2649,  2651,  2653,  2655,
    2657,  2659,  2661,  2663,  2665,  2667,  2669,  2671,  2673,  2675,
    2677,  2679,  2681,  2683,  2685,  2687,  2689,  2691,  2693,  2695,
    2697,  2699,  2701,  2703,  2705,  2707,  2709,  2711,  2713,  2715,
    2717,  2719,  2721,  2723,  2725,  2727,  2729,  2731,  2733,  2735,
    2737,  2739,  2741,  2743,  2745,  2747,  2749,  2751,  2753,  2755,
    2757,  2759,  2761,  2763,  2765,  2767,  2769,  2771,  2773,  2775,
    2777,  2779,  2781,  2783,  2785,  2787,  2789,  2791,  2793,  2795,
    2797,  2801,  2805,  2809,  2813,  2817,  2821,  2825,  2829,  2833,
    2837,  2841,  2844,  2847,  2850,  2853,  2855,  2857,  2860,  2863,
    2867,  2871,  2873,  2877,  2881,  2883,  2885,  2889,  2893,  2895,
    2897,  2899,  2903,  2906,  2908,  2911,  2913,  2915,  2917,  2919,
    2921,  2925,  2929,  2933,  2937,  2941,  2945,  2949,  2953,  2957,
    2961,  2965,  2969,  2973,  2976,  2979,  2982,  2985,  2989,  2993,
    2997,  3001,  3005,  3008,  3011,  3015,  3019,  3023,  3027,  3031,
    3035,  3039,  3043,  3047,  3051,  3055,  3059,  3063,  3066,  3069,
    3072,  3075,  3079,  3083,  3087,  3091,  3095,  3098,  3101,  3105,
    3108,  3110,  3112,  3114,  3116,  3119,  3122,  3124,  3126,  3128,
    3129,  3133,  3135,  3136,  3140,  3142,  3144,  3146,  3148,  3150,
    3152,  3154,  3156,  3158,  3160,  3162,  3164,  3166,  3168,  3170,
    3172,  3174,  3176,  3178,  3180,  3182,  3184,  3186,  3188,  3190,
    3192,  3194,  3196,  3198,  3200,  3202,  3204,  3206,  3208,  3210,
    3212,  3214,  3216,  3218,  3220,  3222,  3224,  3226,  3228,  3230,
    3232,  3234,  3236,  3238,  3240,  3242,  3244,  3246,  3248,  3250,
    3252,  3254,  3256,  3258,  3260,  3262,  3264,  3266,  3268,  3270,
    3272,  3274,  3276,  3278,  3280,  3282,  3284,  3286,  3288,  3290,
    3292,  3294,  3296,  3298,  3300,  3302,  3304,  3306,  3308,  3310,
    3312,  3314,  3316,  3318,  3320,  3322,  3324,  3326,  3328,  3330,
    3332,  3334,  3336,  3338,  3340,  3342,  3344,  3346,  3348,  3350,
    3352,  3354,  3356,  3358,  3360,  3362,  3364,  3366,  3368,  3370,
    3372,  3374,  3376,  3378,  3380,  3382,  3384,  3386,  3388,  3390,
    3392,  3394,  3396,  3398,  3400,  3402,  3404,  3406,  3408,  3410,
    3412,  3414,  3416,  3418,  3420,  3422,  3424,  3426,  3428,  3430,
    3432,  3434,  3436,  3438,  3440,  3442,  3444,  3446,  3448,  3450,
    3452,  3454,  3456,  3458,  3460,  3462,  3464,  3466,  3468,  3470,
    3472,  3474,  3476,  3478,  3480,  3482,  3484,  3486,  3488,  3490,
    3492,  3494,  3496,  3498,  3500,  3502,  3504,  3506,  3508,  3510,
    3512,  3514,  3516,  3518,  3520,  3522,  3524,  3526,  3528,  3530,
    3532,  3534,  3536,  3538,  3540
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     290,     0,    -1,    15,    -1,   267,    -1,    16,    -1,    29,
      -1,    17,    -1,    30,    -1,    19,    -1,   260,    -1,    24,
      -1,    33,    -1,    20,    -1,   261,    -1,    22,    -1,   262,
      -1,    21,    -1,    31,    -1,    23,    -1,    32,    -1,    39,
      -1,   176,    -1,   257,    -1,    73,    -1,   177,    -1,   256,
      -1,    18,    -1,   258,    -1,   104,    -1,   178,    -1,    -1,
     291,    -1,   150,    -1,   138,    -1,     1,    -1,   292,    -1,
       3,   293,    -1,   297,    -1,   405,    -1,   321,    -1,   294,
      -1,   299,    -1,   405,    -1,   323,    -1,   295,    -1,   296,
      -1,   438,    -1,   445,    -1,   461,    -1,   469,    -1,   473,
      -1,   502,    -1,   509,    -1,   519,    -1,   530,    -1,   546,
      -1,   575,    -1,   587,    -1,   651,    -1,   659,    -1,   679,
      -1,   683,    -1,   687,    -1,   691,    -1,   695,    -1,   707,
      -1,   714,    -1,   720,    -1,   724,    -1,   728,    -1,   732,
      -1,   740,    -1,   748,    -1,   296,    -1,   439,    -1,   446,
      -1,   463,    -1,   471,    -1,   474,    -1,   504,    -1,   511,
      -1,   520,    -1,   531,    -1,   548,    -1,   576,    -1,   588,
      -1,   653,    -1,   660,    -1,   681,    -1,   685,    -1,   689,
      -1,   693,    -1,   697,    -1,   708,    -1,   716,    -1,   722,
      -1,   726,    -1,   730,    -1,   734,    -1,   742,    -1,   750,
      -1,     5,    -1,   345,    -1,   346,    -1,   347,    -1,   348,
      -1,   349,    -1,   355,    -1,   359,    -1,   363,    -1,   364,
      -1,   366,    -1,   367,    -1,   368,    -1,   371,    -1,   372,
      -1,   373,    -1,   375,    -1,   376,    -1,   377,    -1,   378,
      -1,   379,    -1,   380,    -1,   381,    -1,   382,    -1,   383,
      -1,   398,    -1,   402,    -1,   412,    -1,   414,    -1,   415,
      -1,   416,    -1,   424,    -1,   425,    -1,   426,    -1,   427,
      -1,   429,    -1,   431,    -1,   432,    -1,   434,    -1,   435,
      -1,   517,    -1,   528,    -1,   584,    -1,   669,    -1,   673,
      -1,   677,    -1,   699,    -1,   436,    -1,   455,    -1,   457,
      -1,   459,    -1,   465,    -1,   467,    -1,   479,    -1,   498,
      -1,   500,    -1,   483,    -1,   481,    -1,   485,    -1,   487,
      -1,   489,    -1,   491,    -1,   493,    -1,   495,    -1,   506,
      -1,   513,    -1,   515,    -1,   638,    -1,   550,    -1,   552,
      -1,   554,    -1,   556,    -1,   558,    -1,   564,    -1,   560,
      -1,   562,    -1,   566,    -1,   568,    -1,   570,    -1,   572,
      -1,   634,    -1,   636,    -1,   640,    -1,   644,    -1,   648,
      -1,   655,    -1,   657,    -1,   667,    -1,   671,    -1,   675,
      -1,   701,    -1,   703,    -1,   705,    -1,   718,    -1,   736,
      -1,   738,    -1,   744,    -1,   746,    -1,   752,    -1,   754,
      -1,   756,    -1,    -1,     9,   298,   301,    -1,    -1,     9,
     300,   303,    -1,   369,    -1,   352,    -1,   353,    -1,   403,
      -1,    -1,   268,   302,   305,    -1,   313,    -1,   369,    -1,
     352,    -1,   353,    -1,   403,    -1,    -1,   268,   304,   307,
      -1,   315,    -1,   369,    -1,   352,    -1,   353,    -1,    -1,
     306,   309,    -1,   369,    -1,   352,    -1,   353,    -1,    -1,
     308,   311,    -1,    -1,    10,   310,   331,    -1,   294,    -1,
       1,    -1,    -1,    10,   312,   331,    -1,   295,    -1,     1,
      -1,    -1,   314,   317,    -1,    -1,   316,   319,    -1,    -1,
      -1,    10,   318,   331,    -1,   294,    -1,     1,    -1,    -1,
      10,   320,   331,    -1,   295,    -1,     1,    -1,    -1,    10,
     322,   325,    -1,    -1,    10,   324,   328,    -1,    -1,   268,
     326,   294,    -1,    -1,   163,   327,   409,    -1,   331,    -1,
      -1,   268,   329,   295,    -1,    -1,   163,   330,   409,    -1,
     331,    -1,   332,    -1,     5,    -1,    -1,   269,   333,   332,
      -1,    -1,   267,   334,   340,    -1,    -1,    11,   335,   344,
      -1,    -1,     1,   336,   337,    -1,     5,    -1,    -1,   269,
     338,   332,    -1,    -1,     1,   339,   337,    -1,    -1,   858,
     341,   342,    -1,     1,    -1,     5,    -1,    -1,   798,   343,
     332,    -1,     5,    -1,   798,   332,    -1,   133,   797,    -1,
     135,   797,    -1,   143,   797,    -1,   134,    -1,   883,   350,
      -1,   858,     5,    -1,    -1,   858,   269,   351,   350,    -1,
     858,     1,    -1,     1,    -1,   138,   354,    -1,   181,   354,
      -1,    -1,   858,   797,    -1,     1,    -1,   885,   356,   797,
      -1,   858,   357,    -1,     1,    -1,     5,    -1,   799,   358,
      -1,   858,   797,    -1,     1,    -1,   884,   360,    -1,   362,
     361,    -1,     5,    -1,   799,   360,    -1,   858,    -1,     1,
      -1,   145,   797,    -1,   146,   365,    -1,     5,    -1,   858,
     797,    -1,     1,    -1,   147,   797,    -1,   148,   797,    -1,
     149,   797,    -1,   150,   370,    -1,    -1,   858,   797,    -1,
       1,    -1,   151,   797,    -1,   152,   757,    -1,   153,   374,
     797,    -1,   858,    -1,     1,    -1,   154,   797,    -1,   155,
     797,    -1,   156,   804,   797,    -1,   157,   804,   797,    -1,
     158,   807,   797,    -1,   159,   807,   797,    -1,    13,   807,
     797,    -1,    12,   797,    -1,   160,   384,    -1,     9,   798,
     385,    -1,     1,    -1,    -1,   261,   386,   393,    -1,     5,
      -1,    -1,   269,   387,   389,    -1,    -1,     1,   388,   389,
      -1,     5,    -1,    -1,   799,   390,   391,    -1,     5,    -1,
      -1,     1,   392,   389,    -1,    -1,     5,    -1,   262,    -1,
      -1,    11,   394,   397,    -1,    -1,   269,   395,   393,    -1,
      -1,     1,   396,   397,    -1,   269,   393,    -1,   262,    -1,
       1,    -1,   161,   399,    -1,    -1,   804,   798,   400,   401,
      -1,     8,    -1,    11,    -1,     9,    -1,     7,    -1,     1,
      -1,   162,   757,    -1,    -1,   163,   404,   409,    -1,   163,
     406,    -1,    -1,     9,   407,   408,    -1,     5,    -1,   798,
     409,    -1,     5,    -1,   411,   410,    -1,     5,    -1,   798,
     411,   410,    -1,     9,    -1,   872,    -1,     1,    -1,   164,
     413,   797,    -1,   858,    -1,     1,    -1,   165,   797,    -1,
     166,   757,    -1,   167,   417,    -1,   858,   418,    -1,     1,
      -1,     5,    -1,   799,   419,    -1,     9,   420,    -1,     1,
      -1,     5,    -1,   799,   421,    -1,   858,   422,    -1,     1,
      -1,     5,    -1,   799,   423,    -1,   858,   797,    -1,     1,
      -1,   168,   797,    -1,   169,    -1,   170,   797,    -1,   171,
     428,    -1,     8,   797,    -1,     1,    -1,   172,   430,    -1,
     858,   797,    -1,     1,    -1,   173,   797,    -1,   174,   433,
      -1,     8,   797,    -1,     1,    -1,   175,   797,    -1,   888,
     797,    -1,    -1,   184,   437,   761,    -1,    37,   440,    -1,
      37,   442,    -1,    -1,   105,   798,   441,   762,    -1,   115,
     798,   444,   797,    -1,     1,    -1,    -1,   105,   798,   443,
     763,    -1,   115,   798,   444,   797,    -1,     1,    -1,   825,
      -1,     1,    -1,    38,   447,    -1,    38,   449,    -1,    -1,
     105,   798,   448,   762,    -1,   451,    -1,     1,    -1,    -1,
     105,   798,   450,   763,    -1,   451,    -1,     1,    -1,   115,
     798,   452,   797,    -1,   117,   798,   453,   797,    -1,   120,
     798,   454,   797,    -1,   825,    -1,     1,    -1,   831,    -1,
     117,    -1,     1,    -1,   831,    -1,   120,    -1,     1,    -1,
      -1,   182,   456,   760,    -1,    -1,   183,   458,   760,    -1,
      -1,   185,   460,   761,    -1,    -1,    39,   462,   764,    -1,
      -1,    39,   464,   765,    -1,    -1,   186,   466,   760,    -1,
      -1,   187,   468,   761,    -1,    -1,    40,   470,   788,   789,
      -1,    -1,    40,   472,   788,   790,    -1,    41,   475,   797,
      -1,    41,   476,   797,    -1,   808,   798,   477,    -1,   858,
      -1,     1,    -1,   808,   798,   478,    -1,   858,    -1,     1,
      -1,   858,    -1,     1,    -1,   858,    -1,     1,    -1,   188,
     480,   797,    -1,   858,    -1,     1,    -1,    -1,   189,   482,
     497,    -1,    -1,   190,   484,   497,    -1,    -1,   194,   486,
     497,    -1,    -1,   195,   488,   497,    -1,    -1,   196,   490,
     497,    -1,    -1,   197,   492,   497,    -1,    -1,   199,   494,
     497,    -1,    -1,   200,   496,   497,    -1,   858,   797,    -1,
       1,    -1,    -1,   193,   499,   760,    -1,    -1,   198,   501,
     761,    -1,    -1,    43,   503,   764,    -1,    -1,    43,   505,
     765,    -1,    -1,   201,   508,   507,   797,    -1,   826,    -1,
       1,    -1,    -1,    50,   510,   769,    -1,    -1,    50,   512,
     770,    -1,    -1,   202,   514,   779,    -1,    -1,   203,   516,
     780,    -1,    52,   518,    -1,   858,   797,    -1,     1,    -1,
      54,   521,   797,    -1,    54,   522,   797,    -1,   523,    -1,
     843,   526,    -1,   270,     1,    -1,     1,    -1,   523,    -1,
       1,    -1,   106,   798,   524,    -1,   112,   798,   525,    -1,
     844,   526,    -1,   271,     1,    -1,   107,    -1,     1,    -1,
     115,    -1,     1,    -1,   798,   527,    -1,   115,    -1,   117,
      -1,   120,    -1,     1,    -1,    57,   529,   797,    -1,   858,
      -1,     1,    -1,    58,   532,   797,    -1,    58,   533,   797,
      -1,   534,   535,    -1,   105,   798,   540,    -1,     1,    -1,
     534,   536,    -1,   105,   798,   541,    -1,     1,    -1,   813,
     798,    -1,   271,   538,    -1,   270,   537,    -1,     1,    -1,
     271,   538,    -1,     1,    -1,   806,   800,   539,    -1,   806,
     801,   539,    -1,   797,    -1,   271,   542,    -1,   270,   543,
      -1,     1,    -1,   271,   542,    -1,     1,    -1,   128,   801,
     544,    -1,   858,   801,   545,    -1,     1,    -1,   128,   800,
     544,    -1,   858,   800,   545,    -1,     1,    -1,   797,    -1,
     797,    -1,    -1,    59,   547,   769,    -1,    -1,    59,   549,
     770,    -1,   205,   551,   797,    -1,   858,    -1,     1,    -1,
      -1,   206,   553,   779,    -1,    -1,   207,   555,   780,    -1,
     210,   557,   797,    -1,   858,    -1,     1,    -1,    -1,   208,
     559,   574,    -1,    -1,   211,   561,   574,    -1,    -1,   212,
     563,   574,    -1,    -1,   209,   565,   574,    -1,    -1,   213,
     567,   574,    -1,    -1,   214,   569,   574,    -1,    -1,   215,
     571,   574,    -1,    -1,   216,   573,   574,    -1,   858,   797,
      -1,     1,    -1,    64,   577,   797,    -1,    64,   578,   797,
      -1,   580,    -1,   271,   579,    -1,   841,    -1,   845,    -1,
     846,    -1,   858,    -1,   270,   871,    -1,     1,    -1,   581,
      -1,   271,   579,    -1,   858,    -1,     1,    -1,   848,    -1,
     849,    -1,   850,    -1,     1,    -1,   808,   798,   582,    -1,
     808,   798,   583,    -1,   858,    -1,     1,    -1,   858,    -1,
       1,    -1,    65,   585,   797,    -1,   809,   798,   586,    -1,
     858,    -1,     1,    -1,   858,    -1,     1,    -1,    66,   589,
      -1,    66,   590,    -1,   591,    -1,   105,   798,   613,    -1,
     817,   798,   605,   797,    -1,   814,   798,   608,   797,    -1,
     596,   797,    -1,   619,    -1,   116,   798,   630,    -1,   841,
     798,   617,    -1,   837,   618,    -1,   839,   618,    -1,   855,
     798,   595,   797,    -1,   270,   858,   800,   798,   633,    -1,
       1,    -1,   591,    -1,   105,   798,   614,    -1,   817,   798,
     606,   797,    -1,   814,   798,   609,   797,    -1,   597,   797,
      -1,   620,    -1,   116,   798,   631,    -1,     1,    -1,    -1,
     123,   592,   798,   594,    -1,    -1,   124,   593,   798,   594,
      -1,   842,   798,   617,    -1,   838,   618,    -1,   840,   618,
      -1,   854,   798,   595,   797,    -1,   836,   798,   633,    -1,
     105,   797,    -1,     1,    -1,   811,    -1,   858,    -1,   818,
     798,   598,    -1,   819,   798,   599,    -1,   818,   798,   600,
      -1,   819,   798,   601,    -1,   602,    -1,   604,    -1,     1,
      -1,   603,    -1,   604,    -1,     1,    -1,   602,    -1,   858,
      -1,     1,    -1,   603,    -1,   858,    -1,     1,    -1,   823,
      -1,   824,    -1,   858,    -1,   610,    -1,   820,    -1,     1,
      -1,   611,    -1,   820,    -1,     1,    -1,   852,    -1,   848,
      -1,     1,    -1,   610,    -1,   820,    -1,   611,    -1,   820,
      -1,   612,    -1,   855,    -1,   841,    -1,   858,    -1,   612,
      -1,   858,    -1,   271,   607,    -1,   811,    -1,   615,    -1,
     855,    -1,   837,   797,    -1,   839,   797,    -1,   841,   797,
      -1,   858,   797,    -1,     1,    -1,   615,    -1,   858,   797,
      -1,   616,   797,    -1,   811,   797,    -1,   820,   797,    -1,
     854,    -1,   838,   797,    -1,   840,   797,    -1,   842,   797,
      -1,   271,   858,   801,   797,    -1,   271,     1,    -1,   123,
      -1,   124,    -1,   811,   797,    -1,   858,   797,    -1,   798,
     805,   797,    -1,   115,   798,   621,    -1,   834,   798,   622,
      -1,   851,   798,   627,    -1,   115,   798,   623,    -1,   834,
     798,   624,    -1,   851,   798,   628,    -1,   271,   625,    -1,
     858,   797,    -1,     1,    -1,   271,   626,    -1,   858,   797,
      -1,     1,    -1,   271,   625,    -1,   858,   797,    -1,   271,
     626,    -1,   858,   797,    -1,   858,   801,   797,    -1,     1,
      -1,   858,   801,   797,    -1,     1,    -1,   271,   629,    -1,
     858,   797,    -1,     1,    -1,   271,   629,    -1,   858,   797,
      -1,   858,   801,   797,    -1,     1,    -1,   632,   797,    -1,
     858,   797,    -1,   632,   797,    -1,   858,   797,    -1,   115,
      -1,   117,    -1,   120,    -1,   836,    -1,   105,    -1,   109,
      -1,   112,    -1,   115,    -1,   116,    -1,   117,    -1,   120,
      -1,   217,   635,   797,    -1,   858,    -1,     1,    -1,   218,
     637,   797,    -1,   108,    -1,   110,    -1,     1,    -1,   219,
     639,   797,    -1,   858,    -1,     1,    -1,   220,   641,   797,
      -1,   829,   798,   643,    -1,   113,   798,   642,    -1,     1,
      -1,   858,    -1,     1,    -1,   858,    -1,     1,    -1,   222,
     645,   797,    -1,   811,   798,   646,    -1,   812,   798,   647,
      -1,     1,    -1,   810,    -1,     1,    -1,   811,    -1,     1,
      -1,   221,   649,   797,    -1,   810,   798,   650,    -1,     1,
      -1,   858,    -1,     1,    -1,    -1,    73,   652,   764,    -1,
      -1,    73,   654,   765,    -1,    -1,   223,   656,   760,    -1,
      -1,   224,   658,   761,    -1,    76,   661,   797,    -1,    76,
     662,   797,    -1,   271,   663,    -1,   270,   664,    -1,     1,
      -1,   271,   663,    -1,     1,    -1,   128,   801,   798,   665,
      -1,   858,   801,   666,    -1,     1,    -1,   128,   800,   798,
     665,    -1,   858,   800,   666,    -1,     1,    -1,   811,    -1,
       1,    -1,   798,   805,    -1,   225,   668,   797,    -1,   858,
      -1,     1,    -1,    -1,    79,   670,   781,    -1,    -1,   227,
     672,   783,    -1,    -1,    80,   674,   781,    -1,    -1,   228,
     676,   783,    -1,    82,   678,    -1,     5,    -1,   808,   797,
      -1,     1,    -1,    -1,    81,   680,   788,   789,    -1,    -1,
      81,   682,   788,   790,    -1,    -1,    85,   684,   784,    -1,
      -1,    85,   686,   785,    -1,    -1,    87,   688,   784,    -1,
      -1,    87,   690,   785,    -1,    -1,    90,   692,   784,    -1,
      -1,    90,   694,   785,    -1,    -1,    92,   696,   784,    -1,
      -1,    92,   698,   785,    -1,    95,   700,    -1,   858,   797,
      -1,     1,    -1,   241,   702,    -1,   858,   797,    -1,     1,
      -1,    -1,   243,   704,   760,    -1,    -1,   244,   706,   761,
      -1,    96,   709,    -1,    96,   711,    -1,    -1,   105,   798,
     710,   762,    -1,   115,   798,   713,    -1,     1,    -1,    -1,
     105,   798,   712,   763,    -1,   115,   798,   713,    -1,     1,
      -1,   825,   797,    -1,     1,    -1,    -1,    98,   715,   788,
     789,    -1,    -1,    98,   717,   788,   790,    -1,   245,   719,
     797,    -1,   858,    -1,     1,    -1,    -1,    99,   721,   784,
      -1,    -1,    99,   723,   785,    -1,    -1,   100,   725,   786,
      -1,    -1,   100,   727,   787,    -1,    -1,   101,   729,   784,
      -1,    -1,   101,   731,   785,    -1,    -1,   102,   733,   784,
      -1,    -1,   102,   735,   785,    -1,   247,   737,   797,    -1,
     858,    -1,     1,    -1,   248,   739,   797,    -1,   108,    -1,
     110,    -1,     1,    -1,    -1,   103,   741,   764,    -1,    -1,
     103,   743,   765,    -1,    -1,   250,   745,   760,    -1,    -1,
     251,   747,   761,    -1,    -1,   104,   749,   764,    -1,    -1,
     104,   751,   765,    -1,    -1,   253,   753,   760,    -1,    -1,
     254,   755,   761,    -1,   255,   797,    -1,   759,   758,    -1,
       5,    -1,   799,   757,    -1,     9,    -1,     1,    -1,   810,
     797,    -1,     1,    -1,   858,   797,    -1,     1,    -1,   766,
      -1,   858,   797,    -1,   270,   871,    -1,   271,     1,    -1,
       1,    -1,   767,    -1,   271,     1,    -1,     1,    -1,   766,
      -1,   858,   797,    -1,   270,   871,    -1,   271,     1,    -1,
       1,    -1,   767,    -1,   271,     1,    -1,     1,    -1,   768,
      -1,   841,   797,    -1,   855,   797,    -1,   768,    -1,   858,
     797,    -1,   811,   797,    -1,   820,   797,    -1,   842,   797,
      -1,   854,   797,    -1,   771,   797,    -1,   777,   797,    -1,
       1,    -1,   771,   797,    -1,     1,    -1,   772,    -1,   773,
      -1,   774,    -1,   775,    -1,   811,    -1,   825,    -1,   117,
      -1,   120,    -1,   820,    -1,   271,   776,    -1,   848,    -1,
     852,    -1,     1,    -1,   270,   778,    -1,   847,    -1,   853,
      -1,     1,    -1,   810,   797,    -1,     1,    -1,   826,   797,
      -1,     1,    -1,   782,   797,    -1,     1,    -1,   832,    -1,
     117,    -1,   120,    -1,   827,   797,    -1,     1,    -1,   791,
     797,    -1,   270,     1,    -1,   271,     1,    -1,     1,    -1,
     792,   797,    -1,   271,     1,    -1,     1,    -1,   794,   797,
      -1,   270,     1,    -1,   271,     1,    -1,     1,    -1,   795,
     797,    -1,   271,     1,    -1,     1,    -1,   858,   798,    -1,
       1,    -1,   794,   797,    -1,   270,     1,    -1,   271,     1,
      -1,     1,    -1,   795,   797,    -1,   271,     1,    -1,     1,
      -1,   793,    -1,   841,    -1,   855,    -1,   793,    -1,   811,
      -1,   842,    -1,   854,    -1,   820,    -1,   796,    -1,   841,
      -1,   855,    -1,   796,    -1,   811,    -1,   842,    -1,   854,
      -1,     5,    -1,     1,    -1,   269,    -1,     1,    -1,   269,
      -1,     1,    -1,   272,    -1,     1,    -1,   273,    -1,     1,
      -1,   272,    -1,     1,    -1,   273,    -1,     1,    -1,     9,
      -1,     1,    -1,   105,    -1,     1,    -1,   128,    -1,     1,
      -1,     8,    -1,     1,    -1,   809,    -1,   129,    -1,   130,
      -1,   131,    -1,   132,    -1,   125,    -1,   126,    -1,   127,
      -1,   128,    -1,   811,    -1,   812,    -1,   816,    -1,   813,
      -1,   179,    -1,   817,    -1,   814,    -1,   113,    -1,   114,
      -1,   817,    -1,   816,    -1,   105,    -1,   108,    -1,   128,
      -1,   110,    -1,   111,    -1,   118,    -1,   119,    -1,   121,
      -1,   122,    -1,   818,    -1,   819,    -1,   118,    -1,   119,
      -1,   121,    -1,   122,    -1,   821,    -1,   815,    -1,   822,
      -1,   815,    -1,   833,    -1,   835,    -1,   828,    -1,   835,
      -1,   828,    -1,   180,    -1,   105,    -1,   830,    -1,   113,
      -1,   830,    -1,   835,    -1,   108,    -1,   110,    -1,   834,
      -1,   835,    -1,   833,    -1,   106,    -1,   834,    -1,   115,
      -1,   109,    -1,   112,    -1,   116,    -1,   271,   858,   803,
      -1,   270,   109,   800,    -1,   271,   109,   801,    -1,   270,
     112,   800,    -1,   271,   112,   801,    -1,   270,   115,   800,
      -1,   271,   115,   801,    -1,   270,   116,   800,    -1,   271,
     116,   801,    -1,   270,   117,   800,    -1,   270,   120,   800,
      -1,   115,   800,    -1,   115,   801,    -1,   117,   801,    -1,
     120,   801,    -1,   117,    -1,   120,    -1,   851,   857,    -1,
     851,   856,    -1,   271,   851,   857,    -1,   270,   851,   856,
      -1,   272,    -1,   264,   858,   800,    -1,   263,   858,   800,
      -1,     1,    -1,   273,    -1,   264,   858,   801,    -1,   263,
     858,   801,    -1,     1,    -1,   859,    -1,   863,    -1,   863,
     274,   860,    -1,   858,   861,    -1,     1,    -1,   268,   862,
      -1,     1,    -1,   859,    -1,     1,    -1,   864,    -1,   865,
      -1,   863,   265,   863,    -1,   863,   266,   863,    -1,   863,
     277,   863,    -1,   863,   278,   863,    -1,   863,   279,   863,
      -1,   863,   264,   863,    -1,   863,   263,   863,    -1,   863,
     280,   863,    -1,   863,   281,   863,    -1,   863,   284,   863,
      -1,   863,   282,   863,    -1,   863,   283,   863,    -1,   863,
     285,   863,    -1,   264,   863,    -1,   263,   863,    -1,   288,
     863,    -1,   259,   863,    -1,   863,   286,   863,    -1,   863,
     287,   863,    -1,   863,   289,   863,    -1,   863,    34,   863,
      -1,   863,    35,   863,    -1,    27,   863,    -1,    28,   863,
      -1,   863,   265,     1,    -1,   863,   266,     1,    -1,   863,
     277,     1,    -1,   863,   278,     1,    -1,   863,   279,     1,
      -1,   863,   264,     1,    -1,   863,   263,     1,    -1,   863,
     280,     1,    -1,   863,   281,     1,    -1,   863,   284,     1,
      -1,   863,   282,     1,    -1,   863,   283,     1,    -1,   863,
     285,     1,    -1,   264,     1,    -1,   263,     1,    -1,   288,
       1,    -1,   259,     1,    -1,   863,   286,     1,    -1,   863,
     287,     1,    -1,   863,   289,     1,    -1,   863,    34,     1,
      -1,   863,    35,     1,    -1,    27,     1,    -1,    28,     1,
      -1,   270,   858,   802,    -1,   270,     1,    -1,     7,    -1,
       9,    -1,     8,    -1,   275,    -1,    26,   866,    -1,    25,
     867,    -1,     9,    -1,     1,    -1,     5,    -1,    -1,     1,
     868,   869,    -1,     5,    -1,    -1,     1,   870,   869,    -1,
       5,    -1,   874,    -1,   880,    -1,   875,    -1,   882,    -1,
     886,    -1,   873,    -1,   880,    -1,   882,    -1,   886,    -1,
     876,    -1,   874,    -1,   115,    -1,   117,    -1,   120,    -1,
     105,    -1,   108,    -1,   110,    -1,   111,    -1,   113,    -1,
     114,    -1,   123,    -1,   124,    -1,   118,    -1,   119,    -1,
     121,    -1,   122,    -1,   106,    -1,   109,    -1,   112,    -1,
     116,    -1,   877,    -1,   878,    -1,   878,    -1,   879,    -1,
     265,    -1,   266,    -1,   267,    -1,    29,    -1,    30,    -1,
     260,    -1,    33,    -1,    31,    -1,   261,    -1,    32,    -1,
     262,    -1,   257,    -1,   256,    -1,    34,    -1,    35,    -1,
      15,    -1,    16,    -1,    17,    -1,    19,    -1,    24,    -1,
      21,    -1,    20,    -1,    23,    -1,    22,    -1,    39,    -1,
      73,    -1,   104,    -1,    27,    -1,    28,    -1,   881,    -1,
     129,    -1,   130,    -1,   131,    -1,   132,    -1,   125,    -1,
     126,    -1,   127,    -1,   128,    -1,   883,    -1,   884,    -1,
     885,    -1,   145,    -1,   146,    -1,   147,    -1,   148,    -1,
     149,    -1,   150,    -1,   152,    -1,   138,    -1,   153,    -1,
     154,    -1,   155,    -1,   156,    -1,   157,    -1,   158,    -1,
     159,    -1,   160,    -1,   161,    -1,   162,    -1,   163,    -1,
     164,    -1,   165,    -1,   166,    -1,   167,    -1,   151,    -1,
     168,    -1,   170,    -1,   171,    -1,   172,    -1,   173,    -1,
     174,    -1,   175,    -1,   137,    -1,   136,    -1,   139,    -1,
     141,    -1,   144,    -1,   140,    -1,   142,    -1,   887,    -1,
     888,    -1,    37,    -1,    38,    -1,    40,    -1,    41,    -1,
      43,    -1,    50,    -1,    52,    -1,    54,    -1,    57,    -1,
      58,    -1,    59,    -1,    64,    -1,    65,    -1,    66,    -1,
      76,    -1,    79,    -1,    80,    -1,    81,    -1,    82,    -1,
      85,    -1,    87,    -1,    90,    -1,    92,    -1,    95,    -1,
      96,    -1,    98,    -1,    99,    -1,   100,    -1,   101,    -1,
     102,    -1,   103,    -1,    42,    -1,    44,    -1,    45,    -1,
      46,    -1,    47,    -1,    48,    -1,    49,    -1,    51,    -1,
      53,    -1,    55,    -1,    56,    -1,    60,    -1,    61,    -1,
      62,    -1,    63,    -1,    67,    -1,    68,    -1,    69,    -1,
      70,    -1,    71,    -1,    72,    -1,    74,    -1,    75,    -1,
      77,    -1,    78,    -1,    97,    -1,    83,    -1,    84,    -1,
      86,    -1,    88,    -1,    89,    -1,    91,    -1,    93,    -1,
      94,    -1,   191,    -1,   192,    -1,   204,    -1,   226,    -1,
     229,    -1,   230,    -1,   232,    -1,   234,    -1,   231,    -1,
     235,    -1,   236,    -1,   237,    -1,   238,    -1,   239,    -1,
     240,    -1,   233,    -1,   242,    -1,   246,    -1,   249,    -1,
     252,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   448,   448,   448,   449,   449,   450,   450,   451,   451,
     452,   452,   453,   453,   454,   454,   455,   455,   456,   456,
     457,   457,   457,   458,   458,   458,   459,   459,   460,   460,
     463,   464,   465,   466,   467,   471,   472,   476,   477,   478,
     479,   483,   484,   485,   486,   490,   491,   492,   493,   494,
     495,   496,   497,   498,   499,   500,   501,   502,   503,   504,
     505,   506,   507,   508,   509,   510,   511,   512,   513,   514,
     515,   516,   517,   521,   522,   523,   524,   525,   526,   527,
     528,   529,   530,   531,   532,   533,   534,   535,   536,   537,
     538,   539,   540,   541,   542,   543,   544,   545,   546,   547,
     548,   552,   553,   554,   555,   556,   557,   558,   559,   560,
     561,   562,   563,   564,   565,   566,   567,   568,   569,   570,
     571,   572,   573,   574,   575,   576,   577,   578,   579,   580,
     581,   582,   584,   585,   586,   587,   588,   589,   590,   591,
     593,   594,   595,   596,   597,   598,   599,   600,   602,   603,
     604,   605,   606,   607,   608,   609,   610,   611,   612,   613,
     614,   615,   616,   617,   618,   619,   620,   621,   622,   623,
     624,   625,   626,   627,   628,   629,   630,   631,   632,   633,
     634,   635,   636,   637,   638,   639,   640,   641,   642,   643,
     644,   645,   646,   647,   648,   649,   650,   651,   652,   653,
     654,   655,   664,   663,   673,   672,   681,   682,   683,   684,
     685,   685,   686,   690,   691,   692,   693,   694,   694,   695,
     699,   700,   701,   702,   702,   706,   707,   708,   709,   709,
     713,   713,   714,   715,   719,   719,   720,   721,   724,   724,
     726,   726,   728,   729,   729,   730,   731,   735,   735,   736,
     737,   746,   745,   752,   751,   757,   757,   758,   758,   759,
     763,   763,   764,   764,   765,   768,   771,   772,   772,   773,
     773,   775,   774,   781,   780,   789,   790,   790,   792,   791,
     801,   801,   802,   806,   807,   807,   811,   812,   825,   831,
     837,   843,   849,   869,   870,   870,   871,   872,   879,   880,
     882,   883,   884,   891,   894,   895,   899,   900,   904,   905,
     912,   914,   917,   918,   922,   923,   930,   936,   939,   940,
     941,   948,   954,   960,   966,   968,   969,   970,   977,   983,
     989,   992,   993,  1000,  1006,  1012,  1023,  1034,  1045,  1049,
    1053,  1061,  1064,  1069,  1073,  1073,  1074,  1075,  1075,  1077,
    1076,  1085,  1086,  1086,  1090,  1092,  1091,  1099,  1100,  1101,
    1103,  1102,  1109,  1108,  1112,  1111,  1121,  1122,  1123,  1130,
    1134,  1133,  1143,  1144,  1145,  1146,  1150,  1157,  1163,  1163,
    1165,  1170,  1169,  1178,  1179,  1183,  1184,  1188,  1189,  1193,
    1194,  1195,  1202,  1205,  1206,  1213,  1219,  1226,  1229,  1230,
    1234,  1241,  1245,  1250,  1254,  1259,  1263,  1264,  1268,  1269,
    1273,  1274,  1287,  1293,  1299,  1305,  1308,  1313,  1320,  1323,
    1324,  1331,  1337,  1340,  1345,  1352,  1365,  1376,  1376,  1382,
    1383,  1386,  1386,  1387,  1388,  1392,  1392,  1393,  1394,  1398,
    1399,  1406,  1407,  1410,  1410,  1411,  1412,  1416,  1416,  1417,
    1418,  1422,  1424,  1426,  1431,  1432,  1436,  1437,  1438,  1442,
    1443,  1444,  1451,  1451,  1457,  1457,  1463,  1463,  1469,  1469,
    1470,  1470,  1476,  1476,  1482,  1482,  1488,  1488,  1489,  1489,
    1495,  1496,  1499,  1500,  1505,  1509,  1510,  1511,  1515,  1520,
    1524,  1525,  1532,  1535,  1536,  1543,  1543,  1544,  1544,  1545,
    1545,  1546,  1546,  1547,  1547,  1548,  1548,  1549,  1549,  1550,
    1550,  1553,  1554,  1561,  1561,  1567,  1567,  1573,  1573,  1574,
    1574,  1580,  1580,  1583,  1584,  1591,  1591,  1592,  1592,  1598,
    1598,  1604,  1604,  1610,  1613,  1614,  1621,  1622,  1625,  1626,
    1627,  1628,  1632,  1633,  1637,  1638,  1639,  1640,  1646,  1647,
    1653,  1654,  1659,  1662,  1663,  1664,  1665,  1672,  1675,  1676,
    1683,  1684,  1687,  1688,  1689,  1693,  1694,  1695,  1698,  1703,
    1704,  1705,  1709,  1710,  1713,  1714,  1716,  1721,  1722,  1723,
    1727,  1728,  1732,  1733,  1734,  1738,  1739,  1740,  1743,  1744,
    1750,  1750,  1751,  1751,  1757,  1760,  1761,  1768,  1768,  1774,
    1774,  1780,  1783,  1784,  1791,  1791,  1792,  1792,  1793,  1793,
    1794,  1794,  1795,  1795,  1796,  1796,  1797,  1797,  1798,  1798,
    1800,  1801,  1808,  1809,  1812,  1813,  1814,  1815,  1816,  1817,
    1822,  1823,  1827,  1828,  1829,  1830,  1834,  1835,  1836,  1837,
    1840,  1841,  1844,  1849,  1853,  1854,  1861,  1864,  1865,  1866,
    1870,  1871,  1878,  1879,  1882,  1883,  1884,  1885,  1886,  1887,
    1888,  1889,  1890,  1891,  1892,  1893,  1894,  1898,  1899,  1900,
    1901,  1902,  1903,  1904,  1905,  1909,  1909,  1910,  1910,  1911,
    1912,  1913,  1914,  1915,  1919,  1920,  1924,  1925,  1929,  1930,
    1934,  1935,  1939,  1940,  1941,  1945,  1946,  1947,  1951,  1952,
    1953,  1957,  1958,  1959,  1963,  1967,  1970,  1978,  1979,  1980,
    1984,  1985,  1986,  1990,  1991,  1992,  1996,  1997,  2001,  2002,
    2006,  2007,  2008,  2009,  2017,  2018,  2022,  2023,  2027,  2028,
    2029,  2030,  2031,  2032,  2041,  2045,  2046,  2050,  2051,  2052,
    2053,  2054,  2055,  2056,  2057,  2061,  2065,  2066,  2070,  2071,
    2074,  2077,  2078,  2079,  2083,  2084,  2085,  2089,  2090,  2097,
    2101,  2102,  2109,  2113,  2114,  2118,  2119,  2123,  2124,  2128,
    2129,  2133,  2134,  2141,  2145,  2146,  2150,  2151,  2155,  2156,
    2166,  2167,  2174,  2175,  2176,  2177,  2181,  2182,  2183,  2184,
    2185,  2186,  2187,  2194,  2197,  2198,  2205,  2208,  2209,  2210,
    2217,  2220,  2221,  2228,  2231,  2232,  2233,  2237,  2238,  2242,
    2243,  2250,  2254,  2255,  2256,  2259,  2260,  2265,  2266,  2273,
    2275,  2276,  2279,  2280,  2287,  2287,  2288,  2288,  2294,  2294,
    2300,  2300,  2306,  2307,  2310,  2311,  2312,  2316,  2317,  2321,
    2322,  2323,  2327,  2328,  2329,  2333,  2334,  2337,  2343,  2346,
    2347,  2354,  2354,  2360,  2360,  2366,  2366,  2372,  2372,  2378,
    2381,  2382,  2383,  2390,  2390,  2391,  2391,  2397,  2397,  2398,
    2398,  2404,  2404,  2405,  2405,  2411,  2411,  2412,  2412,  2418,
    2418,  2419,  2419,  2425,  2428,  2429,  2436,  2439,  2440,  2447,
    2447,  2453,  2453,  2459,  2460,  2463,  2463,  2464,  2465,  2469,
    2469,  2470,  2471,  2475,  2476,  2483,  2483,  2484,  2484,  2490,
    2493,  2494,  2501,  2501,  2502,  2502,  2508,  2508,  2509,  2509,
    2515,  2515,  2516,  2516,  2522,  2522,  2523,  2523,  2529,  2532,
    2537,  2544,  2547,  2548,  2549,  2556,  2556,  2557,  2557,  2563,
    2563,  2569,  2569,  2575,  2575,  2576,  2576,  2582,  2582,  2588,
    2588,  2594,  2608,  2611,  2612,  2616,  2621,  2629,  2630,  2638,
    2639,  2647,  2648,  2653,  2654,  2655,  2659,  2660,  2661,  2669,
    2670,  2675,  2676,  2677,  2681,  2682,  2683,  2691,  2692,  2693,
    2697,  2698,  2702,  2703,  2704,  2705,  2713,  2714,  2715,  2719,
    2720,  2724,  2725,  2726,  2727,  2732,  2737,  2738,  2739,  2745,
    2750,  2753,  2754,  2755,  2760,  2763,  2764,  2765,  2773,  2774,
    2782,  2783,  2791,  2792,  2796,  2797,  2798,  2806,  2807,  2815,
    2816,  2817,  2818,  2822,  2823,  2824,  2828,  2829,  2830,  2831,
    2835,  2836,  2837,  2845,  2846,  2850,  2851,  2852,  2853,  2857,
    2858,  2859,  2867,  2868,  2869,  2873,  2877,  2878,  2879,  2880,
    2884,  2885,  2886,  2890,  2894,  2895,  2896,  2906,  2907,  2911,
    2912,  2916,  2917,  2921,  2922,  2926,  2927,  2931,  2932,  2936,
    2937,  2941,  2942,  2946,  2947,  2951,  2952,  2956,  2957,  2961,
    2962,  2963,  2964,  2965,  2969,  2970,  2971,  2972,  2977,  2978,
    2982,  2983,  2986,  2990,  2991,  2995,  2996,  3000,  3001,  3004,
    3007,  3008,  3009,  3010,  3014,  3015,  3019,  3020,  3024,  3025,
    3029,  3030,  3034,  3035,  3039,  3040,  3044,  3045,  3049,  3050,
    3054,  3055,  3059,  3060,  3061,  3065,  3066,  3070,  3071,  3075,
    3076,  3080,  3081,  3085,  3086,  3090,  3091,  3095,  3096,  3099,
    3101,  3103,  3104,  3105,  3106,  3107,  3108,  3109,  3110,  3111,
    3112,  3114,  3115,  3116,  3117,  3120,  3121,  3125,  3129,  3133,
    3137,  3141,  3142,  3143,  3144,  3148,  3149,  3150,  3151,  3160,
    3163,  3164,  3168,  3169,  3173,  3174,  3178,  3179,  3183,  3185,
    3187,  3189,  3191,  3193,  3195,  3197,  3199,  3201,  3203,  3205,
    3207,  3209,  3211,  3213,  3215,  3217,  3219,  3221,  3223,  3225,
    3227,  3229,  3231,  3233,  3238,  3239,  3240,  3241,  3242,  3243,
    3244,  3245,  3246,  3247,  3248,  3249,  3250,  3251,  3252,  3253,
    3254,  3255,  3256,  3257,  3258,  3259,  3260,  3261,  3265,  3266,
    3270,  3271,  3272,  3273,  3274,  3275,  3279,  3284,  3288,  3289,
    3289,  3293,  3294,  3294,  3304,  3305,  3306,  3307,  3308,  3309,
    3313,  3314,  3315,  3316,  3317,  3321,  3322,  3322,  3322,  3326,
    3326,  3326,  3326,  3326,  3326,  3327,  3327,  3328,  3328,  3328,
    3328,  3329,  3329,  3329,  3329,  3332,  3332,  3334,  3334,  3337,
    3337,  3338,  3338,  3338,  3339,  3339,  3339,  3339,  3339,  3339,
    3340,  3340,  3340,  3340,  3344,  3344,  3344,  3345,  3345,  3345,
    3345,  3345,  3345,  3346,  3346,  3346,  3355,  3355,  3358,  3359,
    3359,  3359,  3359,  3362,  3362,  3362,  3362,  3365,  3365,  3365,
    3366,  3367,  3368,  3369,  3370,  3371,  3372,  3373,  3374,  3375,
    3376,  3377,  3378,  3379,  3380,  3381,  3382,  3383,  3384,  3385,
    3386,  3387,  3388,  3389,  3390,  3391,  3392,  3393,  3394,  3395,
    3396,  3399,  3399,  3399,  3401,  3401,  3403,  3403,  3406,  3407,
    3423,  3423,  3423,  3423,  3423,  3424,  3424,  3424,  3424,  3424,
    3425,  3425,  3425,  3425,  3425,  3426,  3426,  3426,  3426,  3426,
    3427,  3427,  3427,  3427,  3427,  3428,  3428,  3428,  3428,  3428,
    3429,  3433,  3434,  3435,  3436,  3437,  3438,  3439,  3440,  3441,
    3442,  3443,  3444,  3445,  3446,  3447,  3448,  3449,  3450,  3451,
    3452,  3453,  3454,  3455,  3456,  3457,  3458,  3459,  3460,  3461,
    3462,  3463,  3464,  3465,  3466,  3468,  3469,  3470,  3471,  3472,
    3473,  3474,  3475,  3476,  3477,  3478,  3479,  3480,  3481,  3482,
    3483,  3484,  3485,  3486,  3487
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TOK_BracketOnly", "TypeUndef",
  "TypeEndLine", "TypeComment", "TypeNumber", "TypeLiteral",
  "TypeIdentifier", "TypeMacroName", "TypeMacroArg", "TypeEndOfINCLUDE",
  "TypeNoFileINCLUDE", "TypeWhiteSpace", "TypeMOD", "TypeSHL", "TypeSHR",
  "TypeNOT", "TypeEQ", "TypeLT", "TypeLE", "TypeGT", "TypeGE", "TypeNE",
  "TypeNUL", "TypeDEFINED", "TypeHIGH", "TypeLOW", "\"<<\"", "\">>\"",
  "\"<=\"", "\">=\"", "\"!=\"", "\"&&\"", "\"||\"", "\"##\"", "TypeADC",
  "TypeADD", "TypeAND", "TypeBIT", "TypeCALL", "TypeCCF", "TypeCP",
  "TypeCPD", "TypeCPDR", "TypeCPI", "TypeCPIR", "TypeCPL", "TypeDAA",
  "TypeDEC", "TypeDI", "TypeDJNZ", "TypeEI", "TypeEX", "TypeEXX",
  "TypeHALT", "TypeIM", "TypeIN", "TypeINC", "TypeIND", "TypeINDR",
  "TypeINI", "TypeINIR", "TypeJP", "TypeJR", "TypeLD", "TypeLDD",
  "TypeLDDR", "TypeLDI", "TypeLDIR", "TypeNEG", "TypeNOP", "TypeOR",
  "TypeOTDR", "TypeOTIR", "TypeOUT", "TypeOUTD", "TypeOUTI", "TypePOP",
  "TypePUSH", "TypeRES", "TypeRET", "TypeRETI", "TypeRETN", "TypeRL",
  "TypeRLA", "TypeRLC", "TypeRLCA", "TypeRLD", "TypeRR", "TypeRRA",
  "TypeRRC", "TypeRRCA", "TypeRRD", "TypeRST", "TypeSBC", "TypeSCF",
  "TypeSET", "TypeSLA", "TypeSLL", "TypeSRA", "TypeSRL", "TypeSUB",
  "TypeXOR", "TypeA", "TypeAF", "TypeAFp", "TypeB", "TypeBC", "TypeD",
  "TypeE", "TypeDE", "TypeH", "TypeL", "TypeHL", "TypeSP", "TypeIX",
  "TypeIXH", "TypeIXL", "TypeIY", "TypeIYH", "TypeIYL", "TypeI", "TypeR",
  "TypeNZ", "TypeZ", "TypeNC", "TypeC", "TypePO", "TypePE", "TypeP",
  "TypeM", "TypeASEG", "TypeCOMMON", "TypeCSEG", "TypeDB", "TypeDEFB",
  "TypeDEFL", "TypeDEFM", "TypeDEFS", "TypeDEFW", "TypeDS", "TypeDSEG",
  "TypeDW", "TypeELSE", "TypeEND", "TypeENDIF", "TypeENDM", "TypeENDP",
  "TypeEQU", "TypeEXITM", "TypeEXTRN", "TypeIF", "TypeIF1", "TypeIF2",
  "TypeIFDEF", "TypeIFNDEF", "TypeINCBIN", "TypeINCLUDE", "TypeIRP",
  "TypeIRPC", "TypeLOCAL", "TypeMACRO", "TypeORG", "TypePROC",
  "TypePUBLIC", "TypeREPT", "Type_8080", "Type_8086", "Type_DEPHASE",
  "Type_ERROR", "Type_PHASE", "Type_SHIFT", "Type_WARNING", "Type_Z80",
  "TypeAND_8080", "TypeOR_8080", "TypeXOR_8080", "TypeM_8080",
  "TypePSW_8080", "TypeSET_8080", "TypeADC_8080", "TypeADD_8080",
  "TypeACI_8080", "TypeADI_8080", "TypeANA_8080", "TypeANI_8080",
  "TypeCALL_8080", "TypeCC_8080", "TypeCM_8080", "TypeCMA_8080",
  "TypeCMC_8080", "TypeCMP_8080", "TypeCNC_8080", "TypeCNZ_8080",
  "TypeCP_8080", "TypeCPE_8080", "TypeCPI_8080", "TypeCPO_8080",
  "TypeCZ_8080", "TypeDAD_8080", "TypeDCR_8080", "TypeDCX_8080",
  "TypeHLT_8080", "TypeIN_8080", "TypeINR_8080", "TypeINX_8080",
  "TypeJC_8080", "TypeJM_8080", "TypeJMP_8080", "TypeJNC_8080",
  "TypeJNZ_8080", "TypeJP_8080", "TypeJPE_8080", "TypeJPO_8080",
  "TypeJZ_8080", "TypeLDA_8080", "TypeLDAX_8080", "TypeLHLD_8080",
  "TypeLXI_8080", "TypeMVI_8080", "TypeMOV_8080", "TypeORA_8080",
  "TypeORI_8080", "TypeOUT_8080", "TypePCHL_8080", "TypePOP_8080",
  "TypePUSH_8080", "TypeRAL_8080", "TypeRAR_8080", "TypeRLC_8080",
  "TypeRC_8080", "TypeRRC_8080", "TypeRET_8080", "TypeRM_8080",
  "TypeRNC_8080", "TypeRNZ_8080", "TypeRP_8080", "TypeRPE_8080",
  "TypeRPO_8080", "TypeRST_8080", "TypeRZ_8080", "TypeSBB_8080",
  "TypeSBI_8080", "TypeSHLD_8080", "TypeSPHL_8080", "TypeSTA_8080",
  "TypeSTAX_8080", "TypeSTC_8080", "TypeSUB_8080", "TypeSUI_8080",
  "TypeXCHG_8080", "TypeXRA_8080", "TypeXRI_8080", "TypeXTHL_8080", "'|'",
  "'&'", "'~'", "'!'", "'='", "'<'", "'>'", "'-'", "'+'", "'*'", "'/'",
  "'%'", "':'", "','", "'('", "'['", "')'", "']'", "'?'", "'$'", "$accept",
  ".mod.", ".shl.", ".shr.", ".eq.", ".ne.", ".lt.", ".gt.", ".le.",
  ".ge.", ".and.", ".or.", ".not.", ".xor.", "pasmo_exp", "pasmoinstruc",
  "instruction", "br_instruction", "nolabelinstruction",
  "br_nolabelinstruction", "common_instruction", "instLabel", "@1",
  "br_instLabel", "@2", "labeled", "@3", "br_labeled", "@4", "label_colon",
  "@5", "br_label_colon", "@6", "label_colon_inst", "@7",
  "br_label_colon_inst", "@8", "label_nocolon", "@9", "br_label_nocolon",
  "@10", "afterlabel", "@11", "br_afterlabel", "@12", "instMacroName",
  "@13", "br_instMacroName", "@14", "MacroNameArgs", "@15", "@16",
  "br_MacroNameArgs", "@17", "@18", "MacroArgs", "MacroArgList", "@19",
  "@20", "@21", "@22", "MacroMore", "@23", "@24", "MacroArgValue", "@25",
  "MacroAfterValue", "@26", "MacroAfterArg", "instASEG", "instCSEG",
  "instDSEG", "instCOMMON", "instDEFB", "defblist", "@27", "instDEFL",
  "instSET_8080", "DEFLargs", "instDEFS", "DEFS_args", "DEFS_count_arg",
  "DEFS_value", "instDEFW", "DEFWargs", "DEFWmore", "DEFWitem", "instELSE",
  "instEND", "ENDargs", "instENDIF", "instENDM", "instENDP", "instEQU",
  "EQUargs", "instEXITM", "instEXTRN", "instIF", "IFarg", "instIF1",
  "instIF2", "instIFDEF", "instIFNDEF", "instINCBIN", "instINCLUDE",
  "instNoFileINCLUDE", "instEndOfINCLUDE", "instIRP", "IRPargs",
  "IRParglist", "@28", "@29", "@30", "IRPnextitem", "@31", "IRPitem",
  "@32", "IRPmacroarglist", "@33", "@34", "@35", "IRPmacronextitem",
  "instIRPC", "IRPCargs", "@36", "IRPCvalue", "instLOCAL", "instMACRO",
  "@37", "instMACROnolabel", "MACROnolabelname", "@38", "MACROnolabelargs",
  "MACROargs", "MACROmoreargs", "MACROitem", "instORG", "ORGarg",
  "instPROC", "instPUBLIC", "instREPT", "REPT_count", "REPT_count_args",
  "REPT_var", "REPT_var_args", "REPT_initial", "REPT_initial_arg",
  "REPT_step", "inst_8080", "inst_8086", "inst_DEPHASE", "inst_ERROR",
  "args_ERROR", "inst_PHASE", "_phase", "inst_SHIFT", "inst_WARNING",
  "args_WARNING", "inst_Z80", "instNoArgs", "instACI_8080", "@39",
  "instADC", "br_instADC", "ADCargs", "@40", "br_ADCargs", "@41",
  "ADC_HL_arg", "instADD", "br_instADD", "ADDargs", "@42", "br_ADDargs",
  "@43", "com_ADDargs", "ADD_HL_arg", "ADD_IX_arg", "ADD_IY_arg",
  "instADC_8080", "@44", "instADD_8080", "@45", "instADI_8080", "@46",
  "instAND", "@47", "br_instAND", "@48", "instANA_8080", "@49",
  "instANI_8080", "@50", "instBIT", "@51", "br_instBIT", "@52", "instCALL",
  "br_instCALL", "CALLargs", "br_CALLargs", "CALL_flag", "br_CALL_flag",
  "instCALL_8080", "CALL_8080args", "instCC_8080", "@53", "instCM_8080",
  "@54", "instCNC_8080", "@55", "instCNZ_8080", "@56", "instCP_8080",
  "@57", "instCPE_8080", "@58", "instCPO_8080", "@59", "instCZ_8080",
  "@60", "Cflag_arg", "instCMP_8080", "@61", "instCPI_8080", "@62",
  "instCP", "@63", "br_instCP", "@64", "instDAD_8080", "@65", "DADarg",
  "instDEC", "@66", "br_instDEC", "@67", "instDCR_8080", "@68",
  "instDCX_8080", "@69", "instDJNZ", "DJNZarg", "instEX", "br_instEX",
  "EX_args", "br_EX_args", "com_EX_args", "EX_AF_arg", "EX_DE_arg",
  "EX_indSP", "EX_indSP_arg", "instIM", "IMarg", "instIN", "br_instIN",
  "INargs", "br_INargs", "regNoA_IN", "IN_r_arg", "br_IN_r_arg",
  "IN_r_paren", "IN_r_bracket", "IN_r_indCend", "IN_A_arg", "br_IN_A_arg",
  "IN_A_bracket", "IN_A_paren", "IN_A_C_end", "IN_A_exp_end", "instINC",
  "@70", "br_instINC", "@71", "instIN_8080", "IN_8080arg", "instINR_8080",
  "@72", "instINX_8080", "@73", "instJMP_8080", "JMParg", "instJC_8080",
  "@74", "instJNC_8080", "@75", "instJNZ_8080", "@76", "instJM_8080",
  "@77", "instJP_8080", "@78", "instJPE_8080", "@79", "instJPO_8080",
  "@80", "instJZ_8080", "@81", "Jflag_arg", "instJP", "br_instJP", "JParg",
  "br_JParg", "JP_bracket", "JP_flag", "br_JP_flag", "JP_flag_arg",
  "br_JP_flag_arg", "instJR", "JRarg", "JR_flag_arg", "instLD",
  "br_instLD", "LDargs", "br_LDargs", "com_LDargs", "@82", "@83",
  "LD_IorR", "LD_idesp_arg", "LD_undoc", "br_LD_undoc", "LD_undocix_arg",
  "LD_undociy_arg", "br_LD_undocix_arg", "br_LD_undociy_arg",
  "LD_undoc_rx", "LD_undoc_ry", "LD_undoc_exp", "LD_r_arg", "br_LD_r_arg",
  "LD_r_bracket", "LD_HorL", "br_LD_HorL", "LD_anyr_arg", "br_LD_anyr_arg",
  "com_LD_anyr_arg", "LD_A_args", "br_LD_A_args", "com_LD_A_args",
  "LD_A_IorR", "LD_indHL_arg", "LD_indBCorDE", "LD_rr", "br_LD_rr",
  "LD_HL_arg", "LD_rr_arg", "br_LD_HL_arg", "br_LD_rr_arg",
  "LD_HL_bracket", "LD_rr_bracket", "LD_IXY_arg", "br_LD_IXY_arg",
  "LD_IXY_bracket", "LD_SP_arg", "br_LD_SP_arg", "com_LD_SP_arg",
  "LD_indexp_arg", "instLDA_8080", "LDAarg", "instLDAX_8080", "LDAXarg",
  "instLHLD_8080", "LHLDarg", "instLXI_8080", "LXIargs", "LXI_H_arg",
  "LXI_rr_arg", "instMOV_8080", "MOVargs", "MOV_noM_arg", "MOV_M_arg",
  "instMVI_8080", "MVIargs", "MVI_r", "instOR", "@84", "br_instOR", "@85",
  "instORA_8080", "@86", "instORI_8080", "@87", "instOUT", "br_instOUT",
  "OUTargs", "br_OUTargs", "OUTbracket", "OUTparen", "OUT_c_", "OUT_exp_",
  "instOUT_8080", "OUT_8080arg", "instPOP", "@88", "instPOP_8080", "@89",
  "instPUSH", "@90", "instPUSH_8080", "@91", "instRET", "RET_arg",
  "instRES", "@92", "br_instRES", "@93", "instRL", "@94", "br_instRL",
  "@95", "instRLC", "@96", "br_instRLC", "@97", "instRR", "@98",
  "br_instRR", "@99", "instRRC", "@100", "br_instRRC", "@101", "instRST",
  "RSTarg", "instRST_8080", "RST_8080arg", "instSBB_8080", "@102",
  "instSBI_8080", "@103", "instSBC", "br_instSBC", "SBC_args", "@104",
  "br_SBC_args", "@105", "SBC_HL_arg", "instSET", "@106", "br_instSET",
  "@107", "instSHLD_8080", "SHLDargs", "instSLA", "@108", "br_instSLA",
  "@109", "instSLL", "@110", "br_instSLL", "@111", "instSRA", "@112",
  "br_instSRA", "@113", "instSRL", "@114", "br_instSRL", "@115",
  "instSTA_8080", "STAarg", "instSTAX_8080", "STAXarg", "instSUB", "@116",
  "br_instSUB", "@117", "instSUB_8080", "@118", "instSUI_8080", "@119",
  "instXOR", "@120", "br_instXOR", "@121", "instXRA_8080", "@122",
  "instXRI_8080", "@123", "instXTHL_8080", "VarList", "VarListMore",
  "VarListItem", "likeADD_8080", "likeADI", "likeADD_A_arg",
  "br_likeADD_A_arg", "likeCParg", "br_likeCParg", "likeCP_or_ADD_arg",
  "br_likeCP_or_ADD_arg", "com_likeCP_or_ADD_arg", "likeINC_arg",
  "br_likeINC_arg", "com_likeINC_arg", "likeINC_r", "likeINC_double",
  "likeINC_undoc", "likeINC_bracket", "likeINC_bracketarg",
  "likeINC_paren", "likeINC_parenarg", "likeINR", "likeINX", "likePUSH",
  "likePUSH_arg", "likePUSH_8080", "likeRL_arg", "br_likeRL_arg",
  "likeRL_noundoc", "br_likeRL_noundoc", "BIT_exp", "likeBIT_second",
  "br_likeBIT_second", "likeRLorBIT_arg", "br_likeRLorBIT_arg",
  "com_likeRLorBIT_arg", "likeRLorBIT_noundoc", "br_likeRLorBIT_noundoc",
  "com_likeRLorBIT_noundoc", "requireEndLine", "requireComma",
  "check_next_arg", "requireClose", "requireBrClose", "requireCloseExp",
  "requireBrCloseExp", "requireIdentifier", "requireA", "requireC",
  "requireLiteral", "flag_val", "jr_flag_val", "regsimple8080",
  "regsimple_code", "regM_8080", "regsimpleNoA_code", "regsimpleHL_code",
  "regsimpleNoHL_code", "regsimpleA_code", "regsimpleNoHLA_code",
  "regundocix", "regundociy", "regundoc", "regundocX_code",
  "regundocY_code", "regsimpleorundocX_code", "regsimpleorundocY_code",
  "regBCDEHLSP_code", "regdouble8080", "regdouble8080PSW",
  "regdouble8080common", "regdoubleBorDorSP", "regdoubleBorD",
  "regBCDESP_code", "regAFBCDEHL_code", "regBCDEHL_code", "regBCDE_code",
  "regSP_code", "br_exp_br", "par_indBC", "br_indBC", "par_indDE",
  "br_indDE", "par_indHL", "br_indHL", "par_indSP", "br_indSP",
  "par_indIX", "par_indIY", "HLClose", "HLBrClose", "IXBrClose",
  "IYBrClose", "regIXY_pref", "bracket_indIXYdesp_pref",
  "paren_indIXYdesp_pref", "br_indIXYdesp_pref", "par_indIXYdesp_pref",
  "par_tailIXYdesp", "br_tailIXYdesp", "exp", "condexp", "conditional",
  "condarg", "condarg2", "base", "paren", "primary", "DEFINEDarg",
  "NULarg", "@124", "eatline", "@125", "any_but_HL_IX_IY_exp",
  "token_with_name", "reg_not_flag", "reg_but_HL_IX_IY_notflag",
  "binary_op", "op_name", "binary_op_noname", "binary_op_name",
  "unary_op_name", "token_flag", "token_jr_flag", "token_directive",
  ".defb.", ".defw.", ".defs.", "token_instruction_notoperator",
  "token_instruction_with_args_notoperator",
  "token_instruction_without_args", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,   456,   457,   458,   459,   460,   461,   462,   463,   464,
     465,   466,   467,   468,   469,   470,   471,   472,   473,   474,
     475,   476,   477,   478,   479,   480,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,   492,   493,   494,
     495,   496,   497,   498,   499,   500,   501,   502,   503,   504,
     505,   506,   507,   508,   509,   510,   124,    38,   126,    33,
      61,    60,    62,    45,    43,    42,    47,    37,    58,    44,
      40,    91,    41,    93,    63,    36
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   276,   277,   277,   278,   278,   279,   279,   280,   280,
     281,   281,   282,   282,   283,   283,   284,   284,   285,   285,
     286,   286,   286,   287,   287,   287,   288,   288,   289,   289,
     290,   290,   290,   290,   290,   291,   291,   292,   292,   292,
     292,   293,   293,   293,   293,   294,   294,   294,   294,   294,
     294,   294,   294,   294,   294,   294,   294,   294,   294,   294,
     294,   294,   294,   294,   294,   294,   294,   294,   294,   294,
     294,   294,   294,   295,   295,   295,   295,   295,   295,   295,
     295,   295,   295,   295,   295,   295,   295,   295,   295,   295,
     295,   295,   295,   295,   295,   295,   295,   295,   295,   295,
     295,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   296,   298,   297,   300,   299,   301,   301,   301,   301,
     302,   301,   301,   303,   303,   303,   303,   304,   303,   303,
     305,   305,   305,   306,   305,   307,   307,   307,   308,   307,
     310,   309,   309,   309,   312,   311,   311,   311,   314,   313,
     316,   315,   317,   318,   317,   317,   317,   320,   319,   319,
     319,   322,   321,   324,   323,   326,   325,   327,   325,   325,
     329,   328,   330,   328,   328,   331,   332,   333,   332,   334,
     332,   335,   332,   336,   332,   337,   338,   337,   339,   337,
     341,   340,   340,   342,   343,   342,   344,   344,   345,   346,
     347,   348,   349,   350,   351,   350,   350,   350,   352,   353,
     354,   354,   354,   355,   356,   356,   357,   357,   358,   358,
     359,   360,   361,   361,   362,   362,   363,   364,   365,   365,
     365,   366,   367,   368,   369,   370,   370,   370,   371,   372,
     373,   374,   374,   375,   376,   377,   378,   379,   380,   381,
     382,   383,   384,   384,   386,   385,   385,   387,   385,   388,
     385,   389,   390,   389,   391,   392,   391,   393,   393,   393,
     394,   393,   395,   393,   396,   393,   397,   397,   397,   398,
     400,   399,   401,   401,   401,   401,   401,   402,   404,   403,
     405,   407,   406,   408,   408,   409,   409,   410,   410,   411,
     411,   411,   412,   413,   413,   414,   415,   416,   417,   417,
     418,   418,   419,   419,   420,   420,   421,   421,   422,   422,
     423,   423,   424,   425,   426,   427,   428,   428,   429,   430,
     430,   431,   432,   433,   433,   434,   435,   437,   436,   438,
     439,   441,   440,   440,   440,   443,   442,   442,   442,   444,
     444,   445,   446,   448,   447,   447,   447,   450,   449,   449,
     449,   451,   451,   451,   452,   452,   453,   453,   453,   454,
     454,   454,   456,   455,   458,   457,   460,   459,   462,   461,
     464,   463,   466,   465,   468,   467,   470,   469,   472,   471,
     473,   474,   475,   475,   475,   476,   476,   476,   477,   477,
     478,   478,   479,   480,   480,   482,   481,   484,   483,   486,
     485,   488,   487,   490,   489,   492,   491,   494,   493,   496,
     495,   497,   497,   499,   498,   501,   500,   503,   502,   505,
     504,   507,   506,   508,   508,   510,   509,   512,   511,   514,
     513,   516,   515,   517,   518,   518,   519,   520,   521,   521,
     521,   521,   522,   522,   523,   523,   523,   523,   524,   524,
     525,   525,   526,   527,   527,   527,   527,   528,   529,   529,
     530,   531,   532,   532,   532,   533,   533,   533,   534,   535,
     535,   535,   536,   536,   537,   538,   539,   540,   540,   540,
     541,   541,   542,   542,   542,   543,   543,   543,   544,   545,
     547,   546,   549,   548,   550,   551,   551,   553,   552,   555,
     554,   556,   557,   557,   559,   558,   561,   560,   563,   562,
     565,   564,   567,   566,   569,   568,   571,   570,   573,   572,
     574,   574,   575,   576,   577,   577,   577,   577,   577,   577,
     577,   577,   578,   578,   578,   578,   579,   579,   579,   579,
     580,   581,   582,   582,   583,   583,   584,   585,   585,   585,
     586,   586,   587,   588,   589,   589,   589,   589,   589,   589,
     589,   589,   589,   589,   589,   589,   589,   590,   590,   590,
     590,   590,   590,   590,   590,   592,   591,   593,   591,   591,
     591,   591,   591,   591,   594,   594,   595,   595,   596,   596,
     597,   597,   598,   598,   598,   599,   599,   599,   600,   600,
     600,   601,   601,   601,   602,   603,   604,   605,   605,   605,
     606,   606,   606,   607,   607,   607,   608,   608,   609,   609,
     610,   610,   610,   610,   611,   611,   612,   612,   613,   613,
     613,   613,   613,   613,   613,   614,   614,   615,   615,   615,
     615,   615,   615,   615,   615,   615,   616,   616,   617,   617,
     618,   619,   619,   619,   620,   620,   620,   621,   621,   621,
     622,   622,   622,   623,   623,   624,   624,   625,   625,   626,
     626,   627,   627,   627,   628,   628,   629,   629,   630,   630,
     631,   631,   632,   632,   632,   632,   633,   633,   633,   633,
     633,   633,   633,   634,   635,   635,   636,   637,   637,   637,
     638,   639,   639,   640,   641,   641,   641,   642,   642,   643,
     643,   644,   645,   645,   645,   646,   646,   647,   647,   648,
     649,   649,   650,   650,   652,   651,   654,   653,   656,   655,
     658,   657,   659,   660,   661,   661,   661,   662,   662,   663,
     663,   663,   664,   664,   664,   665,   665,   666,   667,   668,
     668,   670,   669,   672,   671,   674,   673,   676,   675,   677,
     678,   678,   678,   680,   679,   682,   681,   684,   683,   686,
     685,   688,   687,   690,   689,   692,   691,   694,   693,   696,
     695,   698,   697,   699,   700,   700,   701,   702,   702,   704,
     703,   706,   705,   707,   708,   710,   709,   709,   709,   712,
     711,   711,   711,   713,   713,   715,   714,   717,   716,   718,
     719,   719,   721,   720,   723,   722,   725,   724,   727,   726,
     729,   728,   731,   730,   733,   732,   735,   734,   736,   737,
     737,   738,   739,   739,   739,   741,   740,   743,   742,   745,
     744,   747,   746,   749,   748,   751,   750,   753,   752,   755,
     754,   756,   757,   758,   758,   759,   759,   760,   760,   761,
     761,   762,   762,   762,   762,   762,   763,   763,   763,   764,
     764,   764,   764,   764,   765,   765,   765,   766,   766,   766,
     767,   767,   768,   768,   768,   768,   769,   769,   769,   770,
     770,   771,   771,   771,   771,   772,   773,   773,   773,   774,
     775,   776,   776,   776,   777,   778,   778,   778,   779,   779,
     780,   780,   781,   781,   782,   782,   782,   783,   783,   784,
     784,   784,   784,   785,   785,   785,   786,   786,   786,   786,
     787,   787,   787,   788,   788,   789,   789,   789,   789,   790,
     790,   790,   791,   791,   791,   792,   793,   793,   793,   793,
     794,   794,   794,   795,   796,   796,   796,   797,   797,   798,
     798,   799,   799,   800,   800,   801,   801,   802,   802,   803,
     803,   804,   804,   805,   805,   806,   806,   807,   807,   808,
     808,   808,   808,   808,   809,   809,   809,   809,   810,   810,
     811,   811,   812,   813,   813,   814,   814,   815,   815,   816,
     817,   817,   817,   817,   818,   818,   819,   819,   820,   820,
     821,   821,   822,   822,   823,   823,   824,   824,   825,   825,
     826,   826,   827,   827,   827,   828,   828,   829,   829,   830,
     830,   831,   831,   832,   832,   833,   833,   834,   834,   835,
     836,   837,   838,   839,   840,   841,   842,   843,   844,   845,
     846,   847,   848,   849,   850,   851,   851,   852,   853,   854,
     855,   856,   856,   856,   856,   857,   857,   857,   857,   858,
     859,   859,   860,   860,   861,   861,   862,   862,   863,   863,
     863,   863,   863,   863,   863,   863,   863,   863,   863,   863,
     863,   863,   863,   863,   863,   863,   863,   863,   863,   863,
     863,   863,   863,   863,   863,   863,   863,   863,   863,   863,
     863,   863,   863,   863,   863,   863,   863,   863,   863,   863,
     863,   863,   863,   863,   863,   863,   863,   863,   864,   864,
     865,   865,   865,   865,   865,   865,   866,   866,   867,   868,
     867,   869,   870,   869,   871,   871,   871,   871,   871,   871,
     872,   872,   872,   872,   872,   873,   873,   873,   873,   874,
     874,   874,   874,   874,   874,   874,   874,   874,   874,   874,
     874,   874,   874,   874,   874,   875,   875,   876,   876,   877,
     877,   877,   877,   877,   877,   877,   877,   877,   877,   877,
     877,   877,   877,   877,   878,   878,   878,   878,   878,   878,
     878,   878,   878,   878,   878,   878,   879,   879,   880,   880,
     880,   880,   880,   881,   881,   881,   881,   882,   882,   882,
     882,   882,   882,   882,   882,   882,   882,   882,   882,   882,
     882,   882,   882,   882,   882,   882,   882,   882,   882,   882,
     882,   882,   882,   882,   882,   882,   882,   882,   882,   882,
     882,   883,   883,   883,   884,   884,   885,   885,   886,   886,
     887,   887,   887,   887,   887,   887,   887,   887,   887,   887,
     887,   887,   887,   887,   887,   887,   887,   887,   887,   887,
     887,   887,   887,   887,   887,   887,   887,   887,   887,   887,
     887,   888,   888,   888,   888,   888,   888,   888,   888,   888,
     888,   888,   888,   888,   888,   888,   888,   888,   888,   888,
     888,   888,   888,   888,   888,   888,   888,   888,   888,   888,
     888,   888,   888,   888,   888,   888,   888,   888,   888,   888,
     888,   888,   888,   888,   888,   888,   888,   888,   888,   888,
     888,   888,   888,   888,   888
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       0,     1,     1,     1,     1,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     3,     0,     3,     1,     1,     1,     1,
       0,     3,     1,     1,     1,     1,     1,     0,     3,     1,
       1,     1,     1,     0,     2,     1,     1,     1,     0,     2,
       0,     3,     1,     1,     0,     3,     1,     1,     0,     2,
       0,     2,     0,     0,     3,     1,     1,     0,     3,     1,
       1,     0,     3,     0,     3,     0,     3,     0,     3,     1,
       0,     3,     0,     3,     1,     1,     1,     0,     3,     0,
       3,     0,     3,     0,     3,     1,     0,     3,     0,     3,
       0,     3,     1,     1,     0,     3,     1,     2,     2,     2,
       2,     1,     2,     2,     0,     4,     2,     1,     2,     2,
       0,     2,     1,     3,     2,     1,     1,     2,     2,     1,
       2,     2,     1,     2,     1,     1,     2,     2,     1,     2,
       1,     2,     2,     2,     2,     0,     2,     1,     2,     2,
       3,     1,     1,     2,     2,     3,     3,     3,     3,     3,
       2,     2,     3,     1,     0,     3,     1,     0,     3,     0,
       3,     1,     0,     3,     1,     0,     3,     0,     1,     1,
       0,     3,     0,     3,     0,     3,     2,     1,     1,     2,
       0,     4,     1,     1,     1,     1,     1,     2,     0,     3,
       2,     0,     3,     1,     2,     1,     2,     1,     3,     1,
       1,     1,     3,     1,     1,     2,     2,     2,     2,     1,
       1,     2,     2,     1,     1,     2,     2,     1,     1,     2,
       2,     1,     2,     1,     2,     2,     2,     1,     2,     2,
       1,     2,     2,     2,     1,     2,     2,     0,     3,     2,
       2,     0,     4,     4,     1,     0,     4,     4,     1,     1,
       1,     2,     2,     0,     4,     1,     1,     0,     4,     1,
       1,     4,     4,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     3,     0,     3,     0,     3,     0,     3,
       0,     3,     0,     3,     0,     3,     0,     4,     0,     4,
       3,     3,     3,     1,     1,     3,     1,     1,     1,     1,
       1,     1,     3,     1,     1,     0,     3,     0,     3,     0,
       3,     0,     3,     0,     3,     0,     3,     0,     3,     0,
       3,     2,     1,     0,     3,     0,     3,     0,     3,     0,
       3,     0,     4,     1,     1,     0,     3,     0,     3,     0,
       3,     0,     3,     2,     2,     1,     3,     3,     1,     2,
       2,     1,     1,     1,     3,     3,     2,     2,     1,     1,
       1,     1,     2,     1,     1,     1,     1,     3,     1,     1,
       3,     3,     2,     3,     1,     2,     3,     1,     2,     2,
       2,     1,     2,     1,     3,     3,     1,     2,     2,     1,
       2,     1,     3,     3,     1,     3,     3,     1,     1,     1,
       0,     3,     0,     3,     3,     1,     1,     0,     3,     0,
       3,     3,     1,     1,     0,     3,     0,     3,     0,     3,
       0,     3,     0,     3,     0,     3,     0,     3,     0,     3,
       2,     1,     3,     3,     1,     2,     1,     1,     1,     1,
       2,     1,     1,     2,     1,     1,     1,     1,     1,     1,
       3,     3,     1,     1,     1,     1,     3,     3,     1,     1,
       1,     1,     2,     2,     1,     3,     4,     4,     2,     1,
       3,     3,     2,     2,     4,     5,     1,     1,     3,     4,
       4,     2,     1,     3,     1,     0,     4,     0,     4,     3,
       2,     2,     4,     3,     2,     1,     1,     1,     3,     3,
       3,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     1,     1,     1,
       2,     2,     2,     2,     1,     1,     2,     2,     2,     2,
       1,     2,     2,     2,     4,     2,     1,     1,     2,     2,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     1,
       2,     2,     1,     2,     2,     2,     2,     3,     1,     3,
       1,     2,     2,     1,     2,     2,     3,     1,     2,     2,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     1,     1,     3,     1,     1,     1,
       3,     1,     1,     3,     3,     3,     1,     1,     1,     1,
       1,     3,     3,     3,     1,     1,     1,     1,     1,     3,
       3,     1,     1,     1,     0,     3,     0,     3,     0,     3,
       0,     3,     3,     3,     2,     2,     1,     2,     1,     4,
       3,     1,     4,     3,     1,     1,     1,     2,     3,     1,
       1,     0,     3,     0,     3,     0,     3,     0,     3,     2,
       1,     2,     1,     0,     4,     0,     4,     0,     3,     0,
       3,     0,     3,     0,     3,     0,     3,     0,     3,     0,
       3,     0,     3,     2,     2,     1,     2,     2,     1,     0,
       3,     0,     3,     2,     2,     0,     4,     3,     1,     0,
       4,     3,     1,     2,     1,     0,     4,     0,     4,     3,
       1,     1,     0,     3,     0,     3,     0,     3,     0,     3,
       0,     3,     0,     3,     0,     3,     0,     3,     3,     1,
       1,     3,     1,     1,     1,     0,     3,     0,     3,     0,
       3,     0,     3,     0,     3,     0,     3,     0,     3,     0,
       3,     2,     2,     1,     2,     1,     1,     2,     1,     2,
       1,     1,     2,     2,     2,     1,     1,     2,     1,     1,
       2,     2,     2,     1,     1,     2,     1,     1,     2,     2,
       1,     2,     2,     2,     2,     2,     2,     2,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     1,     1,     1,     2,     1,     1,     1,     2,     1,
       2,     1,     2,     1,     1,     1,     1,     2,     1,     2,
       2,     2,     1,     2,     2,     1,     2,     2,     2,     1,
       2,     2,     1,     2,     1,     2,     2,     2,     1,     2,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     2,     2,     1,     1,     2,     2,     3,
       3,     1,     3,     3,     1,     1,     3,     3,     1,     1,
       1,     3,     2,     1,     2,     1,     1,     1,     1,     1,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     2,     2,     2,     3,     3,     3,
       3,     3,     2,     2,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     2,     2,     2,
       2,     3,     3,     3,     3,     3,     2,     2,     3,     2,
       1,     1,     1,     1,     2,     2,     1,     1,     1,     0,
       3,     1,     0,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,    34,     0,   101,   202,   251,     0,     0,     0,     0,
     468,   476,     0,  1391,   517,  1392,  1393,  1394,  1395,  1396,
    1397,   525,  1398,     0,  1399,     0,  1400,  1401,     0,     0,
     590,  1402,  1403,  1404,  1405,     0,     0,     0,  1406,  1407,
    1408,  1409,  1410,  1411,   824,  1412,  1413,     0,  1414,  1415,
     851,   855,   863,     0,  1417,  1418,   867,  1419,   871,  1420,
    1421,   875,  1422,   879,  1423,  1424,     0,     0,  1416,   905,
     912,   916,   920,   924,   935,   943,     0,   291,     0,  1352,
    1351,    33,  1353,  1356,  1354,  1357,     0,  1355,     0,     0,
       0,     0,     0,    32,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   413,     0,     0,     0,     0,     0,     0,   462,
     464,   427,   466,   472,   474,     0,   495,   497,  1425,  1426,
     513,   499,   501,   503,   505,   515,   507,   509,     0,   529,
     531,  1427,     0,   597,   599,   604,   610,     0,   606,   608,
     612,   614,   616,   618,     0,     0,     0,     0,     0,     0,
     828,   830,     0,  1428,   853,   857,  1429,  1430,  1433,  1431,
    1440,  1432,  1434,  1435,  1436,  1437,  1438,  1439,     0,  1441,
     889,   891,     0,  1442,     0,     0,  1443,   939,   941,  1444,
     947,   949,     0,     0,    31,    35,    40,    45,    37,    39,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    38,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     148,    46,    47,   149,   150,   151,    48,   152,   153,    49,
      50,   154,   158,   157,   159,   160,   161,   162,   163,   164,
     155,   156,    51,   165,    52,   166,   167,   141,    53,   142,
      54,    55,   169,   170,   171,   172,   173,   175,   176,   174,
     177,   178,   179,   180,    56,   143,    57,   181,   182,   168,
     183,   184,   185,    58,   186,   187,    59,   188,   144,   189,
     145,   190,   146,    60,    61,    62,    63,    64,   147,   191,
     192,   193,    65,    66,   194,    67,    68,    69,    70,   195,
     196,    71,   197,   198,    72,   199,   200,   201,     0,     0,
       0,     0,   204,   253,     0,     0,   470,   478,     0,   519,
     527,     0,     0,   592,     0,     0,   826,     0,   865,   869,
     873,   877,   881,     0,   907,   914,   918,   922,   926,   937,
     945,    36,    44,    73,    41,    43,    42,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   238,     0,  1058,  1057,   340,  1078,
    1077,     0,   434,     0,     0,   429,   446,     0,     0,     0,
       0,   441,   445,     0,     0,   484,  1230,  1232,  1231,    26,
       0,     0,     0,     0,  1084,  1085,  1086,  1087,  1080,  1081,
    1082,  1083,    27,     0,     0,     0,     0,  1233,     0,     0,
       0,  1079,   483,  1169,  1170,  1178,  1179,     0,     0,   535,
     533,     0,   541,     0,     0,     0,     0,     0,   538,     0,
       0,   559,     0,   558,   564,     0,  1100,  1102,  1103,  1095,
    1096,  1101,     0,     0,     0,  1094,  1093,     0,   631,     0,
       0,     0,   624,     0,   626,   627,   628,   629,   649,     0,
       0,   648,   666,     0,  1137,  1138,     0,     0,  1155,  1104,
    1105,  1156,  1106,  1107,   675,   677,     0,     0,   652,   654,
       0,   659,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   836,     0,
       0,     0,     0,     0,     0,   862,   860,   859,     0,     0,
       0,     0,     0,   885,   883,     0,   898,     0,     0,   893,
       0,     0,     0,     0,     0,     0,     0,   288,   289,   290,
     316,   320,   318,   317,     0,   321,   322,   323,   328,   956,
     955,   329,     0,   332,     0,   331,   333,   334,  1072,  1071,
       0,     0,     0,     0,   343,     0,   341,   369,     0,   377,
     381,   380,   394,     0,   393,   395,   396,   399,   397,     0,
     412,   414,   417,     0,   415,   420,   418,     0,   421,   424,
       0,   422,   425,     0,     0,     0,     0,     0,     0,   494,
       0,   493,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   524,  1129,  1130,  1126,  1139,   521,   523,  1120,
    1125,  1121,     0,     0,   596,     0,   595,     0,     0,     0,
       0,   603,     0,   602,     0,     0,     0,     0,     0,     0,
     795,     0,   794,   799,   797,   798,     0,   802,     0,   801,
     806,     0,     0,     0,  1127,  1128,   821,  1099,  1092,     0,
       0,  1088,  1089,  1091,  1090,   814,     0,     0,     0,     0,
       0,   850,     0,   849,     0,     0,   888,   886,     0,     0,
       0,   911,     0,   910,   930,     0,   929,   934,   932,   933,
       0,     0,     0,     0,     0,   951,     1,   297,   292,     0,
     315,   310,     0,   314,   305,     0,     0,   426,   240,     0,
     438,     0,     0,   430,   450,     0,   442,   449,     0,     0,
     487,     0,     0,   486,     0,     0,   543,     0,   542,   567,
       0,     0,     0,     0,   635,     0,     0,   632,     0,   634,
     674,     0,     0,     0,   653,   667,     0,   672,     0,     0,
       0,     0,     0,     0,     0,   838,     0,     0,     0,     0,
       0,     0,     0,   902,     0,     0,   894,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   378,     0,   210,   203,
     212,     0,   207,   208,   206,   209,   273,   266,   271,   257,
     269,   255,   267,   252,   259,   265,   339,  1060,  1059,   431,
       0,   443,     0,     0,     0,   973,     0,     0,   469,   969,
     977,     0,  1108,  1109,     0,     0,     0,     0,     0,     0,
    1034,     0,     0,  1239,  1238,  1235,  1237,  1236,  1234,  1226,
    1202,  1227,  1203,  1220,  1196,  1218,  1194,  1217,  1193,  1229,
       0,  1219,  1195,   480,     0,     2,     4,     6,     8,    12,
      16,    14,    18,    10,     5,     7,    17,    19,    11,     0,
       0,    20,    23,    28,    21,    24,    29,    25,    22,     9,
      13,    15,     0,     0,     0,     0,     3,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     518,   988,  1136,   997,   998,     0,     0,   526,     0,   991,
     992,   993,   994,     0,   995,   999,   996,  1118,  1135,  1119,
     534,     0,     0,   540,     0,   547,     0,   536,   539,     0,
     546,   557,     0,   560,   571,     0,     0,   562,   568,   591,
    1244,  1294,  1295,  1296,  1297,  1300,  1299,  1302,  1301,  1298,
    1282,  1283,  1286,  1288,  1285,  1292,  1293,  1360,  1361,  1303,
    1362,  1363,  1364,  1365,  1366,  1367,  1368,  1369,  1370,  1371,
    1372,  1373,  1304,  1374,  1375,  1376,  1377,  1378,  1379,  1380,
    1381,  1382,  1383,  1384,  1385,  1386,  1387,  1388,  1389,  1390,
    1305,  1259,  1271,  1260,  1272,  1261,  1262,  1273,  1263,  1264,
       0,  1274,     0,  1267,  1268,     0,  1269,  1270,  1265,  1266,
    1313,  1314,  1315,  1316,  1309,  1310,  1311,  1312,  1327,  1320,
    1321,  1322,  1323,  1324,  1325,  1343,  1326,  1328,  1329,  1330,
    1331,  1332,  1333,  1334,  1335,  1336,  1337,  1338,  1339,  1340,
    1341,  1342,  1344,  1345,  1346,  1347,  1348,  1349,  1350,  1291,
    1290,  1284,  1287,  1289,  1279,  1280,  1281,   630,  1245,  1247,
    1275,  1276,  1246,  1308,  1248,  1317,  1318,  1319,  1249,  1358,
    1359,   639,     0,     0,     0,   625,   636,   637,   638,   622,
       0,   646,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   658,     0,     0,
       0,     0,     0,     0,   662,     0,   680,   663,   681,     0,
       0,     0,     0,     0,   825,   844,     0,   835,     0,   841,
       0,   834,     0,   832,  1013,  1134,  1015,  1016,   852,     0,
    1014,  1133,   856,     0,   861,  1022,     0,     0,   868,     0,
    1042,  1046,  1049,  1043,  1047,  1048,  1044,   872,   876,   880,
     884,   895,     0,     0,   913,  1029,     0,     0,   917,     0,
    1050,  1054,  1051,  1055,  1056,  1052,   921,   925,   936,   944,
     319,  1062,   953,  1061,   952,     0,   330,   335,   336,   337,
     338,     0,   370,     0,   392,   400,   398,     0,   416,   419,
     423,   958,   463,     0,   465,   960,   428,     0,   467,   473,
     475,   492,   512,   496,     0,   498,   514,   500,   502,   504,
     506,   516,   508,   510,     0,  1009,   530,     0,  1011,   532,
       0,   594,   598,   600,   621,   605,     0,   611,   601,   607,
     609,   613,   615,   617,   619,   793,   796,   800,     0,   803,
       0,   819,     0,   811,     0,     0,   829,   831,   848,  1018,
    1124,  1123,   854,     0,  1122,   858,   887,   890,   892,   909,
     928,   931,   940,   942,   948,   950,   296,   293,   294,   312,
     311,     0,   303,   306,   304,     0,   217,   205,   219,     0,
     214,   215,   213,   216,   262,   260,   254,   264,   435,     0,
     447,   976,     0,   471,   974,   980,     0,     0,   481,     0,
     520,   990,   528,     0,   537,     0,   561,   573,     0,   565,
     593,   633,   623,     0,     0,     0,     0,   671,     0,     0,
       0,     0,     0,     0,   827,   837,   833,     0,  1025,     0,
     870,     0,  1045,   874,   878,   882,   899,     0,     0,   915,
    1032,     0,   919,     0,  1053,   923,   927,   938,   946,   302,
     298,     0,   327,   324,     0,     0,   299,   223,   246,   243,
     245,   239,     0,     0,     0,     0,     0,     0,     0,   440,
       0,   439,     0,   455,     0,   454,   458,   457,     0,   456,
    1131,  1132,   461,   460,     0,   459,   971,   972,   982,   983,
     978,   984,   985,   979,   970,  1038,     0,     0,   477,     0,
    1033,     0,  1068,  1067,  1228,   489,   482,   488,  1224,  1200,
    1225,  1201,  1210,  1186,  1209,  1185,  1204,  1180,  1205,  1181,
    1173,     0,  1171,  1206,  1182,  1207,  1183,  1208,  1184,  1211,
    1187,  1212,  1188,  1214,  1190,  1215,  1191,  1213,  1189,  1216,
    1192,  1221,  1197,  1222,  1198,  1223,  1199,  1007,     0,  1004,
    1005,     0,  1006,  1003,  1000,  1001,     0,  1002,   986,   987,
     549,   548,   544,   551,   550,   545,  1064,  1063,  1147,  1066,
    1065,  1148,   556,   553,   554,   555,   552,   579,     0,     0,
     563,  1076,  1075,   570,     0,   569,     0,  1145,  1149,  1150,
    1152,  1153,  1154,   643,   640,   642,   651,   647,   650,   734,
     746,   747,     0,     0,   655,   728,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   740,   729,     0,   759,     0,
     751,     0,   782,   783,   784,     0,   660,     0,   785,     0,
       0,     0,  1141,  1143,  1164,     0,     0,  1161,  1160,     0,
    1142,  1144,  1146,  1168,     0,     0,  1165,  1159,  1070,  1069,
    1140,     0,     0,     0,   716,   720,   727,   717,   722,   721,
     723,   709,     0,   707,   708,   694,  1110,  1111,   688,   692,
     693,  1115,  1098,  1097,  1114,   704,   706,   697,  1112,  1113,
     689,   695,   696,  1117,  1116,   705,   762,     0,   752,     0,
     786,   787,   788,   789,   790,   791,   792,   683,  1074,  1073,
       0,   661,     0,     0,   679,   773,     0,   753,     0,     0,
     686,   687,     0,     0,     0,     0,     0,  1012,   864,  1020,
    1021,  1019,     0,   904,   897,     0,   906,  1027,  1028,  1026,
     954,   349,   346,   344,   347,   342,     0,   383,   382,     0,
     403,     0,   401,   957,   959,   511,   522,  1008,  1010,   620,
     808,   805,   807,   810,   804,   809,   823,   820,   822,   816,
     812,   815,   818,   813,   817,  1017,     0,   313,   309,   307,
       0,   228,   250,   247,   249,   241,     0,     0,     0,     0,
       0,   975,   981,  1041,     0,   479,     0,   491,   485,   490,
     989,   581,     0,   566,   572,   645,   641,   644,   668,   735,
       0,     0,   754,     0,   673,     0,     0,     0,   718,   724,
     719,   725,   712,     0,   710,   711,   700,   690,   698,   699,
     703,   691,   701,   702,     0,   755,     0,     0,   756,     0,
     866,  1024,  1023,     0,   901,   908,  1031,  1030,   301,   326,
     391,   385,   389,  1306,  1307,  1256,  1257,  1258,   379,     0,
     390,  1250,  1255,  1254,  1277,  1278,  1251,  1252,  1253,   211,
       0,   221,   222,   220,     0,   278,   275,   276,   274,   286,
     272,     0,   258,   282,   270,   280,   256,   268,   965,     0,
       0,   432,   961,     0,   433,   444,   451,   452,   453,  1036,
    1037,  1035,  1242,  1241,  1240,  1175,     0,  1172,  1151,  1158,
    1157,   587,     0,   578,     0,   584,     0,   577,     0,     0,
       0,   745,     0,   737,   738,   739,   730,   741,   731,   742,
     732,   743,   733,   768,   757,     0,   758,   778,   779,   685,
       0,   676,   678,     0,     0,     0,     0,     0,   715,   726,
     714,   713,   657,   656,   770,   760,     0,   761,   750,   748,
     749,   777,   771,     0,   772,   682,   664,     0,   843,     0,
       0,   840,   896,   903,     0,     0,     0,   376,   375,   372,
     374,   373,   371,   384,   404,   402,     0,   295,   308,   218,
       0,   226,   227,   225,     0,   263,   261,   968,     0,   436,
     966,   437,   448,  1040,  1039,   580,   736,   763,   764,   780,
     781,   670,   669,   765,   766,   774,   775,   900,   387,   386,
       0,   233,   230,   232,   224,   244,     0,     0,   287,     0,
     963,   964,   962,     0,  1177,  1176,  1174,     0,     0,     0,
       0,   574,   576,   575,     0,     0,   684,  1163,  1162,   665,
    1167,  1166,     0,     0,   846,   842,   845,   847,   839,   351,
     350,   352,   364,   358,   360,   359,   362,   345,   348,   407,
     405,     0,   237,   234,   236,   229,   248,   967,     0,     0,
     279,   277,   283,   281,   284,  1243,   585,   588,   586,   589,
     582,   583,   744,   767,   769,   776,     0,     0,     0,     0,
     408,   406,     0,     0,   388,   231,     0,   355,   354,   353,
     368,   367,     0,   365,   361,   363,   411,   409,     0,   235,
     285,     0,   366,   410,   356
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,   898,   899,   900,   901,   902,   903,   904,   905,   906,
     907,   908,   438,   909,   193,   194,   195,   361,   196,   362,
     197,   198,   394,   364,   728,   799,  1367,  1287,  1681,  1769,
    1770,  1889,  1890,  1924,  1979,  1975,  2003,   800,   801,  1288,
    1289,  1371,  1774,  1685,  1894,   199,   395,   365,   729,   813,
    1376,  1374,  1296,  1687,  1686,   814,   815,  1377,  1375,  1373,
    1372,  1778,  1927,  1926,  1784,  1929,  1983,  2006,  1780,   200,
     201,   202,   203,   204,   718,  1676,   802,   803,  1360,   205,
     725,  1284,  1679,   206,   721,  1280,   722,   207,   208,   563,
     209,   210,   211,   804,  1363,   212,   213,   214,   574,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   586,  1645,
    1875,  1876,  1874,  1960,  1996,  2009,  2021,  1967,  1998,  1999,
    1997,  2013,   224,   587,  1646,  1882,   225,   805,  1365,   226,
     591,  1193,  1648,  1758,  1919,  1759,   227,   593,   228,   229,
     230,   598,  1196,  1652,  1885,  1970,  2001,  2017,   231,   232,
     233,   234,   604,   235,   606,   236,   237,   611,   238,   239,
     240,   615,   241,   367,   405,  1378,   733,  1688,  1380,   242,
     368,   411,  1382,   736,  1690,   412,  1384,  1388,  1394,   243,
     613,   244,   614,   245,   616,   246,   413,   369,   738,   247,
     617,   248,   618,   249,   414,   370,   739,   250,   371,   439,
     741,  1416,  1698,   251,   620,   252,   622,   253,   623,   254,
     625,   255,   626,   256,   627,   257,   628,   258,   630,   259,
     631,  1213,   260,   624,   261,   629,   262,   447,   372,   744,
     263,  1224,   637,   264,   448,   373,   745,   265,   642,   266,
     643,   267,   450,   268,   374,   457,   747,   458,  1472,  1475,
     938,  1486,   269,   462,   270,   375,   472,   751,   473,   947,
    1319,  1493,  1495,  1941,  1490,  1703,  1817,  1813,  1986,  1988,
     271,   477,   376,   753,   272,   645,   273,   647,   274,   648,
     275,   652,   276,   649,   277,   654,   278,   655,   279,   650,
     280,   656,   281,   657,   282,   658,   283,   659,  1235,   284,
     377,   481,   756,  1085,   482,   757,  1504,  1706,   285,   489,
    1507,   286,   378,   508,   764,   509,  1096,  1097,  1841,  1619,
     510,   766,  1578,  1590,  1727,  1731,  1579,  1591,  1580,  1572,
    1723,  1849,  1563,  1717,  1564,  1718,  1565,  1514,  1708,  1515,
    1516,  1611,  1114,   511,   767,  1530,  1598,  1712,  1735,  1834,
    1855,  1617,  1738,  1862,  1536,  1714,  1537,  1607,   287,   661,
     288,   666,   289,   668,   290,   672,  1661,  1664,   291,   686,
    1670,  1673,   292,   679,  1667,   293,   527,   379,   774,   294,
     689,   295,   690,   296,   380,   531,   777,  1131,  1127,  1955,
    1868,   297,   692,   298,   532,   299,   694,   300,   533,   301,
     695,   302,   537,   303,   534,   381,   778,   304,   539,   382,
     779,   305,   540,   383,   780,   306,   541,   384,   781,   307,
     542,   385,   782,   308,   544,   309,   697,   310,   699,   311,
     700,   312,   386,   549,  1632,   786,  1743,  1634,   313,   550,
     387,   787,   314,   702,   315,   551,   388,   788,   316,   552,
     389,   789,   317,   553,   390,   790,   318,   554,   391,   791,
     319,   705,   320,   710,   321,   555,   392,   792,   322,   711,
     323,   712,   324,   556,   393,   793,   325,   713,   326,   714,
     327,   571,  1184,   572,  1202,  1206,  1791,  1899,   828,  1303,
     829,  1304,   830,   917,  1312,   918,   919,   920,   921,   922,
    1464,   923,  1459,  1226,  1229,  1138,  1139,  1262,  1148,  1340,
    1168,  1352,   841,  1408,  1695,  1149,  1341,  1150,  1409,  1696,
    1170,  1942,  1115,  1961,  1478,  1481,  1414,  1560,   580,  1610,
    1496,   401,   440,   441,  1203,   831,   682,   683,   475,  1581,
     684,   476,   832,   833,   834,  1584,  1594,  1585,  1595,   926,
    1230,  1263,   639,   673,   640,  1389,  1140,   927,   928,   929,
     517,   518,   519,   520,   521,   835,   836,   459,   460,   485,
     486,  1460,  1086,  1087,  1088,  1105,  1467,  1462,   837,   838,
    1548,  1557,  1207,   443,  1432,  1807,  1936,   444,   445,   446,
     848,   845,  1411,  1804,  1933,  1067,  1760,  1761,  1762,  1069,
    1763,  1070,  1764,  1765,  1766,  1073,  1767,   328,   329,   330,
    1768,  1079,   331
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -1910
static const yytype_int16 yypact[] =
{
    6592, -1910,  8668, -1910, -1910, -1910,   113,   261,   125,   442,
   -1910, -1910,   601, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910,  3700, -1910,    77, -1910, -1910,  3722,  1489,
   -1910, -1910, -1910, -1910, -1910,  1796,  1461,   167, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910,    45, -1910, -1910,
   -1910, -1910, -1910,   917, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910,  3758,   152, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910,   113, -1910,   113, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910,   113, -1910,   113,   238,
     113,   113,   113, -1910,   113,    55,  3796,   113,   113,   168,
     168,   261,   261,   241,   168,    55,    72,  3818,   113,    55,
    3851,   113, -1910,   113,   424,  3924,   113,   452,   113, -1910,
   -1910, -1910, -1910, -1910, -1910,  3975, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,   581, -1910,
   -1910, -1910,  4011, -1910, -1910, -1910, -1910,  4071, -1910, -1910,
   -1910, -1910, -1910, -1910,  4104,   105,  4126,   677,   783,   821,
   -1910, -1910,  4139, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,  4177, -1910,
   -1910, -1910,  4199, -1910,  4228,   150, -1910, -1910, -1910, -1910,
   -1910, -1910,   113,    95, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,  4264,  4286,
    4299,   113, -1910, -1910,   439,   527, -1910, -1910,  3283, -1910,
   -1910,    59,  1680, -1910,  2318,   298, -1910,    51, -1910, -1910,
   -1910, -1910, -1910,   456, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910,   453,   218, -1910, -1910, -1910, -1910,
   -1910,   113, -1910,    37,    37, -1910, -1910,    37,    37,    37,
      37, -1910, -1910,  2123,  4330, -1910, -1910, -1910, -1910, -1910,
     162,   302,  4398,  4452, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910,  4481,  4558,  4589,  4602, -1910,  4617,   113,
      37, -1910, -1910, -1910,  6354, -1910, -1910,  2123,   877, -1910,
   -1910,   113, -1910,    37,    37,    76,    86,   113, -1910,    37,
      37, -1910,   113, -1910, -1910,    37, -1910, -1910, -1910, -1910,
   -1910, -1910,   113,    61,    37, -1910, -1910,   877, -1910,  2665,
     559,   113, -1910,    37, -1910, -1910, -1910, -1910, -1910,   113,
      37, -1910, -1910,    37, -1910, -1910,    37,    37, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910,  6001,  6206, -1910, -1910,
     113, -1910,    37,    37,    37,    37,    37,    37,    37,    37,
      37,    37,    37,    37,    37,    37,    37,  2123, -1910,  3543,
    3592,   113,  1284,  1284,  4330, -1910, -1910, -1910,   113,  1338,
    1338,  1338,  1338, -1910, -1910,   113, -1910,    37,    37, -1910,
    4330,  1338,    98,  1338,  1338,  2123,  2123, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910,   113, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910,    66, -1910,   113, -1910, -1910, -1910, -1910, -1910,
     113,   113,   113,   113, -1910,    37, -1910, -1910,    37, -1910,
   -1910, -1910, -1910,   113, -1910, -1910, -1910, -1910, -1910,    88,
   -1910, -1910, -1910,   113, -1910, -1910, -1910,   113, -1910, -1910,
     113, -1910, -1910,  1436,  1436,  4651,  4651,  1436,  4651, -1910,
     113, -1910,  4673,  4673,  1436,  4673,  4673,  4673,  4673,  4651,
    4673,  4673, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910,  1734,   951, -1910,   113, -1910,  1734,   951,  4705,
    4705, -1910,   113, -1910,  4705,  4705,  4705,  4705,  4705,  4705,
   -1910,   113, -1910, -1910, -1910, -1910,   113, -1910,   113, -1910,
   -1910,    37,   113,    37, -1910, -1910, -1910, -1910, -1910,   113,
      37, -1910, -1910, -1910, -1910, -1910,   113,    37,    37,  1436,
    4651, -1910,   113, -1910,   440,   440, -1910, -1910,   113,  1436,
    4651, -1910,   113, -1910, -1910,   113, -1910, -1910, -1910, -1910,
     113,  1436,  4651,  1436,  4651, -1910, -1910, -1910, -1910,   110,
   -1910, -1910,   116, -1910, -1910,   113,   165, -1910,   455,   389,
   -1910,    37,    37, -1910, -1910,    37, -1910, -1910,  2164,  4330,
   -1910,   113,    37, -1910,  2164,  1758, -1910,   113, -1910, -1910,
      37,   113,    62,  1758, -1910,   559,   113, -1910,    37, -1910,
   -1910,    37,    37,    37, -1910, -1910,   113, -1910,    37,    37,
      37,    37,    37,    37,  2164, -1910,  3592,   113,  4330,   958,
     958,   958,   958, -1910,    37,    37, -1910,  4330,   958,   629,
     958,   958,  2164,  2164,   129,   172, -1910,   129, -1910, -1910,
   -1910,  6848, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
    1180, -1910,  1244,   639,   480, -1910,  2985,   605, -1910, -1910,
   -1910,   113, -1910, -1910,   113,   113,   113,   113,   113,   113,
   -1910,   401,    37, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
    7854, -1910,  7854, -1910,  7921, -1910,  7921, -1910,  7921, -1910,
      48, -1910,  7921, -1910,  4734, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,  4756,
    4769, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910,  4784,  4817,  4877,  4926, -1910,  4958,  5044,  5076,
    5089,  5104,  5136,  5149,  5179,  5211,  5233,  5246,  5261,  5297,
   -1910, -1910, -1910, -1910, -1910,   632,   644, -1910,   113, -1910,
   -1910, -1910, -1910,   113, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910,   100,    78, -1910,    49, -1910,    44, -1910, -1910,   686,
   -1910, -1910,    73, -1910, -1910,    57,    57, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
      49, -1910,    49, -1910, -1910,    49, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910,    44,    44,    44, -1910, -1910, -1910, -1910, -1910,
    5364, -1910,  5408,  1634,  2280,  6228,    37,    37,    49,    49,
      64,    49,    44,    44,    44,    41,    46, -1910,  6049,  2352,
    1109,  3317,  2937,  1423, -1910,   139, -1910, -1910, -1910,  6097,
    6097,  3629,  6097,  6097, -1910, -1910,    49, -1910,    49, -1910,
      44, -1910,    44, -1910, -1910, -1910, -1910, -1910, -1910,   113,
   -1910, -1910, -1910,   401, -1910, -1910,   701,   762, -1910,   113,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910,  1384,   401, -1910, -1910,   813,   846, -1910,   113,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910,    55, -1910, -1910, -1910, -1910,
   -1910,   115, -1910,   177, -1910, -1910, -1910,   377, -1910, -1910,
   -1910, -1910, -1910,   113, -1910, -1910, -1910,   113, -1910, -1910,
   -1910, -1910, -1910, -1910,   113, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910,   113, -1910, -1910,   113, -1910, -1910,
     113, -1910, -1910, -1910, -1910, -1910,   113, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,  5438, -1910,
    5521, -1910,  5550, -1910,  1886,  1722, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910,   113, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910,  4286, -1910, -1910, -1910,  5572, -1910, -1910, -1910,  7103,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,  1180,
   -1910, -1910,   852, -1910, -1910, -1910,   113,   735, -1910,  5585,
   -1910, -1910, -1910,   113, -1910,    67, -1910, -1910,    57, -1910,
   -1910, -1910, -1910,  5617,  6027,  1012,  6228, -1910,  6071,  2382,
    3332,  3376,  1405,  1949, -1910, -1910, -1910,   735, -1910,   908,
   -1910,   113, -1910, -1910, -1910, -1910, -1910,  1384,   735, -1910,
   -1910,   912, -1910,   113, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910,   113, -1910, -1910,   113,  8188, -1910,   121, -1910, -1910,
   -1910, -1910,   183,   186,  8188,  5661,  8919,    93,  3160, -1910,
     113, -1910,  3160, -1910,   113, -1910, -1910, -1910,   113, -1910,
   -1910, -1910, -1910, -1910,   113, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910,   936,   979, -1910,   113,
   -1910,   443, -1910, -1910, -1910, -1910, -1910, -1910, -1910,  7900,
   -1910,  7875, -1910,   448, -1910,   448, -1910, -1910, -1910, -1910,
   -1910,    99, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
     928, -1910,   928, -1910,   928, -1910,   928, -1910,   928, -1910,
     928, -1910,  7921, -1910,  7703, -1910,  7703, -1910,    49, -1910,
   -1910,    64, -1910, -1910, -1910, -1910,    41, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,  3651,  3665,
   -1910, -1910, -1910, -1910,    49, -1910,    44, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910,  3447,  3494, -1910, -1910,   113,   113,   113,   113,
     113,   113,   113,   113,   113, -1910, -1910,   113, -1910,  5691,
   -1910,   113, -1910, -1910, -1910,  6254, -1910,   113, -1910,   113,
     163,   163, -1910, -1910, -1910,  6254,  6254, -1910, -1910,    37,
   -1910, -1910, -1910, -1910,  6254,  6254, -1910, -1910, -1910, -1910,
   -1910,   209,  1009,   113, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910,   113, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910,  5713, -1910,   113,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
     113, -1910,   113,   113, -1910, -1910,  5726, -1910,   113,   113,
   -1910, -1910,   113,    37,    37,    37,    37, -1910, -1910, -1910,
   -1910, -1910,  3160, -1910, -1910,   113, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910,   614, -1910, -1910,  8188,
   -1910,   200, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910,  4264, -1910, -1910, -1910,
     113,   121, -1910, -1910, -1910, -1910,  8188,  9170,  3268,   113,
    3268, -1910, -1910, -1910,  1051, -1910,   113, -1910, -1910, -1910,
   -1910, -1910,  3665, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
     113,  5691, -1910,   113, -1910,   113,   113,   113, -1910, -1910,
   -1910, -1910, -1910,   113, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910,  5713, -1910,   113,  5726, -1910,   113,
   -1910, -1910, -1910,  3268, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,   220,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
    7358, -1910, -1910, -1910,    93, -1910, -1910, -1910, -1910, -1910,
   -1910,    93, -1910, -1910, -1910, -1910, -1910, -1910, -1910,  2985,
    1092, -1910, -1910,   113, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910,  5741, -1910, -1910, -1910,
   -1910, -1910,    49, -1910,    49, -1910,    44, -1910,    44,   113,
     113, -1910,    44, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910,    44, -1910, -1910, -1910, -1910,
     113, -1910, -1910,    49,    49,  1423,    44,    44, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910,    44, -1910, -1910, -1910,
   -1910, -1910, -1910,    44, -1910, -1910, -1910,  1893, -1910,   139,
    1893, -1910, -1910, -1910,   247,   108,   247, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910,  5774, -1910, -1910, -1910,
    7613, -1910, -1910, -1910,    93, -1910, -1910, -1910,  1118, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
    8430, -1910, -1910, -1910, -1910, -1910,   183,    93, -1910,   248,
   -1910, -1910, -1910,   443, -1910, -1910, -1910,   113,   113,   113,
     113, -1910, -1910, -1910,   113,   113, -1910, -1910, -1910, -1910,
   -1910, -1910,   113,   113, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910,   250, -1910, -1910, -1910, -1910, -1910, -1910,   220,    93,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910,   469,   127,   127,   108,
   -1910, -1910,  5844,    93, -1910, -1910,    93, -1910, -1910, -1910,
   -1910, -1910,   108, -1910, -1910, -1910, -1910, -1910,   113, -1910,
   -1910,   247, -1910, -1910, -1910
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,  -791, -1259,
      -1, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910,  -724, -1354, -1910, -1910, -1910,
   -1910, -1860, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1579, -1910,  -716,  -703,  -658, -1910,
   -1910, -1910, -1910, -1910, -1043, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910,  -700, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1793, -1910, -1910, -1910, -1909, -1910, -1910,
   -1910, -1685, -1910, -1910, -1910, -1910, -1910,  -389, -1910,   357,
   -1910, -1910, -1910, -1331, -1581, -1491, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,  -860, -1910,
   -1910, -1910, -1910, -1910, -1910,   114, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910,  1066, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910,   147, -1910, -1910,
      43, -1910, -1910, -1910, -1910, -1910, -1910, -1910,   112, -1910,
   -1910, -1910,  -808, -1315, -1910, -1910, -1182, -1910, -1413, -1410,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,   904, -1910,
   -1910, -1910, -1910,  -228, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910,   196, -1910, -1910, -1010,  -580,
   -1910, -1910, -1910, -1910, -1910, -1910,  -784,  -776,  -546, -1910,
   -1910, -1910, -1910, -1910,  -543,  -762, -1187, -1910, -1910,  -754,
   -1910,  -548,   -29, -1910, -1910, -1910, -1910, -1910, -1910, -1133,
   -1154, -1910, -1910, -1156, -1910, -1910,  -743, -1261, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910,  -181, -1910, -1272,
   -1027, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910,  -740, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910, -1910,
   -1910,   -85, -1910, -1910,   491,   546, -1334, -1594,  -323,  -369,
   -1234, -1529,  -717,   134,  -141,  -295, -1910, -1910, -1910, -1910,
   -1910, -1910, -1910,   -34,    -7,   104, -1910,   -47,   333,   332,
   -1910, -1910,  -280,  -902, -1117, -1910, -1910,   692,    97,  -139,
    -758,    -6,  1579,  -564,  -976,  -908, -1910, -1910,   376, -1215,
    -286,   199,     2,   625,  -147,   792,   504,     0,    -4, -1038,
   -1057,   -22,     4,     7,  -408, -1910, -1910, -1910, -1910,  -761,
     531, -1910,  -233, -1910,   508,  -143, -1910,   -39,   -11,  -106,
    -968,  -416,  -958,  -410,  -933,   257,   462, -1910, -1910, -1910,
   -1910, -1910,  -894, -1910, -1910,   -35,  -874, -1910,  1161,   -18,
    -768,  -771,   528, -1108, -1910, -1910, -1910,  -126, -1910, -1910,
   -1910, -1910, -1910, -1230, -1910,  -813, -1910, -1910,  -472, -1910,
   -1910, -1910,  -465, -1910,  -463, -1910,  -462,  -476,  -475,  -473,
    -461, -1910,  -470
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -358
static const yytype_int16 yytable[] =
{
     398,   363,   524,  1075,  1076,  1297,  1077,  1068,  1185,  1080,
    1370,   680,  1290,  1396,  1071,   513,  1072,  1074,  1078,   526,
     589,  1305,  1465,  1787,   596,  1291,   516,  1305,  1292,   474,
    1684,  1354,   641,   512,  1497,  1197,  1498,   483,   817,  1499,
     925,   514,  1553,  1782,   515,  1479,   528,  1558,  1795,  1412,
    1476,   675,   775,  1582,  1582,   538,   569,  1305,  1491,  1381,
     746,  1385,   944,  1317,   570,  1544,  1980,  1181,  1701,   925,
     557,  1182,   558,  1593,  1487,  1305,  1305,   933,   452,  1473,
     559,   590,   560,  1968,   565,   566,   567,   935,   568,  1181,
    2015,   576,   577,  1195,   806,   716,  1902,  1887,   807,  1165,
    1805,  1470,   595,  2022,   808,   600,   663,   601,  -357,  1962,
     608,  1276,   612,  1963,   396,  1277,  1641,  1181,   397,  1964,
    1642,  1279,  1542,  1543,   910,  1549,   402,  1538,  2010,  -300,
    1359,  1152,  1152,  1152,  1152,  1520,   416,   417,   418,  1366,
    1608,  1719,  1719,  1152,  1792,  1152,  1152,   419,  1792,  1917,
    1623,   707,  1624,   546,   420,   421,   422,   423,  1281,  1900,
    1522,  1900,  1285,   843,  1839,   453,  1181,   844,   492,   578,
    1283,   454,  -325,  1362,  1500,  1501,  1502,   579,   817,   416,
     417,   418,  1647,   453,  1775,  1492,   715,   817,  1776,   454,
     419,  1779,   934,  1474,  1550,  1551,  1552,   420,   421,   422,
     423,  1181,   936,   677,  1124,  1884,   466,  1471,   467,   468,
     859,   469,   470,   664,  1900,   665,   416,   417,   418,   806,
    1740,   817,  1625,   807,  1626,  1918,   471,   419,  2024,   808,
     403,  1745,  1178,  1179,   420,   421,   422,   423,  1677,   561,
     404,  1628,   584,   562,  1609,   416,   417,   418,  1181,   817,
     585,  1181,  1959,  1982,  1143,  2000,   419,   547,   708,   794,
     709,  1636,   399,   420,   421,   422,   423,   548,  1840,   400,
    1163,   795,   493,  1582,  1582,   466,   494,   467,   468,   495,
     469,   470,   496,   497,   498,   499,   500,   501,   502,   503,
     504,   505,   484,  1593,   522,   471,   850,   852,  1872,   760,
     582,   583,   797,   846,  1554,  1555,   818,   854,   856,   858,
     773,   847,   862,  2014,  1556,   529,   530,  1480,  1883,  1559,
    1413,  1477,   776,   769,  1010,   727,   498,  1545,  1546,   501,
     456,   945,   946,  1318,   772,  1183,  1547,   925,  1702,  1293,
     742,   768,   474,  1488,  1489,   925,   758,   455,   456,   770,
    1075,  1076,   771,  1077,  1068,  1895,  1080,  1183,  1538,   366,
     810,  1071,   812,  1072,  1074,  1078,  1520,  1806,  1166,  1167,
    1965,  1152,  1152,  1152,  1152,  1310,  1643,  1966,  1650,  1278,
    1152,   809,  1152,  1152,  1644,  1183,  1651,   432,   433,  2011,
     806,  1522,   434,   435,   807,   816,  2012,  2004,  1792,   436,
     808,  1635,  1405,   761,   437,  1334,   466,   494,   467,   468,
     495,   469,   470,   762,   763,   498,   499,   500,   501,   502,
     503,   504,   505,  1357,  1358,   602,   471,  1928,  1896,  1978,
     432,   433,   603,   863,  1183,   434,   435,   506,   507,  1689,
     730,  1259,   436,   406,  1802,   930,   818,   437,  1803,   737,
    1313,   937,  1777,   609,   752,   818,   941,   783,  1313,  1307,
     610,  1264,  1264,   865,   866,   867,   943,   432,   433,  1183,
    2007,  1100,   434,   435,  2008,  1089,   581,   874,   875,   436,
     588,  1392,  1808,  1091,   437,   810,   811,   812,   748,   818,
    1116,  1117,  1118,  1141,  1141,  1227,   432,   433,  1337,   523,
    1227,   434,   435,   940,  1107,  1943,   677,  1348,   436,   466,
    1704,   467,   468,   437,   469,   470,  1183,   818,  1819,  1183,
    1905,  1156,  1156,  1156,  1156,  1133,  1990,  1321,   734,   471,
    1991,  1842,  1144,  1156,  1175,  1156,  1156,   641,  1381,  1160,
     442,   765,   641,  1622,   731,  1260,  1728,   407,   633,  1354,
     634,   451,  1294,   635,   732,  1732,   463,   408,  1180,   409,
    1081,   784,   410,   487,   491,  1592,  1573,  1724,  1186,   507,
    1709,   785,  1614,  1981,  1187,  1188,  1189,  1190,  1907,  1354,
    1913,  1915,   632,  1715,  1949,  1786,  1635,  1194,  1820,   494,
    1354,   794,   495,   794,   545,  1335,   636,  1198,  1958,  1871,
    1393,  1199,   415,   795,  1200,   795,  1397,  1744,   416,   417,
     418,   949,  1320,  1232,  1211,  1877,   796,   564,   796,   419,
    1261,  1878,  1879,  1880,   575,  1881,   420,   421,   422,   423,
    1350,  1974,   735,  1457,   797,   594,   797,  1142,   599,  1231,
    1386,  1233,   408,   607,   409,  1463,  1238,   410,  1265,  1169,
    1353,  1771,  2020,   621,  1957,  1245,   810,  1295,   812,  1494,
    1246,   490,  1247,   688,  1772,   674,  1249,  1773,  1850,   638,
     646,  1406,  1407,  1251,  1082,   653,  1083,  1519,   670,  1084,
    1253,  1395,   662,  1521,   669,  1518,  1258,  1482,  1851,   633,
     693,   634,  1266,  1809,   635,  1810,  1269,   636,  1935,  1270,
    1567,  1574,  1629,  1985,  1271,     0,   698,     0,     0,     0,
     703,     0,   706,   894,   895,   896,     0,  1391,  1391,  1282,
    1104,   798,   498,  1286,     0,   501,   424,   425,   426,   427,
     428,   429,   430,   431,   677,  1308,  1693,   466,     0,   467,
     468,  1314,   469,   470,     0,  1316,     0,  1458,   494,   498,
    1322,   495,   501,  1419,  1421,   636,  1387,   471,     0,  1082,
    1327,   498,     0,  1630,   501,     0,  1423,  1425,  1427,  1429,
       0,  1336,  1434,  1436,  1438,  1440,  1442,  1444,  1446,  1448,
    1450,  1452,  1454,  1456,   676,   633,     0,   634,     0,     0,
     671,  1100,     0,   636,     0,     0,  1153,  1153,  1153,  1153,
       0,  1483,     0,  1484,     0,     0,  1485,   523,  1153,  1172,
    1153,  1153,  1390,  1390,  1637,     0,  1010,     0,   498,     0,
       0,   501,   685,  1175,     0,  1398,     0,     0,  1399,  1400,
    1401,  1402,  1403,  1404,     0,     0,  1937,     0,  1938,     0,
     677,     0,     0,   466,     0,   467,   468,  1638,   469,   470,
       0,     0,     0,  1691,     0,     0,   719,   723,   726,   432,
     433,     0,     0,   471,   434,   435,   743,  1947,  1948,     0,
       0,   436,   759,  1157,  1158,  1159,   437,  1104,   911,   498,
    1461,  1466,   501,     0,  1164,     0,  1176,  1177,   677,  1075,
    1076,   466,  1077,   467,   468,  1080,   469,   470,  1075,  1076,
    1351,  1077,     0,     0,  1080,     0,     0,     0,  1939,  1741,
    1940,   471,  1468,  1746,  1944,     0,  1518,  1469,   535,     0,
    1720,  1725,   536,     0,     0,     0,   677,  1945,  1010,   466,
     498,   467,   468,   501,   469,   470,     0,  1799,  1950,  1951,
       0,   839,   842,   865,   866,   867,     0,     0,  1952,   471,
     681,   687,  1228,     0,     0,  1953,     0,   874,   875,  1338,
       0,  1104,   678,   498,   860,  1891,   501,  1104,     0,   498,
       0,  1305,   501,  1305,     0,   839,  1930,     0,  1892,  1923,
    1800,  1893,   677,     0,     0,   466,   494,   467,   468,   495,
     469,   470,   912,   636,   913,   499,   500,   914,   502,   503,
     678,  1154,  1154,  1154,  1154,   471,  1694,   860,     0,     0,
    1848,     0,     0,  1154,  1173,  1154,  1154,     0,     0,   416,
     417,   418,     0,  1104,     0,   498,  1305,  1104,   501,   498,
     419,     0,   501,     0,  1101,  1106,     0,   420,   421,   422,
     423,     0,   424,   425,   426,   427,   428,   429,   430,   431,
    1925,  1010,  1903,   498,     0,   839,   501,  1128,  1132,   633,
       0,   634,   842,   677,   635,     0,   466,   636,   467,   468,
       0,   469,   470,     0,     0,  1526,   499,   500,   842,   502,
     503,     0,     0,   839,   839,     0,   471,  1886,  1583,  1583,
    1569,  1569,     0,  1931,  1104,     0,   498,     0,  1172,   501,
    1640,     0,     0,     0,     0,  1204,     0,  1671,  1209,     0,
    1575,  1100,  1343,  1344,  1345,  1216,   416,   417,   418,  1977,
    1349,     0,  1355,  1356,  1082,  1175,   498,   419,     0,   501,
       0,  1100,     0,  1627,   420,   421,   422,   423,     0,     0,
       0,     0,     0,  1631,     0,  1175,     0,   915,   916,     0,
    1214,  1214,     0,  1214,  1214,  1214,  1214,     0,  1214,  1214,
       0,     0,  1208,  1639,  1210,     0,  1104,     0,   498,     0,
    1976,   501,     0,  1075,  1076,  1221,  1077,  1236,  1236,  1080,
    1256,  1379,  1236,  1236,  1236,  1236,  1236,  1236,     0,     0,
    1267,   892,   893,   894,   895,   896,     0,  1653,   525,     0,
       0,  1654,  1272,     0,  1274,     0,     0,  1104,  1655,   498,
    1075,  1076,   501,  1077,   677,     0,  1080,   466,  1656,   467,
     468,  1657,     0,     0,  1658,     0,     0,  1576,  1577,  1339,
    1659,     0,     0,  1104,     0,   498,  1257,   471,   501,     0,
     924,  1154,  1154,  1154,  1154,  1383,  1268,     0,     0,     0,
    1154,  1173,  1154,  1154,     0,  2005,     0,  1675,  1273,     0,
    1275,     0,     0,     0,     0,     0,  1306,   842,     0,   924,
     432,   433,  1306,     0,     0,   434,   435,     0,     0,  2019,
       0,     0,   436,  1711,     0,  1134,     0,   437,   363,   494,
       0,     0,   495,     0,     0,   912,   636,     0,     0,     0,
    1692,     0,  1306,  1173,  1132,     0,   842,  1700,  1583,  1583,
       0,     0,     0,  1075,  1076,   842,  1077,  1068,     0,  1080,
    1306,  1306,  1361,  1364,  1071,  1361,  1072,  1074,  1078,     0,
       0,  1151,  1151,  1151,  1151,  1742,     0,     0,     0,  1145,
       0,     0,     0,  1151,  1171,  1151,  1151,  1747,     0,     0,
    1523,     0,     0,   494,   860,  1748,   495,     0,  1749,   912,
     636,     0,     0,     0,     0,  1568,  1568,   432,   433,     0,
       0,  1100,   434,   435,  1794,     0,     0,     0,  1796,   436,
       0,     0,  1797,     0,   437,  1633,     0,     0,  1798,     0,
    1135,     0,  1417,   494,     0,     0,   495,     0,     0,   912,
    1172,  1136,     0,  1801,  1137,   681,   681,  2002,     0,   681,
       0,     0,   416,   417,   418,     0,   681,     0,     0,     0,
    1172,     0,     0,   419,     0,  1431,     0,     0,     0,     0,
     420,   421,   422,   423,   681,     0,     0,  1201,     0,   681,
       0,     0,     0,   677,  1075,  1076,   466,  1077,   467,   468,
    1080,   469,   470,     0,     0,     0,   499,   500,     0,   502,
     503,     0,   488,     0,     0,     0,   471,     0,   416,   417,
     418,  1342,  1342,  1342,  1342,     0,     0,  1100,     0,   419,
    1342,   681,  1342,  1342,     0,     0,   420,   421,   422,   423,
     464,   681,     0,   494,     0,     0,   495,     0,     0,   912,
     636,     0,     0,   681,     0,   681,   525,     0,     0,     0,
    1823,  1824,  1825,  1826,  1827,  1828,  1829,  1830,  1831,     0,
       0,  1832,     0,     0,     0,  1836,  1100,  1466,  1600,     0,
       0,  1837,  1601,  1838,     0,  1602,     0,   924,  1603,  1604,
    1605,   677,     0,  1606,   466,   924,   467,   468,     0,   469,
     470,     0,     0,     0,  1237,  1524,     0,  1852,  1239,  1240,
    1241,  1242,  1243,  1244,   471,     0,  1853,     0,     0,     0,
       0,  1151,  1151,  1151,  1151,     0,     0,     0,     0,     0,
    1151,  1171,  1151,  1151,     0,     0,   424,   425,   426,   427,
       0,     0,     0,  1857,   465,     0,     0,   466,     0,   467,
     468,     0,   469,   470,  1858,  1173,  1859,  1860,  1146,  1147,
       0,     0,  1864,  1865,     0,   678,  1866,   471,  1505,     0,
    1508,  1527,  1531,  1539,     0,  1173,     0,     0,     0,  1873,
       0,     0,     0,  1171,     0,  1509,  1570,  1570,  1586,  1586,
    1599,   416,   417,   418,     0,     0,     0,  1613,  1613,  1618,
    1621,  1621,   419,     0,     0,     0,     0,     0,     0,   420,
     421,   422,   423,   432,   433,     0,     0,     0,   434,   435,
       0,     0,     0,     0,  1888,   436,  1734,     0,     0,     0,
     437,   749,     0,  1901,     0,     0,   363,     0,     0,  1215,
    1904,  1217,  1218,  1219,  1220,     0,  1222,  1223,     0,     0,
    1155,  1155,  1155,  1155,  1906,     0,     0,  1908,     0,  1909,
    1910,  1911,  1155,  1174,  1155,  1155,     0,  1912,     0,   432,
     433,     0,     0,  1672,   434,   435,     0,     0,     0,     0,
    1914,   436,     0,  1916,     0,  1225,   437,     0,     0,   677,
       0,     0,   466,     0,   467,   468,     0,   469,   470,     0,
       0,     0,   499,   500,  1100,   502,   503,  1510,  1511,  1311,
       0,     0,   471,     0,     0,     0,     0,     0,     0,  1173,
       0,     0,     0,     0,     0,     0,  1662,     0,  1665,     0,
    1668,     0,     0,     0,     0,   750,  1524,  1932,   466,     0,
     467,   468,     0,   469,   470,     0,     0,   478,     0,  1173,
       0,     0,     0,   416,   417,   418,     0,     0,   471,   723,
    1173,     0,     0,  1680,   419,     0,     0,     0,     0,     0,
       0,   420,   421,   422,   423,     0,     0,   677,     0,     0,
     466,     0,   467,   468,  1946,   469,   470,  1699,     0,   677,
       0,     0,   466,     0,   467,   468,     0,   469,   470,     0,
     471,  1707,  1710,  1713,  1716,     0,  1721,  1721,  1729,  1733,
    1736,  1739,   471,   677,     0,     0,   466,   494,   467,   468,
     495,   469,   470,   912,   636,   913,   499,   500,   914,   502,
     503,     0,     0,     0,     0,  1517,   471,  1669,     0,   363,
       0,     0,   432,   433,  1954,     0,     0,   434,   435,     0,
    1566,  1566,     0,  1785,  1512,  1513,  1793,     0,     0,   437,
    1793,  1612,  1612,   678,  1620,  1620,     0,     0,     0,     0,
       0,   424,   425,   426,   427,   428,   429,   430,   431,     0,
       0,  1987,  1989,  1987,  1989,  1171,     0,     0,  1992,  1993,
    1155,  1155,  1155,  1155,     0,     0,  1994,  1995,     0,  1155,
    1174,  1155,  1155,     0,     0,  1171,   416,   417,   418,     0,
       0,     0,     0,     0,     0,     0,     0,   419,     0,     0,
       0,     0,     0,     0,   420,   421,   422,   423,     0,     0,
       0,     0,   819,   820,     0,     0,   821,   822,   823,   824,
       0,   677,     0,     0,   466,     0,   467,   468,   677,   469,
     470,   466,  1174,   467,   468,     0,   469,   470,     0,     0,
       0,     0,  2023,     0,   471,     0,  1814,  1818,     0,   864,
       0,   471,     0,     0,     0,     0,     0,     0,     0,   916,
       0,     0,   931,   932,     0,     0,     0,     0,   939,   939,
     860,  1822,     0,     0,   942,     0,   681,  1674,     0,     0,
       0,     0,     0,   948,   432,   433,     0,  1835,     0,   434,
     435,     0,  1090,  1106,     0,   678,   479,   480,     0,  1092,
       0,   437,  1093,  1843,  1844,  1094,  1095,     0,     0,     0,
       0,     0,  1846,  1847,     0,     0,     0,     0,     0,   860,
       0,  1108,  1109,  1110,  1111,  1112,  1113,     0,     0,  1171,
       0,  1119,  1120,  1121,  1122,  1123,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1517,     0,     0,     0,
    1566,  1566,     0,     0,   825,  1856,  1161,  1162,     0,  1171,
     416,   417,   418,     0,     0,     0,     0,     0,     0,     0,
    1171,   419,     0,     0,  1863,     0,     0,     0,   420,   421,
     422,   423,     0,     0,     0,     0,     0,     0,     0,     0,
    1793,     0,     0,     0,  1191,  1301,     0,  1192,     0,     0,
       0,   416,   417,   418,     0,     0,     0,     0,     0,     0,
       0,     0,   419,     0,     0,     0,     0,     0,     0,   420,
     421,   422,   423,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   719,     0,     0,   432,   433,     0,
       0,     0,   434,   435,     0,     0,  1306,     0,  1306,   436,
    1737,     0,     0,     0,   437,     0,     0,     0,   677,     0,
    1818,   466,     0,   467,   468,     0,   469,   470,     0,  1835,
       0,   499,   500,     0,   502,   503,     0,     0,     0,     0,
    1248,   471,  1250,     0,  1525,     0,     0,     0,     0,  1252,
       0,     0,  1856,     0,     0,  1863,  1254,  1255,     0,   677,
       0,  1306,   466,     0,   467,   468,     0,   469,   470,     0,
       0,  1528,   499,   500,     0,   502,   503,   416,   417,   418,
       0,     0,   471,     0,     0,     0,     0,     0,   419,     0,
       0,     0,     0,     0,  1174,   420,   421,   422,   423,     0,
    1298,  1299,     0,     0,  1300,     0,     0,   860,     0,   754,
       0,  1309,     0,     0,  1174,   416,   417,   418,     0,  1315,
       0,     0,     0,     0,     0,     0,   419,  1323,     0,     0,
    1324,  1325,  1326,   420,   421,   422,   423,  1328,  1329,  1330,
    1331,  1332,  1333,  1571,     0,     0,     0,     0,     0,   416,
     417,   418,     0,  1346,  1347,     0,     0,     0,     0,     0,
     419,     0,     0,     0,     0,     0,     0,   420,   421,   422,
     423,   432,   433,  1722,     0,     0,   434,   435,     0,   416,
     417,   418,     0,   826,   827,     0,     0,     0,   437,     0,
     419,     0,     0,     0,     0,     0,     0,   420,   421,   422,
     423,     0,     0,     0,  1971,     0,     0,     0,     0,     0,
       0,  1410,   432,   433,     0,     0,     0,   434,   435,     0,
       0,     0,     0,     0,   436,  1302,     0,     0,     0,   437,
       0,     0,     0,   424,   425,   426,   427,   428,   429,   430,
     431,     0,     0,     0,     0,     0,     0,   677,     0,     0,
     466,     0,   467,   468,     0,   469,   470,     0,  1174,     0,
     499,   500,     0,   502,   503,     0,     0,     0,     0,     0,
     471,     0,     0,     0,     0,  1525,     0,   677,     0,     0,
     466,     0,   467,   468,     0,   469,   470,     0,  1174,     0,
     499,   500,     0,   502,   503,     0,     0,     0,     0,  1174,
     471,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    2018,     0,     0,     0,     0,     0,     0,     0,   432,   433,
       0,     0,     0,   434,   435,     0,     0,     0,     0,     0,
     436,  1529,     0,     0,     0,   437,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   432,   433,     0,     0,
       0,   434,   435,     0,     0,     0,     0,     0,   436,   755,
       0,     0,     0,   437,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     432,   433,     0,     0,     0,   434,   435,     0,     0,     0,
       0,     0,  1561,  1562,     0,     0,     0,   437,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     432,   433,     0,     0,     0,   434,   435,     0,     0,     0,
       0,     0,   436,  1562,     0,     0,     0,   437,     0,  1956,
       0,     0,  1956,     0,     0,     0,   859,     0,     0,     0,
     950,     0,   416,   417,   418,  1540,  1541,     0,     0,     0,
     951,   952,   953,   419,   954,   955,   956,   957,   958,   959,
     420,   421,   422,   423,   960,   961,   962,   963,   964,   965,
     966,     0,   967,   968,   969,   970,   971,    13,   972,    15,
      16,    17,    18,    19,    20,   973,    22,   974,    24,   975,
      26,    27,   976,   977,   978,    31,    32,    33,    34,   979,
     980,   981,    38,    39,    40,    41,    42,    43,   982,    45,
      46,   983,    48,    49,   984,   985,   986,   987,    54,    55,
     988,    57,   989,    59,    60,   990,    62,   991,    64,    65,
     992,   993,    68,   994,   995,   996,   997,   998,   999,  1000,
    1001,  1002,  1649,  1003,  1004,  1005,  1006,  1007,  1008,  1009,
    1010,  1011,  1012,  1013,  1014,  1015,  1016,  1017,  1018,  1019,
    1020,  1021,  1022,  1023,  1024,  1025,  1026,  1027,     0,     0,
       0,    79,    80,  1028,    82,    83,    84,    85,     0,    87,
    1029,  1030,  1031,  1032,  1033,  1034,  1035,  1036,  1037,  1038,
    1039,  1040,  1041,  1042,  1043,  1044,  1045,  1046,  1047,  1048,
    1049,  1050,  1051,  1052,     0,  1053,  1054,  1055,  1056,  1057,
    1058,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   128,   129,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   141,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   163,     0,     0,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,     0,   179,     0,     0,
       0,   183,     0,     0,   186,     0,     0,   189,     0,     0,
       0,  1059,  1060,   432,   433,  1061,  1062,  1063,   434,   435,
    1064,  1065,  1066,     0,     0,   436,     0,     0,  1596,     0,
     437,     0,     0,     0,   416,   417,   418,     0,     0,     0,
       0,     0,  1781,     0,     0,   419,     0,     0,     0,     0,
       0,     0,   420,   421,   422,   423,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   859,     0,     0,     0,
     950,     0,   416,   417,   418,     0,     0,     0,     0,     0,
     951,   952,   953,   419,   954,   955,   956,   957,   958,   959,
     420,   421,   422,   423,   960,   961,   962,   963,   964,   965,
     966,     0,   967,   968,   969,   970,   971,    13,   972,    15,
      16,    17,    18,    19,    20,   973,    22,   974,    24,   975,
      26,    27,   976,   977,   978,    31,    32,    33,    34,   979,
     980,   981,    38,    39,    40,    41,    42,    43,   982,    45,
      46,   983,    48,    49,   984,   985,   986,   987,    54,    55,
     988,    57,   989,    59,    60,   990,    62,   991,    64,    65,
     992,   993,    68,   994,   995,   996,   997,   998,   999,  1000,
    1001,  1002,     0,  1003,  1004,  1005,  1006,  1007,  1008,  1009,
    1010,  1011,   498,  1013,  1014,   501,  1016,  1017,  1018,  1019,
    1020,  1021,  1022,  1023,  1024,  1025,  1026,  1027,     0,     0,
       0,    79,    80,  1028,    82,    83,    84,    85,  1845,    87,
    1029,  1030,  1031,  1032,  1033,  1034,  1035,  1036,  1037,  1038,
    1039,  1040,  1041,  1042,  1043,  1044,  1045,  1046,  1047,  1048,
    1049,  1050,  1051,  1052,     0,  1053,  1054,  1055,  1056,  1057,
    1058,  1788,     0,     0,     0,     0,     0,   416,   417,   418,
       0,     0,     0,     0,     0,     0,   128,   129,   419,     0,
       0,     0,     0,     0,     0,   420,   421,   422,   423,   141,
       0,     0,     0,     0,     0,   432,   433,     0,     0,     0,
     434,   435,  1867,  1869,  1870,  1869,     0,   436,  1597,     0,
       0,   163,   437,     0,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,     0,   179,     0,     0,
       0,   183,     0,     0,   186,     0,     0,   189,     0,     0,
       0,  1059,  1060,   432,   433,  1061,  1062,  1063,   434,   435,
    1064,  1065,  1066,     0,     0,   436,     0,     0,     0,     0,
     437,     0,     0,     0,     0,   677,     0,     0,   466,  1897,
     467,   468,     0,   469,   470,   416,   417,   418,   499,   500,
       0,   502,   503,     0,   740,     0,   419,     0,   471,     0,
     416,   417,   418,   420,   421,   422,   423,     0,     0,     0,
       0,   419,     0,     0,     0,     0,     0,     0,   420,   421,
     422,   423,     0,     0,     0,     0,     0,     0,  1587,     0,
       0,     0,     0,     0,   416,   417,   418,     0,     0,     0,
       0,     0,     0,  1726,     0,   419,     0,     0,  1920,   416,
     417,   418,   420,   421,   422,   423,     0,     0,     0,     0,
     419,     0,     0,     0,     0,     0,     0,   420,   421,   422,
     423,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   677,     0,     0,   466,  1730,   467,   468,
       0,   469,   470,   416,   417,   418,   499,   500,     0,   502,
     503,     0,     0,     0,   419,     0,   471,     0,     0,     0,
       0,   420,   421,   422,   423,     0,     0,     0,   424,   425,
     426,   427,   428,   429,   430,   431,     0,     0,   432,   433,
       0,     0,   677,   434,   435,   466,     0,   467,   468,     0,
    1789,  1790,     0,     0,     0,   437,     0,   677,  1588,  1589,
     466,     0,   467,   468,     0,   471,     0,     0,   859,     0,
    1576,  1577,     0,     0,   416,   417,   418,     0,     0,     0,
     471,     0,     0,     0,     0,   419,     0,     0,     0,     0,
       0,     0,   420,   421,   422,   423,     0,     0,     0,     0,
       0,   677,     0,     0,   466,     0,   467,   468,     0,     0,
       0,     0,     0,     0,     0,  1821,     0,  1588,  1589,     0,
       0,   416,   417,   418,   471,     0,     0,     0,  1984,     0,
       0,     0,   419,     0,     0,     0,     0,     0,     0,   420,
     421,   422,   423,     0,     0,     0,   432,   433,     0,     0,
       0,   434,   435,     0,     0,     0,     0,     0,   436,  1898,
       0,   432,   433,   437,  1125,     0,   434,   435,     0,     0,
     416,   417,   418,   436,     0,     0,  1098,  1920,   437,  1099,
       0,   419,  1010,     0,   498,     0,     0,   501,   420,   421,
     422,   423,     0,     0,     0,   432,   433,     0,     0,     0,
     434,   435,     0,     0,     0,     0,     0,   436,     0,     0,
     432,   433,   437,  1129,     0,   434,   435,     0,     0,   416,
     417,   418,   436,  1102,     0,     0,  1103,   437,     0,  1104,
     419,   498,     0,     0,   501,     0,     0,   420,   421,   422,
     423,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1615,     0,     0,     0,   432,   433,   416,   417,   418,   434,
     435,     0,     0,     0,     0,     0,   436,   419,     0,     0,
       0,   437,  1811,     0,   420,   421,   422,   423,   416,   417,
     418,     0,     0,     0,     0,     0,  1815,     0,     0,   419,
       0,  1126,   416,   417,   418,     0,   420,   421,   422,   423,
       0,     0,     0,   419,     0,     0,     0,     0,     0,     0,
     420,   421,   422,   423,     0,     0,     0,     0,     0,     0,
       0,   449,     0,     0,     0,   432,   433,   416,   417,   418,
     434,   435,     0,     0,     0,     0,     0,   436,   419,     0,
    1130,     0,   437,   461,     0,   420,   421,   422,   423,   416,
     417,   418,     0,     0,     0,     0,     0,     0,     0,     0,
     419,     0,     0,     0,     0,     0,     0,   420,   421,   422,
     423,     0,   432,   433,     0,     0,     0,   434,   435,   543,
       0,     0,     0,     0,   436,   416,   417,   418,     0,   437,
       0,     0,     0,     0,     0,     0,   419,     0,     0,  1812,
       0,     0,     0,   420,   421,   422,   423,     0,     0,     0,
       0,     0,     0,  1816,     0,     0,     0,   573,     0,     0,
       0,   432,   433,   416,   417,   418,   434,   435,     0,     0,
       0,     0,     0,   436,   419,     0,     0,     0,   437,   592,
       0,   420,   421,   422,   423,   416,   417,   418,     0,     0,
       0,     0,     0,     0,     0,     0,   419,     0,     0,     0,
       0,     0,     0,   420,   421,   422,   423,     0,     0,     0,
     432,   433,   597,     0,     0,   434,   435,     0,   416,   417,
     418,     0,   436,     0,     0,     0,     0,   437,     0,   419,
       0,     0,     0,     0,     0,     0,   420,   421,   422,   423,
       0,     0,     0,     0,     0,     0,     0,   432,   433,     0,
       0,     0,   434,   435,     0,     0,     0,     0,     0,   436,
    1616,     0,     0,     0,   437,     0,     0,     0,     0,   432,
     433,     0,     0,     0,   434,   435,     0,     0,     0,     0,
       0,   436,     0,   432,   433,   605,   437,     0,   434,   435,
       0,   416,   417,   418,     0,   436,     0,     0,     0,     0,
     437,     0,   419,     0,     0,     0,     0,     0,     0,   420,
     421,   422,   423,     0,     0,     0,     0,     0,   432,   433,
       0,     0,     0,   434,   435,     0,     0,     0,     0,     0,
     436,     0,     0,     0,     0,   437,   619,     0,     0,     0,
     432,   433,   416,   417,   418,   434,   435,     0,     0,     0,
       0,     0,   436,   419,     0,     0,     0,   437,     0,     0,
     420,   421,   422,   423,     0,     0,     0,     0,     0,     0,
       0,     0,   644,     0,     0,     0,   432,   433,   416,   417,
     418,   434,   435,     0,     0,     0,     0,     0,   436,   419,
       0,     0,     0,   437,     0,     0,   420,   421,   422,   423,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   432,   433,     0,     0,     0,   434,
     435,     0,     0,     0,     0,     0,   436,     0,     0,     0,
       0,   437,   651,     0,     0,     0,   432,   433,   416,   417,
     418,   434,   435,     0,     0,     0,     0,     0,   436,   419,
       0,     0,     0,   437,     0,     0,   420,   421,   422,   423,
       0,     0,     0,     0,     0,   660,     0,     0,     0,   432,
     433,   416,   417,   418,   434,   435,     0,     0,     0,     0,
       0,   436,   419,     0,     0,     0,   437,   667,     0,   420,
     421,   422,   423,   416,   417,   418,     0,     0,     0,     0,
     691,     0,     0,     0,   419,     0,   416,   417,   418,     0,
       0,   420,   421,   422,   423,     0,     0,   419,     0,     0,
       0,     0,     0,     0,   420,   421,   422,   423,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   696,     0,
       0,     0,   432,   433,   416,   417,   418,   434,   435,     0,
       0,     0,     0,     0,   436,   419,     0,     0,     0,   437,
     701,     0,   420,   421,   422,   423,   416,   417,   418,     0,
       0,     0,     0,     0,     0,     0,     0,   419,     0,     0,
       0,     0,     0,     0,   420,   421,   422,   423,     0,   704,
       0,     0,     0,   432,   433,   416,   417,   418,   434,   435,
       0,     0,     0,     0,     0,   436,   419,     0,     0,     0,
     437,     0,     0,   420,   421,   422,   423,     0,     0,     0,
       0,     0,     0,     0,     0,   717,     0,     0,     0,   432,
     433,   416,   417,   418,   434,   435,     0,     0,     0,     0,
       0,   436,   419,     0,     0,     0,   437,   720,     0,   420,
     421,   422,   423,   416,   417,   418,     0,     0,     0,     0,
     724,     0,     0,     0,   419,     0,   416,   417,   418,     0,
       0,   420,   421,   422,   423,     0,     0,   419,     0,     0,
       0,     0,     0,     0,   420,   421,   422,   423,     0,   432,
     433,   840,     0,     0,   434,   435,     0,   416,   417,   418,
       0,   436,     0,     0,     0,     0,   437,     0,   419,     0,
       0,     0,     0,     0,     0,   420,   421,   422,   423,     0,
       0,     0,   432,   433,     0,     0,     0,   434,   435,     0,
       0,     0,     0,     0,   436,     0,     0,     0,     0,   437,
       0,     0,     0,     0,   432,   433,     0,     0,     0,   434,
     435,     0,     0,     0,     0,     0,   436,   432,   433,   849,
       0,   437,   434,   435,     0,   416,   417,   418,     0,   436,
       0,     0,     0,     0,   437,     0,   419,     0,     0,     0,
       0,     0,     0,   420,   421,   422,   423,     0,     0,     0,
       0,     0,     0,     0,     0,   432,   433,     0,     0,     0,
     434,   435,     0,     0,     0,     0,     0,   436,     0,     0,
       0,     0,   437,   851,     0,     0,     0,   432,   433,   416,
     417,   418,   434,   435,     0,     0,     0,     0,     0,   436,
     419,     0,     0,     0,   437,     0,     0,   420,   421,   422,
     423,     0,   853,     0,     0,     0,   432,   433,   416,   417,
     418,   434,   435,     0,     0,     0,     0,     0,   436,   419,
       0,     0,     0,   437,     0,     0,   420,   421,   422,   423,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   432,   433,     0,     0,     0,   434,   435,     0,
       0,     0,     0,     0,   436,     0,     0,     0,     0,   437,
       0,     0,     0,     0,   432,   433,     0,     0,     0,   434,
     435,     0,     0,     0,     0,     0,   436,   432,   433,   855,
       0,   437,   434,   435,     0,   416,   417,   418,     0,   436,
       0,     0,     0,     0,   437,     0,   419,     0,     0,     0,
       0,     0,     0,   420,   421,   422,   423,     0,   432,   433,
     857,     0,     0,   434,   435,     0,   416,   417,   418,     0,
     436,     0,     0,   859,     0,   437,     0,   419,     0,   416,
     417,   418,     0,     0,   420,   421,   422,   423,   861,     0,
     419,     0,     0,     0,   416,   417,   418,   420,   421,   422,
     423,     0,     0,     0,     0,   419,     0,     0,     0,     0,
       0,     0,   420,   421,   422,   423,     0,     0,     0,     0,
       0,     0,  1205,     0,     0,     0,   432,   433,   416,   417,
     418,   434,   435,     0,     0,     0,     0,     0,   436,   419,
       0,     0,     0,   437,  1212,     0,   420,   421,   422,   423,
     416,   417,   418,     0,     0,     0,     0,     0,     0,     0,
       0,   419,     0,     0,     0,     0,     0,     0,   420,   421,
     422,   423,     0,     0,     0,     0,  1234,     0,     0,     0,
     432,   433,   416,   417,   418,   434,   435,     0,     0,     0,
       0,     0,   436,   419,     0,     0,     0,   437,     0,     0,
     420,   421,   422,   423,     0,  1415,     0,     0,     0,   432,
     433,   416,   417,   418,   434,   435,     0,     0,     0,     0,
       0,   436,   419,     0,     0,     0,   437,  1418,     0,   420,
     421,   422,   423,   416,   417,   418,     0,     0,     0,     0,
    1420,     0,     0,     0,   419,     0,   416,   417,   418,     0,
       0,   420,   421,   422,   423,  1422,     0,   419,     0,     0,
       0,   416,   417,   418,   420,   421,   422,   423,     0,     0,
       0,     0,   419,     0,     0,     0,     0,     0,     0,   420,
     421,   422,   423,     0,     0,     0,   432,   433,  1424,     0,
       0,   434,   435,     0,   416,   417,   418,     0,   436,     0,
       0,     0,     0,   437,     0,   419,     0,     0,     0,     0,
       0,     0,   420,   421,   422,   423,     0,   432,   433,     0,
       0,     0,   434,   435,     0,     0,     0,     0,     0,   436,
     432,   433,     0,     0,   437,   434,   435,     0,     0,     0,
       0,     0,   436,     0,     0,   432,   433,   437,  1426,     0,
     434,   435,     0,     0,   416,   417,   418,   436,     0,     0,
       0,     0,   437,     0,     0,   419,     0,     0,     0,     0,
       0,     0,   420,   421,   422,   423,     0,     0,     0,   432,
     433,     0,     0,     0,   434,   435,     0,     0,     0,     0,
       0,   436,     0,     0,     0,     0,   437,  1428,     0,     0,
       0,   432,   433,   416,   417,   418,   434,   435,     0,     0,
       0,     0,     0,   436,   419,     0,     0,     0,   437,     0,
       0,   420,   421,   422,   423,     0,     0,     0,     0,  1430,
       0,     0,     0,   432,   433,   416,   417,   418,   434,   435,
       0,     0,     0,     0,     0,   436,   419,     0,     0,     0,
     437,     0,     0,   420,   421,   422,   423,     0,     0,     0,
       0,     0,   432,   433,     0,     0,     0,   434,   435,     0,
       0,     0,     0,     0,   436,     0,     0,     0,     0,   437,
       0,     0,     0,     0,   432,   433,     0,     0,     0,   434,
     435,     0,     0,     0,     0,     0,   436,   432,   433,     0,
       0,   437,   434,   435,     0,     0,     0,     0,     0,   436,
       0,     0,   432,   433,   437,  1433,     0,   434,   435,     0,
       0,   416,   417,   418,   436,     0,     0,     0,     0,   437,
       0,     0,   419,     0,     0,     0,     0,     0,     0,   420,
     421,   422,   423,     0,     0,   432,   433,  1435,     0,     0,
     434,   435,     0,   416,   417,   418,     0,   436,     0,     0,
    1437,     0,   437,     0,   419,     0,   416,   417,   418,     0,
       0,   420,   421,   422,   423,  1439,     0,   419,     0,     0,
       0,   416,   417,   418,   420,   421,   422,   423,     0,     0,
       0,     0,   419,     0,     0,     0,     0,     0,     0,   420,
     421,   422,   423,     0,     0,   432,   433,  1441,     0,     0,
     434,   435,     0,   416,   417,   418,     0,   436,     0,     0,
    1443,     0,   437,     0,   419,     0,   416,   417,   418,     0,
       0,   420,   421,   422,   423,     0,     0,   419,     0,     0,
       0,     0,     0,     0,   420,   421,   422,   423,     0,     0,
    1445,     0,     0,     0,   432,   433,   416,   417,   418,   434,
     435,     0,     0,     0,     0,     0,   436,   419,     0,     0,
       0,   437,     0,     0,   420,   421,   422,   423,     0,     0,
       0,     0,  1447,     0,     0,     0,   432,   433,   416,   417,
     418,   434,   435,     0,     0,     0,     0,     0,   436,   419,
       0,     0,     0,   437,  1449,     0,   420,   421,   422,   423,
     416,   417,   418,     0,     0,     0,     0,  1451,     0,     0,
       0,   419,     0,   416,   417,   418,     0,     0,   420,   421,
     422,   423,  1453,     0,   419,     0,     0,     0,   416,   417,
     418,   420,   421,   422,   423,     0,     0,     0,     0,   419,
       0,     0,     0,     0,     0,     0,   420,   421,   422,   423,
       0,     0,     0,     0,     0,     0,     0,     0,  1455,     0,
       0,     0,   432,   433,   416,   417,   418,   434,   435,     0,
       0,     0,     0,     0,   436,   419,     0,     0,     0,   437,
       0,     0,   420,   421,   422,   423,     0,     0,     0,     0,
       0,     0,     0,     0,   432,   433,     0,     0,     0,   434,
     435,     0,     0,     0,     0,     0,   436,   432,   433,     0,
       0,   437,   434,   435,     0,     0,     0,     0,     0,   436,
       0,     0,   432,   433,   437,  1503,     0,   434,   435,     0,
       0,   416,   417,   418,   436,     0,     0,     0,     0,   437,
       0,     0,   419,     0,     0,     0,     0,     0,     0,   420,
     421,   422,   423,     0,   432,   433,     0,     0,     0,   434,
     435,     0,     0,     0,     0,     0,   436,   432,   433,  1506,
       0,   437,   434,   435,     0,   416,   417,   418,     0,   436,
       0,     0,     0,     0,   437,     0,   419,     0,     0,     0,
       0,     0,     0,   420,   421,   422,   423,   432,   433,  1660,
       0,     0,   434,   435,     0,   416,   417,   418,     0,   436,
       0,     0,     0,     0,   437,     0,   419,     0,     0,     0,
       0,     0,     0,   420,   421,   422,   423,     0,     0,   432,
     433,     0,     0,     0,   434,   435,     0,     0,     0,     0,
       0,   436,     0,     0,     0,     0,   437,     0,     0,     0,
       0,   432,   433,     0,     0,     0,   434,   435,     0,     0,
       0,     0,     0,   436,   432,   433,     0,     0,   437,   434,
     435,     0,     0,     0,     0,     0,   436,     0,     0,   432,
     433,   437,  1663,     0,   434,   435,     0,     0,   416,   417,
     418,   436,     0,     0,     0,     0,   437,     0,     0,   419,
       0,     0,     0,     0,     0,     0,   420,   421,   422,   423,
       0,  1666,     0,     0,     0,   432,   433,   416,   417,   418,
     434,   435,     0,     0,     0,     0,     0,   436,   419,     0,
       0,     0,   437,  1678,     0,   420,   421,   422,   423,   416,
     417,   418,     0,     0,     0,     0,  1697,     0,     0,     0,
     419,     0,   416,   417,   418,     0,     0,   420,   421,   422,
     423,     0,     0,   419,     0,     0,     0,     0,     0,     0,
     420,   421,   422,   423,     0,     0,     0,     0,  1705,     0,
       0,     0,   432,   433,   416,   417,   418,   434,   435,     0,
       0,     0,     0,     0,   436,   419,     0,     0,     0,   437,
       0,     0,   420,   421,   422,   423,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1783,     0,     0,     0,   432,   433,   416,   417,
     418,   434,   435,     0,     0,     0,     0,     0,   436,   419,
       0,     0,     0,   437,     0,     0,   420,   421,   422,   423,
       0,     0,  1833,     0,     0,     0,   432,   433,   416,   417,
     418,   434,   435,     0,     0,     0,     0,     0,   436,   419,
       0,     0,     0,   437,  1854,     0,   420,   421,   422,   423,
     416,   417,   418,     0,     0,     0,     0,  1861,     0,     0,
       0,   419,     0,   416,   417,   418,     0,     0,   420,   421,
     422,   423,  1934,     0,   419,     0,     0,     0,   416,   417,
     418,   420,   421,   422,   423,     0,     0,     0,     0,   419,
       0,     0,     0,     0,     0,     0,   420,   421,   422,   423,
       0,     0,     0,     0,     0,  1969,     0,     0,     0,   432,
     433,   416,   417,   418,   434,   435,     0,     0,     0,     0,
       0,   436,   419,     0,     0,     0,   437,     0,     0,   420,
     421,   422,   423,     0,     0,     0,     0,     0,   432,   433,
       0,     0,     0,   434,   435,     0,     0,     0,     0,     0,
     436,     0,     0,     0,     0,   437,     0,     0,     0,     0,
     432,   433,     0,     0,     0,   434,   435,     0,     0,     0,
       0,     0,   436,   432,   433,  2016,     0,   437,   434,   435,
       0,   416,   417,   418,     0,   436,     0,     0,     0,     0,
     437,     0,   419,     0,     0,     0,     0,     0,     0,   420,
     421,   422,   423,     0,     0,   432,   433,     0,     0,     0,
     434,   435,     0,     0,     0,     0,     0,   436,     0,     0,
       0,     0,   437,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   432,
     433,     0,     0,     0,   434,   435,     0,     0,     0,     0,
       0,   436,     0,     0,     0,     0,   437,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   432,
     433,     0,     0,     0,   434,   435,     0,     0,     0,     0,
       0,   436,     0,     0,     0,     0,   437,     0,     0,     0,
       0,   432,   433,     0,     0,     0,   434,   435,     0,     0,
       0,     0,     0,   436,   432,   433,     0,     0,   437,   434,
     435,     0,     0,     0,     0,     0,   436,     0,     0,   432,
     433,   437,     0,     0,   434,   435,     0,     0,   416,   417,
     418,   436,     0,     0,     0,     0,   437,     0,     0,   419,
       0,     0,     0,     0,     0,     0,   420,   421,   422,   423,
       0,     0,   432,   433,   416,   417,   418,   434,   435,     0,
       0,     0,     0,     0,   436,   419,     0,     0,     0,   437,
       0,     0,   420,   421,   422,   423,   416,   417,   418,     0,
       0,     0,     0,     0,     0,     0,     0,   419,     0,     0,
       0,     0,     0,     0,   420,   421,   422,   423,   416,   417,
     418,     0,     0,     0,     0,     0,     0,     0,     0,   419,
       0,     0,     0,     0,     0,     0,   420,   421,   422,   423,
       0,     0,   432,   433,   416,   417,   418,   434,   435,     0,
    1098,     0,     0,  1099,   436,   419,  1010,     0,   498,   437,
       0,   501,   420,   421,   422,   423,     0,     0,     0,     0,
       0,     0,   677,     0,     0,   466,     0,   467,   468,     0,
     469,   470,     0,     0,     0,   499,   500,     0,   502,   503,
    1510,  1511,     0,     0,   677,   471,     0,   466,     0,   467,
     468,     0,   469,   470,     0,     0,     0,   499,   500,     0,
     502,   503,     0,     0,     0,     0,   677,   471,     0,   466,
       0,   467,   468,     0,   469,   470,     0,     0,     0,   499,
     500,     0,   502,   503,     0,     0,     0,     0,     0,   471,
       0,     0,   677,     0,     0,   466,     0,   467,   468,     0,
     469,   470,     0,   416,   417,   418,     0,     0,     0,     0,
       0,     0,     0,     0,   419,   471,     0,     0,     0,     0,
       0,   420,   421,   422,   423,   416,   417,   418,     0,     0,
       0,     0,     0,     0,     0,     0,   419,     0,     0,     0,
       0,     0,     0,   420,   421,   422,   423,     0,     0,   432,
     433,   416,   417,   418,   434,   435,     0,     0,     0,     0,
       0,   436,   419,     0,     0,     0,   437,     0,     0,   420,
     421,   422,   423,     0,     0,   432,   433,     0,     0,     0,
     434,   435,     0,     0,     0,     0,     0,   436,  1513,     0,
       0,     0,   437,     0,     0,     0,     0,   432,   433,     0,
       0,     0,   434,   435,     0,  1102,     0,     0,  1103,  1561,
    1562,  1104,     0,   498,   437,     0,   501,     0,     0,   432,
     433,     0,     0,     0,   434,   435,     0,     0,     0,     0,
       0,   436,  1562,  1532,     0,  1533,   437,     0,  1534,     0,
       0,     0,     0,     0,     0,   432,   433,     0,     0,     0,
     434,   435,     0,     0,     0,     0,     0,   436,     0,   865,
     866,   867,   437,   868,   869,   870,   871,   872,   873,     0,
       0,     0,     0,   874,   875,   876,   877,   878,   879,   880,
       0,     0,     0,   881,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   882,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   883,     0,
       0,     0,     0,     0,   432,   433,     0,     0,     0,   434,
     435,     0,     0,     0,     0,     0,   436,     0,     0,     0,
       0,   437,     0,     0,     0,     0,   432,   433,     0,     0,
       0,   434,   435,     0,     0,     0,     0,     0,   436,  1535,
       0,     0,     0,   437,     0,     0,     0,     0,     0,     0,
       0,     0,   432,   433,     0,     0,     0,   434,   435,     0,
       0,     0,     0,     0,   436,     0,     0,     0,     0,   437,
     884,   885,   886,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   -30,     1,     0,     2,     0,     3,     0,     0,
       0,     4,     5,     0,     6,     7,     0,     0,     0,     0,
     887,   888,     0,     0,   889,   890,   891,   892,   893,   894,
     895,   896,     0,     0,     0,     0,     0,     0,   897,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,     0,     0,
       0,     0,     0,     0,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,  -242,  1368,
       0,     0,     0,     3,     0,     0,     0,     0,  1369,     0,
       6,     7,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    76,    77,    78,    79,    80,     0,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,     0,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,     0,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,     0,     0,     0,     0,     0,     0,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   192,  1682,     0,     0,     0,     3,     0,
       0,     0,     0,  1683,     0,     6,     7,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     334,   335,   336,   337,   338,    13,   339,    15,    16,    17,
      18,    19,    20,   340,    22,    23,    24,   341,    26,    27,
      28,   342,   343,    31,    32,    33,    34,   344,    36,   345,
      38,    39,    40,    41,    42,    43,   346,    45,    46,   347,
      48,    49,    50,    51,   348,    53,    54,    55,   349,    57,
     350,    59,    60,   351,    62,   352,    64,    65,    66,   353,
      68,   354,   355,   356,   357,   358,   359,   360,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    76,    77,    78,    79,
      80,     0,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,     0,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,     0,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,     0,
       0,     0,     0,     0,     0,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   192,  1921,
       0,     0,     0,     3,     0,     0,     0,     0,  1922,     0,
       6,     7,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    76,    77,    78,    79,    80,     0,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,     0,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,     0,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,     0,     0,     0,     0,     0,     0,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   192,  1972,     0,     0,     0,     3,     0,
       0,     0,     0,  1973,     0,     6,     7,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     334,   335,   336,   337,   338,    13,   339,    15,    16,    17,
      18,    19,    20,   340,    22,    23,    24,   341,    26,    27,
      28,   342,   343,    31,    32,    33,    34,   344,    36,   345,
      38,    39,    40,    41,    42,    43,   346,    45,    46,   347,
      48,    49,    50,    51,   348,    53,    54,    55,   349,    57,
     350,    59,    60,   351,    62,   352,    64,    65,    66,   353,
      68,   354,   355,   356,   357,   358,   359,   360,   865,   866,
     867,     0,   868,   869,   870,   871,   872,   873,     0,     0,
       0,     0,   874,   875,   876,   877,   878,     0,     0,     0,
       0,     0,   881,     0,     0,     0,    76,    77,    78,    79,
      80,     0,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,     0,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,     0,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,     0,
       0,     0,     0,     0,     0,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,   188,   189,   190,   191,   192,   865,
     866,   867,     0,   868,   869,   870,   871,   872,   873,   884,
       0,     0,     0,   874,   875,   876,   877,   878,   879,   880,
     865,   866,   867,   881,   868,   869,   870,   871,   872,   873,
       0,     0,     0,     0,   874,   875,   876,   877,   878,   879,
       0,     0,     0,     0,   881,   865,   866,   867,     0,   868,
     869,   870,   871,   872,   873,     0,     0,   882,     0,   874,
     875,   876,   877,   878,     0,     0,   865,   866,   867,   881,
     868,   869,   870,   871,   872,   873,     0,     0,   882,     0,
     874,   875,   876,   877,   878,     0,     0,     0,   883,     0,
     888,     0,     0,   889,   890,   891,   892,   893,   894,   895,
     896,     0,     0,   882,     0,     0,     0,     0,     0,   883,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   883,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     884,   885,   886,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   884,   885,   886,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   884,   885,   886,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     887,   888,     0,     0,   889,   890,   891,   892,   893,   894,
     895,   896,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   887,   888,     0,     0,   889,   890,   891,   892,   893,
     894,   895,   896,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   887,   888,     0,     0,
     889,   890,   891,   892,   893,   894,   895,   896,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   889,   890,   891,   892,   893,   894,   895,   896,  1750,
       0,     0,     0,  1751,     0,     0,     0,  1752,     0,     0,
       0,     0,     0,   951,   952,   953,     0,   954,   955,   956,
     957,   958,   959,     0,     0,  1753,  1754,     0,     0,     0,
       0,     0,     0,     0,     0,   967,   968,   969,   970,   971,
      13,   972,    15,    16,    17,    18,    19,    20,   973,    22,
     974,    24,   975,    26,    27,   976,   977,   978,    31,    32,
      33,    34,   979,   980,   981,    38,    39,    40,    41,    42,
      43,   982,    45,    46,   983,    48,    49,   984,   985,   986,
     987,    54,    55,   988,    57,   989,    59,    60,   990,    62,
     991,    64,    65,   992,   993,    68,   994,   995,   996,   997,
     998,   999,  1000,  1001,  1002,     0,  1003,  1004,  1005,  1006,
    1007,  1008,  1009,  1755,  1011,  1756,  1013,  1014,  1757,  1016,
    1017,  1018,  1019,  1020,  1021,  1022,  1023,  1024,  1025,  1026,
    1027,     0,     0,     0,    79,    80,  1028,    82,    83,    84,
      85,     0,    87,  1029,  1030,  1031,  1032,  1033,  1034,  1035,
    1036,  1037,  1038,  1039,  1040,  1041,  1042,  1043,  1044,  1045,
    1046,  1047,  1048,  1049,  1050,  1051,  1052,     0,  1053,  1054,
    1055,  1056,  1057,  1058,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   128,
     129,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   141,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   163,     0,     0,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,     0,
     179,  1750,     0,     0,   183,     0,     0,   186,     0,  1752,
     189,     0,     0,     0,     0,   951,   952,   953,     0,   954,
     955,   956,   957,   958,   959,     0,     0,  1753,  1754,     0,
       0,     0,     0,     0,     0,     0,     0,   967,   968,   969,
     970,   971,    13,   972,    15,    16,    17,    18,    19,    20,
     973,    22,   974,    24,   975,    26,    27,   976,   977,   978,
      31,    32,    33,    34,   979,   980,   981,    38,    39,    40,
      41,    42,    43,   982,    45,    46,   983,    48,    49,   984,
     985,   986,   987,    54,    55,   988,    57,   989,    59,    60,
     990,    62,   991,    64,    65,   992,   993,    68,   994,   995,
     996,   997,   998,   999,  1000,  1001,  1002,     0,  1003,  1004,
    1005,  1006,  1007,  1008,  1009,  1755,  1011,  1756,  1013,  1014,
    1757,  1016,  1017,  1018,  1019,  1020,  1021,  1022,  1023,  1024,
    1025,  1026,  1027,     0,     0,     0,    79,    80,  1028,    82,
      83,    84,    85,     0,    87,  1029,  1030,  1031,  1032,  1033,
    1034,  1035,  1036,  1037,  1038,  1039,  1040,  1041,  1042,  1043,
    1044,  1045,  1046,  1047,  1048,  1049,  1050,  1051,  1052,     0,
    1053,  1054,  1055,  1056,  1057,  1058,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   128,   129,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   141,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   163,     0,     0,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,     0,   179,     3,     0,     0,   183,   332,   333,   186,
       6,     7,   189,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   334,   335,   336,   337,   338,
      13,   339,    15,    16,    17,    18,    19,    20,   340,    22,
      23,    24,   341,    26,    27,    28,   342,   343,    31,    32,
      33,    34,   344,    36,   345,    38,    39,    40,    41,    42,
      43,   346,    45,    46,   347,    48,    49,    50,    51,   348,
      53,    54,    55,   349,    57,   350,    59,    60,   351,    62,
     352,    64,    65,    66,   353,    68,   354,   355,   356,   357,
     358,   359,   360,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    76,    77,    78,    79,    80,     0,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,     0,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,     0,     0,     0,     0,     0,     0,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     189,   190,   191,   192,     3,     0,     0,     0,     0,     0,
       0,     6,     7,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    76,    77,    78,    79,    80,     0,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,     0,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,     0,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,     0,     0,     0,     0,     0,
       0,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,     3,     0,     0,     0,     0,
       0,     0,     6,     7,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   334,   335,   336,
     337,   338,    13,   339,    15,    16,    17,    18,    19,    20,
     340,    22,    23,    24,   341,    26,    27,    28,   342,   343,
      31,    32,    33,    34,   344,    36,   345,    38,    39,    40,
      41,    42,    43,   346,    45,    46,   347,    48,    49,    50,
      51,   348,    53,    54,    55,   349,    57,   350,    59,    60,
     351,    62,   352,    64,    65,    66,   353,    68,   354,   355,
     356,   357,   358,   359,   360,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    76,    77,    78,    79,    80,     0,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
       0,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,     0,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,     0,     0,     0,     0,
       0,     0,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192
};

static const yytype_int16 yycheck[] =
{
       6,     2,    37,   479,   479,   729,   479,   479,   572,   479,
     801,   158,   728,   826,   479,    37,   479,   479,   479,    37,
     105,   738,   916,  1377,   109,   728,    37,   744,   728,    29,
    1289,   789,   138,    37,  1010,   599,  1012,    35,     1,  1015,
     448,    37,     1,  1374,    37,     1,     1,     1,  1382,     1,
       1,   157,     1,  1110,  1111,    53,     1,   774,     1,   820,
       1,   822,     1,     1,     9,     1,  1926,     1,     1,   477,
      76,     5,    78,  1111,     1,   792,   793,     1,     1,     1,
      86,     9,    88,  1876,    90,    91,    92,     1,    94,     1,
    1999,    97,    98,     5,     1,     0,  1690,  1676,     5,     1,
       1,     1,   108,  2012,    11,   111,     1,   113,     0,     1,
     116,     1,   118,     5,     1,     5,     1,     1,     5,    11,
       5,     5,  1098,  1099,   447,  1101,     1,  1095,     1,     0,
       1,   539,   540,   541,   542,  1093,     7,     8,     9,   797,
       1,  1328,  1329,   551,  1378,   553,   554,    18,  1382,  1743,
    1126,     1,  1128,     1,    25,    26,    27,    28,   722,  1688,
    1093,  1690,   726,     1,     1,   106,     1,     5,     1,     1,
       5,   112,     0,     1,  1082,  1083,  1084,     9,     1,     7,
       8,     9,     5,   106,     1,   128,   192,     1,     5,   112,
      18,     5,   116,   115,  1102,  1103,  1104,    25,    26,    27,
      28,     1,   116,   105,   527,     5,   108,   107,   110,   111,
       1,   113,   114,   108,  1743,   110,     7,     8,     9,     1,
    1337,     1,  1130,     5,  1132,     5,   128,    18,  2021,    11,
     105,  1348,   555,   556,    25,    26,    27,    28,  1281,     1,
     115,  1143,     1,     5,   105,     7,     8,     9,     1,     1,
       9,     1,     5,     5,   534,     5,    18,   105,   108,   138,
     110,  1163,     1,    25,    26,    27,    28,   115,   105,     8,
     550,   150,   105,  1330,  1331,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,    35,  1331,    37,   128,   422,   423,  1632,     1,
     101,   102,   181,     1,   263,   264,   269,   433,   434,   435,
     345,     9,   438,  1998,   273,   270,   271,   273,  1649,   273,
     272,   272,   271,   345,   115,   331,   117,   263,   264,   120,
     271,   270,   271,   271,   345,   269,   272,   745,   271,   728,
     338,   345,   342,   270,   271,   753,   344,   270,   271,   345,
     826,   826,   345,   826,   826,  1686,   826,   269,  1326,     2,
     267,   826,   269,   826,   826,   826,  1324,   268,   270,   271,
     262,   779,   780,   781,   782,   744,   261,   269,     1,   269,
     788,   163,   790,   791,   269,   269,     9,   258,   259,   262,
       1,  1324,   263,   264,     5,   401,   269,  1978,  1632,   270,
      11,  1162,     1,   105,   275,   774,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   792,   793,     1,   128,  1781,  1687,  1920,
     258,   259,     8,   439,   269,   263,   264,   270,   271,  1299,
       1,     1,   270,     1,     1,   451,   269,   275,     5,   335,
     745,   457,   269,     1,   342,   269,   462,     1,   753,   739,
       8,   694,   695,    15,    16,    17,   472,   258,   259,   269,
       1,   506,   263,   264,     5,   481,   100,    29,    30,   270,
     104,     1,  1458,   489,   275,   267,   268,   269,   341,   269,
     519,   520,   521,   532,   533,   642,   258,   259,   778,    37,
     647,   263,   264,   460,   510,  1820,   105,   787,   270,   108,
    1318,   110,   111,   275,   113,   114,   269,   269,  1494,   269,
    1702,   539,   540,   541,   542,   531,  1939,   755,     1,   128,
    1940,  1541,   538,   551,   552,   553,   554,   643,  1299,   545,
      12,   345,   648,  1123,   105,   105,  1330,   105,   108,  1307,
     110,    23,   163,   113,   115,  1331,    28,   115,   564,   117,
       1,   105,   120,    35,    36,  1111,  1109,  1329,   574,   271,
    1324,   115,  1120,  1927,   580,   581,   582,   583,  1711,  1337,
    1734,  1737,     1,  1326,  1845,  1376,  1347,   593,  1496,   109,
    1348,   138,   112,   138,    66,   776,   116,   603,  1870,  1626,
     120,   607,     1,   150,   610,   150,     1,  1347,     7,     8,
       9,   477,   753,   647,   620,     1,   163,    89,   163,    18,
     180,     7,     8,     9,    96,    11,    25,    26,    27,    28,
       1,  1890,   105,     1,   181,   107,   181,   533,   110,   645,
       1,   648,   115,   115,   117,     1,   652,   120,   695,   552,
     789,  1367,  2006,   125,  1869,   661,   267,   268,   269,   945,
     666,    36,   668,   159,  1367,   157,   672,  1367,  1562,   138,
     142,   270,   271,   679,   115,   147,   117,  1093,     1,   120,
     686,   824,   154,  1093,   156,  1093,   692,     1,  1562,   108,
     162,   110,   698,  1461,   113,  1466,   702,   116,  1806,   705,
    1108,  1109,     1,  1933,   710,    -1,   178,    -1,    -1,    -1,
     182,    -1,   184,   265,   266,   267,    -1,   823,   824,   725,
     115,   268,   117,   268,    -1,   120,   125,   126,   127,   128,
     129,   130,   131,   132,   105,   741,     1,   108,    -1,   110,
     111,   747,   113,   114,    -1,   751,    -1,   115,   109,   117,
     756,   112,   120,   879,   880,   116,   117,   128,    -1,   115,
     766,   117,    -1,     1,   120,    -1,   892,   893,   894,   895,
      -1,   777,   898,   899,   900,   901,   902,   903,   904,   905,
     906,   907,   908,   909,     1,   108,    -1,   110,    -1,    -1,
     113,   826,    -1,   116,    -1,    -1,   539,   540,   541,   542,
      -1,   115,    -1,   117,    -1,    -1,   120,   345,   551,   552,
     553,   554,   823,   824,     1,    -1,   115,    -1,   117,    -1,
      -1,   120,     1,   841,    -1,   831,    -1,    -1,   834,   835,
     836,   837,   838,   839,    -1,    -1,  1812,    -1,  1814,    -1,
     105,    -1,    -1,   108,    -1,   110,   111,     1,   113,   114,
      -1,    -1,    -1,     1,    -1,    -1,   328,   329,   330,   258,
     259,    -1,    -1,   128,   263,   264,   338,  1843,  1844,    -1,
      -1,   270,   344,   540,   541,   542,   275,   115,     1,   117,
     915,   916,   120,    -1,   551,    -1,   553,   554,   105,  1365,
    1365,   108,  1365,   110,   111,  1365,   113,   114,  1374,  1374,
     271,  1374,    -1,    -1,  1374,    -1,    -1,    -1,  1816,     1,
    1818,   128,   918,     1,  1822,    -1,  1324,   923,     1,    -1,
    1328,  1329,     5,    -1,    -1,    -1,   105,  1835,   115,   108,
     117,   110,   111,   120,   113,   114,    -1,     1,  1846,  1847,
      -1,   413,   414,    15,    16,    17,    -1,    -1,  1856,   128,
     158,   159,     1,    -1,    -1,  1863,    -1,    29,    30,     1,
      -1,   115,   179,   117,   436,  1681,   120,   115,    -1,   117,
      -1,  1688,   120,  1690,    -1,   447,  1789,    -1,  1681,  1770,
       1,  1681,   105,    -1,    -1,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     179,   539,   540,   541,   542,   128,   271,   479,    -1,    -1,
       1,    -1,    -1,   551,   552,   553,   554,    -1,    -1,     7,
       8,     9,    -1,   115,    -1,   117,  1743,   115,   120,   117,
      18,    -1,   120,    -1,   506,   507,    -1,    25,    26,    27,
      28,    -1,   125,   126,   127,   128,   129,   130,   131,   132,
    1774,   115,     1,   117,    -1,   527,   120,   529,   530,   108,
      -1,   110,   534,   105,   113,    -1,   108,   116,   110,   111,
      -1,   113,   114,    -1,    -1,  1093,   118,   119,   550,   121,
     122,    -1,    -1,   555,   556,    -1,   128,  1651,  1110,  1111,
    1108,  1109,    -1,     1,   115,    -1,   117,    -1,   841,   120,
    1185,    -1,    -1,    -1,    -1,   614,    -1,  1254,   617,    -1,
       1,  1146,   780,   781,   782,   624,     7,     8,     9,     1,
     788,    -1,   790,   791,   115,  1143,   117,    18,    -1,   120,
      -1,  1166,    -1,  1139,    25,    26,    27,    28,    -1,    -1,
      -1,    -1,    -1,  1149,    -1,  1163,    -1,   270,   271,    -1,
     622,   623,    -1,   625,   626,   627,   628,    -1,   630,   631,
      -1,    -1,   616,  1169,   618,    -1,   115,    -1,   117,    -1,
    1894,   120,    -1,  1649,  1649,   629,  1649,   649,   650,  1649,
     689,     1,   654,   655,   656,   657,   658,   659,    -1,    -1,
     699,   263,   264,   265,   266,   267,    -1,  1203,    37,    -1,
      -1,  1207,   711,    -1,   713,    -1,    -1,   115,  1214,   117,
    1686,  1686,   120,  1686,   105,    -1,  1686,   108,  1224,   110,
     111,  1227,    -1,    -1,  1230,    -1,    -1,   118,   119,   271,
    1236,    -1,    -1,   115,    -1,   117,   690,   128,   120,    -1,
     448,   779,   780,   781,   782,     1,   700,    -1,    -1,    -1,
     788,   789,   790,   791,    -1,  1979,    -1,  1263,   712,    -1,
     714,    -1,    -1,    -1,    -1,    -1,   738,   739,    -1,   477,
     258,   259,   744,    -1,    -1,   263,   264,    -1,    -1,  2003,
      -1,    -1,   270,   271,    -1,     1,    -1,   275,  1289,   109,
      -1,    -1,   112,    -1,    -1,   115,   116,    -1,    -1,    -1,
    1306,    -1,   774,   841,   776,    -1,   778,  1313,  1330,  1331,
      -1,    -1,    -1,  1789,  1789,   787,  1789,  1789,    -1,  1789,
     792,   793,   794,   795,  1789,   797,  1789,  1789,  1789,    -1,
      -1,   539,   540,   541,   542,  1341,    -1,    -1,    -1,     1,
      -1,    -1,    -1,   551,   552,   553,   554,  1353,    -1,    -1,
    1093,    -1,    -1,   109,   826,  1361,   112,    -1,  1364,   115,
     116,    -1,    -1,    -1,    -1,  1108,  1109,   258,   259,    -1,
      -1,  1406,   263,   264,  1380,    -1,    -1,    -1,  1384,   270,
      -1,    -1,  1388,    -1,   275,     1,    -1,    -1,  1394,    -1,
     106,    -1,   864,   109,    -1,    -1,   112,    -1,    -1,   115,
    1143,   117,    -1,  1409,   120,   613,   614,  1971,    -1,   617,
      -1,    -1,     7,     8,     9,    -1,   624,    -1,    -1,    -1,
    1163,    -1,    -1,    18,    -1,   897,    -1,    -1,    -1,    -1,
      25,    26,    27,    28,   642,    -1,    -1,     1,    -1,   647,
      -1,    -1,    -1,   105,  1920,  1920,   108,  1920,   110,   111,
    1920,   113,   114,    -1,    -1,    -1,   118,   119,    -1,   121,
     122,    -1,     1,    -1,    -1,    -1,   128,    -1,     7,     8,
       9,   779,   780,   781,   782,    -1,    -1,  1512,    -1,    18,
     788,   689,   790,   791,    -1,    -1,    25,    26,    27,    28,
       1,   699,    -1,   109,    -1,    -1,   112,    -1,    -1,   115,
     116,    -1,    -1,   711,    -1,   713,   345,    -1,    -1,    -1,
    1516,  1517,  1518,  1519,  1520,  1521,  1522,  1523,  1524,    -1,
      -1,  1527,    -1,    -1,    -1,  1531,  1561,  1562,   105,    -1,
      -1,  1537,   109,  1539,    -1,   112,    -1,   745,   115,   116,
     117,   105,    -1,   120,   108,   753,   110,   111,    -1,   113,
     114,    -1,    -1,    -1,   650,  1093,    -1,  1563,   654,   655,
     656,   657,   658,   659,   128,    -1,  1572,    -1,    -1,    -1,
      -1,   779,   780,   781,   782,    -1,    -1,    -1,    -1,    -1,
     788,   789,   790,   791,    -1,    -1,   125,   126,   127,   128,
      -1,    -1,    -1,  1599,   105,    -1,    -1,   108,    -1,   110,
     111,    -1,   113,   114,  1610,  1143,  1612,  1613,   270,   271,
      -1,    -1,  1618,  1619,    -1,   179,  1622,   128,  1090,    -1,
    1092,  1093,  1094,  1095,    -1,  1163,    -1,    -1,    -1,  1635,
      -1,    -1,    -1,   841,    -1,     1,  1108,  1109,  1110,  1111,
    1112,     7,     8,     9,    -1,    -1,    -1,  1119,  1120,  1121,
    1122,  1123,    18,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,   258,   259,    -1,    -1,    -1,   263,   264,
      -1,    -1,    -1,    -1,  1680,   270,   271,    -1,    -1,    -1,
     275,     1,    -1,  1689,    -1,    -1,  1687,    -1,    -1,   623,
    1696,   625,   626,   627,   628,    -1,   630,   631,    -1,    -1,
     539,   540,   541,   542,  1710,    -1,    -1,  1713,    -1,  1715,
    1716,  1717,   551,   552,   553,   554,    -1,  1723,    -1,   258,
     259,    -1,    -1,     1,   263,   264,    -1,    -1,    -1,    -1,
    1736,   270,    -1,  1739,    -1,     1,   275,    -1,    -1,   105,
      -1,    -1,   108,    -1,   110,   111,    -1,   113,   114,    -1,
      -1,    -1,   118,   119,  1789,   121,   122,   123,   124,     1,
      -1,    -1,   128,    -1,    -1,    -1,    -1,    -1,    -1,  1307,
      -1,    -1,    -1,    -1,    -1,    -1,  1248,    -1,  1250,    -1,
    1252,    -1,    -1,    -1,    -1,   105,  1324,  1793,   108,    -1,
     110,   111,    -1,   113,   114,    -1,    -1,     1,    -1,  1337,
      -1,    -1,    -1,     7,     8,     9,    -1,    -1,   128,  1281,
    1348,    -1,    -1,  1285,    18,    -1,    -1,    -1,    -1,    -1,
      -1,    25,    26,    27,    28,    -1,    -1,   105,    -1,    -1,
     108,    -1,   110,   111,  1840,   113,   114,  1309,    -1,   105,
      -1,    -1,   108,    -1,   110,   111,    -1,   113,   114,    -1,
     128,  1323,  1324,  1325,  1326,    -1,  1328,  1329,  1330,  1331,
    1332,  1333,   128,   105,    -1,    -1,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,    -1,    -1,    -1,    -1,  1093,   128,     1,    -1,  1890,
      -1,    -1,   258,   259,     1,    -1,    -1,   263,   264,    -1,
    1108,  1109,    -1,  1375,   270,   271,  1378,    -1,    -1,   275,
    1382,  1119,  1120,   179,  1122,  1123,    -1,    -1,    -1,    -1,
      -1,   125,   126,   127,   128,   129,   130,   131,   132,    -1,
      -1,  1937,  1938,  1939,  1940,  1143,    -1,    -1,  1944,  1945,
     779,   780,   781,   782,    -1,    -1,  1952,  1953,    -1,   788,
     789,   790,   791,    -1,    -1,  1163,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    18,    -1,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    -1,    -1,
      -1,    -1,   403,   404,    -1,    -1,   407,   408,   409,   410,
      -1,   105,    -1,    -1,   108,    -1,   110,   111,   105,   113,
     114,   108,   841,   110,   111,    -1,   113,   114,    -1,    -1,
      -1,    -1,  2018,    -1,   128,    -1,  1488,  1489,    -1,   440,
      -1,   128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   271,
      -1,    -1,   453,   454,    -1,    -1,    -1,    -1,   459,   460,
    1512,  1513,    -1,    -1,   465,    -1,  1254,  1255,    -1,    -1,
      -1,    -1,    -1,   474,   258,   259,    -1,  1529,    -1,   263,
     264,    -1,   483,  1535,    -1,   179,   270,   271,    -1,   490,
      -1,   275,   493,  1545,  1546,   496,   497,    -1,    -1,    -1,
      -1,    -1,  1554,  1555,    -1,    -1,    -1,    -1,    -1,  1561,
      -1,   512,   513,   514,   515,   516,   517,    -1,    -1,  1307,
      -1,   522,   523,   524,   525,   526,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1324,    -1,    -1,    -1,
    1328,  1329,    -1,    -1,     1,  1597,   547,   548,    -1,  1337,
       7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1348,    18,    -1,    -1,  1616,    -1,    -1,    -1,    25,    26,
      27,    28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1632,    -1,    -1,    -1,   585,     1,    -1,   588,    -1,    -1,
      -1,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1676,    -1,    -1,   258,   259,    -1,
      -1,    -1,   263,   264,    -1,    -1,  1688,    -1,  1690,   270,
     271,    -1,    -1,    -1,   275,    -1,    -1,    -1,   105,    -1,
    1702,   108,    -1,   110,   111,    -1,   113,   114,    -1,  1711,
      -1,   118,   119,    -1,   121,   122,    -1,    -1,    -1,    -1,
     671,   128,   673,    -1,  1093,    -1,    -1,    -1,    -1,   680,
      -1,    -1,  1734,    -1,    -1,  1737,   687,   688,    -1,   105,
      -1,  1743,   108,    -1,   110,   111,    -1,   113,   114,    -1,
      -1,     1,   118,   119,    -1,   121,   122,     7,     8,     9,
      -1,    -1,   128,    -1,    -1,    -1,    -1,    -1,    18,    -1,
      -1,    -1,    -1,    -1,  1143,    25,    26,    27,    28,    -1,
     731,   732,    -1,    -1,   735,    -1,    -1,  1789,    -1,     1,
      -1,   742,    -1,    -1,  1163,     7,     8,     9,    -1,   750,
      -1,    -1,    -1,    -1,    -1,    -1,    18,   758,    -1,    -1,
     761,   762,   763,    25,    26,    27,    28,   768,   769,   770,
     771,   772,   773,     1,    -1,    -1,    -1,    -1,    -1,     7,
       8,     9,    -1,   784,   785,    -1,    -1,    -1,    -1,    -1,
      18,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,
      28,   258,   259,     1,    -1,    -1,   263,   264,    -1,     7,
       8,     9,    -1,   270,   271,    -1,    -1,    -1,   275,    -1,
      18,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,
      28,    -1,    -1,    -1,  1886,    -1,    -1,    -1,    -1,    -1,
      -1,   842,   258,   259,    -1,    -1,    -1,   263,   264,    -1,
      -1,    -1,    -1,    -1,   270,   271,    -1,    -1,    -1,   275,
      -1,    -1,    -1,   125,   126,   127,   128,   129,   130,   131,
     132,    -1,    -1,    -1,    -1,    -1,    -1,   105,    -1,    -1,
     108,    -1,   110,   111,    -1,   113,   114,    -1,  1307,    -1,
     118,   119,    -1,   121,   122,    -1,    -1,    -1,    -1,    -1,
     128,    -1,    -1,    -1,    -1,  1324,    -1,   105,    -1,    -1,
     108,    -1,   110,   111,    -1,   113,   114,    -1,  1337,    -1,
     118,   119,    -1,   121,   122,    -1,    -1,    -1,    -1,  1348,
     128,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    2002,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   258,   259,
      -1,    -1,    -1,   263,   264,    -1,    -1,    -1,    -1,    -1,
     270,   271,    -1,    -1,    -1,   275,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   258,   259,    -1,    -1,
      -1,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,   271,
      -1,    -1,    -1,   275,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     258,   259,    -1,    -1,    -1,   263,   264,    -1,    -1,    -1,
      -1,    -1,   270,   271,    -1,    -1,    -1,   275,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     258,   259,    -1,    -1,    -1,   263,   264,    -1,    -1,    -1,
      -1,    -1,   270,   271,    -1,    -1,    -1,   275,    -1,  1867,
      -1,    -1,  1870,    -1,    -1,    -1,     1,    -1,    -1,    -1,
       5,    -1,     7,     8,     9,  1096,  1097,    -1,    -1,    -1,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    -1,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,  1193,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,    -1,    -1,
      -1,   136,   137,   138,   139,   140,   141,   142,    -1,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,    -1,   170,   171,   172,   173,   174,
     175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   191,   192,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   204,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   226,    -1,    -1,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,    -1,   242,    -1,    -1,
      -1,   246,    -1,    -1,   249,    -1,    -1,   252,    -1,    -1,
      -1,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,    -1,    -1,   270,    -1,    -1,     1,    -1,
     275,    -1,    -1,    -1,     7,     8,     9,    -1,    -1,    -1,
      -1,    -1,  1373,    -1,    -1,    18,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    27,    28,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,
       5,    -1,     7,     8,     9,    -1,    -1,    -1,    -1,    -1,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    -1,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,    -1,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,    -1,    -1,
      -1,   136,   137,   138,   139,   140,   141,   142,  1549,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,    -1,   170,   171,   172,   173,   174,
     175,     1,    -1,    -1,    -1,    -1,    -1,     7,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,   191,   192,    18,    -1,
      -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,   204,
      -1,    -1,    -1,    -1,    -1,   258,   259,    -1,    -1,    -1,
     263,   264,  1623,  1624,  1625,  1626,    -1,   270,   271,    -1,
      -1,   226,   275,    -1,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,    -1,   242,    -1,    -1,
      -1,   246,    -1,    -1,   249,    -1,    -1,   252,    -1,    -1,
      -1,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,    -1,    -1,   270,    -1,    -1,    -1,    -1,
     275,    -1,    -1,    -1,    -1,   105,    -1,    -1,   108,     1,
     110,   111,    -1,   113,   114,     7,     8,     9,   118,   119,
      -1,   121,   122,    -1,     1,    -1,    18,    -1,   128,    -1,
       7,     8,     9,    25,    26,    27,    28,    -1,    -1,    -1,
      -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,
      27,    28,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,    -1,    -1,     7,     8,     9,    -1,    -1,    -1,
      -1,    -1,    -1,     1,    -1,    18,    -1,    -1,  1759,     7,
       8,     9,    25,    26,    27,    28,    -1,    -1,    -1,    -1,
      18,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,
      28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   105,    -1,    -1,   108,     1,   110,   111,
      -1,   113,   114,     7,     8,     9,   118,   119,    -1,   121,
     122,    -1,    -1,    -1,    18,    -1,   128,    -1,    -1,    -1,
      -1,    25,    26,    27,    28,    -1,    -1,    -1,   125,   126,
     127,   128,   129,   130,   131,   132,    -1,    -1,   258,   259,
      -1,    -1,   105,   263,   264,   108,    -1,   110,   111,    -1,
     270,   271,    -1,    -1,    -1,   275,    -1,   105,   121,   122,
     108,    -1,   110,   111,    -1,   128,    -1,    -1,     1,    -1,
     118,   119,    -1,    -1,     7,     8,     9,    -1,    -1,    -1,
     128,    -1,    -1,    -1,    -1,    18,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    27,    28,    -1,    -1,    -1,    -1,
      -1,   105,    -1,    -1,   108,    -1,   110,   111,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     1,    -1,   121,   122,    -1,
      -1,     7,     8,     9,   128,    -1,    -1,    -1,  1929,    -1,
      -1,    -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    -1,    -1,    -1,   258,   259,    -1,    -1,
      -1,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,   271,
      -1,   258,   259,   275,     1,    -1,   263,   264,    -1,    -1,
       7,     8,     9,   270,    -1,    -1,   109,  1978,   275,   112,
      -1,    18,   115,    -1,   117,    -1,    -1,   120,    25,    26,
      27,    28,    -1,    -1,    -1,   258,   259,    -1,    -1,    -1,
     263,   264,    -1,    -1,    -1,    -1,    -1,   270,    -1,    -1,
     258,   259,   275,     1,    -1,   263,   264,    -1,    -1,     7,
       8,     9,   270,   109,    -1,    -1,   112,   275,    -1,   115,
      18,   117,    -1,    -1,   120,    -1,    -1,    25,    26,    27,
      28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,   258,   259,     7,     8,     9,   263,
     264,    -1,    -1,    -1,    -1,    -1,   270,    18,    -1,    -1,
      -1,   275,     1,    -1,    25,    26,    27,    28,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,    18,
      -1,   128,     7,     8,     9,    -1,    25,    26,    27,    28,
      -1,    -1,    -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,
      25,    26,    27,    28,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,     1,    -1,    -1,    -1,   258,   259,     7,     8,     9,
     263,   264,    -1,    -1,    -1,    -1,    -1,   270,    18,    -1,
     128,    -1,   275,     1,    -1,    25,    26,    27,    28,     7,
       8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      18,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,
      28,    -1,   258,   259,    -1,    -1,    -1,   263,   264,     1,
      -1,    -1,    -1,    -1,   270,     7,     8,     9,    -1,   275,
      -1,    -1,    -1,    -1,    -1,    -1,    18,    -1,    -1,   128,
      -1,    -1,    -1,    25,    26,    27,    28,    -1,    -1,    -1,
      -1,    -1,    -1,   128,    -1,    -1,    -1,     1,    -1,    -1,
      -1,   258,   259,     7,     8,     9,   263,   264,    -1,    -1,
      -1,    -1,    -1,   270,    18,    -1,    -1,    -1,   275,     1,
      -1,    25,    26,    27,    28,     7,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    18,    -1,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    -1,    -1,    -1,
     258,   259,     1,    -1,    -1,   263,   264,    -1,     7,     8,
       9,    -1,   270,    -1,    -1,    -1,    -1,   275,    -1,    18,
      -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   258,   259,    -1,
      -1,    -1,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,
     271,    -1,    -1,    -1,   275,    -1,    -1,    -1,    -1,   258,
     259,    -1,    -1,    -1,   263,   264,    -1,    -1,    -1,    -1,
      -1,   270,    -1,   258,   259,     1,   275,    -1,   263,   264,
      -1,     7,     8,     9,    -1,   270,    -1,    -1,    -1,    -1,
     275,    -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    -1,    -1,    -1,    -1,    -1,   258,   259,
      -1,    -1,    -1,   263,   264,    -1,    -1,    -1,    -1,    -1,
     270,    -1,    -1,    -1,    -1,   275,     1,    -1,    -1,    -1,
     258,   259,     7,     8,     9,   263,   264,    -1,    -1,    -1,
      -1,    -1,   270,    18,    -1,    -1,    -1,   275,    -1,    -1,
      25,    26,    27,    28,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     1,    -1,    -1,    -1,   258,   259,     7,     8,
       9,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,    18,
      -1,    -1,    -1,   275,    -1,    -1,    25,    26,    27,    28,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   258,   259,    -1,    -1,    -1,   263,
     264,    -1,    -1,    -1,    -1,    -1,   270,    -1,    -1,    -1,
      -1,   275,     1,    -1,    -1,    -1,   258,   259,     7,     8,
       9,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,    18,
      -1,    -1,    -1,   275,    -1,    -1,    25,    26,    27,    28,
      -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,   258,
     259,     7,     8,     9,   263,   264,    -1,    -1,    -1,    -1,
      -1,   270,    18,    -1,    -1,    -1,   275,     1,    -1,    25,
      26,    27,    28,     7,     8,     9,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    18,    -1,     7,     8,     9,    -1,
      -1,    25,    26,    27,    28,    -1,    -1,    18,    -1,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,   258,   259,     7,     8,     9,   263,   264,    -1,
      -1,    -1,    -1,    -1,   270,    18,    -1,    -1,    -1,   275,
       1,    -1,    25,    26,    27,    28,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    18,    -1,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    -1,     1,
      -1,    -1,    -1,   258,   259,     7,     8,     9,   263,   264,
      -1,    -1,    -1,    -1,    -1,   270,    18,    -1,    -1,    -1,
     275,    -1,    -1,    25,    26,    27,    28,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,   258,
     259,     7,     8,     9,   263,   264,    -1,    -1,    -1,    -1,
      -1,   270,    18,    -1,    -1,    -1,   275,     1,    -1,    25,
      26,    27,    28,     7,     8,     9,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    18,    -1,     7,     8,     9,    -1,
      -1,    25,    26,    27,    28,    -1,    -1,    18,    -1,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    -1,   258,
     259,     1,    -1,    -1,   263,   264,    -1,     7,     8,     9,
      -1,   270,    -1,    -1,    -1,    -1,   275,    -1,    18,    -1,
      -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,    -1,
      -1,    -1,   258,   259,    -1,    -1,    -1,   263,   264,    -1,
      -1,    -1,    -1,    -1,   270,    -1,    -1,    -1,    -1,   275,
      -1,    -1,    -1,    -1,   258,   259,    -1,    -1,    -1,   263,
     264,    -1,    -1,    -1,    -1,    -1,   270,   258,   259,     1,
      -1,   275,   263,   264,    -1,     7,     8,     9,    -1,   270,
      -1,    -1,    -1,    -1,   275,    -1,    18,    -1,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   258,   259,    -1,    -1,    -1,
     263,   264,    -1,    -1,    -1,    -1,    -1,   270,    -1,    -1,
      -1,    -1,   275,     1,    -1,    -1,    -1,   258,   259,     7,
       8,     9,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,
      18,    -1,    -1,    -1,   275,    -1,    -1,    25,    26,    27,
      28,    -1,     1,    -1,    -1,    -1,   258,   259,     7,     8,
       9,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,    18,
      -1,    -1,    -1,   275,    -1,    -1,    25,    26,    27,    28,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   258,   259,    -1,    -1,    -1,   263,   264,    -1,
      -1,    -1,    -1,    -1,   270,    -1,    -1,    -1,    -1,   275,
      -1,    -1,    -1,    -1,   258,   259,    -1,    -1,    -1,   263,
     264,    -1,    -1,    -1,    -1,    -1,   270,   258,   259,     1,
      -1,   275,   263,   264,    -1,     7,     8,     9,    -1,   270,
      -1,    -1,    -1,    -1,   275,    -1,    18,    -1,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    -1,   258,   259,
       1,    -1,    -1,   263,   264,    -1,     7,     8,     9,    -1,
     270,    -1,    -1,     1,    -1,   275,    -1,    18,    -1,     7,
       8,     9,    -1,    -1,    25,    26,    27,    28,     1,    -1,
      18,    -1,    -1,    -1,     7,     8,     9,    25,    26,    27,
      28,    -1,    -1,    -1,    -1,    18,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    27,    28,    -1,    -1,    -1,    -1,
      -1,    -1,     1,    -1,    -1,    -1,   258,   259,     7,     8,
       9,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,    18,
      -1,    -1,    -1,   275,     1,    -1,    25,    26,    27,    28,
       7,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,    25,    26,
      27,    28,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,
     258,   259,     7,     8,     9,   263,   264,    -1,    -1,    -1,
      -1,    -1,   270,    18,    -1,    -1,    -1,   275,    -1,    -1,
      25,    26,    27,    28,    -1,     1,    -1,    -1,    -1,   258,
     259,     7,     8,     9,   263,   264,    -1,    -1,    -1,    -1,
      -1,   270,    18,    -1,    -1,    -1,   275,     1,    -1,    25,
      26,    27,    28,     7,     8,     9,    -1,    -1,    -1,    -1,
       1,    -1,    -1,    -1,    18,    -1,     7,     8,     9,    -1,
      -1,    25,    26,    27,    28,     1,    -1,    18,    -1,    -1,
      -1,     7,     8,     9,    25,    26,    27,    28,    -1,    -1,
      -1,    -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    -1,    -1,    -1,   258,   259,     1,    -1,
      -1,   263,   264,    -1,     7,     8,     9,    -1,   270,    -1,
      -1,    -1,    -1,   275,    -1,    18,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    27,    28,    -1,   258,   259,    -1,
      -1,    -1,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,
     258,   259,    -1,    -1,   275,   263,   264,    -1,    -1,    -1,
      -1,    -1,   270,    -1,    -1,   258,   259,   275,     1,    -1,
     263,   264,    -1,    -1,     7,     8,     9,   270,    -1,    -1,
      -1,    -1,   275,    -1,    -1,    18,    -1,    -1,    -1,    -1,
      -1,    -1,    25,    26,    27,    28,    -1,    -1,    -1,   258,
     259,    -1,    -1,    -1,   263,   264,    -1,    -1,    -1,    -1,
      -1,   270,    -1,    -1,    -1,    -1,   275,     1,    -1,    -1,
      -1,   258,   259,     7,     8,     9,   263,   264,    -1,    -1,
      -1,    -1,    -1,   270,    18,    -1,    -1,    -1,   275,    -1,
      -1,    25,    26,    27,    28,    -1,    -1,    -1,    -1,     1,
      -1,    -1,    -1,   258,   259,     7,     8,     9,   263,   264,
      -1,    -1,    -1,    -1,    -1,   270,    18,    -1,    -1,    -1,
     275,    -1,    -1,    25,    26,    27,    28,    -1,    -1,    -1,
      -1,    -1,   258,   259,    -1,    -1,    -1,   263,   264,    -1,
      -1,    -1,    -1,    -1,   270,    -1,    -1,    -1,    -1,   275,
      -1,    -1,    -1,    -1,   258,   259,    -1,    -1,    -1,   263,
     264,    -1,    -1,    -1,    -1,    -1,   270,   258,   259,    -1,
      -1,   275,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,
      -1,    -1,   258,   259,   275,     1,    -1,   263,   264,    -1,
      -1,     7,     8,     9,   270,    -1,    -1,    -1,    -1,   275,
      -1,    -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    -1,    -1,   258,   259,     1,    -1,    -1,
     263,   264,    -1,     7,     8,     9,    -1,   270,    -1,    -1,
       1,    -1,   275,    -1,    18,    -1,     7,     8,     9,    -1,
      -1,    25,    26,    27,    28,     1,    -1,    18,    -1,    -1,
      -1,     7,     8,     9,    25,    26,    27,    28,    -1,    -1,
      -1,    -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    -1,    -1,   258,   259,     1,    -1,    -1,
     263,   264,    -1,     7,     8,     9,    -1,   270,    -1,    -1,
       1,    -1,   275,    -1,    18,    -1,     7,     8,     9,    -1,
      -1,    25,    26,    27,    28,    -1,    -1,    18,    -1,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,    -1,    -1,
       1,    -1,    -1,    -1,   258,   259,     7,     8,     9,   263,
     264,    -1,    -1,    -1,    -1,    -1,   270,    18,    -1,    -1,
      -1,   275,    -1,    -1,    25,    26,    27,    28,    -1,    -1,
      -1,    -1,     1,    -1,    -1,    -1,   258,   259,     7,     8,
       9,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,    18,
      -1,    -1,    -1,   275,     1,    -1,    25,    26,    27,    28,
       7,     8,     9,    -1,    -1,    -1,    -1,     1,    -1,    -1,
      -1,    18,    -1,     7,     8,     9,    -1,    -1,    25,    26,
      27,    28,     1,    -1,    18,    -1,    -1,    -1,     7,     8,
       9,    25,    26,    27,    28,    -1,    -1,    -1,    -1,    18,
      -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,   258,   259,     7,     8,     9,   263,   264,    -1,
      -1,    -1,    -1,    -1,   270,    18,    -1,    -1,    -1,   275,
      -1,    -1,    25,    26,    27,    28,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   258,   259,    -1,    -1,    -1,   263,
     264,    -1,    -1,    -1,    -1,    -1,   270,   258,   259,    -1,
      -1,   275,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,
      -1,    -1,   258,   259,   275,     1,    -1,   263,   264,    -1,
      -1,     7,     8,     9,   270,    -1,    -1,    -1,    -1,   275,
      -1,    -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    -1,   258,   259,    -1,    -1,    -1,   263,
     264,    -1,    -1,    -1,    -1,    -1,   270,   258,   259,     1,
      -1,   275,   263,   264,    -1,     7,     8,     9,    -1,   270,
      -1,    -1,    -1,    -1,   275,    -1,    18,    -1,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,   258,   259,     1,
      -1,    -1,   263,   264,    -1,     7,     8,     9,    -1,   270,
      -1,    -1,    -1,    -1,   275,    -1,    18,    -1,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    -1,    -1,   258,
     259,    -1,    -1,    -1,   263,   264,    -1,    -1,    -1,    -1,
      -1,   270,    -1,    -1,    -1,    -1,   275,    -1,    -1,    -1,
      -1,   258,   259,    -1,    -1,    -1,   263,   264,    -1,    -1,
      -1,    -1,    -1,   270,   258,   259,    -1,    -1,   275,   263,
     264,    -1,    -1,    -1,    -1,    -1,   270,    -1,    -1,   258,
     259,   275,     1,    -1,   263,   264,    -1,    -1,     7,     8,
       9,   270,    -1,    -1,    -1,    -1,   275,    -1,    -1,    18,
      -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      -1,     1,    -1,    -1,    -1,   258,   259,     7,     8,     9,
     263,   264,    -1,    -1,    -1,    -1,    -1,   270,    18,    -1,
      -1,    -1,   275,     1,    -1,    25,    26,    27,    28,     7,
       8,     9,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,
      18,    -1,     7,     8,     9,    -1,    -1,    25,    26,    27,
      28,    -1,    -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,
      25,    26,    27,    28,    -1,    -1,    -1,    -1,     1,    -1,
      -1,    -1,   258,   259,     7,     8,     9,   263,   264,    -1,
      -1,    -1,    -1,    -1,   270,    18,    -1,    -1,    -1,   275,
      -1,    -1,    25,    26,    27,    28,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     1,    -1,    -1,    -1,   258,   259,     7,     8,
       9,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,    18,
      -1,    -1,    -1,   275,    -1,    -1,    25,    26,    27,    28,
      -1,    -1,     1,    -1,    -1,    -1,   258,   259,     7,     8,
       9,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,    18,
      -1,    -1,    -1,   275,     1,    -1,    25,    26,    27,    28,
       7,     8,     9,    -1,    -1,    -1,    -1,     1,    -1,    -1,
      -1,    18,    -1,     7,     8,     9,    -1,    -1,    25,    26,
      27,    28,     1,    -1,    18,    -1,    -1,    -1,     7,     8,
       9,    25,    26,    27,    28,    -1,    -1,    -1,    -1,    18,
      -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,    -1,   258,
     259,     7,     8,     9,   263,   264,    -1,    -1,    -1,    -1,
      -1,   270,    18,    -1,    -1,    -1,   275,    -1,    -1,    25,
      26,    27,    28,    -1,    -1,    -1,    -1,    -1,   258,   259,
      -1,    -1,    -1,   263,   264,    -1,    -1,    -1,    -1,    -1,
     270,    -1,    -1,    -1,    -1,   275,    -1,    -1,    -1,    -1,
     258,   259,    -1,    -1,    -1,   263,   264,    -1,    -1,    -1,
      -1,    -1,   270,   258,   259,     1,    -1,   275,   263,   264,
      -1,     7,     8,     9,    -1,   270,    -1,    -1,    -1,    -1,
     275,    -1,    18,    -1,    -1,    -1,    -1,    -1,    -1,    25,
      26,    27,    28,    -1,    -1,   258,   259,    -1,    -1,    -1,
     263,   264,    -1,    -1,    -1,    -1,    -1,   270,    -1,    -1,
      -1,    -1,   275,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   258,
     259,    -1,    -1,    -1,   263,   264,    -1,    -1,    -1,    -1,
      -1,   270,    -1,    -1,    -1,    -1,   275,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   258,
     259,    -1,    -1,    -1,   263,   264,    -1,    -1,    -1,    -1,
      -1,   270,    -1,    -1,    -1,    -1,   275,    -1,    -1,    -1,
      -1,   258,   259,    -1,    -1,    -1,   263,   264,    -1,    -1,
      -1,    -1,    -1,   270,   258,   259,    -1,    -1,   275,   263,
     264,    -1,    -1,    -1,    -1,    -1,   270,    -1,    -1,   258,
     259,   275,    -1,    -1,   263,   264,    -1,    -1,     7,     8,
       9,   270,    -1,    -1,    -1,    -1,   275,    -1,    -1,    18,
      -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      -1,    -1,   258,   259,     7,     8,     9,   263,   264,    -1,
      -1,    -1,    -1,    -1,   270,    18,    -1,    -1,    -1,   275,
      -1,    -1,    25,    26,    27,    28,     7,     8,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    18,    -1,    -1,
      -1,    -1,    -1,    -1,    25,    26,    27,    28,     7,     8,
       9,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    18,
      -1,    -1,    -1,    -1,    -1,    -1,    25,    26,    27,    28,
      -1,    -1,   258,   259,     7,     8,     9,   263,   264,    -1,
     109,    -1,    -1,   112,   270,    18,   115,    -1,   117,   275,
      -1,   120,    25,    26,    27,    28,    -1,    -1,    -1,    -1,
      -1,    -1,   105,    -1,    -1,   108,    -1,   110,   111,    -1,
     113,   114,    -1,    -1,    -1,   118,   119,    -1,   121,   122,
     123,   124,    -1,    -1,   105,   128,    -1,   108,    -1,   110,
     111,    -1,   113,   114,    -1,    -1,    -1,   118,   119,    -1,
     121,   122,    -1,    -1,    -1,    -1,   105,   128,    -1,   108,
      -1,   110,   111,    -1,   113,   114,    -1,    -1,    -1,   118,
     119,    -1,   121,   122,    -1,    -1,    -1,    -1,    -1,   128,
      -1,    -1,   105,    -1,    -1,   108,    -1,   110,   111,    -1,
     113,   114,    -1,     7,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    18,   128,    -1,    -1,    -1,    -1,
      -1,    25,    26,    27,    28,     7,     8,     9,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    18,    -1,    -1,    -1,
      -1,    -1,    -1,    25,    26,    27,    28,    -1,    -1,   258,
     259,     7,     8,     9,   263,   264,    -1,    -1,    -1,    -1,
      -1,   270,    18,    -1,    -1,    -1,   275,    -1,    -1,    25,
      26,    27,    28,    -1,    -1,   258,   259,    -1,    -1,    -1,
     263,   264,    -1,    -1,    -1,    -1,    -1,   270,   271,    -1,
      -1,    -1,   275,    -1,    -1,    -1,    -1,   258,   259,    -1,
      -1,    -1,   263,   264,    -1,   109,    -1,    -1,   112,   270,
     271,   115,    -1,   117,   275,    -1,   120,    -1,    -1,   258,
     259,    -1,    -1,    -1,   263,   264,    -1,    -1,    -1,    -1,
      -1,   270,   271,   115,    -1,   117,   275,    -1,   120,    -1,
      -1,    -1,    -1,    -1,    -1,   258,   259,    -1,    -1,    -1,
     263,   264,    -1,    -1,    -1,    -1,    -1,   270,    -1,    15,
      16,    17,   275,    19,    20,    21,    22,    23,    24,    -1,
      -1,    -1,    -1,    29,    30,    31,    32,    33,    34,    35,
      -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   104,    -1,
      -1,    -1,    -1,    -1,   258,   259,    -1,    -1,    -1,   263,
     264,    -1,    -1,    -1,    -1,    -1,   270,    -1,    -1,    -1,
      -1,   275,    -1,    -1,    -1,    -1,   258,   259,    -1,    -1,
      -1,   263,   264,    -1,    -1,    -1,    -1,    -1,   270,   271,
      -1,    -1,    -1,   275,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   258,   259,    -1,    -1,    -1,   263,   264,    -1,
      -1,    -1,    -1,    -1,   270,    -1,    -1,    -1,    -1,   275,
     176,   177,   178,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     0,     1,    -1,     3,    -1,     5,    -1,    -1,
      -1,     9,    10,    -1,    12,    13,    -1,    -1,    -1,    -1,
     256,   257,    -1,    -1,   260,   261,   262,   263,   264,   265,
     266,   267,    -1,    -1,    -1,    -1,    -1,    -1,   274,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,    -1,    -1,
      -1,    -1,    -1,    -1,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,   207,
     208,   209,   210,   211,   212,   213,   214,   215,   216,   217,
     218,   219,   220,   221,   222,   223,   224,   225,   226,   227,
     228,   229,   230,   231,   232,   233,   234,   235,   236,   237,
     238,   239,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,   251,   252,   253,   254,   255,     0,     1,
      -1,    -1,    -1,     5,    -1,    -1,    -1,    -1,    10,    -1,
      12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   133,   134,   135,   136,   137,    -1,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,    -1,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   250,   251,
     252,   253,   254,   255,     1,    -1,    -1,    -1,     5,    -1,
      -1,    -1,    -1,    10,    -1,    12,    13,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   133,   134,   135,   136,
     137,    -1,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,    -1,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,    -1,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,    -1,
      -1,    -1,    -1,    -1,    -1,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,   251,   252,   253,   254,   255,     1,
      -1,    -1,    -1,     5,    -1,    -1,    -1,    -1,    10,    -1,
      12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   133,   134,   135,   136,   137,    -1,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,    -1,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   250,   251,
     252,   253,   254,   255,     1,    -1,    -1,    -1,     5,    -1,
      -1,    -1,    -1,    10,    -1,    12,    13,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,    15,    16,
      17,    -1,    19,    20,    21,    22,    23,    24,    -1,    -1,
      -1,    -1,    29,    30,    31,    32,    33,    -1,    -1,    -1,
      -1,    -1,    39,    -1,    -1,    -1,   133,   134,   135,   136,
     137,    -1,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,    -1,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,    -1,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,    -1,
      -1,    -1,    -1,    -1,    -1,   182,   183,   184,   185,   186,
     187,   188,   189,   190,   191,   192,   193,   194,   195,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
     207,   208,   209,   210,   211,   212,   213,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,   251,   252,   253,   254,   255,    15,
      16,    17,    -1,    19,    20,    21,    22,    23,    24,   176,
      -1,    -1,    -1,    29,    30,    31,    32,    33,    34,    35,
      15,    16,    17,    39,    19,    20,    21,    22,    23,    24,
      -1,    -1,    -1,    -1,    29,    30,    31,    32,    33,    34,
      -1,    -1,    -1,    -1,    39,    15,    16,    17,    -1,    19,
      20,    21,    22,    23,    24,    -1,    -1,    73,    -1,    29,
      30,    31,    32,    33,    -1,    -1,    15,    16,    17,    39,
      19,    20,    21,    22,    23,    24,    -1,    -1,    73,    -1,
      29,    30,    31,    32,    33,    -1,    -1,    -1,   104,    -1,
     257,    -1,    -1,   260,   261,   262,   263,   264,   265,   266,
     267,    -1,    -1,    73,    -1,    -1,    -1,    -1,    -1,   104,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     176,   177,   178,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   176,   177,   178,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   176,   177,   178,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     256,   257,    -1,    -1,   260,   261,   262,   263,   264,   265,
     266,   267,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   256,   257,    -1,    -1,   260,   261,   262,   263,   264,
     265,   266,   267,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   256,   257,    -1,    -1,
     260,   261,   262,   263,   264,   265,   266,   267,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   260,   261,   262,   263,   264,   265,   266,   267,     1,
      -1,    -1,    -1,     5,    -1,    -1,    -1,     9,    -1,    -1,
      -1,    -1,    -1,    15,    16,    17,    -1,    19,    20,    21,
      22,    23,    24,    -1,    -1,    27,    28,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,    -1,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,    -1,    -1,    -1,   136,   137,   138,   139,   140,   141,
     142,    -1,   144,   145,   146,   147,   148,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,    -1,   170,   171,
     172,   173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   191,
     192,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   204,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   226,    -1,    -1,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,    -1,
     242,     1,    -1,    -1,   246,    -1,    -1,   249,    -1,     9,
     252,    -1,    -1,    -1,    -1,    15,    16,    17,    -1,    19,
      20,    21,    22,    23,    24,    -1,    -1,    27,    28,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,    -1,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,    -1,    -1,    -1,   136,   137,   138,   139,
     140,   141,   142,    -1,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   157,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,    -1,
     170,   171,   172,   173,   174,   175,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   191,   192,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   204,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   226,    -1,    -1,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,    -1,   242,     5,    -1,    -1,   246,     9,    10,   249,
      12,    13,   252,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   133,   134,   135,   136,   137,    -1,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,    -1,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,   220,   221,
     222,   223,   224,   225,   226,   227,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   250,   251,
     252,   253,   254,   255,     5,    -1,    -1,    -1,    -1,    -1,
      -1,    12,    13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   133,   134,   135,   136,   137,    -1,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,    -1,
     151,   152,   153,   154,   155,   156,   157,   158,   159,   160,
     161,   162,    -1,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,    -1,    -1,    -1,    -1,    -1,
      -1,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,   207,   208,   209,   210,
     211,   212,   213,   214,   215,   216,   217,   218,   219,   220,
     221,   222,   223,   224,   225,   226,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,     5,    -1,    -1,    -1,    -1,
      -1,    -1,    12,    13,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   133,   134,   135,   136,   137,    -1,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
      -1,   151,   152,   153,   154,   155,   156,   157,   158,   159,
     160,   161,   162,    -1,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,    -1,    -1,    -1,    -1,
      -1,    -1,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   212,   213,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,   238,   239,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     1,     3,     5,     9,    10,    12,    13,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,   192,
     193,   194,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,   205,   206,   207,   208,   209,   210,   211,   212,
     213,   214,   215,   216,   217,   218,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,   229,   230,   231,   232,
     233,   234,   235,   236,   237,   238,   239,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,   251,   252,
     253,   254,   255,   290,   291,   292,   294,   296,   297,   321,
     345,   346,   347,   348,   349,   355,   359,   363,   364,   366,
     367,   368,   371,   372,   373,   375,   376,   377,   378,   379,
     380,   381,   382,   383,   398,   402,   405,   412,   414,   415,
     416,   424,   425,   426,   427,   429,   431,   432,   434,   435,
     436,   438,   445,   455,   457,   459,   461,   465,   467,   469,
     473,   479,   481,   483,   485,   487,   489,   491,   493,   495,
     498,   500,   502,   506,   509,   513,   515,   517,   519,   528,
     530,   546,   550,   552,   554,   556,   558,   560,   562,   564,
     566,   568,   570,   572,   575,   584,   587,   634,   636,   638,
     640,   644,   648,   651,   655,   657,   659,   667,   669,   671,
     673,   675,   677,   679,   683,   687,   691,   695,   699,   701,
     703,   705,   707,   714,   718,   720,   724,   728,   732,   736,
     738,   740,   744,   746,   748,   752,   754,   756,   883,   884,
     885,   888,     9,    10,    37,    38,    39,    40,    41,    43,
      50,    54,    58,    59,    64,    66,    73,    76,    81,    85,
      87,    90,    92,    96,    98,    99,   100,   101,   102,   103,
     104,   293,   295,   296,   299,   323,   405,   439,   446,   463,
     471,   474,   504,   511,   520,   531,   548,   576,   588,   653,
     660,   681,   685,   689,   693,   697,   708,   716,   722,   726,
     730,   734,   742,   750,   298,   322,     1,     5,   797,     1,
       8,   807,     1,   105,   115,   440,     1,   105,   115,   117,
     120,   447,   451,   462,   470,     1,     7,     8,     9,    18,
      25,    26,    27,    28,   125,   126,   127,   128,   129,   130,
     131,   132,   258,   259,   263,   264,   270,   275,   288,   475,
     808,   809,   858,   859,   863,   864,   865,   503,   510,     1,
     518,   858,     1,   106,   112,   270,   271,   521,   523,   843,
     844,     1,   529,   858,     1,   105,   108,   110,   111,   113,
     114,   128,   532,   534,   813,   814,   817,   547,     1,   270,
     271,   577,   580,   808,   841,   845,   846,   858,     1,   585,
     809,   858,     1,   105,   109,   112,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   270,   271,   589,   591,
     596,   619,   814,   817,   818,   819,   834,   836,   837,   838,
     839,   840,   841,   842,   851,   854,   855,   652,     1,   270,
     271,   661,   670,   674,   680,     1,     5,   678,   808,   684,
     688,   692,   696,     1,   700,   858,     1,   105,   115,   709,
     715,   721,   725,   729,   733,   741,   749,   797,   797,   797,
     797,     1,     5,   365,   858,   797,   797,   797,   797,     1,
       9,   757,   759,     1,   374,   858,   797,   797,     1,     9,
     804,   804,   807,   807,     1,     9,   384,   399,   804,   757,
       9,   406,     1,   413,   858,   797,   757,     1,   417,   858,
     797,   797,     1,     8,   428,     1,   430,   858,   797,     1,
       8,   433,   797,   456,   458,   437,   460,   466,   468,     1,
     480,   858,   482,   484,   499,   486,   488,   490,   492,   501,
     494,   496,     1,   108,   110,   113,   116,   508,   826,   828,
     830,   835,   514,   516,     1,   551,   858,   553,   555,   559,
     565,     1,   557,   858,   561,   563,   567,   569,   571,   573,
       1,   635,   858,     1,   108,   110,   637,     1,   639,   858,
       1,   113,   641,   829,   830,   835,     1,   105,   179,   649,
     810,   811,   812,   813,   816,     1,   645,   811,   812,   656,
     658,     1,   668,   858,   672,   676,     1,   702,   858,   704,
     706,     1,   719,   858,     1,   737,   858,     1,   108,   110,
     739,   745,   747,   753,   755,   797,     0,     1,   350,   858,
       1,   360,   362,   858,     1,   356,   858,   797,   300,   324,
       1,   105,   115,   442,     1,   105,   449,   451,   464,   472,
       1,   476,   808,   858,   505,   512,     1,   522,   523,     1,
     105,   533,   534,   549,     1,   271,   578,   581,   808,   858,
       1,   105,   115,   116,   590,   591,   597,   620,   814,   817,
     818,   819,   834,   851,   654,     1,   271,   662,   682,   686,
     690,   694,   698,     1,   105,   115,   711,   717,   723,   727,
     731,   735,   743,   751,   138,   150,   163,   181,   268,   301,
     313,   314,   352,   353,   369,   403,     1,     5,    11,   163,
     267,   268,   269,   325,   331,   332,   797,     1,   269,   798,
     798,   798,   798,   798,   798,     1,   270,   271,   764,   766,
     768,   811,   818,   819,   820,   841,   842,   854,   855,   858,
       1,   788,   858,     1,     5,   867,     1,     9,   866,     1,
     863,     1,   863,     1,   863,     1,   863,     1,   863,     1,
     858,     1,   863,   797,   798,    15,    16,    17,    19,    20,
      21,    22,    23,    24,    29,    30,    31,    32,    33,    34,
      35,    39,    73,   104,   176,   177,   178,   256,   257,   260,
     261,   262,   263,   264,   265,   266,   267,   274,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   289,
     764,     1,   115,   117,   120,   270,   271,   769,   771,   772,
     773,   774,   775,   777,   811,   820,   825,   833,   834,   835,
     797,   798,   798,     1,   116,     1,   116,   797,   526,   798,
     526,   797,   798,   797,     1,   270,   271,   535,   798,   769,
       5,    15,    16,    17,    19,    20,    21,    22,    23,    24,
      29,    30,    31,    32,    33,    34,    35,    37,    38,    39,
      40,    41,    43,    50,    52,    54,    57,    58,    59,    64,
      65,    66,    73,    76,    79,    80,    81,    82,    85,    87,
      90,    92,    95,    96,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   138,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   170,   171,   172,   173,   174,   175,   256,
     257,   260,   261,   262,   265,   266,   267,   871,   874,   875,
     877,   878,   880,   881,   882,   883,   884,   885,   886,   887,
     888,     1,   115,   117,   120,   579,   848,   849,   850,   797,
     798,   797,   798,   798,   798,   798,   592,   593,   109,   112,
     851,   858,   109,   112,   115,   851,   858,   797,   798,   798,
     798,   798,   798,   798,   618,   798,   618,   618,   618,   798,
     798,   798,   798,   798,   764,     1,   128,   664,   858,     1,
     128,   663,   858,   797,     1,   106,   117,   120,   781,   782,
     832,   833,   781,   788,   797,     1,   270,   271,   784,   791,
     793,   811,   820,   841,   842,   854,   855,   784,   784,   784,
     797,   798,   798,   788,   784,     1,   270,   271,   786,   794,
     796,   811,   841,   842,   854,   855,   784,   784,   764,   764,
     797,     1,     5,   269,   758,   799,   797,   797,   797,   797,
     797,   798,   798,   407,   797,     5,   418,   799,   797,   797,
     797,     1,   760,   810,   760,     1,   761,   858,   761,   760,
     761,   797,     1,   497,   858,   497,   760,   497,   497,   497,
     497,   761,   497,   497,   507,     1,   779,   810,     1,   780,
     826,   797,   779,   780,     1,   574,   858,   574,   797,   574,
     574,   574,   574,   574,   574,   797,   797,   797,   798,   797,
     798,   797,   798,   797,   798,   798,   760,   761,   797,     1,
     105,   180,   783,   827,   828,   783,   797,   760,   761,   797,
     797,   797,   760,   761,   760,   761,     1,     5,   269,     5,
     361,   799,   797,     5,   357,   799,   268,   303,   315,   316,
     352,   353,   369,   403,   163,   268,   328,   331,   798,   798,
     798,     1,   271,   765,   767,   768,   858,   788,   797,   798,
     765,     1,   770,   771,   797,   798,   797,     1,   271,   536,
     770,   579,   797,   798,   798,   798,   798,   797,   798,   798,
     798,   798,   798,   798,   765,   663,   797,   788,     1,   271,
     785,   792,   793,   785,   785,   785,   798,   798,   788,   785,
       1,   271,   787,   795,   796,   785,   785,   765,   765,     1,
     354,   858,     1,   370,   858,   404,   354,   302,     1,    10,
     294,   317,   336,   335,   327,   334,   326,   333,   441,     1,
     444,   825,   448,     1,   452,   825,     1,   117,   453,   831,
     834,   835,     1,   120,   454,   831,   871,     1,   797,   797,
     797,   797,   797,   797,   797,     1,   270,   271,   789,   794,
     798,   868,     1,   272,   802,     1,   477,   858,     1,   863,
       1,   863,     1,   863,     1,   863,     1,   863,     1,   863,
       1,   858,   860,     1,   863,     1,   863,     1,   863,     1,
     863,     1,   863,     1,   863,     1,   863,     1,   863,     1,
     863,     1,   863,     1,   863,     1,   863,     1,   115,   778,
     847,   851,   853,     1,   776,   848,   851,   852,   797,   797,
       1,   107,   524,     1,   115,   525,     1,   272,   800,     1,
     273,   801,     1,   115,   117,   120,   527,     1,   270,   271,
     540,     1,   128,   537,   806,   538,   806,   800,   800,   800,
     801,   801,   801,     1,   582,   858,     1,   586,   858,     1,
     123,   124,   270,   271,   613,   615,   616,   811,   820,   837,
     838,   839,   840,   841,   842,   854,   855,   858,     1,   271,
     621,   858,   115,   117,   120,   271,   630,   632,   836,   858,
     798,   798,   800,   800,     1,   263,   264,   272,   856,   800,
     801,   801,   801,     1,   263,   264,   273,   857,     1,   273,
     803,   270,   271,   608,   610,   612,   811,   820,   841,   855,
     858,     1,   605,   610,   820,     1,   118,   119,   598,   602,
     604,   815,   816,   817,   821,   823,   858,     1,   121,   122,
     599,   603,   604,   815,   822,   824,     1,   271,   622,   858,
     105,   109,   112,   115,   116,   117,   120,   633,     1,   105,
     805,   617,   811,   858,   617,     1,   271,   627,   858,   595,
     811,   858,   595,   800,   800,   801,   801,   797,   789,     1,
       1,   797,   710,     1,   713,   825,   789,     1,     1,   797,
     757,     1,     5,   261,   269,   385,   400,     5,   408,   798,
       1,     9,   419,   797,   797,   797,   797,   797,   797,   797,
       1,   642,   858,     1,   643,   858,     1,   650,   858,     1,
     646,   810,     1,   647,   811,   797,   351,   360,     1,   358,
     858,   304,     1,    10,   295,   319,   330,   329,   443,   444,
     450,     1,   797,     1,   271,   790,   795,     1,   478,   858,
     797,     1,   271,   541,   538,     1,   583,   858,   614,   615,
     858,   271,   623,   858,   631,   632,   858,   609,   611,   612,
     820,   858,     1,   606,   611,   820,     1,   600,   602,   858,
       1,   601,   603,   858,   271,   624,   858,   271,   628,   858,
     790,     1,   797,   712,   713,   790,     1,   797,   797,   797,
       1,     5,     9,    27,    28,   115,   117,   120,   409,   411,
     872,   873,   874,   876,   878,   879,   880,   882,   886,   305,
     306,   352,   353,   369,   318,     1,     5,   269,   337,     5,
     344,   798,   409,     1,   340,   858,   294,   332,     1,   270,
     271,   762,   766,   858,   797,   762,   797,   797,   797,     1,
       1,   797,     1,     5,   869,     1,   268,   861,   800,   856,
     857,     1,   128,   543,   858,     1,   128,   542,   858,   800,
     801,     1,   858,   797,   797,   797,   797,   797,   797,   797,
     797,   797,   797,     1,   625,   858,   797,   797,   797,     1,
     105,   594,   594,   858,   858,   798,   858,   858,     1,   607,
     848,   852,   797,   797,     1,   626,   858,   797,   797,   797,
     797,     1,   629,   858,   797,   797,   797,   798,   666,   798,
     798,   666,   762,   797,   388,   386,   387,     1,     7,     8,
       9,    11,   401,   409,     5,   420,   799,   350,   797,   307,
     308,   352,   353,   369,   320,   409,   295,     1,   271,   763,
     767,   797,   763,     1,   797,   542,   797,   625,   797,   797,
     797,   797,   797,   626,   797,   629,   797,   763,     5,   410,
     798,     1,    10,   294,   309,   331,   339,   338,   332,   341,
     871,     1,   797,   870,     1,   859,   862,   800,   800,   801,
     801,   539,   797,   539,   801,   801,   797,   800,   800,   633,
     801,   801,   801,   801,     1,   665,   811,   805,   665,     5,
     389,   799,     1,     5,    11,   262,   269,   393,   389,     1,
     421,   858,     1,    10,   295,   311,   331,     1,   411,   310,
     337,   332,     5,   342,   798,   869,   544,   797,   545,   797,
     544,   545,   797,   797,   797,   797,   390,   396,   394,   395,
       5,   422,   799,   312,   410,   331,   343,     1,     5,   391,
       1,   262,   269,   397,   397,   393,     1,   423,   858,   331,
     332,   392,   393,   797,   389
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (mach, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, mach)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, mach); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, Machine & mach)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, mach)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    Machine & mach;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (mach);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, Machine & mach)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, mach)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    Machine & mach;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, mach);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, Machine & mach)
#else
static void
yy_reduce_print (yyvsp, yyrule, mach)
    YYSTYPE *yyvsp;
    int yyrule;
    Machine & mach;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , mach);
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, mach); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, Machine & mach)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, mach)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    Machine & mach;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (mach);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (Machine & mach);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (Machine & mach)
#else
int
yyparse (mach)
    Machine & mach;
#endif
#endif
{
  /* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 31:
#line 464 "../parser.yxx"
    { YYACCEPT; }
    break;

  case 32:
#line 465 "../parser.yxx"
    { mach.errmsg ("EQU without label"); }
    break;

  case 33:
#line 466 "../parser.yxx"
    { mach.errmsg ("DEFL without label"); }
    break;

  case 34:
#line 467 "../parser.yxx"
    { badinstruction (mach, (yyvsp[(1) - (1)])); }
    break;

  case 101:
#line 552 "../parser.yxx"
    { mach.addcode (OpEmpty); }
    break;

  case 202:
#line 664 "../parser.yxx"
    {
			mach.addlabel ((yyvsp[(1) - (1)]).identifier () );
			mach.expectmacro ();
		}
    break;

  case 204:
#line 673 "../parser.yxx"
    {
			mach.addlabel ((yyvsp[(1) - (1)]).identifier () );
			mach.expectmacro ();
		}
    break;

  case 210:
#line 685 "../parser.yxx"
    { mach.expectmacro (); }
    break;

  case 217:
#line 694 "../parser.yxx"
    { mach.expectmacro (); }
    break;

  case 223:
#line 702 "../parser.yxx"
    { mach.addcode (OpGenLabel); }
    break;

  case 228:
#line 709 "../parser.yxx"
    { mach.addcode (OpGenLabel); }
    break;

  case 230:
#line 713 "../parser.yxx"
    { mach.addmacroname ((yyvsp[(1) - (1)])); }
    break;

  case 233:
#line 715 "../parser.yxx"
    { badinstruction (mach, (yyvsp[(1) - (1)])); }
    break;

  case 234:
#line 719 "../parser.yxx"
    { mach.addmacroname ((yyvsp[(1) - (1)])); }
    break;

  case 237:
#line 721 "../parser.yxx"
    { badinstruction (mach, (yyvsp[(1) - (1)])); }
    break;

  case 238:
#line 724 "../parser.yxx"
    { mach.addcode (OpGenLabel); }
    break;

  case 240:
#line 726 "../parser.yxx"
    { mach.addcode (OpGenLabel); }
    break;

  case 243:
#line 729 "../parser.yxx"
    { mach.addmacroname ((yyvsp[(1) - (1)])); }
    break;

  case 246:
#line 731 "../parser.yxx"
    { badinstruction (mach, (yyvsp[(1) - (1)])); }
    break;

  case 247:
#line 735 "../parser.yxx"
    { mach.addmacroname ((yyvsp[(1) - (1)])); }
    break;

  case 250:
#line 737 "../parser.yxx"
    { badinstruction (mach, (yyvsp[(1) - (1)])); }
    break;

  case 251:
#line 746 "../parser.yxx"
    { mach.addmacroname ((yyvsp[(1) - (1)])); }
    break;

  case 253:
#line 752 "../parser.yxx"
    { mach.addmacroname ((yyvsp[(1) - (1)])); }
    break;

  case 255:
#line 757 "../parser.yxx"
    { mach.cancelmacroname (); }
    break;

  case 257:
#line 758 "../parser.yxx"
    { mach.redefmacro (); }
    break;

  case 260:
#line 763 "../parser.yxx"
    { mach.cancelmacroname (); }
    break;

  case 262:
#line 764 "../parser.yxx"
    { mach.redefmacro (); }
    break;

  case 265:
#line 768 "../parser.yxx"
    { mach.addcode (OpExpandMacro); }
    break;

  case 267:
#line 772 "../parser.yxx"
    { mach.addcode (OpMacroArg); }
    break;

  case 269:
#line 773 "../parser.yxx"
    { mach.nomacroargs (); }
    break;

  case 271:
#line 775 "../parser.yxx"
    {
			mach.addmacroitem ((yyvsp[(1) - (1)]));
			mach.addcode (OpMacroArg);
		}
    break;

  case 273:
#line 781 "../parser.yxx"
    {
			mach.addmacroitem ((yyvsp[(1) - (1)]));
			yyclearin;
		}
    break;

  case 275:
#line 789 "../parser.yxx"
    { mach.addcode (OpMacroArg); }
    break;

  case 276:
#line 790 "../parser.yxx"
    { mach.addcode (OpMacroArg); }
    break;

  case 278:
#line 792 "../parser.yxx"
    {
			mach.addmacroitem (Token (TypeWhiteSpace) );
			mach.addmacroitem ((yyvsp[(1) - (1)]));
			yyclearin;
		}
    break;

  case 280:
#line 801 "../parser.yxx"
    { mach.addcode (OpMacroValue); }
    break;

  case 282:
#line 802 "../parser.yxx"
    { mach.errmsg ("Value argument expected"); }
    break;

  case 284:
#line 807 "../parser.yxx"
    { mach.macroargs (); }
    break;

  case 288:
#line 825 "../parser.yxx"
    { mach.addcode (OpASEG); }
    break;

  case 289:
#line 831 "../parser.yxx"
    { mach.addcode (OpCSEG); }
    break;

  case 290:
#line 837 "../parser.yxx"
    { mach.addcode (OpDSEG); }
    break;

  case 291:
#line 843 "../parser.yxx"
    { unimplemented (mach, (yyvsp[(1) - (1)])); }
    break;

  case 292:
#line 849 "../parser.yxx"
    { mach.addcode (OpDEFBend); }
    break;

  case 293:
#line 869 "../parser.yxx"
    { mach.addcode (OpDEFBnum); }
    break;

  case 294:
#line 870 "../parser.yxx"
    { mach.addcode (OpDEFBnum); }
    break;

  case 296:
#line 871 "../parser.yxx"
    { badnextarg (mach, (yyvsp[(2) - (2)])); }
    break;

  case 297:
#line 872 "../parser.yxx"
    { badDEFB (mach, (yyvsp[(1) - (1)])); }
    break;

  case 301:
#line 883 "../parser.yxx"
    { mach.addcode (OpDEFL); }
    break;

  case 302:
#line 884 "../parser.yxx"
    { badDEFL (mach, (yyvsp[(1) - (1)])); }
    break;

  case 305:
#line 895 "../parser.yxx"
    { badDEFS (mach, (yyvsp[(1) - (1)])); }
    break;

  case 306:
#line 899 "../parser.yxx"
    { mach.addcode (OpDEFS); }
    break;

  case 308:
#line 904 "../parser.yxx"
    { mach.addcode (OpDEFSvalue); }
    break;

  case 309:
#line 905 "../parser.yxx"
    { badDEFS_value (mach, (yyvsp[(1) - (1)])); }
    break;

  case 310:
#line 912 "../parser.yxx"
    { mach.addcode (OpDEFWend); }
    break;

  case 314:
#line 922 "../parser.yxx"
    { mach.addcode (OpDEFWnum); }
    break;

  case 315:
#line 923 "../parser.yxx"
    { badDEFW (mach, (yyvsp[(1) - (1)])); }
    break;

  case 316:
#line 930 "../parser.yxx"
    { mach.addcode (OpELSE); }
    break;

  case 318:
#line 939 "../parser.yxx"
    { mach.addcode (OpEND); }
    break;

  case 319:
#line 940 "../parser.yxx"
    { mach.addcode (OpENDn); }
    break;

  case 320:
#line 941 "../parser.yxx"
    { badEND (mach, (yyvsp[(1) - (1)])); }
    break;

  case 321:
#line 948 "../parser.yxx"
    { mach.addcode (OpENDIF); }
    break;

  case 322:
#line 954 "../parser.yxx"
    { mach.addcode (OpENDM); }
    break;

  case 323:
#line 960 "../parser.yxx"
    { mach.addcode (OpENDP); }
    break;

  case 326:
#line 969 "../parser.yxx"
    { mach.addcode (OpEQU); }
    break;

  case 327:
#line 970 "../parser.yxx"
    { badEQU (mach, (yyvsp[(1) - (1)])); }
    break;

  case 328:
#line 977 "../parser.yxx"
    { mach.addcode (OpEXITM); }
    break;

  case 329:
#line 983 "../parser.yxx"
    { mach.addcode (OpEXTRN); }
    break;

  case 331:
#line 992 "../parser.yxx"
    { mach.addcode (OpIF); }
    break;

  case 332:
#line 993 "../parser.yxx"
    { badIF (mach, (yyvsp[(1) - (1)])); }
    break;

  case 333:
#line 1000 "../parser.yxx"
    { mach.addcode (OpIF1); }
    break;

  case 334:
#line 1006 "../parser.yxx"
    { mach.addcode (OpIF2); }
    break;

  case 335:
#line 1013 "../parser.yxx"
    {
			mach.addcode (OpIFDEF);
			mach.addcode ((yyvsp[(2) - (3)]).identifier () );
		}
    break;

  case 336:
#line 1024 "../parser.yxx"
    {
			mach.addcode (OpIFNDEF);
			mach.addcode ((yyvsp[(2) - (3)]).identifier () );
		}
    break;

  case 337:
#line 1035 "../parser.yxx"
    {
		mach.addcode (OpINCBIN);
		mach.addcodeliteral ((yyvsp[(2) - (3)]).literal () );
	}
    break;

  case 338:
#line 1046 "../parser.yxx"
    { mach.addcode (OpINCLUDE); }
    break;

  case 339:
#line 1050 "../parser.yxx"
    { mach.errmsg ("Can't open INCLUDE file: " + (yyvsp[(2) - (3)]).raw () ); }
    break;

  case 340:
#line 1054 "../parser.yxx"
    { mach.addcode (OpEndINCLUDE); }
    break;

  case 342:
#line 1065 "../parser.yxx"
    {
			mach.addcode (OpIRP);
			mach.addcodeliteral ((yyvsp[(1) - (3)]).identifier () );
		}
    break;

  case 343:
#line 1069 "../parser.yxx"
    { badIRP (mach, (yyvsp[(1) - (1)])); }
    break;

  case 344:
#line 1073 "../parser.yxx"
    { mach.macroargs (); }
    break;

  case 346:
#line 1074 "../parser.yxx"
    { mach.addcode (OpMacroArg); }
    break;

  case 347:
#line 1075 "../parser.yxx"
    { mach.addcode (OpMacroArg); }
    break;

  case 349:
#line 1077 "../parser.yxx"
    {
			mach.addmacroitem ((yyvsp[(1) - (1)]));
			yyclearin;
		}
    break;

  case 351:
#line 1085 "../parser.yxx"
    { mach.addcode (OpMacroArg); }
    break;

  case 352:
#line 1086 "../parser.yxx"
    { mach.addcode (OpMacroArg); }
    break;

  case 354:
#line 1090 "../parser.yxx"
    { mach.addcode (OpMacroArg); }
    break;

  case 355:
#line 1092 "../parser.yxx"
    {
			mach.addmacroitem ((yyvsp[(1) - (1)]));
			yyclearin;
		}
    break;

  case 358:
#line 1100 "../parser.yxx"
    { mach.errmsg ("Macro arg list unclosed in IRP"); }
    break;

  case 360:
#line 1103 "../parser.yxx"
    {
			mach.addmacroitem ((yyvsp[(1) - (1)]));
			mach.addcode (OpMacroArg);
		}
    break;

  case 362:
#line 1109 "../parser.yxx"
    { mach.addcode (OpMacroArg); }
    break;

  case 364:
#line 1112 "../parser.yxx"
    {
			mach.addmacroitem ((yyvsp[(1) - (1)]));
			mach.addcode (OpMacroArg);
			yyclearin;
		}
    break;

  case 368:
#line 1123 "../parser.yxx"
    { mach.unexpected ((yyvsp[(1) - (1)]), "in IRP args, ',' or '>' expected"); }
    break;

  case 370:
#line 1134 "../parser.yxx"
    {
			mach.addcode (OpIRPC);
			mach.addcodeliteral ((yyvsp[(1) - (2)]).identifier () );
			mach.macroargs ();
		}
    break;

  case 372:
#line 1143 "../parser.yxx"
    { mach.addcodeliteral ((yyvsp[(1) - (1)]).literal () ); }
    break;

  case 373:
#line 1144 "../parser.yxx"
    { mach.addcodeliteral ((yyvsp[(1) - (1)]).macroarg () ); }
    break;

  case 374:
#line 1145 "../parser.yxx"
    { mach.addcodeliteral ((yyvsp[(1) - (1)]).identifier () ); }
    break;

  case 375:
#line 1146 "../parser.yxx"
    { mach.addcodeliteral ((yyvsp[(1) - (1)]).numstr () ); }
    break;

  case 376:
#line 1150 "../parser.yxx"
    { mach.addcodeliteral ((yyvsp[(1) - (1)]).raw () ); }
    break;

  case 377:
#line 1157 "../parser.yxx"
    { mach.addcode (OpLOCAL); }
    break;

  case 378:
#line 1163 "../parser.yxx"
    { mach.addcode (OpMACRO); }
    break;

  case 381:
#line 1170 "../parser.yxx"
    {
			mach.addlabel ((yyvsp[(1) - (1)]).identifier () );
			mach.addcode (OpMACRO);
		}
    break;

  case 383:
#line 1178 "../parser.yxx"
    { mach.addcode (OpMACROend); }
    break;

  case 385:
#line 1183 "../parser.yxx"
    { mach.addcode (OpMACROend); }
    break;

  case 387:
#line 1188 "../parser.yxx"
    { mach.addcode (OpMACROend); }
    break;

  case 389:
#line 1193 "../parser.yxx"
    { mach.addcodeliteral ((yyvsp[(1) - (1)]).identifier () ); }
    break;

  case 390:
#line 1194 "../parser.yxx"
    { mach.addcodeliteral ((yyvsp[(1) - (1)]).raw () ); }
    break;

  case 391:
#line 1195 "../parser.yxx"
    { badMACROitem (mach, (yyvsp[(1) - (1)])); }
    break;

  case 393:
#line 1205 "../parser.yxx"
    { mach.addcode (OpORG); }
    break;

  case 394:
#line 1206 "../parser.yxx"
    { badORG (mach, (yyvsp[(1) - (1)])); }
    break;

  case 395:
#line 1213 "../parser.yxx"
    { mach.addcode (OpPROC); }
    break;

  case 396:
#line 1219 "../parser.yxx"
    { mach.addcode (OpPUBLIC); }
    break;

  case 399:
#line 1230 "../parser.yxx"
    { badREPT (mach, (yyvsp[(1) - (1)])); }
    break;

  case 400:
#line 1235 "../parser.yxx"
    {
			mach.addcode (value_0);
			mach.addcode (value_1);
			mach.addcode (OpREPT);
			mach.addcodeliteral (std::string () );
		}
    break;

  case 402:
#line 1246 "../parser.yxx"
    {
			mach.addcode (OpREPT);
			mach.addcodeliteral ((yyvsp[(1) - (2)]).identifier () );
		}
    break;

  case 403:
#line 1250 "../parser.yxx"
    { badREPT_var (mach, (yyvsp[(1) - (1)])); }
    break;

  case 404:
#line 1255 "../parser.yxx"
    {
			mach.addcode (value_0);
			mach.addcode (value_1);
		}
    break;

  case 407:
#line 1264 "../parser.yxx"
    { badREPT_initial (mach, (yyvsp[(1) - (1)])); }
    break;

  case 408:
#line 1268 "../parser.yxx"
    { mach.addcode (value_1); }
    break;

  case 411:
#line 1274 "../parser.yxx"
    { badREPT_step (mach, (yyvsp[(1) - (1)])); }
    break;

  case 412:
#line 1287 "../parser.yxx"
    { mach.addcode (Op_8080); }
    break;

  case 413:
#line 1293 "../parser.yxx"
    { unimplemented (mach, (yyvsp[(1) - (1)])); }
    break;

  case 414:
#line 1299 "../parser.yxx"
    { mach.addcode (Op_DEPHASE); }
    break;

  case 416:
#line 1309 "../parser.yxx"
    {
			mach.addcode (Op_ERROR);
			mach.addcodeliteral ((yyvsp[(1) - (2)]).literal () );
		}
    break;

  case 417:
#line 1313 "../parser.yxx"
    { badErrorOrWarning (mach, (yyvsp[(1) - (1)]), (yyvsp[(0) - (1)])); }
    break;

  case 419:
#line 1323 "../parser.yxx"
    { mach.addcode (Op_PHASE); }
    break;

  case 420:
#line 1324 "../parser.yxx"
    { bad_PHASE (mach, (yyvsp[(1) - (1)])); }
    break;

  case 421:
#line 1331 "../parser.yxx"
    { mach.addcode (Op_SHIFT); }
    break;

  case 423:
#line 1341 "../parser.yxx"
    {
			mach.addcode (Op_WARNING);
			mach.addcodeliteral ((yyvsp[(1) - (2)]).literal () );
		}
    break;

  case 424:
#line 1345 "../parser.yxx"
    { badErrorOrWarning (mach, (yyvsp[(1) - (1)]), (yyvsp[(0) - (1)])); }
    break;

  case 425:
#line 1352 "../parser.yxx"
    { mach.addcode (Op_Z80); }
    break;

  case 426:
#line 1366 "../parser.yxx"
    {
			mach.addcode (static_cast <address> ((yyvsp[(1) - (2)]).type () ) );
			mach.addcode (OpNoargInst);
		}
    break;

  case 427:
#line 1376 "../parser.yxx"
    { mach.addcode (valtiADCA); }
    break;

  case 431:
#line 1386 "../parser.yxx"
    { mach.addcode (valtiADCA); }
    break;

  case 434:
#line 1388 "../parser.yxx"
    { badADC (mach, (yyvsp[(1) - (1)])); }
    break;

  case 435:
#line 1392 "../parser.yxx"
    { mach.addcode (valtiADCA); }
    break;

  case 438:
#line 1394 "../parser.yxx"
    { badADC (mach, (yyvsp[(1) - (1)])); }
    break;

  case 439:
#line 1398 "../parser.yxx"
    { mach.addcode (OpADC_HL); }
    break;

  case 440:
#line 1399 "../parser.yxx"
    { badADC_HL (mach, (yyvsp[(1) - (1)])); }
    break;

  case 443:
#line 1410 "../parser.yxx"
    { mach.addcode (valtiADDA); }
    break;

  case 446:
#line 1412 "../parser.yxx"
    { badADD (mach, (yyvsp[(1) - (1)])); }
    break;

  case 447:
#line 1416 "../parser.yxx"
    { mach.addcode (valtiADDA); }
    break;

  case 450:
#line 1418 "../parser.yxx"
    { badADD (mach, (yyvsp[(1) - (1)])); }
    break;

  case 451:
#line 1423 "../parser.yxx"
    { mach.addcode (OpADD_HL); }
    break;

  case 452:
#line 1425 "../parser.yxx"
    { mach.addcode (OpADD_IX); }
    break;

  case 453:
#line 1427 "../parser.yxx"
    { mach.addcode (OpADD_IY); }
    break;

  case 455:
#line 1432 "../parser.yxx"
    { badADD_HL (mach, (yyvsp[(1) - (1)])); }
    break;

  case 457:
#line 1437 "../parser.yxx"
    { mach.addcode (valregHL); }
    break;

  case 458:
#line 1438 "../parser.yxx"
    { badADD_IX (mach, (yyvsp[(1) - (1)])); }
    break;

  case 460:
#line 1443 "../parser.yxx"
    { mach.addcode (valregHL); }
    break;

  case 461:
#line 1444 "../parser.yxx"
    { badADD_IY (mach, (yyvsp[(1) - (1)])); }
    break;

  case 462:
#line 1451 "../parser.yxx"
    { mach.addcode (valtiADCA); }
    break;

  case 464:
#line 1457 "../parser.yxx"
    { mach.addcode (valtiADDA); }
    break;

  case 466:
#line 1463 "../parser.yxx"
    { mach.addcode (valtiADDA); }
    break;

  case 468:
#line 1469 "../parser.yxx"
    { mach.addcode (valtiAND); }
    break;

  case 470:
#line 1470 "../parser.yxx"
    { mach.addcode (valtiAND); }
    break;

  case 472:
#line 1476 "../parser.yxx"
    { mach.addcode (valtiAND); }
    break;

  case 474:
#line 1482 "../parser.yxx"
    { mach.addcode (valtiAND); }
    break;

  case 476:
#line 1488 "../parser.yxx"
    { mach.addcode (valcodeBIT); }
    break;

  case 478:
#line 1489 "../parser.yxx"
    { mach.addcode (valcodeBIT); }
    break;

  case 483:
#line 1501 "../parser.yxx"
    {
			checkugly (mach, (yyvsp[(1) - (1)]));
			mach.addcode (OpCALL);
		}
    break;

  case 484:
#line 1505 "../parser.yxx"
    { badCALL (mach, (yyvsp[(1) - (1)])); }
    break;

  case 486:
#line 1510 "../parser.yxx"
    { mach.addcode (OpCALL); }
    break;

  case 487:
#line 1511 "../parser.yxx"
    { badCALL (mach, (yyvsp[(1) - (1)])); }
    break;

  case 488:
#line 1516 "../parser.yxx"
    {
			checkugly (mach, (yyvsp[(1) - (1)]));
			mach.addcode (OpCALL_flag);
		}
    break;

  case 489:
#line 1520 "../parser.yxx"
    { badCALL_flag (mach, (yyvsp[(1) - (1)]), (yyvsp[(-2) - (1)])); }
    break;

  case 490:
#line 1524 "../parser.yxx"
    { mach.addcode (OpCALL_flag); }
    break;

  case 491:
#line 1525 "../parser.yxx"
    { badCALL_flag (mach, (yyvsp[(1) - (1)]), (yyvsp[(-2) - (1)])); }
    break;

  case 493:
#line 1535 "../parser.yxx"
    { mach.addcode (OpCALL); }
    break;

  case 494:
#line 1536 "../parser.yxx"
    { badCALL (mach, (yyvsp[(1) - (1)])); }
    break;

  case 495:
#line 1543 "../parser.yxx"
    { mach.addcode (valflagC); }
    break;

  case 497:
#line 1544 "../parser.yxx"
    { mach.addcode (valflagM); }
    break;

  case 499:
#line 1545 "../parser.yxx"
    { mach.addcode (valflagNC); }
    break;

  case 501:
#line 1546 "../parser.yxx"
    { mach.addcode (valflagNZ); }
    break;

  case 503:
#line 1547 "../parser.yxx"
    { mach.addcode (valflagP); }
    break;

  case 505:
#line 1548 "../parser.yxx"
    { mach.addcode (valflagPE); }
    break;

  case 507:
#line 1549 "../parser.yxx"
    { mach.addcode (valflagPO); }
    break;

  case 509:
#line 1550 "../parser.yxx"
    { mach.addcode (valflagZ); }
    break;

  case 511:
#line 1553 "../parser.yxx"
    { mach.addcode (OpCALL_flag); }
    break;

  case 512:
#line 1554 "../parser.yxx"
    { badCflag (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 513:
#line 1561 "../parser.yxx"
    { mach.addcode (valtiCP); }
    break;

  case 515:
#line 1567 "../parser.yxx"
    { mach.addcode (valtiCP); }
    break;

  case 517:
#line 1573 "../parser.yxx"
    { mach.addcode (valtiCP); }
    break;

  case 519:
#line 1574 "../parser.yxx"
    { mach.addcode (valtiCP); }
    break;

  case 521:
#line 1580 "../parser.yxx"
    { mach.addcode (OpADD_HL); }
    break;

  case 524:
#line 1584 "../parser.yxx"
    { badDAD (mach, (yyvsp[(1) - (1)])); }
    break;

  case 525:
#line 1591 "../parser.yxx"
    { mach.addcode (value_0); }
    break;

  case 527:
#line 1592 "../parser.yxx"
    { mach.addcode (value_0); }
    break;

  case 529:
#line 1598 "../parser.yxx"
    { mach.addcode (value_0); }
    break;

  case 531:
#line 1604 "../parser.yxx"
    { mach.addcode (value_0); }
    break;

  case 534:
#line 1613 "../parser.yxx"
    { mach.addcode (OpDJNZ); }
    break;

  case 535:
#line 1614 "../parser.yxx"
    { badDJNZarg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 540:
#line 1627 "../parser.yxx"
    { badEX_paren (mach, (yyvsp[(2) - (2)])); }
    break;

  case 541:
#line 1628 "../parser.yxx"
    { badEX (mach, (yyvsp[(1) - (1)])); }
    break;

  case 543:
#line 1633 "../parser.yxx"
    { badEX (mach, (yyvsp[(1) - (1)])); }
    break;

  case 547:
#line 1640 "../parser.yxx"
    { badEX_bracket (mach, (yyvsp[(2) - (2)])); }
    break;

  case 548:
#line 1646 "../parser.yxx"
    { mach.addcode (OpEX_AF_AFP); }
    break;

  case 549:
#line 1647 "../parser.yxx"
    { badEX_AF (mach, (yyvsp[(1) - (1)])); }
    break;

  case 550:
#line 1653 "../parser.yxx"
    { mach.addcode (OpEX_DE_HL); }
    break;

  case 551:
#line 1654 "../parser.yxx"
    { badEX_DE (mach, (yyvsp[(1) - (1)])); }
    break;

  case 553:
#line 1662 "../parser.yxx"
    { mach.addcode (OpEX_indSP_HL); }
    break;

  case 554:
#line 1663 "../parser.yxx"
    { mach.addcode (OpEX_indSP_IX); }
    break;

  case 555:
#line 1664 "../parser.yxx"
    { mach.addcode (OpEX_indSP_IY); }
    break;

  case 556:
#line 1665 "../parser.yxx"
    { badEX_indsp (mach, (yyvsp[(1) - (1)])); }
    break;

  case 558:
#line 1675 "../parser.yxx"
    { mach.addcode (OpIM); }
    break;

  case 559:
#line 1676 "../parser.yxx"
    { badIM (mach, (yyvsp[(1) - (1)])); }
    break;

  case 564:
#line 1689 "../parser.yxx"
    { badIN (mach, (yyvsp[(1) - (1)])); }
    break;

  case 567:
#line 1695 "../parser.yxx"
    { badIN (mach, (yyvsp[(1) - (1)])); }
    break;

  case 568:
#line 1698 "../parser.yxx"
    { (yyval)= (yyvsp[(1) - (2)]); }
    break;

  case 571:
#line 1705 "../parser.yxx"
    { badIN_r (mach, (yyvsp[(1) - (1)]), (yyvsp[(0) - (1)])); }
    break;

  case 573:
#line 1710 "../parser.yxx"
    { badbr_IN_r (mach, (yyvsp[(1) - (1)]), (yyvsp[(0) - (1)])); }
    break;

  case 576:
#line 1716 "../parser.yxx"
    { mach.addcode (OpIN_r_indC); }
    break;

  case 579:
#line 1723 "../parser.yxx"
    { badIN_A (mach, (yyvsp[(1) - (1)])); }
    break;

  case 581:
#line 1728 "../parser.yxx"
    { badbr_IN_A (mach, (yyvsp[(1) - (1)])); }
    break;

  case 584:
#line 1734 "../parser.yxx"
    { badIN_A_bracket (mach, (yyvsp[(1) - (1)])); }
    break;

  case 587:
#line 1740 "../parser.yxx"
    { badIN_A_paren (mach, (yyvsp[(1) - (1)])); }
    break;

  case 588:
#line 1743 "../parser.yxx"
    { mach.addcode (OpIN_A_indC_); }
    break;

  case 589:
#line 1744 "../parser.yxx"
    { mach.addcode (OpIN_A_indn); }
    break;

  case 590:
#line 1750 "../parser.yxx"
    { mach.addcode (value_1); }
    break;

  case 592:
#line 1751 "../parser.yxx"
    { mach.addcode (value_1); }
    break;

  case 595:
#line 1760 "../parser.yxx"
    { mach.addcode (OpIN_A_indn); }
    break;

  case 596:
#line 1761 "../parser.yxx"
    { badIN_8080 (mach, (yyvsp[(1) - (1)])); }
    break;

  case 597:
#line 1768 "../parser.yxx"
    { mach.addcode (value_1); }
    break;

  case 599:
#line 1774 "../parser.yxx"
    { mach.addcode (value_1); }
    break;

  case 602:
#line 1783 "../parser.yxx"
    { mach.addcode (OpJP); }
    break;

  case 603:
#line 1784 "../parser.yxx"
    { badJParg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 604:
#line 1791 "../parser.yxx"
    { mach.addcode (valflagC); }
    break;

  case 606:
#line 1792 "../parser.yxx"
    { mach.addcode (valflagNC); }
    break;

  case 608:
#line 1793 "../parser.yxx"
    { mach.addcode (valflagNZ); }
    break;

  case 610:
#line 1794 "../parser.yxx"
    { mach.addcode (valflagM); }
    break;

  case 612:
#line 1795 "../parser.yxx"
    { mach.addcode (valflagP); }
    break;

  case 614:
#line 1796 "../parser.yxx"
    { mach.addcode (valflagPE); }
    break;

  case 616:
#line 1797 "../parser.yxx"
    { mach.addcode (valflagPO); }
    break;

  case 618:
#line 1798 "../parser.yxx"
    { mach.addcode (valflagZ); }
    break;

  case 620:
#line 1800 "../parser.yxx"
    { mach.addcode (OpJP_flag); }
    break;

  case 621:
#line 1801 "../parser.yxx"
    { badCflag (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 626:
#line 1814 "../parser.yxx"
    { mach.addcode (OpJP_indHL); }
    break;

  case 627:
#line 1815 "../parser.yxx"
    { mach.addcode (OpJP_indIX); }
    break;

  case 628:
#line 1816 "../parser.yxx"
    { mach.addcode (OpJP_indIY); }
    break;

  case 629:
#line 1818 "../parser.yxx"
    {
			checkugly (mach, (yyvsp[(1) - (1)]));
			mach.addcode (OpJP);
		}
    break;

  case 630:
#line 1822 "../parser.yxx"
    { badJPparen (mach, (yyvsp[(2) - (2)])); }
    break;

  case 631:
#line 1823 "../parser.yxx"
    { badJParg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 634:
#line 1829 "../parser.yxx"
    { mach.addcode (OpJP); }
    break;

  case 635:
#line 1830 "../parser.yxx"
    { badbr_JParg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 636:
#line 1834 "../parser.yxx"
    { mach.addcode (OpJP_indHL); }
    break;

  case 637:
#line 1835 "../parser.yxx"
    { mach.addcode (OpJP_indIX); }
    break;

  case 638:
#line 1836 "../parser.yxx"
    { mach.addcode (OpJP_indIY); }
    break;

  case 639:
#line 1837 "../parser.yxx"
    { badJPbracket (mach, (yyvsp[(1) - (1)])); }
    break;

  case 642:
#line 1845 "../parser.yxx"
    {
			checkugly (mach, (yyvsp[(1) - (1)]));
			mach.addcode (OpJP_flag);
		}
    break;

  case 643:
#line 1849 "../parser.yxx"
    { badJPflag_arg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 644:
#line 1853 "../parser.yxx"
    { mach.addcode (OpJP_flag); }
    break;

  case 645:
#line 1854 "../parser.yxx"
    { badJPflag_arg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 648:
#line 1865 "../parser.yxx"
    { mach.addcode (OpJR); }
    break;

  case 649:
#line 1866 "../parser.yxx"
    { badJRarg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 650:
#line 1870 "../parser.yxx"
    { mach.addcode (OpJR_flag); }
    break;

  case 651:
#line 1871 "../parser.yxx"
    { badJRflag_arg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 662:
#line 1890 "../parser.yxx"
    { mach.addcode (OpLD_indBC); }
    break;

  case 663:
#line 1891 "../parser.yxx"
    { mach.addcode (OpLD_indDE); }
    break;

  case 666:
#line 1894 "../parser.yxx"
    { badLD (mach, (yyvsp[(1) - (1)])); }
    break;

  case 674:
#line 1905 "../parser.yxx"
    { badLD (mach, (yyvsp[(1) - (1)])); }
    break;

  case 675:
#line 1909 "../parser.yxx"
    { mach.addcode (OpLD_I_A); }
    break;

  case 677:
#line 1910 "../parser.yxx"
    { mach.addcode (OpLD_R_A); }
    break;

  case 680:
#line 1912 "../parser.yxx"
    { mach.addcode (OpLD_indBC); }
    break;

  case 681:
#line 1913 "../parser.yxx"
    { mach.addcode (OpLD_indDE); }
    break;

  case 685:
#line 1920 "../parser.yxx"
    { badLD_IorR (mach, (yyvsp[(1) - (1)]), (yyvsp[(-2) - (1)])); }
    break;

  case 686:
#line 1924 "../parser.yxx"
    { mach.addcode (OpLD_idesp_r); }
    break;

  case 687:
#line 1925 "../parser.yxx"
    { mach.addcode (OpLD_idesp_n); }
    break;

  case 694:
#line 1941 "../parser.yxx"
    { badLD_undocix (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 697:
#line 1947 "../parser.yxx"
    { badLD_undociy (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 699:
#line 1952 "../parser.yxx"
    { mach.addcode (OpLD_undoc_n); }
    break;

  case 700:
#line 1953 "../parser.yxx"
    { badLD_undocix (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 702:
#line 1958 "../parser.yxx"
    { mach.addcode (OpLD_undoc_n); }
    break;

  case 703:
#line 1959 "../parser.yxx"
    { badLD_undociy (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 704:
#line 1963 "../parser.yxx"
    { mach.addcode (OpLD_undoc_r); }
    break;

  case 705:
#line 1967 "../parser.yxx"
    { mach.addcode (OpLD_undoc_r); }
    break;

  case 706:
#line 1971 "../parser.yxx"
    {
			checkugly (mach, (yyvsp[(1) - (1)]));
			mach.addcode (OpLD_undoc_n);
		}
    break;

  case 708:
#line 1979 "../parser.yxx"
    { mach.addcode (OpLD_r_undoc); }
    break;

  case 709:
#line 1980 "../parser.yxx"
    { badLDr (mach, (yyvsp[(1) - (1)])); }
    break;

  case 711:
#line 1985 "../parser.yxx"
    { mach.addcode (OpLD_r_undoc); }
    break;

  case 712:
#line 1986 "../parser.yxx"
    { badLDr (mach, (yyvsp[(1) - (1)])); }
    break;

  case 713:
#line 1990 "../parser.yxx"
    { mach.addcode (OpLD_r_idesp); }
    break;

  case 714:
#line 1991 "../parser.yxx"
    { mach.addcode (OpLD_r_indHL); }
    break;

  case 715:
#line 1992 "../parser.yxx"
    { mach.unexpected ((yyvsp[(1) - (1)]), "after LD r,["); }
    break;

  case 717:
#line 1997 "../parser.yxx"
    { mach.errmsg ("Invalid register combination"); }
    break;

  case 719:
#line 2002 "../parser.yxx"
    { mach.errmsg ("Invalid register combination"); }
    break;

  case 721:
#line 2007 "../parser.yxx"
    { mach.addcode (OpLD_r_idesp); }
    break;

  case 722:
#line 2008 "../parser.yxx"
    { mach.addcode (OpLD_r_indHL); }
    break;

  case 723:
#line 2010 "../parser.yxx"
    {
			checkugly (mach, (yyvsp[(1) - (1)]));
			mach.addcode (OpLD_r_n);
		}
    break;

  case 725:
#line 2018 "../parser.yxx"
    { mach.addcode (OpLD_r_n); }
    break;

  case 727:
#line 2023 "../parser.yxx"
    { mach.addcode (OpLD_r_r); }
    break;

  case 729:
#line 2028 "../parser.yxx"
    { mach.addcode (OpLD_A_idesp); }
    break;

  case 730:
#line 2029 "../parser.yxx"
    { mach.addcode (OpLD_A_indBC); }
    break;

  case 731:
#line 2030 "../parser.yxx"
    { mach.addcode (OpLD_A_indDE); }
    break;

  case 732:
#line 2031 "../parser.yxx"
    { mach.addcode (OpLD_A_indHL); }
    break;

  case 733:
#line 2033 "../parser.yxx"
    {
			if (isparenexp ((yyvsp[(1) - (2)])) )
				mach.addcode (OpLD_A_indexp);
			else
			{
				mach.addcode (OpLD_A_n);
			}
		}
    break;

  case 734:
#line 2041 "../parser.yxx"
    { badLD_A (mach, (yyvsp[(1) - (1)])); }
    break;

  case 736:
#line 2046 "../parser.yxx"
    { mach.addcode (OpLD_A_n); }
    break;

  case 738:
#line 2051 "../parser.yxx"
    { mach.addcode (OpLD_A_r); }
    break;

  case 739:
#line 2052 "../parser.yxx"
    { mach.addcode (OpLD_A_undoc); }
    break;

  case 740:
#line 2053 "../parser.yxx"
    { mach.addcode (OpLD_A_idesp); }
    break;

  case 741:
#line 2054 "../parser.yxx"
    { mach.addcode (OpLD_A_indBC); }
    break;

  case 742:
#line 2055 "../parser.yxx"
    { mach.addcode (OpLD_A_indDE); }
    break;

  case 743:
#line 2056 "../parser.yxx"
    { mach.addcode (OpLD_A_indHL); }
    break;

  case 744:
#line 2058 "../parser.yxx"
    {
			mach.addcode (OpLD_A_indexp);
		}
    break;

  case 745:
#line 2061 "../parser.yxx"
    { badLD_A_bracket (mach, (yyvsp[(2) - (2)])); }
    break;

  case 746:
#line 2065 "../parser.yxx"
    { mach.addcode (OpLD_A_I); }
    break;

  case 747:
#line 2066 "../parser.yxx"
    { mach.addcode (OpLD_A_R); }
    break;

  case 748:
#line 2070 "../parser.yxx"
    { mach.addcode (OpLD_indHL_r); }
    break;

  case 749:
#line 2071 "../parser.yxx"
    { mach.addcode (OpLD_indHL_n); }
    break;

  case 758:
#line 2091 "../parser.yxx"
    {
			if (isparenexp ((yyvsp[(1) - (2)])) )
				mach.addcode (OpLD_HL_indexp);
			else
				mach.addcode (OpLD_HL_nn);
		}
    break;

  case 759:
#line 2097 "../parser.yxx"
    { mach.unexpected ((yyvsp[(1) - (1)]), "as second argument of LD rr"); }
    break;

  case 761:
#line 2103 "../parser.yxx"
    {
			if (isparenexp ((yyvsp[(1) - (2)])) )
				mach.addcode (OpLD_rr_indexp);
			else
				mach.addcode (OpLD_rr_nn);
		}
    break;

  case 762:
#line 2109 "../parser.yxx"
    { mach.unexpected ((yyvsp[(1) - (1)]), "as second argument of LD rr"); }
    break;

  case 764:
#line 2114 "../parser.yxx"
    { mach.addcode (OpLD_HL_nn); }
    break;

  case 766:
#line 2119 "../parser.yxx"
    { mach.addcode (OpLD_rr_nn); }
    break;

  case 767:
#line 2123 "../parser.yxx"
    { mach.addcode (OpLD_HL_indexp); }
    break;

  case 768:
#line 2124 "../parser.yxx"
    { mach.unexpected ((yyvsp[(1) - (1)]), "in 'LD rr, ['"); }
    break;

  case 769:
#line 2128 "../parser.yxx"
    { mach.addcode (OpLD_rr_indexp); }
    break;

  case 770:
#line 2129 "../parser.yxx"
    { mach.unexpected ((yyvsp[(1) - (1)]), "in 'LD rr, ['"); }
    break;

  case 772:
#line 2135 "../parser.yxx"
    {
			if (isparenexp ((yyvsp[(1) - (2)])) )
				mach.addcode (OpLD_IXY_indexp);
			else
				mach.addcode (OpLD_IXY_nn);
		}
    break;

  case 773:
#line 2141 "../parser.yxx"
    { mach.unexpected ((yyvsp[(1) - (1)]), "as second argument of LD IXY"); }
    break;

  case 775:
#line 2146 "../parser.yxx"
    { mach.addcode (OpLD_IXY_nn); }
    break;

  case 776:
#line 2150 "../parser.yxx"
    { mach.addcode (OpLD_IXY_indexp); }
    break;

  case 777:
#line 2151 "../parser.yxx"
    { mach.unexpected ((yyvsp[(1) - (1)]), "in 'LD IXY, ['"); }
    break;

  case 779:
#line 2157 "../parser.yxx"
    {
			if (isparenexp ((yyvsp[(1) - (2)])) )
				mach.addcode (OpLD_SP_indexp);
			else
				mach.addcode (OpLD_SP_nn);
		}
    break;

  case 781:
#line 2168 "../parser.yxx"
    {
			mach.addcode (OpLD_SP_nn);
		}
    break;

  case 782:
#line 2174 "../parser.yxx"
    { mach.addcode (OpLD_SP_HL); }
    break;

  case 783:
#line 2175 "../parser.yxx"
    { mach.addcode (OpLD_SP_IX); }
    break;

  case 784:
#line 2176 "../parser.yxx"
    { mach.addcode (OpLD_SP_IY); }
    break;

  case 785:
#line 2177 "../parser.yxx"
    { mach.addcode (OpLD_SP_indexp); }
    break;

  case 786:
#line 2181 "../parser.yxx"
    { mach.addcode (OpLD_indexp_A); }
    break;

  case 787:
#line 2182 "../parser.yxx"
    { mach.addcode (OpLD_indexp_BC); }
    break;

  case 788:
#line 2183 "../parser.yxx"
    { mach.addcode (OpLD_indexp_DE); }
    break;

  case 789:
#line 2184 "../parser.yxx"
    { mach.addcode (OpLD_indexp_HL); }
    break;

  case 790:
#line 2185 "../parser.yxx"
    { mach.addcode (OpLD_indexp_SP); }
    break;

  case 791:
#line 2186 "../parser.yxx"
    { mach.addcode (OpLD_indexp_IX); }
    break;

  case 792:
#line 2187 "../parser.yxx"
    { mach.addcode (OpLD_indexp_IY); }
    break;

  case 794:
#line 2197 "../parser.yxx"
    { mach.addcode (OpLD_A_indexp); }
    break;

  case 795:
#line 2198 "../parser.yxx"
    { badLDA (mach, (yyvsp[(1) - (1)])); }
    break;

  case 797:
#line 2208 "../parser.yxx"
    { mach.addcode (OpLD_A_indBC); }
    break;

  case 798:
#line 2209 "../parser.yxx"
    { mach.addcode (OpLD_A_indDE); }
    break;

  case 799:
#line 2210 "../parser.yxx"
    { badLDAX (mach, (yyvsp[(1) - (1)])); }
    break;

  case 801:
#line 2220 "../parser.yxx"
    { mach.addcode (OpLD_HL_indexp); }
    break;

  case 802:
#line 2221 "../parser.yxx"
    { badLHLD (mach, (yyvsp[(1) - (1)])); }
    break;

  case 806:
#line 2233 "../parser.yxx"
    { badLXI (mach, (yyvsp[(1) - (1)])); }
    break;

  case 807:
#line 2237 "../parser.yxx"
    { mach.addcode (OpLD_HL_nn); }
    break;

  case 808:
#line 2238 "../parser.yxx"
    { badLXI_rr (mach, (yyvsp[(1) - (1)])); }
    break;

  case 809:
#line 2242 "../parser.yxx"
    { mach.addcode (OpLD_rr_nn); }
    break;

  case 810:
#line 2243 "../parser.yxx"
    { badLXI_rr (mach, (yyvsp[(1) - (1)])); }
    break;

  case 814:
#line 2256 "../parser.yxx"
    { badMOV (mach, (yyvsp[(1) - (1)])); }
    break;

  case 815:
#line 2259 "../parser.yxx"
    { mach.addcode (OpLD_r_r); }
    break;

  case 816:
#line 2260 "../parser.yxx"
    { badMOV_r (mach, (yyvsp[(1) - (1)])); }
    break;

  case 817:
#line 2265 "../parser.yxx"
    { mach.addcode (OpLD_r_r); }
    break;

  case 818:
#line 2266 "../parser.yxx"
    { badMOV_M (mach, (yyvsp[(1) - (1)])); }
    break;

  case 821:
#line 2276 "../parser.yxx"
    { badLD (mach, (yyvsp[(1) - (1)])); }
    break;

  case 822:
#line 2279 "../parser.yxx"
    { mach.addcode (OpLD_r_n); }
    break;

  case 823:
#line 2280 "../parser.yxx"
    { badLDr (mach, (yyvsp[(1) - (1)])); }
    break;

  case 824:
#line 2287 "../parser.yxx"
    { mach.addcode (valtiOR); }
    break;

  case 826:
#line 2288 "../parser.yxx"
    { mach.addcode (valtiOR); }
    break;

  case 828:
#line 2294 "../parser.yxx"
    { mach.addcode (valtiOR); }
    break;

  case 830:
#line 2300 "../parser.yxx"
    { mach.addcode (valtiOR); }
    break;

  case 836:
#line 2312 "../parser.yxx"
    { badOUTarg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 838:
#line 2317 "../parser.yxx"
    { badbr_OUTarg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 841:
#line 2323 "../parser.yxx"
    { badOUTbracket (mach, (yyvsp[(1) - (1)])); }
    break;

  case 844:
#line 2329 "../parser.yxx"
    { badOUTparen (mach, (yyvsp[(1) - (1)])); }
    break;

  case 845:
#line 2333 "../parser.yxx"
    { mach.addcode (OpOUT_C_); }
    break;

  case 846:
#line 2334 "../parser.yxx"
    { badOUTc_arg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 847:
#line 2337 "../parser.yxx"
    { mach.addcode (OpOUT_n_); }
    break;

  case 849:
#line 2346 "../parser.yxx"
    { mach.addcode (OpOUT_n_); }
    break;

  case 850:
#line 2347 "../parser.yxx"
    { badOUTarg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 851:
#line 2354 "../parser.yxx"
    { mach.addcode (value_0); }
    break;

  case 853:
#line 2360 "../parser.yxx"
    { mach.addcode (value_0); }
    break;

  case 855:
#line 2366 "../parser.yxx"
    { mach.addcode (value_1); }
    break;

  case 857:
#line 2372 "../parser.yxx"
    { mach.addcode (value_1); }
    break;

  case 860:
#line 2381 "../parser.yxx"
    { mach.addcode (OpRET); }
    break;

  case 861:
#line 2382 "../parser.yxx"
    { mach.addcode (OpRET_flag); }
    break;

  case 862:
#line 2383 "../parser.yxx"
    { badRET (mach, (yyvsp[(1) - (1)])); }
    break;

  case 863:
#line 2390 "../parser.yxx"
    { mach.addcode (valcodeRES); }
    break;

  case 865:
#line 2391 "../parser.yxx"
    { mach.addcode (valcodeRES); }
    break;

  case 867:
#line 2397 "../parser.yxx"
    { mach.addcode (valcodeRL); }
    break;

  case 869:
#line 2398 "../parser.yxx"
    { mach.addcode (valcodeRL); }
    break;

  case 871:
#line 2404 "../parser.yxx"
    { mach.addcode (valcodeRLC); }
    break;

  case 873:
#line 2405 "../parser.yxx"
    { mach.addcode (valcodeRLC); }
    break;

  case 875:
#line 2411 "../parser.yxx"
    { mach.addcode (valcodeRR); }
    break;

  case 877:
#line 2412 "../parser.yxx"
    { mach.addcode (valcodeRR); }
    break;

  case 879:
#line 2418 "../parser.yxx"
    { mach.addcode (valcodeRRC); }
    break;

  case 881:
#line 2419 "../parser.yxx"
    { mach.addcode (valcodeRRC); }
    break;

  case 884:
#line 2428 "../parser.yxx"
    { mach.addcode (OpRST); }
    break;

  case 885:
#line 2429 "../parser.yxx"
    { badRSTarg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 887:
#line 2439 "../parser.yxx"
    { mach.addcode (OpRST); }
    break;

  case 888:
#line 2440 "../parser.yxx"
    { badRSTarg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 889:
#line 2447 "../parser.yxx"
    { mach.addcode (valtiSBCA); }
    break;

  case 891:
#line 2453 "../parser.yxx"
    { mach.addcode (valtiSBCA); }
    break;

  case 895:
#line 2463 "../parser.yxx"
    { mach.addcode (valtiSBCA); }
    break;

  case 898:
#line 2465 "../parser.yxx"
    { badSBCarg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 899:
#line 2469 "../parser.yxx"
    { mach.addcode (valtiSBCA); }
    break;

  case 902:
#line 2471 "../parser.yxx"
    { badSBCarg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 903:
#line 2475 "../parser.yxx"
    { mach.addcode (OpSBC_HL); }
    break;

  case 904:
#line 2476 "../parser.yxx"
    { badSBChl_arg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 905:
#line 2483 "../parser.yxx"
    { mach.addcode (valcodeSET); }
    break;

  case 907:
#line 2484 "../parser.yxx"
    { mach.addcode (valcodeSET); }
    break;

  case 910:
#line 2493 "../parser.yxx"
    { mach.addcode (OpLD_indexp_HL); }
    break;

  case 911:
#line 2494 "../parser.yxx"
    { badSHLD (mach, (yyvsp[(1) - (1)])); }
    break;

  case 912:
#line 2501 "../parser.yxx"
    { mach.addcode (valcodeSLA); }
    break;

  case 914:
#line 2502 "../parser.yxx"
    { mach.addcode (valcodeSLA); }
    break;

  case 916:
#line 2508 "../parser.yxx"
    { mach.addcode (valcodeSLL); }
    break;

  case 918:
#line 2509 "../parser.yxx"
    { mach.addcode (valcodeSLL); }
    break;

  case 920:
#line 2515 "../parser.yxx"
    { mach.addcode (valcodeSRA); }
    break;

  case 922:
#line 2516 "../parser.yxx"
    { mach.addcode (valcodeSRA); }
    break;

  case 924:
#line 2522 "../parser.yxx"
    { mach.addcode (valcodeSRL); }
    break;

  case 926:
#line 2523 "../parser.yxx"
    { mach.addcode (valcodeSRL); }
    break;

  case 929:
#line 2533 "../parser.yxx"
    {
			/*mach.addcode (valTypeA);*/
			mach.addcode (OpLD_indexp_A);
		}
    break;

  case 930:
#line 2537 "../parser.yxx"
    { badSTA (mach, (yyvsp[(1) - (1)])); }
    break;

  case 932:
#line 2547 "../parser.yxx"
    { mach.addcode (OpLD_indBC); }
    break;

  case 933:
#line 2548 "../parser.yxx"
    { mach.addcode (OpLD_indDE); }
    break;

  case 934:
#line 2549 "../parser.yxx"
    { mach.unexpected ((yyvsp[(1) - (1)]), "as STAX arg"); }
    break;

  case 935:
#line 2556 "../parser.yxx"
    { mach.addcode (valtiSUB); }
    break;

  case 937:
#line 2557 "../parser.yxx"
    { mach.addcode (valtiSUB); }
    break;

  case 939:
#line 2563 "../parser.yxx"
    { mach.addcode (valtiSUB); }
    break;

  case 941:
#line 2569 "../parser.yxx"
    { mach.addcode (valtiSUB); }
    break;

  case 943:
#line 2575 "../parser.yxx"
    { mach.addcode (valtiXOR); }
    break;

  case 945:
#line 2576 "../parser.yxx"
    { mach.addcode (valtiXOR); }
    break;

  case 947:
#line 2582 "../parser.yxx"
    { mach.addcode (valtiXOR); }
    break;

  case 949:
#line 2588 "../parser.yxx"
    { mach.addcode (valtiXOR); }
    break;

  case 951:
#line 2595 "../parser.yxx"
    { mach.addcode (OpEX_indSP_HL); }
    break;

  case 955:
#line 2617 "../parser.yxx"
    {
			mach.addcode (OpAddVarItem);
			mach.addcodeliteral ((yyvsp[(1) - (1)]).identifier () );
		}
    break;

  case 956:
#line 2621 "../parser.yxx"
    { badIdentifier (mach, (yyvsp[(1) - (1)])); }
    break;

  case 957:
#line 2629 "../parser.yxx"
    { mach.addcode (OpCP_r); }
    break;

  case 958:
#line 2630 "../parser.yxx"
    { badLikeADD_8080 (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 959:
#line 2638 "../parser.yxx"
    { mach.addcode (OpCP_n); }
    break;

  case 960:
#line 2639 "../parser.yxx"
    { badLikeADI (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 962:
#line 2649 "../parser.yxx"
    {
			checkugly (mach, (yyvsp[(1) - (2)]));
			mach.addcode (OpCP_n);
		}
    break;

  case 963:
#line 2653 "../parser.yxx"
    { badLikeADD_A_paren (mach, (yyvsp[(2) - (2)]), (yyvsp[(-3) - (2)])); }
    break;

  case 964:
#line 2654 "../parser.yxx"
    { badLikeADD_A_bracket (mach, (yyvsp[(2) - (2)]), (yyvsp[(-3) - (2)])); }
    break;

  case 965:
#line 2655 "../parser.yxx"
    { badLikeADD_A_arg (mach, (yyvsp[(1) - (1)]), (yyvsp[(-3) - (1)])); }
    break;

  case 967:
#line 2660 "../parser.yxx"
    { badLikeADD_A_bracket (mach, (yyvsp[(2) - (2)]), (yyvsp[(-3) - (2)])); }
    break;

  case 968:
#line 2661 "../parser.yxx"
    { badLikeADD_A_arg (mach, (yyvsp[(1) - (1)]), (yyvsp[(-3) - (1)])); }
    break;

  case 970:
#line 2671 "../parser.yxx"
    {
			checkugly (mach, (yyvsp[(1) - (2)]));
			mach.addcode (OpCP_n);
		}
    break;

  case 971:
#line 2675 "../parser.yxx"
    { badLikeCP_paren (mach, (yyvsp[(2) - (2)]), (yyvsp[(-1) - (2)])); }
    break;

  case 972:
#line 2676 "../parser.yxx"
    { badLikeCP_bracket (mach, (yyvsp[(2) - (2)]), (yyvsp[(-1) - (2)])); }
    break;

  case 973:
#line 2677 "../parser.yxx"
    { badLikeCP_arg (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 975:
#line 2682 "../parser.yxx"
    { badLikeCP_bracket (mach, (yyvsp[(2) - (2)]), (yyvsp[(-1) - (2)])); }
    break;

  case 976:
#line 2683 "../parser.yxx"
    { badLikeCP_arg (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 978:
#line 2692 "../parser.yxx"
    { mach.addcode (OpCP_indHL); }
    break;

  case 979:
#line 2693 "../parser.yxx"
    { mach.addcode (OpCP_idesp); }
    break;

  case 981:
#line 2698 "../parser.yxx"
    { mach.addcode (OpCP_n); }
    break;

  case 982:
#line 2702 "../parser.yxx"
    { mach.addcode (OpCP_r); }
    break;

  case 983:
#line 2703 "../parser.yxx"
    { mach.addcode (OpCP_undoc); }
    break;

  case 984:
#line 2704 "../parser.yxx"
    { mach.addcode (OpCP_indHL); }
    break;

  case 985:
#line 2705 "../parser.yxx"
    { mach.addcode (OpCP_idesp); }
    break;

  case 988:
#line 2715 "../parser.yxx"
    { badINCarg (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 990:
#line 2720 "../parser.yxx"
    { badINCarg (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 995:
#line 2732 "../parser.yxx"
    { mach.addcode (OpINC_r); }
    break;

  case 996:
#line 2737 "../parser.yxx"
    { mach.addcode (OpINC_rr); }
    break;

  case 997:
#line 2738 "../parser.yxx"
    { mach.addcode (OpINC_IX); }
    break;

  case 998:
#line 2739 "../parser.yxx"
    { mach.addcode (OpINC_IY); }
    break;

  case 999:
#line 2745 "../parser.yxx"
    { mach.addcode (OpINC_undoc); }
    break;

  case 1001:
#line 2753 "../parser.yxx"
    { mach.addcode (OpINC_indHL); }
    break;

  case 1002:
#line 2754 "../parser.yxx"
    { mach.addcode (OpINC_idesp); }
    break;

  case 1003:
#line 2755 "../parser.yxx"
    { badINCbracket (mach, (yyvsp[(1) - (1)]), (yyvsp[(-2) - (1)])); }
    break;

  case 1005:
#line 2763 "../parser.yxx"
    { mach.addcode (OpINC_indHL); }
    break;

  case 1006:
#line 2764 "../parser.yxx"
    { mach.addcode (OpINC_idesp); }
    break;

  case 1007:
#line 2765 "../parser.yxx"
    { badINCparen (mach, (yyvsp[(1) - (1)]), (yyvsp[(-2) - (1)])); }
    break;

  case 1008:
#line 2773 "../parser.yxx"
    { mach.addcode (OpINC_r); }
    break;

  case 1009:
#line 2774 "../parser.yxx"
    { badLikeINR (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 1010:
#line 2782 "../parser.yxx"
    { mach.addcode (OpINC_rr); }
    break;

  case 1011:
#line 2783 "../parser.yxx"
    { badLikeINX (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 1013:
#line 2792 "../parser.yxx"
    { badLikePUSH (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 1014:
#line 2796 "../parser.yxx"
    { mach.addcode (OpPUSH_rr); }
    break;

  case 1015:
#line 2797 "../parser.yxx"
    { mach.addcode (OpPUSH_IX); }
    break;

  case 1016:
#line 2798 "../parser.yxx"
    { mach.addcode (OpPUSH_IY); }
    break;

  case 1017:
#line 2806 "../parser.yxx"
    { mach.addcode (OpPUSH_rr); }
    break;

  case 1018:
#line 2807 "../parser.yxx"
    { badLikePUSH (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 1020:
#line 2816 "../parser.yxx"
    { badLikeRLparen   (mach, (yyvsp[(2) - (2)]), (yyvsp[(-1) - (2)])); }
    break;

  case 1021:
#line 2817 "../parser.yxx"
    { badLikeRLbracket (mach, (yyvsp[(2) - (2)]), (yyvsp[(-1) - (2)])); }
    break;

  case 1022:
#line 2818 "../parser.yxx"
    { badLikeRLarg     (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 1024:
#line 2823 "../parser.yxx"
    { badLikeRLbracket (mach, (yyvsp[(2) - (2)]), (yyvsp[(-1) - (2)])); }
    break;

  case 1025:
#line 2824 "../parser.yxx"
    { badLikeRLarg     (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 1027:
#line 2829 "../parser.yxx"
    { badLikeRLparen       (mach, (yyvsp[(2) - (2)]), (yyvsp[(-1) - (2)])); }
    break;

  case 1028:
#line 2830 "../parser.yxx"
    { badLikeRLbracket     (mach, (yyvsp[(2) - (2)]), (yyvsp[(-1) - (2)])); }
    break;

  case 1029:
#line 2831 "../parser.yxx"
    { badLikeRLarg_noundoc (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 1031:
#line 2836 "../parser.yxx"
    { badLikeRLbracket     (mach, (yyvsp[(2) - (2)]), (yyvsp[(-1) - (2)])); }
    break;

  case 1032:
#line 2837 "../parser.yxx"
    { badLikeRLarg_noundoc (mach, (yyvsp[(1) - (1)]), (yyvsp[(-1) - (1)])); }
    break;

  case 1033:
#line 2845 "../parser.yxx"
    { mach.addcode (OpEvalBitInst); }
    break;

  case 1034:
#line 2846 "../parser.yxx"
    { badLikeBITarg (mach, (yyvsp[(1) - (1)]), (yyvsp[(0) - (1)])); }
    break;

  case 1036:
#line 2851 "../parser.yxx"
    { badLikeBITn_paren   (mach, (yyvsp[(2) - (2)]), (yyvsp[(-2) - (2)])); }
    break;

  case 1037:
#line 2852 "../parser.yxx"
    { badLikeBITn_bracket (mach, (yyvsp[(2) - (2)]), (yyvsp[(-2) - (2)])); }
    break;

  case 1038:
#line 2853 "../parser.yxx"
    { badLikeBITn_arg     (mach, (yyvsp[(1) - (1)]), (yyvsp[(-2) - (1)])); }
    break;

  case 1040:
#line 2858 "../parser.yxx"
    { badLikeBITn_bracket (mach, (yyvsp[(2) - (2)]), (yyvsp[(-2) - (2)])); }
    break;

  case 1041:
#line 2859 "../parser.yxx"
    { badLikeBITn_arg     (mach, (yyvsp[(1) - (1)]), (yyvsp[(-2) - (1)])); }
    break;

  case 1043:
#line 2868 "../parser.yxx"
    { mach.addcode (OpRL_indhl); }
    break;

  case 1044:
#line 2869 "../parser.yxx"
    { mach.addcode (OpRL_idesp); }
    break;

  case 1046:
#line 2877 "../parser.yxx"
    { mach.addcode (OpRL_r); }
    break;

  case 1047:
#line 2878 "../parser.yxx"
    { mach.addcode (OpRL_indhl); }
    break;

  case 1048:
#line 2879 "../parser.yxx"
    { mach.addcode (OpRL_idesp); }
    break;

  case 1049:
#line 2880 "../parser.yxx"
    { mach.addcode (OpRL_undoc); }
    break;

  case 1051:
#line 2885 "../parser.yxx"
    { mach.addcode (OpRL_indhl); }
    break;

  case 1052:
#line 2886 "../parser.yxx"
    { mach.addcode (OpRL_idesp); }
    break;

  case 1054:
#line 2894 "../parser.yxx"
    { mach.addcode (OpRL_r); }
    break;

  case 1055:
#line 2895 "../parser.yxx"
    { mach.addcode (OpRL_indhl); }
    break;

  case 1056:
#line 2896 "../parser.yxx"
    { mach.addcode (OpRL_idesp); }
    break;

  case 1058:
#line 2907 "../parser.yxx"
    { mach.expected (Token (TypeEndLine), (yyvsp[(1) - (1)])); }
    break;

  case 1060:
#line 2912 "../parser.yxx"
    { mach.expected (Token (TypeComma), (yyvsp[(1) - (1)])); }
    break;

  case 1062:
#line 2917 "../parser.yxx"
    { badnextarg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 1064:
#line 2922 "../parser.yxx"
    { mach.expected (Token (TypeClose), (yyvsp[(1) - (1)])); }
    break;

  case 1066:
#line 2927 "../parser.yxx"
    { mach.expected (Token (TypeCloseBracket), (yyvsp[(1) - (1)])); }
    break;

  case 1068:
#line 2932 "../parser.yxx"
    { badexpr (mach, (yyvsp[(1) - (1)])); }
    break;

  case 1070:
#line 2937 "../parser.yxx"
    { badexpr (mach, (yyvsp[(1) - (1)])); }
    break;

  case 1072:
#line 2942 "../parser.yxx"
    { mach.expected (Token (TypeIdentifier), (yyvsp[(1) - (1)])); }
    break;

  case 1074:
#line 2947 "../parser.yxx"
    { mach.expected (Token (TypeA), (yyvsp[(1) - (1)])); }
    break;

  case 1076:
#line 2952 "../parser.yxx"
    { mach.expected (Token (TypeC), (yyvsp[(1) - (1)])); }
    break;

  case 1078:
#line 2957 "../parser.yxx"
    { mach.expected (Token (TypeLiteral), (yyvsp[(1) - (1)])); }
    break;

  case 1080:
#line 2962 "../parser.yxx"
    { mach.addcode (valflagPO); }
    break;

  case 1081:
#line 2963 "../parser.yxx"
    { mach.addcode (valflagPE); }
    break;

  case 1082:
#line 2964 "../parser.yxx"
    { mach.addcode (valflagP); }
    break;

  case 1083:
#line 2965 "../parser.yxx"
    { mach.addcode (valflagM); }
    break;

  case 1084:
#line 2969 "../parser.yxx"
    { mach.addcode (valflagNZ); }
    break;

  case 1085:
#line 2970 "../parser.yxx"
    { mach.addcode (valflagZ); }
    break;

  case 1086:
#line 2971 "../parser.yxx"
    { mach.addcode (valflagNC); }
    break;

  case 1087:
#line 2972 "../parser.yxx"
    { mach.addcode (valflagC); }
    break;

  case 1092:
#line 2986 "../parser.yxx"
    { mach.addcode (valreg_HL_); }
    break;

  case 1095:
#line 2995 "../parser.yxx"
    { mach.addcode (valregH); }
    break;

  case 1096:
#line 2996 "../parser.yxx"
    { mach.addcode (valregL); }
    break;

  case 1099:
#line 3004 "../parser.yxx"
    { mach.addcode (valregA); }
    break;

  case 1100:
#line 3007 "../parser.yxx"
    { mach.addcode (valregB); }
    break;

  case 1101:
#line 3008 "../parser.yxx"
    { mach.addcode (valregC); }
    break;

  case 1102:
#line 3009 "../parser.yxx"
    { mach.addcode (valregD); }
    break;

  case 1103:
#line 3010 "../parser.yxx"
    { mach.addcode (valregE); }
    break;

  case 1104:
#line 3014 "../parser.yxx"
    { addregIXH (mach); }
    break;

  case 1105:
#line 3015 "../parser.yxx"
    { addregIXL (mach); }
    break;

  case 1106:
#line 3019 "../parser.yxx"
    { addregIYH (mach); }
    break;

  case 1107:
#line 3020 "../parser.yxx"
    { addregIYL (mach); }
    break;

  case 1110:
#line 3029 "../parser.yxx"
    { mach.addcode (valregH); }
    break;

  case 1111:
#line 3030 "../parser.yxx"
    { mach.addcode (valregL); }
    break;

  case 1112:
#line 3034 "../parser.yxx"
    { mach.addcode (valregH); }
    break;

  case 1113:
#line 3035 "../parser.yxx"
    { mach.addcode (valregL); }
    break;

  case 1123:
#line 3060 "../parser.yxx"
    { mach.addcode (valregAF); }
    break;

  case 1124:
#line 3061 "../parser.yxx"
    { mach.addcode (valregAF); }
    break;

  case 1126:
#line 3066 "../parser.yxx"
    { mach.addcode (valregHL); }
    break;

  case 1129:
#line 3075 "../parser.yxx"
    { mach.addcode (valregBC); }
    break;

  case 1130:
#line 3076 "../parser.yxx"
    { mach.addcode (valregDE); }
    break;

  case 1134:
#line 3086 "../parser.yxx"
    { mach.addcode (valregAF); }
    break;

  case 1136:
#line 3091 "../parser.yxx"
    { mach.addcode (valregHL); }
    break;

  case 1137:
#line 3095 "../parser.yxx"
    { mach.addcode (valregBC); }
    break;

  case 1138:
#line 3096 "../parser.yxx"
    { mach.addcode (valregDE); }
    break;

  case 1139:
#line 3099 "../parser.yxx"
    { mach.addcode (valregSP); }
    break;

  case 1155:
#line 3120 "../parser.yxx"
    { mach.addcode (valprefixIX); }
    break;

  case 1156:
#line 3121 "../parser.yxx"
    { mach.addcode (valprefixIY); }
    break;

  case 1161:
#line 3141 "../parser.yxx"
    { mach.addcode (0); }
    break;

  case 1163:
#line 3143 "../parser.yxx"
    { mach.addcode (OpUnMinus); }
    break;

  case 1164:
#line 3144 "../parser.yxx"
    { mach.unexpected ((yyvsp[(1) - (1)]), "'+', '-' or ')' expected"); }
    break;

  case 1165:
#line 3148 "../parser.yxx"
    { mach.addcode (0); }
    break;

  case 1167:
#line 3150 "../parser.yxx"
    { mach.addcode (OpUnMinus); }
    break;

  case 1168:
#line 3151 "../parser.yxx"
    { mach.unexpected ((yyvsp[(1) - (1)]), "'+', '-' or ']' expected"); }
    break;

  case 1171:
#line 3164 "../parser.yxx"
    { (yyval)= value_0; }
    break;

  case 1173:
#line 3169 "../parser.yxx"
    { badcond (mach, (yyvsp[(1) - (1)])); }
    break;

  case 1175:
#line 3174 "../parser.yxx"
    { badcond2 (mach, (yyvsp[(1) - (1)])); }
    break;

  case 1176:
#line 3178 "../parser.yxx"
    { mach.addcode (OpConditional); }
    break;

  case 1177:
#line 3179 "../parser.yxx"
    { badcond3 (mach, (yyvsp[(1) - (1)])); }
    break;

  case 1178:
#line 3184 "../parser.yxx"
    { (yyval)= value_1; }
    break;

  case 1179:
#line 3186 "../parser.yxx"
    { (yyval)= value_0; }
    break;

  case 1180:
#line 3188 "../parser.yxx"
    { mach.addcode (OpMul); (yyval)= value_0; }
    break;

  case 1181:
#line 3190 "../parser.yxx"
    { mach.addcode (OpDiv); (yyval)= value_0; }
    break;

  case 1182:
#line 3192 "../parser.yxx"
    { mach.addcode (OpMod); (yyval)= value_0; }
    break;

  case 1183:
#line 3194 "../parser.yxx"
    { mach.addcode (OpShl); (yyval)= value_0; }
    break;

  case 1184:
#line 3196 "../parser.yxx"
    { mach.addcode (OpShr); (yyval)= value_0; }
    break;

  case 1185:
#line 3198 "../parser.yxx"
    { mach.addcode (OpAdd); (yyval)= value_0; }
    break;

  case 1186:
#line 3200 "../parser.yxx"
    { mach.addcode (OpSub); (yyval)= value_0; }
    break;

  case 1187:
#line 3202 "../parser.yxx"
    { mach.addcode (OpEqual); (yyval)= value_0; }
    break;

  case 1188:
#line 3204 "../parser.yxx"
    { mach.addcode (OpNotEqual); (yyval)= value_0; }
    break;

  case 1189:
#line 3206 "../parser.yxx"
    { mach.addcode (OpLessEqual); (yyval)= value_0; }
    break;

  case 1190:
#line 3208 "../parser.yxx"
    { mach.addcode (OpLessThan); (yyval)= value_0; }
    break;

  case 1191:
#line 3210 "../parser.yxx"
    { mach.addcode (OpGreaterThan); (yyval)= value_0; }
    break;

  case 1192:
#line 3212 "../parser.yxx"
    { mach.addcode (OpGreaterEqual); (yyval)= value_0; }
    break;

  case 1193:
#line 3214 "../parser.yxx"
    { mach.addcode (OpUnPlus); (yyval)= value_0; }
    break;

  case 1194:
#line 3216 "../parser.yxx"
    { mach.addcode (OpUnMinus); (yyval)= value_0; }
    break;

  case 1195:
#line 3218 "../parser.yxx"
    { mach.addcode (OpNot); (yyval)= value_0; }
    break;

  case 1196:
#line 3220 "../parser.yxx"
    { mach.addcode (OpBoolNot); (yyval)= value_0; }
    break;

  case 1197:
#line 3222 "../parser.yxx"
    { mach.addcode (OpAnd); (yyval)= value_0; }
    break;

  case 1198:
#line 3224 "../parser.yxx"
    { mach.addcode (OpOr); (yyval)= value_0; }
    break;

  case 1199:
#line 3226 "../parser.yxx"
    { mach.addcode (OpXor); (yyval)= value_0; }
    break;

  case 1200:
#line 3228 "../parser.yxx"
    { mach.addcode (OpBoolAnd); (yyval)= value_0; }
    break;

  case 1201:
#line 3230 "../parser.yxx"
    { mach.addcode (OpBoolOr); (yyval)= value_0; }
    break;

  case 1202:
#line 3232 "../parser.yxx"
    { mach.addcode (OpHigh); (yyval)= value_0; }
    break;

  case 1203:
#line 3234 "../parser.yxx"
    { mach.addcode (OpLow); (yyval)= value_0; }
    break;

  case 1204:
#line 3238 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1205:
#line 3239 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1206:
#line 3240 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1207:
#line 3241 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1208:
#line 3242 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1209:
#line 3243 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1210:
#line 3244 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1211:
#line 3245 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1212:
#line 3246 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1213:
#line 3247 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1214:
#line 3248 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1215:
#line 3249 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1216:
#line 3250 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1217:
#line 3251 "../parser.yxx"
    { badexpr (mach, (yyvsp[(2) - (2)])); }
    break;

  case 1218:
#line 3252 "../parser.yxx"
    { badexpr (mach, (yyvsp[(2) - (2)])); }
    break;

  case 1219:
#line 3253 "../parser.yxx"
    { badexpr (mach, (yyvsp[(2) - (2)])); }
    break;

  case 1220:
#line 3254 "../parser.yxx"
    { badexpr (mach, (yyvsp[(2) - (2)])); }
    break;

  case 1221:
#line 3255 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1222:
#line 3256 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1223:
#line 3257 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1224:
#line 3258 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1225:
#line 3259 "../parser.yxx"
    { badexpr (mach, (yyvsp[(3) - (3)])); }
    break;

  case 1226:
#line 3260 "../parser.yxx"
    { badexpr (mach, (yyvsp[(2) - (2)])); }
    break;

  case 1227:
#line 3261 "../parser.yxx"
    { badexpr (mach, (yyvsp[(2) - (2)])); }
    break;

  case 1229:
#line 3266 "../parser.yxx"
    { badexpr (mach, (yyvsp[(2) - (2)])); }
    break;

  case 1230:
#line 3270 "../parser.yxx"
    { mach.addcode ((yyvsp[(1) - (1)]).num () ); }
    break;

  case 1231:
#line 3271 "../parser.yxx"
    { mach.addcode ((yyvsp[(1) - (1)]).identifier () ); }
    break;

  case 1232:
#line 3272 "../parser.yxx"
    { mach.addcodeliteral ((yyvsp[(1) - (1)]).literal () ); }
    break;

  case 1233:
#line 3273 "../parser.yxx"
    { mach.addcode (OpDollar); }
    break;

  case 1236:
#line 3280 "../parser.yxx"
    {
			mach.addcode ((yyvsp[(1) - (1)]).identifier () );
			mach.addcode (OpDefined);
		}
    break;

  case 1237:
#line 3284 "../parser.yxx"
    { badDEFINEDarg (mach, (yyvsp[(1) - (1)])); }
    break;

  case 1238:
#line 3288 "../parser.yxx"
    { mach.addcode (addrTRUE); }
    break;

  case 1239:
#line 3289 "../parser.yxx"
    { mach.addcode (addrFALSE); yyclearin; }
    break;

  case 1242:
#line 3294 "../parser.yxx"
    { yyclearin; }
    break;


/* Line 1267 of yacc.c.  */
#line 8921 "parser.cxx"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (mach, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (mach, yymsg);
	  }
	else
	  {
	    yyerror (mach, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, mach);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, mach);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (mach, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, mach);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, mach);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 3490 "../parser.yxx"


} /* namespace impl */
} /* namespace pasmo */

/* End of parser.yxx */

