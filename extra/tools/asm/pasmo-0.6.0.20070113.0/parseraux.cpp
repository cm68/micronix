// parseraux.cpp
// Revision 23-sep-2005

#include "parseraux.h"

#include <string>

namespace pasmo {
namespace impl {

namespace parser {

const string
	as ("as '"),
	arg ("' argument, "),
	arg2 ("' second argument, "),
	argparen ("' argument after '(', "),
	arg2paren ("' second argument after '(', "),
	argbracket ("' argument after '[', "),
	arg2bracket ("' second argument after '[', "),
	expected (" expected"),

	identifier ("identifier"),
	expression ("expression"),
	expression_or_nothing ("expression or end of line"),
	A_or_HL ("A or HL"),
	A_HL_IX_IY ("A, HL, IX or IY"),
	indCorexp ("(C), [C], (n) or [n]"),
	indCorexp_par ("(C) or (n)"),
	indCorexp_br ("[C] or [n]"),
	flag_or_expression ("flag or expression"),
	HL_IX_IY ("HL, IX or IY"),
	indHL_IX_IY_br ("[HL], [IX] or [IY]"),
	indHL_Ides_par ("(HL), (IX+des) or (IY+des)"),
	indHL_Ides_br ("[HL], [IX+des] or [IY+des]"),
	BC_DE_SP_IX ("BC, DE, SP or IX"),
	BC_DE_SP_IY ("BC, DE, SP or IY"),
	BC_DE_HL_SP ("BC, DE, HL or SP"),
	indBC_DE_HL_IX_IY_exp_br
		("[BC], [DE], [HL], [IX+des], [IY+des] or [nn]"),
	regdouble8080 ("B, D, H or SP"),
	regsimple ("A, B, C, D, E, H or L"),
	regsimple8080 ("A, B, C, D, E, H, L or M"),
	argsLikeINC ("A, B, C, D, E, H, L, BC, DE, HL, SP, IX, IY, "
		"[HL], [IX+des], or [IY+des]" ),
	argsLikeBIT ("A, B, C, D, E, H, L, [HL], [IX+des] or [IY+des]"),
	argsLikeCP ("A, B, C, D, E, H, L, IXH, IXL, IYH, IYL, n, "
		"[HL], [IX+des] or [IY+des]"),
	argsLikeRL ("A, B, C, D, E, H, L, IXH, IXL, IYH, IYL, "
		"[HL], [IX+des] or [IY+des]"),
	argsLD ("A, B, C, D, E, H, L, I, R, BC, DE, HL, SP, IX, IY, "
		"[nn], [BC], [DE], [HL], [IX+des] or [IY+des]"),
	argsLD_A ("[BC], [DE], [HL], [IX+des], [IY+des], [nn], "
		"A, B, C, D, E, H, L, I, R, or n"),
	& argsLDr (argsLikeBIT),
	argsLikePOP ("AF, BC, DE, HL, IX or IY");

string inst_expec (const string & inst, const string & expec)
{ return as + inst + arg + expec + expected; }

string inst_expec_par (const string & inst, const string & expec)
{ return as + inst + argparen + expec + expected; }

string inst_expec_br (const string & inst, const string & expec)
{ return as + inst + argbracket + expec + expected; }

string inst_expec2 (const string & inst, const string & expec)
{ return as + inst + arg2 + expec + expected; }

string inst_expec2_par (const string & inst, const string & expec)
{ return as + inst + arg2paren + expec + expected; }

string inst_expec2_br (const string & inst, const string & expec)
{ return as + inst + arg2bracket + expec + expected; }

void badarg (Machine & mach, const Token & tok,
	const string & inst, const string & expec)
{ mach.unexpected (tok, inst_expec (inst, expec) ); }

void badarg_par (Machine & mach, const Token & tok,
	const string & inst, const string & expec)
{ mach.unexpected (tok, inst_expec_par (inst, expec) ); }

void badarg_br (Machine & mach, const Token & tok,
	const string & inst, const string & expec)
{ mach.unexpected (tok, inst_expec_br (inst, expec) ); }

void badarg2 (Machine & mach, const Token & tok,
	const string & inst, const string & expec)
{ mach.unexpected (tok, inst_expec2 (inst, expec) ); }

void badarg2_par (Machine & mach, const Token & tok,
	const string & inst, const string & expec)
{ mach.unexpected (tok, inst_expec2_par (inst, expec) ); }

void badarg2_br (Machine & mach, const Token & tok,
	const string & inst, const string & expec)
{ mach.unexpected (tok, inst_expec2_br (inst, expec) ); }

void badarg (Machine & mach, const Token & tok,
	const Token & inst, const string & expec)
{ mach.unexpected (tok, inst_expec (inst.str (), expec) ); }

void badarg_par (Machine & mach, const Token & tok,
	const Token & inst, const string & expec)
{ mach.unexpected (tok, inst_expec_par (inst.str (), expec) ); }

void badarg_br (Machine & mach, const Token & tok,
	const Token & inst, const string & expec)
{ mach.unexpected (tok, inst_expec_br (inst.str (), expec) ); }

} // namespace parser

void parser::badexpr (Machine & mach, const Token & tok)
{
	mach.unexpected (tok, "in expression");
}

void parser::badcond (Machine & mach, const Token & tok)
{
	mach.unexpected (tok, "after '?'");
}

void parser::badcond2 (Machine & mach, const Token & tok)
{
	mach.unexpected (tok, "in '?' argument (maybe ':' is missing)");
}

void parser::badcond3 (Machine & mach, const Token & tok)
{
	mach.unexpected (tok, "after ':'");
}

void parser::badnextarg (Machine & mach, const Token & tok)
{
	mach.unexpected (tok, "after argument, ',' or end of line expected");
}

void parser::unimplemented (Machine & mach, const Token & tok)
{
	mach.errmsg (tok.str () + " unimplemented, reserved for future use");
}

void parser::badinstruction (Machine & mach, const Token & tok)
{
	mach.unexpected (tok, "when instruction was expected");
}

void parser::badIdentifier (Machine & mach, const Token & tok)
{
	mach.unexpected (tok, "inside an identifier list");
}

void parser::badDEFB (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "DEFB", "literal or expression");
}

void parser::badDEFL (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "DEFL", expression);
}

