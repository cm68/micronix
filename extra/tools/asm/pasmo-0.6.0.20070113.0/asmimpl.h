#ifndef INCLUDE_ASMIMPL_H
#define INCLUDE_ASMIMPL_H

// asmimpl.h
// Revision 11-oct-2006

#include "asm.h"

#include "token.h"
#include "value.h"

#include <vector>
#include <stdexcept>


namespace pasmo {
namespace impl {


class pasmo_fatal : public std::logic_error {
public:
	pasmo_fatal (const std::string & s);
};

extern pasmo_fatal InvalidPassValue;
extern pasmo_fatal ShiftOutsideMacro;


using std::ostream;
using std::vector;


typedef vector <Token> MacroArg;
typedef vector <MacroArg> MacroArgList;

typedef vector <string> VarnameList;

enum GenCodeMode { gen80, gen86 };


class VarData;


class AsmImpl : public Asm {
public:
	AsmImpl ();

	// This is not a copy constructor, it creates a new
	// instance copying the options and the AsmFile.
	explicit AsmImpl (const AsmImpl & in);

	~AsmImpl ();

	static Asm * create (const AsmOptions & options_n);

	virtual bool getnocase () const= 0;
	virtual int currentpass () const= 0;
	//virtual address getcurrentinstruction () const= 0;
	virtual Value getcurrentinstruction () const= 0;
	virtual string genlocalname (const string & varname)= 0;
	virtual VarData getvar (const string & varname)= 0;

	// Object file generation.

	virtual size_t getcodesize () const= 0;
	virtual void message_emit (const string & type)= 0;
	virtual void writebincode (ostream & out)= 0;
	virtual void writetzxcode (ostream & out)= 0;
	virtual void writecdtcode (ostream & out)= 0;
	virtual string cpcbasicloader ()= 0;
	virtual string spectrumbasicloader ()= 0;

	virtual void warningUglyInstruction ()= 0;
	virtual void parse_error (const string & errmsg)= 0;

	virtual void doEmpty ()= 0;
	virtual void doLabel (const string & name)= 0;
	virtual void doExpandMacro (const string & name,
		const MacroArgList & params)= 0;
	virtual void doASEG ()= 0;
	virtual void doCSEG ()= 0;
	virtual void doDEFBliteral (const string & s)= 0;

	virtual void doDEFBnum (byte b)= 0;
	//virtual void doDEFBnum (const VarData & vd)= 0;

	virtual void doDEFBend ()= 0;
	virtual void doDEFL (const string & label, address value)= 0;
	virtual void doDEFS (address count, byte value)= 0;

	//virtual void doDEFWnum (address num)= 0;
	virtual void doDEFWnum (const VarData & num)= 0;

	virtual void doDEFWend ()= 0;
	virtual void doDSEG ()= 0;
	virtual void doELSE ()= 0;

	//virtual void doEND (address end, bool hasentry)= 0;
	virtual void doEND ()= 0;
	virtual void doEND (const VarData & vd)= 0;

	virtual void doENDIF ()= 0;
	virtual void doENDM ()= 0;
	virtual void doENDP ()= 0;
	virtual void doEQU (const string & label, const VarData & vdata)= 0;
	virtual void doEXITM ()= 0;
	virtual void doEXTRN (const VarnameList & varnamelist)= 0;
	virtual void doIF (address v)= 0;
	virtual void doIF1 ()= 0;
	virtual void doIF2 ()= 0;
	virtual void doIFDEF (const string & varname)= 0;
	virtual void doIFNDEF (const string & varname)= 0;
	virtual void doINCBIN (const string & includefile)= 0;
	virtual void doINCLUDE ()= 0;
	virtual void doEndOfINCLUDE ()= 0;
	virtual void doIRP (const string & varname,
		const MacroArgList & params)= 0;
	virtual void doIRPC (const string & varname,
		const string & charlist)= 0;
	virtual void doLOCAL (const VarnameList & varnamelist)= 0;
	virtual void doMACRO (const string & name,
		const vector <string> & param)= 0;
	virtual void doPROC ()= 0;
	virtual void doORG (address neworg)= 0;
	virtual void doPUBLIC (const VarnameList & varnamelist)= 0;
	virtual void doREPT (address counter, const string & varcounter,
		address valuecounter, address step)= 0;

	virtual void do_8080 ()= 0;
	virtual void do_DEPHASE ()= 0;
	virtual void do_ERROR (const string & msg)= 0;
	virtual void do_PHASE (address value)= 0;
	virtual void do_SHIFT ()= 0;
	virtual void do_WARNING (const string & msg)= 0;
	virtual void do_Z80 ()= 0;

	virtual void doByteInst (TypeByteInst ti, regbCode reg,
		byte prefix= prefixNone, bool hasdesp= false, byte desp= 0)= 0;
	virtual void doByteInmediate (TypeByteInst ti, byte bvalue)= 0;
	virtual void doByteInstCB (byte codereg, regbCode reg,
		byte prefix= prefixNone, bool hasdesp= false, byte desp= 0)= 0;

