#ifndef INCLUDE_PARSERAUX_H
#define INCLUDE_PARSERAUX_H

// parseraux.h
// Revision 23-sep-2005

#include "parsertypes.h"

namespace pasmo {
namespace impl {

namespace parser {

void badexpr (Machine & mach, const Token & tok);
void badcond (Machine & mach, const Token & tok);
void badcond2 (Machine & mach, const Token & tok);
void badcond3 (Machine & mach, const Token & tok);
void badnextarg (Machine & mach, const Token & tok);

void unimplemented (Machine & mach, const Token & tok);
void badinstruction (Machine & mach, const Token & tok);
void badIdentifier (Machine & mach, const Token & tok);

void badDEFB (Machine & mach, const Token & tok);
void badDEFL (Machine & mach, const Token & tok);
void badDEFS (Machine & mach, const Token & tok);
void badDEFS_value (Machine & mach, const Token & tok);
void badDEFW (Machine & mach, const Token & tok);
void badEND (Machine & mach, const Token & tok);
void badEQU (Machine & mach, const Token & tok);
void badIF (Machine & mach, const Token & tok);
void badIRP (Machine & mach, const Token & tok);
void badIRPC (Machine & mach, const Token & tok);
void badMACROitem (Machine & mach, const Token & tok);
void badORG (Machine & mach, const Token & tok);
void badREPT (Machine & mach, const Token & tok);
void badREPT_var (Machine & mach, const Token & tok);
void badREPT_initial (Machine & mach, const Token & tok);
void badREPT_step (Machine & mach, const Token & tok);
void bad_PHASE (Machine & mach, const Token & tok);

void badErrorOrWarning (Machine & mach,
	const Token & tok, const Token & inst);

void badADC (Machine & mach, const Token & tok);
void badADC_HL (Machine & mach, const Token & tok);
void badADD (Machine & mach, const Token & tok);
void badADD_HL (Machine & mach, const Token & tok);
void badADD_IX (Machine & mach, const Token & tok);
void badADD_IY (Machine & mach, const Token & tok);
void badLikeADD_A_arg (Machine & mach,
	const Token & tok, const Token & inst);
void badLikeADD_A_bracket (Machine & mach,
	const Token & tok, const Token & inst);
void badLikeADD_A_paren (Machine & mach,
	const Token & tok, const Token & inst);
void badLikeADD_8080 (Machine & mach,
	const Token & tok, const Token & inst);
void badLikeADI (Machine & mach,
	const Token & tok, const Token & inst);
void badLikeCP_arg (Machine & mach,
	const Token & tok, const Token & inst);
void badLikeCP_bracket (Machine & mach,
	const Token & tok, const Token & inst);
void badLikeCP_paren (Machine & mach,
	const Token & tok, const Token & inst);
void badLikeBITarg (Machine & mach,
	const Token & tok, const Token & inst);
void badLikeBITn_arg (Machine & mach,
	const Token & tok, const Token & inst);
void badLikeBITn_paren (Machine & mach,
	const Token & tok, const Token & inst);
void badLikeBITn_bracket (Machine & mach,
	const Token & tok, const Token & inst);
void badCALL (Machine & mach, const Token & tok);
void badCALL_flag (Machine & mach, const Token & tok, const Token & flag);
void badCflag (Machine & mach, const Token & tok, const Token & inst);
void badDAD (Machine & mach, const Token & tok);
void badDJNZarg (Machine & mach, const Token & tok);
void badEX (Machine & mach, const Token & tok);
void badEX_paren (Machine & mach, const Token & tok);
void badEX_bracket (Machine & mach, const Token & tok);
void badEX_AF (Machine & mach, const Token & tok);
void badEX_DE (Machine & mach, const Token & tok);
void badEX_indsp (Machine & mach, const Token & tok);
void badIM (Machine & mach, const Token & tok);
void badIN (Machine & mach, const Token & tok);
void badIN_r (Machine & mach, const Token & tok, const Token & reg);
void badbr_IN_r (Machine & mach, const Token & tok, const Token & reg);
void badIN_A (Machine & mach, const Token & tok);
void badbr_IN_A (Machine & mach, const Token & tok);
void badIN_A_paren (Machine & mach, const Token & tok);
void badIN_A_bracket (Machine & mach, const Token & tok);
void badIN_8080 (Machine & mach, const Token & tok);
void badINCarg (Machine & mach, const Token & tok, const Token & inst);
void badbr_INCarg (Machine & mach, const Token & tok, const Token & inst);
void badINCbracket (Machine & mach, const Token & tok, const Token & inst);
void badINCparen (Machine & mach, const Token & tok, const Token & inst);
void badLikeINR (Machine & mach, const Token & tok, const Token & inst);
void badLikeINX (Machine & mach, const Token & tok, const Token & inst);
void badJParg (Machine & mach, const Token & tok);
void badbr_JParg (Machine & mach, const Token & tok);
void badJPbracket (Machine & mach, const Token & tok);
void badJPparen (Machine & mach, const Token & tok);
void badJPflag_arg (Machine & mach, const Token & tok);
void badJRarg (Machine & mach, const Token & tok);
void badJRflag_arg (Machine & mach, const Token & tok);
void badLD (Machine & mach, const Token & tok);
void badLDr (Machine & mach, const Token & tok);
void badLD_IorR (Machine & mach, const Token & tok, const Token & reg);
void badLD_A (Machine & mach, const Token & tok);
void badLD_A_bracket (Machine & mach, const Token & tok);
void badLD_undocix (Machine & mach, const Token & tok, const Token & reg);
void badLD_undociy (Machine & mach, const Token & tok, const Token & reg);
void badLDA (Machine & mach, const Token & tok);
void badLDAX (Machine & mach, const Token & tok);
void badLHLD (Machine & mach, const Token & tok);
void badLXI (Machine & mach, const Token & tok);
void badLXI_rr (Machine & mach, const Token & tok);
void badMOV (Machine & mach, const Token & tok);
void badMOV_r (Machine & mach, const Token & tok);
void badMOV_M (Machine & mach, const Token & tok);
void badLikePUSH (Machine & mach, const Token & tok, const Token & inst);
void badOUTarg (Machine & mach, const Token & tok);
void badbr_OUTarg (Machine & mach, const Token & tok);
void badOUTbracket (Machine & mach, const Token & tok);
void badOUTparen (Machine & mach, const Token & tok);
void badOUTc_arg (Machine & mach, const Token & tok);
void badRET (Machine & mach, const Token & tok);
void badLikeRLarg (Machine & mach, const Token & tok, const Token & inst);
void badLikeRLarg_noundoc (Machine & mach, const Token & tok, const Token & inst);
void badLikeRLparen (Machine & mach, const Token & tok, const Token & inst);
void badLikeRLbracket (Machine & mach, const Token & tok, const Token & inst);
void badRSTarg (Machine & mach, const Token & tok);
void badSBCarg (Machine & mach, const Token & tok);
void badSBChl_arg (Machine & mach, const Token & tok);
void badSHLD (Machine & mach, const Token & tok);
void badSTA (Machine & mach, const Token & tok);

void badDEFINEDarg (Machine & mach, const Token & tok);

} /* namespace parser */

} /* namespace impl */
} /* namespace pasmo */

#endif

// End of parseraux.h