void parser::badDEFS (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "DEFS", expression);
}

void parser::badDEFS_value (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "DEFS", expression);
}

void parser::badDEFW (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "DEFW", expression);
}

void parser::badEND (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "END", expression_or_nothing);
}

void parser::badEQU (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "EQU", expression);
}

void parser::badIF (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "IF", expression);
}

void parser::badIRP (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "IRP", expression);
}

void parser::badIRPC (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "IRPC", "IRPC argument");
}

void parser::badMACROitem (Machine & mach, const Token & tok)
{
	mach.unexpected (tok, "cannot be used as macro parameter");
}

void parser::badORG (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "ORG", expression);
}

void parser::badREPT (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "REPT count", expression);
}

void parser::badREPT_var (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "REPT variable", identifier);
}

void parser::badREPT_initial (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "REPT initial value", expression);
}

void parser::badREPT_step (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "REPT step", expression);
}

void parser::bad_PHASE (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "_PHASE", expression);
}

void parser::badErrorOrWarning (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg (mach, tok, inst, "literal");
}

void parser::badADC (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "ADC", A_or_HL);
}

void parser::badADC_HL (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "ADC HL", BC_DE_HL_SP);
}

void parser::badADD (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "ADD", A_HL_IX_IY);
}

void parser::badADD_HL (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "ADD HL", BC_DE_HL_SP);
}

void parser::badADD_IX (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "ADD IX", BC_DE_SP_IX);
}

void parser::badADD_IY (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "ADD IX", BC_DE_SP_IY);
}