	virtual void doNoargInst (TypeToken tt)= 0;
	virtual void doADDADCSBC_HL (byte basecode,
		regwCode reg, byte prefix)= 0;

	virtual void doDJNZ (const VarData & vd)= 0;

	virtual void doCALL (const VarData & addr)= 0;

	//virtual void doCALL_flag (flagCode fcode, address addr)= 0;
	virtual void doCALL_flag (flagCode fcode, const VarData & vd)= 0;

	virtual void doEX_indSP_HL ()= 0;
	virtual void doEX_indSP_IX ()= 0;
	virtual void doEX_indSP_IY ()= 0;
	virtual void doEX_AF_AFP ()= 0;
	virtual void doEX_DE_HL ()= 0;
	virtual void doIN_A_indC ()= 0;
	virtual void doIN_A_indn (byte n)= 0;
	virtual void doINr_c_ (regbCode reg)= 0;
	virtual void doIM (address v)= 0;

	//virtual void doJP (address addr)= 0;
	virtual void doJP (const VarData & addr)= 0;

	virtual void doJP_indHL ()= 0;
	virtual void doJP_indIX ()= 0;
	virtual void doJP_indIY ()= 0;

	//virtual void doJP_flag (flagCode fcode, address addr)= 0;
	virtual void doJP_flag (flagCode fcode, const VarData & addr)= 0;

	virtual void doRelative (byte code, address addr,
		const string instrname)= 0;
	virtual void doJR (const VarData & vd)= 0;
	virtual void doJR_flag (flagCode fcode, const VarData & vd)= 0;
	virtual void doLDir (byte type)= 0;
	virtual void doLD_r_r (regbCode reg1, regbCode reg2)= 0;
	virtual void doLD_r_n (regbCode reg, byte n)= 0;
	virtual void doLD_r_undoc (regbCode reg1, regbCode reg2,
		byte prefix)= 0;
	virtual void doLD_r_idesp (regbCode reg1, byte prefix, byte desp)= 0;
	virtual void doLD_undoc_r (regbCode reg1, byte prefix,
		regbCode reg2)= 0;
	virtual void doLD_undoc_n (regbCode reg, byte prefix, byte n)= 0;
	virtual void doLD_idesp_r (byte prefix, byte desp, regbCode reg2)= 0;

	virtual void doLD_A_indexp (const VarData & vd)= 0;
	virtual void doLD_A_indBC ()= 0;
	virtual void doLD_A_indDE ()= 0;
	virtual void doLD_indBC_A ()= 0;
	virtual void doLD_indDE_A ()= 0;
	virtual void doLD_indexp_A (const VarData & vd)= 0;
	virtual void doLD_indexp_BC (const VarData & vd)= 0;
	virtual void doLD_indexp_DE (const VarData & vd)= 0;
	virtual void doLD_indexp_HL (const VarData & vd)= 0;
	virtual void doLD_indexp_SP (const VarData & vd)= 0;
	virtual void doLD_indexp_IX (const VarData & vd)= 0;
	virtual void doLD_indexp_IY (const VarData & vd)= 0;
	virtual void doLD_idesp_n (byte prefix, byte desp, byte n)= 0;
	virtual void doPUSHPOP (regwCode reg, byte prefix, bool isPUSH)= 0;
	virtual void doLD_SP_HL ()= 0;
	virtual void doLD_SP_IX ()= 0;
	virtual void doLD_SP_IY ()= 0;
	virtual void doLD_SP_nn (const VarData & value)= 0;
	virtual void doLD_SP_indexp (const VarData & value)= 0;
	virtual void doLD_HL_nn (const VarData & value)= 0;
	virtual void doLD_HL_indexp (const VarData & vd)= 0;

	virtual void doLD_rr_nn (regwCode regcode,
		const VarData & addr)= 0;
	virtual void doLD_rr_indexp (regwCode regcode,
		const VarData & addr)= 0;
	virtual void doLD_IXY_nn (byte prefix,
		const VarData & addr)= 0;
	virtual void doLD_IXY_indexp (byte prefix,
		const VarData & addr)= 0;

	virtual void doINC_r (bool isINC, byte prefix, regbCode reg)= 0;
	virtual void doINC_IX (bool isINC, address adesp)= 0;
	virtual void doINC_IY (bool isINC, address adesp)= 0;
	virtual void doINC_rr (bool isINC, regwCode reg, byte prefix)= 0;
	virtual void doOUT_C_ (regbCode rcode)= 0;
	virtual void doOUT_n_ (byte b)= 0;
	virtual void doRET ()= 0;
	virtual void doRETflag (flagCode fcode)= 0;
	virtual void doRST (address addr)= 0;

	virtual bool ismacro (const string & name) const= 0;

private:
	void operator = (const AsmImpl &); // Forbidden.
};

} // namespace impl
} // namespace pasmo


#endif

// End of asmimpl.h
