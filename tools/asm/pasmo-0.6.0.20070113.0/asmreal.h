#ifndef INCLUDE_ASMREAL_H
#define INCLUDE_ASMREAL_H

// asmreal.h
// Revision 10-jan-2007

#include "asmimpl.h"
#include "asmfile.h"
#include "var.h"
#include "macro.h"
#include "macroframe.h"
#include "module.h"
#include "local.h"


#include <iostream>


namespace pasmo {

namespace impl {


class AsmReal : public AsmImpl, public AsmFile,
	public Vars, public MacroStore
{
public:
	AsmReal (const AsmOptions & options_n);

	// This is not a copy constructor, it creates a new
	// instance copying the options and the AsmFile.
	explicit AsmReal (const AsmReal & in);

	~AsmReal ();

	static Asm * create (const AsmOptions & options_n);

	ValueType getinitialsegment ();

	void setbase (unsigned int addr);
	void addincludedir (const string & dirname);
	void addpredef (const string & predef);

	void setfilelisting (ostream & out_n);

	AsmMode getasmmode () const;
	bool getnocase () const;

	void loadfile (const string & filename);

	void link_modules (vector <Module *> & vpmod);
	void loadmodules (vector <Module> & mod);
	void link ();
	void processfile ();

	int currentpass () const;

	//address getcurrentinstruction () const;
	Value getcurrentinstruction () const;

	// Variable access for local classes.

	VarData getvar (const string & varname);
	VarData rawgetvar (const string & varname);

	string genlocalname (const string & varname);

	// Object file generation.

	address getminused () const;
	address getmaxused () const;
	size_t getcodesize () const;
	void message_emit (const string & type);
	void writebincode (ostream & out);

	void emitobject (ostream & out);
	void emitdump (std::ostream & out);
	void emitplus3dos (ostream & out);
	void emittap (ostream & out);

	void writetzxcode (ostream & out);
	void emittzx (ostream & out);

	void writecdtcode (ostream & out);
	void emitcdt (ostream & out);

	string cpcbasicloader ();
	void emitcdtbas (ostream & out);

	string spectrumbasicloader ();

	void emittapbas (ostream & out);
	void emittzxbas (ostream & out);

	void emithex (ostream & out);
	void emitamsdos (ostream & out);

	void emitprl (ostream & out);
	void emitrel (ostream & out);
	void emitcmd (ostream & out);
	void emitcom (ostream & out);

	void emitmsx (ostream & out);

	void emitcode (ostream & out);

	void dumppublic (ostream & out);
	void dumpsymbol (ostream & out);
private:
	void operator = (const AsmReal &); // Forbidden.

	static std::streambuf * pnullbuf ();

	void setentrypoint (address addr);

	void checkendline (const Token & tok);
	void checkendline (Tokenizer & tz);

	//address currentpos () const;
	Value currentpos () const;

	void clearphase ();
	//address phased (address addr) const;
	//address phasedpos () const;
	Value phased (Value addr) const;
	Value phasedpos () const;

	void genbyte (byte abyte);
	void genword (address dataword);

	void gencode (byte code);
	void gencode (const VarData & vd);
	void gencode (byte code1, byte code2);
	void gencode (byte code1, byte code2, byte code3);
	void gencode (byte code1, byte code2, byte code3, byte code4);
	void gencodeED (byte code);

	void gencodeword (address value);
	void gencodeword (const Value & v);
	void gencodeword (const VarData & vd);

	void showcode (const string & instruction);
	void showdebnocodeline (const Tokenizer & tz);
public:
	void warningUglyInstruction ();
	void parse_error (const string & errmsg);

	void doEmpty ();
	void doLabel (const string & varname);
	void doExpandMacro (const string & name,
		const MacroArgList & params);

	void doASEG ();
	void doCSEG ();
	void doDEFBliteral (const string & s);

	void doDEFBnum (byte b);
	//void doDEFBnum (const VarData & vd);

	void doDEFBend ();
	void doDEFL (const string & label, address value);
	void doDEFS (address count, byte value);

	//void doDEFWnum (address num);
	void doDEFWnum (const VarData & num);

	void doDEFWend ();
	void doDSEG ();
	void doELSE ();

	//void doEND (address end, bool hasentry);
	void doEND ();
	void doEND (const VarData & vd);

	void doENDIF ();
	void doENDM ();
	void doENDP ();
	void doEQU (const string & label, const VarData & vdata);
	void doEXITM ();
	void doEXTRN (const VarnameList & varnamelist);
	void doIF (address v);
	void doIF1 ();
	void doIF2 ();
	void doIFDEF (const string & varname);
	void doIFNDEF (const string & varname);
	void doINCBIN (const string & includefile);
	void doINCLUDE ();
	void doEndOfINCLUDE ();
	void doIRP (const string & varname, const MacroArgList & params);
	void doIRPC (const string & varname, const string & charlist);
	void doLOCAL (const VarnameList & varnamelist);
	void doMACRO (const string & name, const vector <string> & param);
	void doPROC ();
	void doORG (address neworg);
	void doPUBLIC (const VarnameList & varnamelist);
	void doREPT (address counter, const string & varcounter,
		address valuecounter, address step);

	void do_8080 ();
	void do_DEPHASE ();
	void do_ERROR (const string & msg);
	void do_PHASE (address value);
	void do_SHIFT ();
	void do_WARNING (const string & msg);
	void do_Z80 ();

	void doByteInst (TypeByteInst ti, regbCode reg,
		byte prefix= prefixNone, bool hasdesp= false, byte desp= 0);
	void doByteInmediate (TypeByteInst ti, byte bvalue);
	void doByteInstCB (byte codereg, regbCode reg,
		byte prefix= prefixNone, bool hasdesp= false, byte desp= 0);

	void doNoargInst (TypeToken tt);
	void doADDADCSBC_HL (byte basecode, regwCode reg, byte prefix);
	void doDJNZ (const VarData & vd);
	//void doCALL (address addr);
	void doCALL (const VarData & addr);
	//void doCALL_flag (flagCode fcode, address addr);
	void doCALL_flag (flagCode fcode, const VarData & vd);

	void doEX_indSP_HL ();
	void doEX_indSP_IX ();
	void doEX_indSP_IY ();
	void doEX_AF_AFP ();
	void doEX_DE_HL ();
	void doIN_A_indC ();
	void doIN_A_indn (byte n);
	void doINr_c_ (regbCode reg);
	void doIM (address v);
	//void doJP (address addr);
	void doJP (const VarData & addr);
	void doJP_indHL ();
	void doJP_indIX ();
	void doJP_indIY ();
	//void doJP_flag (flagCode fcode, address addr);
	void doJP_flag (flagCode fcode, const VarData & addr);
	void doRelative (byte code, address addr, const string instrname);
	void doRelative (byte code, const VarData & vd,
		const string instrname);
	void doJR (const VarData & vd);
	void doJR_flag (flagCode fcode, const VarData & vd);
	void doLDir (byte type);
	void doLD_r_r (regbCode reg1, regbCode reg2);
	void doLD_r_n (regbCode reg, byte n);
	void doLD_r_undoc (regbCode reg1, regbCode reg2, byte prefix);
	void doLD_r_idesp (regbCode reg1, byte prefix, byte desp);
	void doLD_undoc_r (regbCode reg1, byte prefix, regbCode reg2);
	void doLD_undoc_n (regbCode reg, byte prefix, byte n);
	void doLD_idesp_r (byte prefix, byte desp, regbCode reg2);
	void doLD_A_indexp (const VarData & vd);
	void doLD_A_indBC ();
	void doLD_A_indDE ();
	void doLD_indBC_A ();
	void doLD_indDE_A ();
	void doLD_indexp_A (const VarData & vd);
	void doLD_indexp_BC (const VarData & vd);
	void doLD_indexp_DE (const VarData & vd);
	void doLD_indexp_HL (const VarData & vd);
	void doLD_indexp_SP (const VarData & vd);
	void doLD_indexp_IX (const VarData & vd);
	void doLD_indexp_IY (const VarData & vd);
	void doLD_idesp_n (byte prefix, byte desp, byte n);
	void doPUSHPOP (regwCode reg, byte prefix, bool isPUSH);
	void doLD_SP_HL ();
	void doLD_SP_IX ();
	void doLD_SP_IY ();
	void doLD_SP_nn (const VarData & value);
	void doLD_SP_indexp (const VarData & value);
	void doLD_HL_nn (const VarData & value);
	void doLD_HL_indexp (const VarData & vd);

	//void doLD_rr_nn (regwCode regcode, byte prefix, address value);
	void doLD_rr_nn (regwCode regcode,
		const VarData & addr);
	//void doLD_rr_indexp (regwCode regcode, byte prefix, address value);
	void doLD_rr_indexp (regwCode regcode,
		const VarData & addr);
	void doLD_IXY_nn (byte prefix, const VarData & addr);
	void doLD_IXY_indexp (byte prefix, const VarData & addr);

	void doINC_r (bool isINC, byte prefix, regbCode reg);
	void doINC_IX (bool isINC, address adesp);
	void doINC_IY (bool isINC, address adesp);
	void doINC_rr (bool isINC, regwCode reg, byte prefix);
	void doOUT_C_ (regbCode rcode);
	void doOUT_n_ (byte b);
	void doRET ();
	void doRETflag (flagCode fcode);
	void doRST (address addr);
private:
	void showlistingblank (const string & txt);
	void showlisting ();
	void showlistingsymbols ();
	void showlistingequ (address value);
	void showlistingheader ();
	string getlistingstatus () const;
	string getcurrentlistingtext ();

	address getvalue (const string & var, bool required, bool ignored);
	bool setvardef (const string & varname,
		address value, Defined defined);
	bool setvardef (const string & varname,
		const VarData & vdata, Defined defined);
public:
	bool isdefined (const string & varname);
private:
	Tokenizer getcurrenttz ();

	void parseinstruction (Tokenizer & tz);

	void do_iftrue (TypeToken ttif);
	void do_iffalse (TypeToken ttif);
	void do_if (TypeToken ttif, bool valueif);

	void parseline (Tokenizer & tz);
	//void link_rel_module (const string & relname);
	//void link_modules ();
	void dopass ();

	bool setequorlabel (const string & varname, address value);
	bool setlabel (const string & varname, const Value & v);
	bool setequ (const string & varname, const VarData & vdata);
	bool setdefl (const string & varname, address value);

	// Aux error and warning functions.
public:
	void emitwarning (const string & text);
private:
	void no8080 ();
	void no86 ();

	// Z80 instructions.

	//void genCALL (byte code, address addr);
	void genCALL (byte code, const VarData & addr);

	//void genJP (byte code, address addr);
	void genJP (byte code, const VarData & addr);

	// Variables.

	const AsmOptions opt;
	AsmMode asmmode;
	GenCodeMode genmode;

	// ********* Information streams ********

	bool debout_flag;
	bool listing_file;
	bool listing_flag;
	ostream debout;
	ostream errout;
	ostream verbout;
	ostream warnout;
	ostream listout;

	size_t counterr;

	Module mainmodule;
	Segment mainseg;

	address link_base;
	address base;

	//address current;
	bool phase_active;
	address phasing;

	//address currentinstruction;
	Value currentinstruction;

	//address minused;
	//address maxused;

	address entrypoint;
	bool hasentrypoint;

	int pass;
	bool end_reached;

	vector <size_t> ifline;	
	size_t iflevel;

public:
	size_t getiflevel () const;
	void setiflevel (size_t newlevel);
	void deciflevel ();
private:
	size_t includelevel;
	size_t macrolevel;
	int listingpagelen;
	int listinglines;
	int listingspage;
	int listingstep;
	vector <string> hexlisting;

	// ********* Local **********

	size_t localcount;
	vector <string> localnames;

	void initlocal ();

	LocalStack localstack;

public:
	void pushlocal (LocalLevel * plevel);
	LocalLevel * toplocal () const;
	void poplocal ();
private:

	bool isautolocalname (const string & varname);
	AutoLevel * enterautolocal ();
	void finishautolocal ();
	void checkautolocal (const string & varname);

	void verifynoautolocal (const string & varname);

	void enterorfinishautolocal (const string & varname);

	void checkafterprocess ();


	// ********* Macro **********

	//MapMacro mapmacro;

public:
	const Macro & getmacro (const string & name);
	bool ismacro (const string & name) const;
private:
	bool gotoENDM ();
	void domacroexpansion (MacroFrameMacro & mframe);
	void expandIRP (MacroFrameIRPbase & macroirp,
		const MacroArgList & params);
	MacroFrameBase * pcurrentmframe;
public:
	MacroFrameBase * getmframe () const;
	void setmframe (MacroFrameBase * pnew);

	// ********** Extern references **********
private:
	typedef map <address, string> ChainExtern;
	ChainExtern chainextern;
	typedef map <address, address> ExternOffset;
	ExternOffset externoffset;
public:
	typedef map <address, Value> Relative;
	//typedef map <address, string> RefToExtern;
	typedef map <string, address> RefToExtern;
	typedef map <address, Value> Offset;
private:
	Relative relative;
	RefToExtern reftoextern;
	Offset offset;
};


} // namespace impl

} // namespace pasmo


#endif

// End of asmreal.h