void parser::badLikeADD_A_arg (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg2 (mach, tok, inst.str () + " A", argsLikeCP);
}

void parser::badLikeADD_A_bracket (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg2_br (mach, tok, inst.str () + " A", indHL_Ides_br);
}

void parser::badLikeADD_A_paren (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg2_par (mach, tok, inst.str () + " A", indHL_Ides_par);
}

void parser::badLikeADD_8080 (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg (mach, tok, inst, regsimple8080);
}

void parser::badLikeADI (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg (mach, tok, inst, expression);
}

void parser::badLikeCP_arg (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg (mach, tok, inst, argsLikeCP);
}

void parser::badLikeCP_bracket (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg_br (mach, tok, inst, indHL_Ides_br);
}

void parser::badLikeCP_paren (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg_par (mach, tok, inst.str () + " (", indHL_Ides_par);
}

void parser::badLikeBITarg (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg (mach, tok, inst, expression);
}

void parser::badLikeBITn_arg (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg2 (mach, tok, inst.str () + " n", argsLikeBIT);
}

void parser::badLikeBITn_paren (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg2_par (mach, tok, inst.str () + " n", indHL_Ides_par);
}

void parser::badLikeBITn_bracket (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg2_br (mach, tok, inst.str () + " n", indHL_Ides_br);
}

void parser::badCALL (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "CALL", flag_or_expression);
}

void parser::badCALL_flag (Machine & mach,
	const Token & tok, const Token & flag)
{
	badarg2 (mach, tok, "CALL " + flag.str (), expression);
}

void parser::badCflag (Machine & mach, const Token & tok, const Token & inst)
{
	badarg (mach, tok, inst, expression);
}

void parser::badDAD (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "DAD", regdouble8080);
}

void parser::badDJNZarg (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "DJNZ", expression);
}

void parser::badEX (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "EX", "DE, HL or [SP]");
}

void parser::badEX_paren (Machine & mach, const Token & tok)
{
	badarg_par (mach, tok, "EX", "(SP)");
}

void parser::badEX_bracket (Machine & mach, const Token & tok)
{
	badarg_br (mach, tok, "EX", "[SP]");
}

void parser::badEX_AF (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "EX AF", "AF'");
}

void parser::badEX_DE (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "EX DE", "HL");
}

void parser::badEX_indsp (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "EX [SP],", HL_IX_IY);
}

void parser::badIM (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "IM", expression);
}

void parser::badIN (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "IN", regsimple );
}

void parser::badIN_r (Machine & mach, const Token & tok, const Token & reg)
{
	badarg2 (mach, tok, "IN " + reg.str (), "(C) or [C]" );
}

void parser::badbr_IN_r (Machine & mach, const Token & tok, const Token & reg)
{
	badarg2 (mach, tok, "IN " + reg.str (), "[C]" );
}

void parser::badIN_A(Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "IN A", indCorexp);
}

void parser::badbr_IN_A (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "IN A", indCorexp_br);
}

void parser::badIN_A_paren (Machine & mach, const Token & tok)
{
	badarg2_par (mach, tok, "IN A", indCorexp_par);
}

void parser::badIN_A_bracket (Machine & mach, const Token & tok)
{
	badarg2_br (mach, tok, "IN A", indCorexp_br);
}

void parser::badIN_8080 (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "IN", expression);
}

void parser::badINCarg (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg (mach, tok, inst, argsLikeINC);
}

void parser::badbr_INCarg (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg (mach, tok, inst, argsLikeINC);
}

void parser::badINCbracket (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg_br (mach, tok, inst, indHL_Ides_br);
}

void parser::badINCparen (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg_par (mach, tok, inst, indHL_Ides_par);
}

void parser::badLikeINR (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg (mach, tok, inst, regsimple8080);
}

void parser::badLikeINX (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg (mach, tok, inst, regdouble8080);
}

void parser::badJParg (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "JP", "flag, '[', '(' or expression");
}

