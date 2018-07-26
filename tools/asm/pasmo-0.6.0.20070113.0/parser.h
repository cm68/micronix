/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

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




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif


