#ifndef INCLUDE_PARSERTYPES_H
#define INCLUDE_PARSERTYPES_H

// parsertypes.h
// Revision 17-nov-2005


#include "pasmotypes.h"
#include "machine.h"
#include "token.h"


namespace pasmo {
namespace impl {

enum ParseType {
	PassedFirst,
	BracketOnly
};

enum Oper {
	OpNumber,
	OpIdentifier,
	OpLiteral,

	OpAdd,
	OpAnd,
	OpBoolAnd,
	OpBoolNot,
	OpBoolOr,
	OpConditional,
	OpDiv,
	OpDefined,
	OpDollar,
	OpEqual,
	OpGreaterEqual,
	OpGreaterThan,
	OpHigh,
	OpLessEqual,
	OpLessThan,
	OpLow,
	OpMod,
	OpMul,
	OpNot,
	OpNotEqual,
	OpOr,
	OpShl,
	OpShr,
	OpSub,
	OpUnMinus,
	OpUnPlus,
	OpXor,

	OpStop,
	OpEmpty,
	OpGenLabel,
	OpAddVarItem,
	OpExpandMacro,
	OpMacroItem,
	OpMacroArg,
	OpMacroValue,

	OpASEG,
	OpCSEG,
	OpDEFBliteral,
	OpDEFBnum,
	OpDEFBend,
	OpDEFL,
	OpDEFS,
	OpDEFSvalue,
	OpDEFWnum,
	OpDEFWend,
	OpDSEG,
	OpELSE,
	OpEND,
	OpENDn,
	OpENDIF,
	OpENDM,
	OpENDP,
	OpEQU,
	OpEXITM,
	OpEXTRN,
	OpIF,
	OpIF1,
	OpIF2,
	OpIFDEF,
	OpIFNDEF,
	OpINCBIN,
	OpINCLUDE,
	OpEndINCLUDE,
	OpIRP,
	OpIRPC,
	OpLOCAL,
	OpMACRO,
	OpMACROend,
	OpORG,
	OpPROC,
	OpPUBLIC,
	OpREPT,

	Op_8080,
	Op_DEPHASE,
	Op_ERROR,
	Op_PHASE,
	Op_SHIFT,
	Op_WARNING,
	Op_Z80,

	OpEvalBitInst,

	OpNoargInst,
	OpADC_HL,
	OpADD_HL,
	OpADD_IX,
	OpADD_IY,
	OpCALL,
	OpCALL_flag,
	OpCP_r,
	OpCP_undoc,
	OpCP_indHL,
	OpCP_idesp,
	OpCP_n,
	OpDEC,
	OpDJNZ,
	OpEX_indSP_HL,
	OpEX_indSP_IX,
	OpEX_indSP_IY,
	OpEX_AF_AFP,
	OpEX_DE_HL,
	OpIM,
	OpIN_A_indC_,
	OpIN_A_indn,
	OpIN_r_indC,
	OpINC,
	OpINC_r,
	OpINC_undoc,
	OpINC_indHL,
	OpINC_idesp,
	OpINC_rr,
	OpINC_IX,
	OpINC_IY,
	OpJP,
	OpJP_flag,
	OpJP_indHL,
	OpJP_indIX,
	OpJP_indIY,
	OpJR,
	OpJR_flag,
	OpLD_A_I,
	OpLD_A_R,
	OpLD_I_A,
	OpLD_R_A,
	OpLD_r_r,
	OpLD_r_undoc,
	OpLD_r_indHL,
	OpLD_r_idesp,
	OpLD_r_n,
	OpLD_undoc_r,
	OpLD_undoc_n,
	OpLD_A_r,
	OpLD_A_undoc,
	OpLD_A_n,
	OpLD_A_indexp,
	OpLD_A_indBC,
	OpLD_A_indDE,
	OpLD_A_indHL,
	OpLD_A_idesp,
	OpLD_indHL_r,
	OpLD_indHL_n,
	OpLD_indBC,
	OpLD_indDE,
	OpLD_indexp_A,
	OpLD_indexp_BC,
	OpLD_indexp_DE,
	OpLD_indexp_HL,
	OpLD_indexp_SP,
	OpLD_indexp_IX,
	OpLD_indexp_IY,
	OpLD_idesp_r,
	OpLD_idesp_n,
	OpLD_SP_HL,
	OpLD_SP_IX,
	OpLD_SP_IY,
	OpLD_SP_nn,
	OpLD_SP_indexp,
	OpLD_HL_nn,
	OpLD_HL_indexp,
	OpLD_IXY_nn,
	OpLD_IXY_indexp,
	OpLD_rr_nn,
	OpLD_rr_indexp,
	OpOUT_C_,
	OpOUT_n_,
	OpPUSH_rr,
	OpPUSH_IX,
	OpPUSH_IY,
	OpRET,
	OpRET_flag,
	OpRL_r,
	OpRL_undoc,
	OpRL_indhl,
	OpRL_idesp,
	OpRST,
	OpSBC_HL
};

class Machine : public ExecMachine {
public:
	virtual void syntax (const char * s)= 0;
	virtual void ugly ()= 0;
	virtual Token gettoken ()= 0;
	virtual void expectmacro ()= 0;

	virtual void addcode (Oper op)= 0;
	virtual void addcode (address value)= 0;
	virtual void addcode (const string & ident)= 0;
	virtual void addcodeliteral (const string & lit)= 0;
	virtual void addlabel (const string & labelname)= 0;
	virtual void addmacroname (const Token & tok)= 0;
	virtual void cancelmacroname ()= 0;
	virtual void redefmacro ()= 0;

	virtual void macroargs ()= 0;
	virtual void nomacroargs ()= 0;
	virtual void addmacroitem (const Token & tok)= 0;

	virtual void errmsg (const string & errmsg)= 0;
	virtual void invalid ()= 0;
	virtual void expected
		(const Token & tokexp, const Token & tokfound)= 0;
	virtual void unexpected
		(const Token & tokfound, const string & str)= 0;

	virtual void exec ()= 0;
};

class AsmImpl;

Machine * newMachine (AsmImpl & asmin_n, Tokenizer & tz_n,
	ParseType pt_n, bool inpass2_n);

#define YYSTYPE Token

int yylex (YYSTYPE * lvalp, Machine & machine);
void yyerror (Machine & machine, const char *);
int yyparse (Machine & machine);


} // namespace impl
} // namespace pasmo


#endif

// End of parsertypes.h