void parser::badbr_JParg (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "JP", "flag, '[' or expression");
}

void parser::badJPbracket (Machine & mach, const Token & tok)
{
	badarg_br (mach, tok, "JP", indHL_IX_IY_br);
}

void parser::badJPparen (Machine & mach, const Token & tok)
{
	badarg_par (mach, tok, "JP",
		"(HL), (IX), (IY) or expression expected");
}

void parser::badJPflag_arg (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "JP flag", expression);
}

void parser::badJRarg (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "JR", "NZ, Z, NC, C or expression");
}

void parser::badJRflag_arg (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "JR flag,", expression);
}

void parser::badLD (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "LD", argsLD);
}

void parser::badLDr (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "LD r", argsLDr);
}

void parser::badLD_IorR (Machine & mach, const Token & tok, const Token & reg)
{
	badarg (mach, tok, "LD " + reg.str (), "A");
}

void parser::badLD_A (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "LD A", argsLD_A);
}

void parser::badLD_undocix (Machine & mach, const Token & tok, const Token & reg)
{
	badarg2 (mach, tok, "LD " + reg.str (),
		"A, B, C, D, E, IXH, IXL or expression");
}

void parser::badLD_undociy (Machine & mach, const Token & tok, const Token & reg)
{
	badarg2 (mach, tok, "LD " + reg.str (),
		"A, B, C, D, E, IYH, IYL or expression");
}

void parser::badLD_A_bracket (Machine & mach, const Token & tok)
{
	badarg_br (mach, tok, "LD A", indBC_DE_HL_IX_IY_exp_br);
}

void parser::badLDA (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "LDA", expression);
}

void parser::badLDAX (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "LDAX", "B or D");
}

void parser::badLHLD (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "LHLD", expression);
}

void parser::badLXI (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "LXI", regdouble8080);
}

void parser::badLXI_rr (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "LXI", expression);
}

void parser::badMOV (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "MOV", regsimple8080);
}

void parser::badMOV_r (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "MOV", regsimple8080);
}

void parser::badMOV_M (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "MOV", regsimple);
}

void parser::badOUTarg (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "OUT", "'[' or '('");
}

void parser::badbr_OUTarg (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "OUT", "'['");
}

void parser::badOUTbracket (Machine & mach, const Token & tok)
{
	badarg_br (mach, tok, "OUT", indCorexp_br);
}

void parser::badOUTparen (Machine & mach, const Token & tok)
{
	badarg_par (mach, tok, "OUT", indCorexp_par);
}

void parser::badOUTc_arg (Machine & mach, const Token & tok)
{
	badarg2 (mach, tok, "OUT [C]", regsimple);
}

void parser::badLikePUSH (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg (mach, tok, inst, argsLikePOP);
}

void parser::badRET (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "RET", "flag or end of line");
}

void parser::badLikeRLarg (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg (mach, tok, inst, argsLikeRL);
}

void parser::badLikeRLarg_noundoc (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg (mach, tok, inst, argsLikeBIT);
}

void parser::badLikeRLparen (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg_par (mach, tok, inst, HL_IX_IY);
}

void parser::badLikeRLbracket (Machine & mach,
	const Token & tok, const Token & inst)
{
	badarg_br (mach, tok, inst, HL_IX_IY);
}

void parser::badRSTarg (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "RST", expression);
}

void parser::badSBCarg (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "SBC", A_or_HL);
}

void parser::badSBChl_arg (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "SBC HL", BC_DE_HL_SP);
}

void parser::badSHLD (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "SHLD", expression);
}

void parser::badSTA (Machine & mach, const Token & tok)
{
	badarg (mach, tok, "STA", expression);
}

void parser::badDEFINEDarg (Machine & mach, const Token & tok)
{
	mach.unexpected (tok, "after DEFINED, identifier expected");
}

} // namespace impl
} // namespace pasmo

// End of parseraux.cpp
