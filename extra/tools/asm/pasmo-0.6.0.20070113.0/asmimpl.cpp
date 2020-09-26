// asmimpl.cpp
// Revision 10-jan-2007

#include "asmimpl.h"

#include "token.h"
#include "parser.h"
#include "parsertypes.h"
#include "simpleinst.h"
#include "asmfile.h"
#include "var.h"
#include "codeaux.h"
#include "nullstream.h"
#include "cpc.h"
#include "tap.h"
#include "tzx.h"
#include "spectrum.h"
#include "relfile.h"
#include "segment.h"
#include "module.h"
#include "local.h"
#include "macro.h"
#include "macroframe.h"

#include "config_version.h"

#include "trace.h"


#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <set>
#include <map>
#include <memory>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <memory>


using std::ios;
using std::streambuf;
using std::streamsize;
using std::ostream;
using std::cout;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ostringstream;
using std::vector;
using std::stack;
using std::set;
using std::map;
using std::make_pair;
using std::ostream_iterator;
using std::back_inserter;
using std::exception;
using std::runtime_error;
using std::logic_error;
using std::for_each;
using std::fill;
using std::copy;
using std::transform;
using std::remove_copy_if;
using std::auto_ptr;


namespace pasmo
{

namespace impl
{


//	Listing auxilary constant and funcs.

const string listing_progname (PACKAGE_STRING);
const string listing_sep ("                ");

string prefhex4 (address w)
{
	return string (2, ' ') + hex4str (w);
}


//*********************************************************
//		Exceptions.
//*********************************************************


// Errors that must never happen, they are handled for diagnose
// Pasmo bugs.

pasmo_fatal::pasmo_fatal (const std::string & s) :
	logic_error (s)
{ }


logic_error UnexpectedError ("Unexpected error");
logic_error UnexpectedRegisterCode ("Unexpected register code");
logic_error InvalidRegisterUsed ("Invalid register used");
logic_error InvalidInstructionType ("Invalid instruction type");

//logic_error LocalNotExist ("Trying to use a non existent local level");

logic_error LocalNotExpected ("Unexpected local block encountered");
logic_error AutoLocalNotExpected ("Unexpected autolocal block encountered");

pasmo_fatal InvalidPassValue ("Invalid value of pass");

logic_error UnexpectedORG ("Unexpected ORG found");
logic_error UnexpectedMACRO ("Unexpected MACRO found");
logic_error MACROLostENDM ("Unexpected MACRO without ENDM");


// Errors in the code being assembled.

runtime_error ErrorReadingINCBIN ("Error reading INCBIN file");
runtime_error ErrorOutput ("Error writing object file");

runtime_error InvalidPredefine ("Can't predefine invalid identifier");
runtime_error InvalidPredefineValue ("Invalid value for predefined symbol");
runtime_error InvalidPredefineSyntax ("Syntax error in predefined symbol");

class RedefinedDEFL : public runtime_error {
public:
	RedefinedDEFL (const string & varname) :
		runtime_error ("Invalid definition of '" + varname +
			"', previously defined as DEFL")
	{ }
};

class RedefinedEQU : public runtime_error {
public:
	RedefinedEQU (const string & varname) :
		runtime_error ("Invalid definition of '" + varname +
			"', previously defined as EQU or label")
	{ }
};

class RedefinedExtern : public runtime_error {
public:
	RedefinedExtern (const string & varname) :
		runtime_error ("Invalid definition of '" + varname +
			"', previously defined as extern")
	{ }
};

runtime_error InvalidInAutolocal ("Invalid use of name in autolocal mode");
runtime_error InvalidSharpSharp ("Invalid use of ##");
runtime_error IFwithoutENDIF ("IF without ENDIF");
runtime_error ELSEwithoutIF ("ELSE without IF");
runtime_error ELSEwithoutENDIF ("ELSE without ENDIF");
runtime_error ENDIFwithoutIF ("ENDIF without IF");
runtime_error UnbalancedPROC ("Unbalanced PROC");
runtime_error UnbalancedENDP ("Unbalanced ENDP");
runtime_error MACROwithoutENDM ("MACRO without ENDM");
runtime_error REPTwithoutENDM ("REPT without ENDM");
runtime_error ENDMwithoutMacro ("ENDM without corresponding macro directive");
runtime_error IRPwithoutENDM ("IRP without ENDM");
runtime_error ENDMOutOfMacro ("ENDM outside of macro");

pasmo_fatal ShiftOutsideMacro (".SHIFT outside MACRO");

runtime_error InvalidBaseValue ("Invalid base value");

runtime_error ParenInsteadOfBracket ("Expected ] but ) found");
runtime_error BracketInsteadOfParen ("Expected ) but ] found");

runtime_error OffsetOutOfRange ("Offset out of range");
runtime_error RelativeOutOfRange ("Relative jump out of range");
runtime_error BitOutOfRange ("Bit position out of range");

runtime_error InvalidInstruction ("Invalid instruction");
runtime_error InvalidFlagJR ("Invalid flag for JR");
runtime_error InvalidValueRST ("Invalid RST value");
runtime_error InvalidValueIM ("Invalid IM value");

runtime_error NotValid86 ("Instruction not valid in 86 mode");

runtime_error IsPredefined ("Can't redefine, is predefined");

runtime_error OutOfSyncPRL ("PRL generation failed: out of sync");
runtime_error OutOfSyncREL ("REL generation failed: out of sync");

class NoInstruction : public runtime_error {
public:
	NoInstruction (const Token & tok) :
		runtime_error ("Unexpected '" + tok.str () +
			"' used as instruction")
	{ }
};

class UndefinedVar : public runtime_error {
public:
	UndefinedVar (const string & varname) :
		runtime_error ("Symbol '" + varname + "' is undefined")
	{ }
	UndefinedVar (const VarData & vd) :
		runtime_error ("Symbol '" + vd.getname () + "' is undefined")
	{ }
};


class Expected : public runtime_error {
public:
	Expected (const Token & tokexp, const Token & tokfound) :
		runtime_error ("'" + tokexp.str () + "' expected but '" +
			tokfound.str () + "' found")
	{ }
	Expected (const string & expected, const Token & tok) :
		runtime_error (expected + " expected but '" +
			tok.str () + "' found")
	{ }
	Expected (const string & expected, const string & found) :
		runtime_error (expected + " expected but '" +
			found + "' found")
	{ }
};


class EndLineExpected : public Expected {
public:
	EndLineExpected (const Token & tok) :
		Expected ("End line", tok)
	{ }
};

class IdentifierExpected : public Expected {
public:
	IdentifierExpected (const Token & tok) :
		Expected ("Identifier", tok)
	{ }
};

class MacroExpected : public Expected {
public:
	MacroExpected (const string & name) :
		Expected ("Macro name", name)
	{ }
};

class ValueExpected : public Expected {
public:
	ValueExpected (const Token & tok) :
		Expected ("Value", tok)
	{ }
};


class SomeOpenExpected : public Expected {
public:
	SomeOpenExpected (const Token & tok) :
		Expected ("( or [", tok)
	{ }
};


class TokenExpected : public Expected {
public:
	TokenExpected (TypeToken ttexpect, const Token & tokfound) :
		Expected (gettokenname (ttexpect), tokfound)
	{ }
};

class OffsetExpected : public Expected {
public:
	OffsetExpected (const Token & tok) :
		Expected ("Offset expression", tok)
	{ }
};

class ErrorDirective : public runtime_error {
public:
	ErrorDirective (const Token & tok) :
		runtime_error (".ERROR directive: " + tok.str () )
	{ }
	ErrorDirective (const string & msg) :
		runtime_error (".ERROR directive: " + msg)
	{ }
};

class UndefinedInPass1 : public runtime_error {
public:
	UndefinedInPass1 (const string & name) :
		runtime_error ("The symbol '" + name +
			"' must be defined in pass 1")
	{ }
};

void checktoken (TypeToken ttexpected, const Token & tok)
{
	if (tok.type () != ttexpected)
		throw TokenExpected (ttexpected, tok);
}

void checkidentifier (const Token & tok)
{
	checktoken (TypeIdentifier, tok);
}

//*********************************************************
//		Auxiliary functions and constants.
//*********************************************************


const string emptystr;

const string openIndir ("[");
const string closeIndir ("]");

const bool nameSP= true;
const bool nameAF= false;

string incordec (bool isINC)
{
	return string (isINC ? "INC " : "DEC ");
}

string inrordcr (bool isINC)
{
	return string (isINC ? "INR " : "DCR ");
}

string inxordcx (bool isINC)
{
	return string (isINC ? "INX " : "DCX ");
}

string tablabel (string str)
{
	const string::size_type l= str.size ();
	if (l < 8)
		str+= "\t\t";
	else
		if (l < 16)
			str+= '\t';
		else
			str+= ' ';
	return str;
}

bool ismacrodirective (TypeToken tt)
{
	return tt == TypeMACRO || tt == TypeREPT ||
		tt == TypeIRP || tt == TypeIRPC;
}

void putvarnamelist (ostream & out, const VarnameList & varnamelist)
{
	ASSERT (! varnamelist.empty () );

	//VarnameList::const_iterator last= --varnamelist.end ();
	VarnameList::const_iterator last= varnamelist.end ();
	--last;

	copy (varnamelist.begin (), last,
		ostream_iterator <string> (out, ", ") );
	out << * last << endl;
}


//*********************************************************
//			class AsmReal
//*********************************************************


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

	// Bug reported by Mauri
	// Extra calification fails is some compilers.
	//static streambuf * AsmReal::pnullbuf ();
	static streambuf * pnullbuf ();

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

	void emitwarning (const string & text);
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


//*********************************************************
//			class AsmImpl definitions
//*********************************************************


AsmImpl::AsmImpl ()
{ }


AsmImpl::AsmImpl (const AsmImpl & in) :
	Asm (in)
{ }

AsmImpl::~AsmImpl ()
{ }

Asm * AsmImpl::create (const AsmOptions & options_n)
{
	return new AsmReal (options_n);
}


//*********************************************************
//		class AsmReal definitions
//*********************************************************


AsmReal::AsmReal (const AsmOptions & options_n) :
	AsmImpl (),
	AsmFile (),
	opt (options_n),
	asmmode (opt.asmmode),
	genmode (opt.mode86 ? gen86 : gen80),
	debout_flag (true),
	listing_file (false),
	listing_flag (false),
	debout (pnullbuf () ),
	errout (opt.redirecterr ? cout.rdbuf () : cerr.rdbuf () ),
	verbout (opt.verbose ? cerr.rdbuf () : pnullbuf () ),
	warnout (cerr.rdbuf () ),
	listout (pnullbuf () ),
	mainmodule (debout, warnout),
	//link_base ( (opt.getObjectType () == ObjectCom) ? 0x100 : 0),
	link_base (opt.getLinkBase () ),
	base (link_base),
	//current (base),
	phase_active (false),
	phasing (0),
	currentinstruction (ValueAbsolute, 0),
	//minused (65535),
	//maxused (0),
	hasentrypoint (false),
	pass (0),
	end_reached (false),
	localcount (0),
	pcurrentmframe (0)
{
}

AsmReal::AsmReal (const AsmReal & in) :
	AsmImpl (in),
	AsmFile (in),
	//Vars (in),
	Vars (),
	MacroStore (),
	opt (in.opt),
	asmmode (AsmZ80),
	genmode (in.genmode),
	debout_flag (true),
	listing_file (false),
	listing_flag (false),
	debout (pnullbuf () ),
	errout (in.errout.rdbuf () ),
	verbout (in.verbout.rdbuf () ),
	warnout (in.warnout.rdbuf () ),
	listout (pnullbuf () ),
	mainmodule (debout, warnout),
	link_base (in.link_base),
	base (link_base),
	//current (base),
	phase_active (false),
	phasing (0),
	currentinstruction (ValueAbsolute, 0),
	//minused (65535),
	//maxused (0),
	hasentrypoint (false),
	localcount (0),
	pcurrentmframe (0)
{
}

AsmReal::~AsmReal ()
{
	TRFUNC (tr, "AsmReal::~AsmReal");
}

Asm * AsmReal::create (const AsmOptions & options_n)
{
	return new AsmReal (options_n);
}

ValueType AsmReal::getinitialsegment ()
{
	const ObjectType otype (opt.getObjectType () );
	if (otype == ObjectRel || otype == ObjectPrl ||
		otype == ObjectCom || otype == ObjectCmd)
	{
		return ValueProgRelative;
	}
	else
	{
		return ValueAbsolute;
	}
}

void AsmReal::setfilelisting (ostream & out_n)
{
	listing_file= true;
	listout.rdbuf (out_n.rdbuf () ),
	listingpagelen= 56;
	listinglines = listingpagelen;
	listingspage = 0;
	listingstep = 1;

	listing_flag= true;
	showlistingheader ();
	listing_flag= false;
}

void AsmReal::setbase (unsigned int addr)
{
	if (addr > 65535)
		throw InvalidBaseValue;

	base= static_cast <address> (addr) + link_base;

	verbout << "Setting base to " << base << endl;

	//current= base;
	//phasing= 0;
	clearphase ();

	//currentinstruction= base;
}

AsmMode AsmReal::getasmmode () const
{
	return asmmode;
}

bool AsmReal::getnocase () const
{
	return opt.nocase;
}

void AsmReal::addincludedir (const string & dirname)
{
	AsmFile::addincludedir (dirname);
}

void AsmReal::addpredef (const string & predef)
{
	// Default value.
	address value= 0xFFFF;

	// Prepare the parsing of the argument.
	Tokenizer trdef (predef, AsmZ80);

	// Get symbol name.
	Token tr (trdef.gettoken () );
	if (tr.type () != TypeIdentifier)
		throw InvalidPredefine;
	string varname= tr.str ();

	// Get the value, if any.
	tr= trdef.gettoken ();
	switch (tr.type () )
	{
	case TypeEqOp:
		tr= trdef.gettoken ();
		if (tr.type () != TypeNumber)
			throw InvalidPredefineValue;
		value= tr.num ();
		tr= trdef.gettoken ();
		if (tr.type () != TypeEndLine)
			throw InvalidPredefineValue;
		break;
	case TypeEndLine:
		break;
	default:
		throw InvalidPredefineSyntax;
	}

	verbout << "Predefining: " << varname << "= " << value << endl;

	//setequorlabel (varname, value);
	setvar (varname, VarData (value, PreDefined, ValueAbsolute) );
}

streambuf * AsmReal::pnullbuf ()
{
	static Nullbuff buff;
	return & buff;
}

void AsmReal::setentrypoint (address addr)
{
	//if (pass < 2)
	//	return;
	if (hasentrypoint)
	{
		emitwarning ("Entry point redefined");
	}
	hasentrypoint= true;
	entrypoint= addr;
}

void AsmReal::checkendline (const Token & tok)
{
	TRFUNC (tr, "AsmReal::checkendline");

	if (tok.type () != TypeEndLine)
		throw EndLineExpected (tok);
}

void AsmReal::checkendline (Tokenizer & tz)
{
	Token tok= tz.gettoken ();
	checkendline (tok);
}

//address AsmReal::currentpos () const
Value AsmReal::currentpos () const
{
	//return mainmodule.getpos ().value;
	return mainmodule.getpos ();
}

void AsmReal::clearphase ()
{
	phase_active= false;
	phasing= 0;
}

#if 0
address AsmReal::phased (address addr) const
{
	if (phase_active)
		return addr + phasing;
	else
		return addr;
}

address AsmReal::phasedpos () const
{
	return phased (currentpos () );
}
#endif

Value AsmReal::phased (Value addr) const
{
	if (phase_active)
	{
		return Value (ValueAbsolute, addr.value + phasing);
	}
	else
		return addr;
}

Value AsmReal::phasedpos () const
{
	return phased (currentpos () );
}

void AsmReal::genbyte (byte b)
{
	mainmodule.gencode (b);
}

void AsmReal::genword (address dataword)
{
	genbyte (lobyte (dataword) );
	genbyte (hibyte (dataword) );
}

void AsmReal::gencode (byte code)
{
	//gendata (code);
	if (pass > 1)
		hexlisting.push_back (hex2str (code) );

	genbyte (code);
}

void AsmReal::gencode (const VarData & vd)
{
	TRFDEBS (vd.gettype () << "-" << vd.getvalue () );

	const Defined defined= vd.def ();
	switch (defined)
	{
	case NoDefined:
		if (pass >= 2)
		{
			throw UndefinedVar (vd);
		}
		break;
	default:
		// Nothing
		break;
	}

	gencode (vd.getvalue () );
}


void AsmReal::gencode (byte code1, byte code2)
{
	gencode (code1);
	gencode (code2);
}

void AsmReal::gencode (byte code1, byte code2, byte code3)
{
	gencode (code1);
	gencode (code2);
	gencode (code3);
}

void AsmReal::gencode (byte code1, byte code2, byte code3, byte code4)
{
	gencode (code1);
	gencode (code2);
	gencode (code3);
	gencode (code4);
}

void AsmReal::gencodeED (byte code)
{
	gencode (0xED);
	gencode (code);
}

void AsmReal::gencodeword (address value)
{
	//gendataword (value);
	if (pass > 1)
		hexlisting.push_back (hex4str (value) );

	genword (value);
}

void AsmReal::gencodeword (const Value & v)
{
	TRF;

	const address w= v.value;
	switch (v.type)
	{
	case ValueAbsolute:
		TRDEB ("ABSOLUTE");
		mainmodule.genword (w);
		break;
	case ValueProgRelative:
	case ValueDataRelative:
		TRDEB ("relative");
		mainmodule.genrelative (v);
		break;
	default:
		throw logic_error ("Unexpected type in code word");
	}
	if (pass > 1)
	{
		hexlisting.push_back (hex4str (w) );
	}
}

void AsmReal::gencodeword (const VarData & vd)
{
	TRFDEBS (vd.gettype () << "-" << vd.getvalue () );

	//if (vd.isextern () && pass > 1)
	if (pass > 1)
	{
		const Defined d (vd.def () );
		switch (d)
		{
		case NoDefined:
			throw UndefinedVar (vd);
		default:
			break;
		}
		if (vd.isextern () )
		{
			TRDEB ("extern");

			//const string name= vd.getname ();
			const string name= stripdollar (vd.getname () );

			const address offset= vd.getvalue ();

			mainmodule.genextern (name, offset);
			if (pass > 1)
				hexlisting.push_back (hex4str (offset) );
		}
		else
		{
			gencodeword (vd.getval () );
		}
	}
	else
	{
		gencodeword (vd.getval () );
	}
}

string AsmReal::getlistingstatus () const
{
	string status (2, ' ');
	if (includelevel > 0)
		status [0]= 'C';
	if (macrolevel > 0)
		status [1]= '+';
	return status;
}

void AsmReal::showlistingblank (const string & txt)
{
	listout << "\t\t      " << getlistingstatus () << txt << endl;
	showlistingheader ();
}

void AsmReal::showlisting ()
{
	if (! listing_flag)
		return;

	string instr = getcurrentlistingtext ();
	if (hexlisting.empty () )
	{
		showlistingblank (instr);
		return;
	}
	bool showadd = true;
	bool showlin = true;
	size_t c= 0;
	//address addr = currentinstruction;
	address addr = currentinstruction.value;
	string ll;

	for (size_t i = 0; i < hexlisting.size (); ++i)
	{
		c += (hexlisting [i].size () + (i == 0 ? 0 : 1) );
		if (c > 11)
		{
			while (ll.size () != 22)
				ll += " ";
			if (showlin)
			{
				ll += getlistingstatus () + instr;
				showlin = false;
			}
			listout << ll << endl;
			showlistingheader ();
			ll = "";
			c = hexlisting [i].size ();
			showadd = true;
		}
		if (showadd)
		{
			ll += prefhex4 (addr) + "   ";
			showadd = false;
		}
		if (c != hexlisting [i].size () )
			ll += " ";
		ll += hexlisting [i];
		addr += (hexlisting [i].size () / 2);
	}

	if (ll.size () != 0)
	{
		while (ll.size () != 22)
			ll += " ";
		if (showlin)
		{
			ll += getlistingstatus () + instr;
		}
		listout << ll;
	}
	listout << endl;
	showlistingheader ();
	hexlisting.clear ();
}

void AsmReal::showlistingsymbols ()
{
	if (! listing_flag)
		return;

	listingstep = 2;

	listingspage = 0;
	listinglines = listingpagelen;
	showlistingheader ();

	listout << "Macros:" << endl;

	#if 0
	// Pendiente.

	int n = 0;
	for (MapMacro::const_iterator it= mapmacro.begin ();
		it != mapmacro.end ();
		++it)
	{
		string macroname = it->first;
		if (macroname.size () > 16)
		{
			macroname = macroname.substr (0, 16);
		}
		else
		{
			while (macroname.size () < 16)
				macroname += " ";
		}
		listout << macroname;
		++n;
		if (n == 5)
		{
			listout << endl;
			showlistingheader ();
			n = 0;
		}
	}
	if (n != 0)
	{
		listout << endl;
		showlistingheader ();
	}
	showlistingheader ();

	#endif

	listout << endl;

	showlistingheader ();
	listout << "Symbols:" << endl;
	size_t n = 0;
	for (mapvar_t::const_iterator it= varbegin ();
		it != varend ();
		++it)
	{
		string varname = it->first;
		if (varname.size () > 16)
		{
			varname = varname.substr (0, 16);
		}
		else
		{
			while (varname.size () < 16)
				varname += " ";
		}
		const VarData & vd= it->second;

		// Dump only EQU and label valid symbols.
		if (vd.def () != DefinedPass2)
			continue;
		listout << hex4 (vd.getvalue () ) << "\t" << varname;
		++n;
		if (n == 3)
		{
			listout << endl;
			showlistingheader ();
			n = 0;
		}
	}
	if (n != 0)
	{
		listout << endl;
		showlistingheader ();
	}
}

void AsmReal::showlistingheader ()
{
	++listinglines;
	if (listinglines >= listingpagelen)
	{
		listout << "\f\t" << listing_progname << "\tPAGE ";
		if (listingstep == 1)
			listout << "1";
		else
			listout << "S";
		if (listingspage != 0)
			listout << "-" << listingspage;
		listout << endl << endl << endl;
		listinglines = 0;
		++listingspage;
	}
}

void AsmReal::showlistingequ (address value)
{
	if (! listing_flag)
		return;

	listout << prefhex4 (value) <<
		listing_sep << getlistingstatus () <<
		getcurrenttext () << endl;
	showlistingheader ();
}

string AsmReal::getcurrentlistingtext ()
{
	//Tokenizer tz= getcurrenttz ();
	Tokenizer tz (getcurrentline (asmmode) );

	//ostringstream oss;
	//oss << tz;
	//return oss.str ();
	string str;
	for (Token tok= tz.getrawtoken (); tok.type () != TypeEndLine;
		tok= tz.getrawtoken () )
	{
		switch (tok.type () )
		{
		case TypeWhiteSpace:
		case TypeNumber:
		case TypeLiteral:
		case TypeMacroArg:
			str+= tok.raw ();
			break;
		case TypeIdentifier:
			str+= opt.nocase ? upper (tok.str () ) : tok.str ();
			break;
		default:
			str+= tok.str ();
		}
	}
	return str;
}

void AsmReal::showcode (const string & instruction)
{
	const address bytesperline= 4;

	//address pos= currentinstruction;
	const address posini= currentinstruction.value;
	address pos= posini;
	//const address posend= currentpos ();
	const address posend= currentpos ().value;

	if (debout_flag)
	{
		bool instshowed= false;
		for (address i= 0; pos != posend; ++i, ++pos)
		{
			if ( (i % bytesperline) == 0)
			{
				if (i != 0)
				{
					if (! instshowed)
					{
						debout << '\t' << instruction;
						instshowed= true;
					}
					debout << '\n';
				}
				debout << hex4 (pos) << ':';
			}
			debout << hex2 (mainmodule.getbyte (pos) );
		}

		if (! instshowed)
		{
			//if (posend == currentinstruction + 1)
			if (posend == posini + 1)
				debout << '\t';
			debout << '\t' << instruction;
		}

		debout << endl;
	}

	// Check that the 64KB limit has not been exceeded in the
	// middle of an instruction.
	//if (posend != 0 && posend < currentinstruction)
	if (posend != 0 && posend < posini)
	{
		emitwarning ("64KB limit passed inside instruction");
	}
}

void AsmReal::showdebnocodeline (const Tokenizer & tz)
{
	if (debout_flag)
	{
		Tokenizer tzaux (tz);
		if (! tzaux.empty () )
			debout << "\t\t- " << tzaux << endl;
	}
}

VarData AsmReal::getvar (const string & varname)
{
	TRF;

	checkautolocal (varname);
	return Vars::getvar (varname);
}

VarData AsmReal::rawgetvar (const string & varname)
{
	return Vars::getvar (varname);
}

address AsmReal::getvalue (const string & varname,
	bool required, bool ignored)
{
	TRF;

	checkautolocal (varname);

	VarData vd= getvar (varname);
	address value= 0;
	if (vd.def () == NoDefined)
	{
		if ( (pass > 1 || required) && ! ignored)
			throw UndefinedVar (varname);
	}
	else
	{
		value= vd.getvalue ();
	}
	return value;
}

bool AsmReal::setvardef (const string & varname,
	address value, Defined defined)
{
	TRF;

	checkautolocal (varname);

	//return setvar (varname, value, defined);
	Value val (mainmodule.getcurrentsegment (), value);
	return setvar (varname, val, defined);
}

bool AsmReal::setvardef (const string & varname,
	const VarData & vdata, Defined defined)
{
	TRF;

	checkautolocal (varname);

	//Value val (vdata.gettype (), vdata.getvalue () );
	//return setvar (varname, val, defined);
	return setvar (varname, vdata.getval (), defined);
}

bool AsmReal::isdefined (const string & varname)
{
	TRF;

	bool result;
	checkautolocal (varname);

	Defined def= getvar (varname).def ();

	if (def == NoDefined || (pass > 1 && def == DefinedPass1) )
		result= false;
	else
		result= true;

	return result;
}

Tokenizer AsmReal::getcurrenttz ()
{
	MacroFrameBase * pframe= getmframe ();
	if (pframe)
	{
		Tokenizer tz (getcurrentline (asmmode) );
		Tokenizer tzaux (pframe->substparams (tz), asmmode);
		return tzaux;
	}
	else
	{
		return Tokenizer (getcurrentline (asmmode) );
	}
}

void AsmReal::doEmpty ()
{
	showlisting ();
}

size_t AsmReal::getiflevel () const
{
	return iflevel;
}

void AsmReal::setiflevel (size_t newlevel)
{
	iflevel= newlevel;
}

void AsmReal::deciflevel ()
{
	--iflevel;
	ifline.pop_back ();
}

void AsmReal::do_iftrue (TypeToken ttif)
{
	ifline.push_back (getline () );
	++iflevel;
	debout << "\t\t" << gettokenname (ttif) << " (true)" << endl;
	showlisting ();
}

void AsmReal::do_iffalse (TypeToken ttif)
{
	TRFUNC (tr, "AsmReal::do_iffalse");

	debout << "\t\t" << gettokenname (ttif) << " (false)" << endl;

	showlisting ();

	size_t ifbeginline= getline ();

	for (size_t level= 1; level > 0 && nextline (); )
	{
		TRMESSAGE (tr, "next line");

		Tokenizer tz (getcurrenttz () );
		showlisting ();
		showdebnocodeline (tz);

		Token tok= tz.gettoken ();
		TypeToken tt= tok.type ();
		if (tt == TypeIdentifier)
		{
			tok= tz.gettoken ();
			tt= tok.type ();
		}
		switch (tt)
		{
		case TypeIF:
		case TypeIF1:
		case TypeIF2:
		case TypeIFDEF:
		case TypeIFNDEF:
			++level;
			break;
		case TypeELSE:
			if (level == 1)
			{
				ifline.push_back (getline () );
				++iflevel;
				--level;
			}
			break;
		case TypeENDIF:
			--level;
			if (level == 0)
				debout << "\t\tENDIF" << endl;
			break;
		case TypeENDM:
			// Let the current line be reexamined
			// for ending macro expansion or emit
			// an error.
			prevline ();
			level= 0;
			break;
		case TypeMACRO:
		case TypeREPT:
		case TypeIRP:
		case TypeIRPC:
			gotoENDM ();
			break;
		default:
			break;
		}
	}
	if (passeof () )
	{
		setline (ifbeginline);
		throw IFwithoutENDIF;
	}
}

void AsmReal::do_if (TypeToken ttif, bool valueif)
{
	if (valueif)
		do_iftrue (ttif);
	else
		do_iffalse (ttif);
}

void AsmReal::doIFDEF (const string & varname)
{
	do_if (TypeIFDEF, isdefined (varname) );
}

void AsmReal::doIFNDEF (const string & varname)
{
	do_if (TypeIFNDEF, ! isdefined (varname) );
}

void AsmReal::doIF1 ()
{
	do_if (TypeIF1, pass == 1);
}

void AsmReal::doIF2 ()
{
	do_if (TypeIF2, pass == 2);
}

void AsmReal::doIF (address v)
{
	do_if (TypeIF, v != 0);
}

void AsmReal::doELSE ()
{
	TRFUNC (tr, "AsmReal::doELSE");

	if (iflevel == 0)
		throw ELSEwithoutIF;

	showlisting ();

	debout << "\t\tELSE (false)" << endl;

	size_t elseline= getline ();
	size_t level= 1;
	while (level > 0 && nextline () )
	{
		TRMESSAGE (tr, "next line");

		Tokenizer tz (getcurrenttz () );
		showlisting ();
		showdebnocodeline (tz);

		Token tok= tz.gettoken ();
		TypeToken tt= tok.type ();
		if (tt == TypeIdentifier)
		{
			tok= tz.gettoken ();
			tt= tok.type ();
		}
		switch (tt)
		{
		case TypeIF:
		case TypeIF1:
		case TypeIF2:
		case TypeIFDEF:
		case TypeIFNDEF:
			++level;
			//debout << "- " << getcurrenttext () << endl;
			break;
		case TypeENDIF:
			--level;
			if (level == 0)
				debout << "\t\tENDIF" << endl;
			//else
			//	debout << "- " << getcurrenttext () << endl;
			break;
		case TypeENDM:
			// Let the current line be reexamined
			// for ending macro expansion or emit
			// an error.
			prevline ();
			level= 0;
			break;
		case TypeMACRO:
		case TypeREPT:
		case TypeIRP:
		case TypeIRPC:
			//debout << "- " << getcurrenttext () << endl;
			gotoENDM ();
			break;
		default:
			//debout << "- " << getcurrenttext () << endl;
			break;
		}
	}
	if (passeof () )
	{
		setline (elseline);
		throw ELSEwithoutENDIF;
	}
	--iflevel;
	ifline.pop_back ();
}

void AsmReal::doENDIF ()
{
	if (iflevel == 0)
		throw ENDIFwithoutIF;
	--iflevel;
	ifline.pop_back ();

	showlisting ();

	debout << "\t\tENDIF" << endl;
}

void AsmReal::parseline (Tokenizer & tz)
{
	TRFUNC (tr, "AsmReal::parseline");

	//currentinstruction= current;
	currentinstruction= currentpos ();
	parseinstruction (tz);
}

void AsmReal::initlocal ()
{
	ASSERT ( (currentpass () == 1 && localcount == 0) ||
		(currentpass () == 2 && localcount == localnames.size () ) );

	localcount= 0;
}

string AsmReal::genlocalname (const string & varname)
{
	switch (pass)
	{
	case 1:
		ASSERT (localnames.size () == localcount);
		localnames.push_back (varname);
		break;
	case 2:
		ASSERT (localcount < localnames.size () );
		ASSERT (varname == localnames [localcount] );
		break;
	default:
		throw logic_error ("Invalid call to genlocalname");
	}

	const string glob= hex8str (localcount);
	++localcount;
	return glob;
}

bool AsmReal::isautolocalname (const string & varname)
{
	static const char AutoLocalPrefix= '_';
	ASSERT (! varname.empty () );

	if (! opt.autolocal)
		return false;
	return varname [0] == AutoLocalPrefix;
}

void AsmReal::pushlocal (LocalLevel * plevel)
{
	localstack.push (plevel);
}

LocalLevel * AsmReal::toplocal () const
{
	return localstack.top ();
}

void AsmReal::poplocal ()
{
	localstack.pop ();
}

AutoLevel * AsmReal::enterautolocal ()
{
	TRFUNC (tr, "AsmReal::enterautolocal");

	AutoLevel * pav;
	if (localstack.empty () || ! localstack.top ()->is_auto () )
	{
		debout << "Enter autolocal level" << endl;
		pav= new AutoLevel (* this);
		localstack.push (pav);
	}
	else
	{
		pav= dynamic_cast <AutoLevel *> (localstack.top () );
		ASSERT (pav);
	}
	return pav;
}

void AsmReal::finishautolocal ()
{
	TRFUNC (tr, "AsmReal::finishautolocal");

	if (opt.autolocal)
	{
		if (! localstack.empty () )
		{
			LocalLevel * plevel= localstack.top ();
			if (plevel->is_auto () )
			{
				debout << "Exit autolocal level" << endl;
				localstack.pop ();
			}
		}
	}
}

void AsmReal::checkautolocal (const string & varname)
{
	TRF;

	if (isautolocalname (varname) )
	{
		AutoLevel * pav= enterautolocal ();
		pav->add (varname);
	}
}

void AsmReal::verifynoautolocal (const string & varname)
{
	if (isautolocalname (varname) )
		throw InvalidInAutolocal;
}

void AsmReal::enterorfinishautolocal (const string & varname)
{
	if (opt.autolocal)
	{
		if (isautolocalname (varname) )
		{
			AutoLevel * pav= enterautolocal ();
			ASSERT (pav);
			pav->add (varname);
		}
		else
			finishautolocal ();
	}
}

const Macro & AsmReal::getmacro (const string & name)
{
	return MacroStore::getmacro (name);
}

bool AsmReal::ismacro (const string & name) const
{
	return MacroStore::ismacro (name);
}

MacroFrameBase * AsmReal::getmframe () const
{
	return pcurrentmframe;
}

void AsmReal::setmframe (MacroFrameBase * pnew)
{
	pcurrentmframe= pnew;
}

void AsmReal::dopass ()
{
	TRFUNC (tr, "AsmReal::dopass");

	verbout << "Entering pass " << pass << endl;

	// Pass initializition.

	end_reached= false;
	counterr= 0;
	initlocal ();
	//mapmacro.clear ();
	MacroStore::clear ();

	clearDEFL ();

	mainmodule.setdebout (debout);
	mainmodule.clear ();
	//if (opt.objecttype != ObjectRel && opt.objecttype != ObjectCom)
	//	mainmodule.setcurrentsegment (ValueAbsolute);
	mainmodule.setcurrentsegment (getinitialsegment () );

	asmmode= opt.asmmode;
	//current= base;
	//phasing= 0;
	clearphase ();

	iflevel= 0;

	includelevel = 0;
	macrolevel = 0;

	// Main loop.

	beginline ();
	if (opt.lines_to_skip > 0)
		verbout << "Skipping " << opt.lines_to_skip << " lines" << endl;
	for (size_t i= 0; i < opt.lines_to_skip; ++i)
	{
		if (! nextline () )
			throw runtime_error ("Skipped too much");
	}
	while (! end_reached && nextline () )
	{
		TRMESSAGE (tr, "next line");

		Tokenizer tz (getcurrenttz () );
		parseline (tz);
	}

	TRMESSAGE (tr, "pass finalization");

	if (iflevel > 0)
	{
		ASSERT (! ifline.empty () );
		size_t lastifline= ifline.back ();
		ifline.pop_back ();
		setline (lastifline);
		throw IFwithoutENDIF;
	}

	finishautolocal ();

	if (! localstack.empty () )
	{
		ProcLevel * proc=
			dynamic_cast <ProcLevel *> (localstack.top () );
		if (proc == NULL)
			throw LocalNotExpected;
		setline (proc->getline () );
		throw UnbalancedPROC;
	}

	if (counterr > 0)
		throw error_already_reported ();

	verbout << "Pass " << pass << " finished" << endl;
}

void AsmReal::loadfile (const string & filename)
{
	TRFUNC (tr, "AsmReal::loadfile");

	AsmFile::loadfile (filename, verbout, errout);
}

void AsmReal::link_modules (vector <Module *> & vpmod)
{
	TRFUNC (tr, "AsmReal::link_modules");

	verbout << "Linking modules" << endl;

	const size_t nmods= vpmod.size ();

	// Set module segment positions.

	//address currentbase= 0;
	//address currentbase= base;
	size_t currentbase= base;

	if (opt.common_after_abs)
	{
		TRMESSAGE (tr, "Evaluating max abs used");
		address after= 0;
		for (size_t i= 0; i < nmods; ++i)
		{
			Module & curmod= * vpmod [i];
			address aftermod= curmod.getafterabs ();
			if (aftermod > after)
				after= aftermod;
		}
		if (after > currentbase)
			currentbase= after;
	}

	for (size_t i= 0; i < nmods; ++i)
	{
		Module & curmod= * vpmod [i];
		curmod.setcodebase (currentbase);

		verbout << '[' << curmod.getname () <<
			"] codebase: " << hex4 (currentbase) << endl;

		currentbase+= curmod.getcodesize ();
	}
	for (size_t i= 0; i < nmods; ++i)
	{
		Module & curmod= * vpmod [i];
		curmod.setdatabase (currentbase);

		verbout << '[' << curmod.getname () <<
			"] database: " << hex4 (currentbase) << endl;

		currentbase+= curmod.getdatasize ();
	}

	// Resolve relative address in modules.

	for (size_t i= 0; i < nmods; ++i)
	{
		Module & curmod= * vpmod [i];
		curmod.evalrelatives ();
	}

	// Make globally available public symbols in modules.

	for (size_t i= 0; i < nmods; ++i)
	{
		Module & curmod= * vpmod [i];
		curmod.publicvars (* this);
	}

	// Solve externs in modules.

	for (size_t i= 0; i < nmods; ++i)
	{
		Module & curmod= * vpmod [i];
		curmod.solveextern (* this);
	}

	// Copy segments of modules to main absolute segment.
	for (size_t i= 0; i < nmods; ++i)
	{
		Module & curmod= * vpmod [i];
		curmod.install (mainseg);
	}

	// Get start address.
	for (size_t i= 0; i < nmods; ++i)
	{
		Module & curmod= * vpmod [i];
		if (curmod.hasstartpos () )
		{
			address newpos= curmod.getstartpos ();
			debout << "Setting start pos to " <<
				hex4 (newpos) << endl;
			if (hasentrypoint)
			{
				emitwarning ("Program entry point "
					"already defined");
			}
			else
			{
				setentrypoint (newpos);
			}
		}
	}

	verbout << "End of link" << endl;
}

void AsmReal::loadmodules (vector <Module> & mod)
{
	TRFUNC (tr, "AsmReal::loadmodules");

	const size_t nmods= opt.module.size ();

	// Load REL modules.

	for (size_t i= 0; i < nmods; ++i)
	{
		const string & modname= opt.module [i];
		TRMESSAGE (tr, "Loading module: " + modname);
		mod [i].loadrelfile (modname);
		TRMESSAGE (tr, "Finished module: " + modname);
	}

}

void AsmReal::link ()
{
	TRFUNC (tr, "AsmReal::link");

	if (opt.debugtype != NoDebug)
	{
		debout_flag= true;
		debout.flush ();
		debout.rdbuf (cout.rdbuf () );
	}

	const size_t nmods= opt.module.size ();
	vector <Module> mod (nmods, Module (debout, warnout) );

	loadmodules (mod);

	vector <Module *> vpmod;
	for (size_t i= 0; i < nmods; ++i)
		vpmod.push_back (& mod [i] );
	link_modules (vpmod);
	showlistingsymbols ();
}

void AsmReal::processfile ()
{
	TRFUNC (tr, "AsmReal::processfile");

	try 
	{
		pass= 1;
		if (opt.debugtype == DebugAll)
		{
			debout_flag= true;
			debout.flush ();
			debout.rdbuf (cout.rdbuf () );
		}
		else
		{
			debout_flag= false;
			debout.flush ();
			debout.rdbuf (pnullbuf () );
		}
		listing_flag= false;
		dopass ();

		pass= 2;
		listing_flag= listing_file;
		if (opt.debugtype != NoDebug)
		{
			debout_flag= true;
			debout.flush ();
			debout.rdbuf (cout.rdbuf () );
		}
		else
		{
			debout_flag= false;
			debout.flush ();
			debout.rdbuf (pnullbuf () );
		}
		dopass ();
		debout.flush ();

		checkafterprocess ();

		// Link

		mainmodule.evallimits ();

		if (opt.getObjectType () != ObjectRel)
		{
			const size_t nmods= opt.module.size ();
			vector <Module> mod (nmods, Module (debout, warnout) );
			loadmodules (mod);
			vector <Module *> vpmod;
			vpmod.push_back (& mainmodule);
			for (size_t i= 0; i < nmods; ++i)
				vpmod.push_back (& mod [i] );
			link_modules (vpmod);
		}

		// Keep debout pointing to something valid.
		debout.rdbuf (cout.rdbuf () );

		showlistingsymbols ();
	}
	catch (error_already_reported &)
	{
		throw;
	}
	catch (exception & e)
	{
		showcurrentlineinfo (errout);
		errout << ' ' << e.what () << endl;
		showcurrentline (verbout);
		throw error_already_reported ();
	}
	catch (...)
	{
		showcurrentlineinfo (errout);
		errout << "Unexpected error" << endl;
		showcurrentline (verbout);
		throw error_already_reported ();
	}
}

int AsmReal::currentpass () const
{
	return pass;
}

//address AsmReal::getcurrentinstruction () const
Value AsmReal::getcurrentinstruction () const
{
	return phased (currentinstruction);
}

void AsmReal::doINCLUDE ()
{
	showlisting ();
	++includelevel;

	debout << "\t\tINCLUDE" << endl;
}

void AsmReal::doEndOfINCLUDE ()
{
	--includelevel;
	debout << "\t\tEnd of INCLUDE" << endl;
}

void AsmReal::do_8080 ()
{
	showlisting ();
	debout << "\t\t.8080" << endl;
	asmmode= Asm8080;
}

void AsmReal::do_PHASE (address value)
{
	//phasing= value - current;
	phase_active= true;
	phasing= mainmodule.phase (value);

	showlisting ();
	debout << "\t\t.PHASE " << hex4 (value) << endl;
}

void AsmReal::do_DEPHASE ()
{
	//phasing= 0;
	clearphase ();

	showlisting ();
	debout << "\t\t.DEPHASE" << endl;
}

void AsmReal::do_Z80 ()
{
	showlisting ();
	debout << "\t\t.Z80" << endl;
	asmmode= AsmZ80;
}

void AsmReal::doORG (address neworg)
{
	//current= neworg;
	//current= neworg + base;

	#if 0
	//mainmodule.setpos (neworg + base);
	mainmodule.setpos (neworg);
	#else

	mainmodule.setorg (neworg);

	#endif

	//phasing= 0;
	clearphase ();

	showlisting ();
	debout << "\t\tORG " << hex4 (neworg) << endl;
}

void AsmReal::doEQU (const string & label, const VarData & vdata)
{
	TRFUNC (tr, "AsmReal::doEQU");
	TRSTREAM (tr, label << "= " << vdata.getvalue () <<
		'-' << vdata.gettype () );

	//bool islocal= setequorlabel (label, vdata.getvalue () );
	bool islocal= setequ (label, vdata);

	debout << tablabel (label) << "EQU ";
	if (islocal)
		debout << "local ";
	debout << vdata.getvalue () << endl;
	showlistingequ (vdata.getvalue () );
}

void AsmReal::doASEG ()
{
	clearphase ();
	mainmodule.setcurrentsegment (ValueAbsolute);
	debout << "\t\tASEG" << endl;
	showlisting ();
}

void AsmReal::doCSEG ()
{
	clearphase ();
	mainmodule.setcurrentsegment (ValueProgRelative);
	debout << "\t\tCSEG" << endl;
	showlisting ();
}

void AsmReal::doDSEG ()
{
	clearphase ();
	mainmodule.setcurrentsegment (ValueDataRelative);
	debout << "\t\tDSEG" << endl;
	showlisting ();
}

void AsmReal::doDEFL (const string & label, address value)
{
	TRFUNC (tr, "AsmReal::doDEFL");
	TRSTREAM (tr, label << "= " << std::hex << value);

	bool islocal= setdefl (label, value);
	debout << label << "\t\tDEFL ";
	if (islocal)
		debout << "local ";
	debout << hex4 (value) << endl;
	showlisting ();
}

class MakePublic {
public:
	MakePublic (AsmReal & asmin_n) :
		asmin (asmin_n)
	{ }
	void operator () (const string & varname)
	{
		asmin.makepublic (stripdollar (varname) );
	}
private:
	AsmReal & asmin;
};


void AsmReal::doPUBLIC (const VarnameList & varnamelist)
{
	for_each (varnamelist.begin (), varnamelist.end (),
		MakePublic (* this) );

	debout << "\t\tPUBLIC ";
	putvarnamelist (debout, varnamelist);
	showlisting ();
}

#if 0
void AsmReal::doEND (address end, bool hasentry)
{
	TRFUNC (tr, "AsmReal::doEND vd");

	//debout << hex4 (current) << ":\t\tEND";
	debout << hex4 (mainmodule.getpos ().value) << ":\t\tEND";

	if (hasentry)
	{
		setentrypoint (end);
		debout << ' ' << hex4 (end) << endl;
	}
	end_reached= true;
	debout << endl;
	showlisting ();
}
#endif

void AsmReal::doEND ()
{
	TRFUNC (tr, "AsmReal::doEND");

	debout << hex4 (mainmodule.getpos ().value) << ":\t\tEND" << endl;
	end_reached= true;
	showlisting ();
}

void AsmReal::doEND (const VarData & vd)
{
	TRFUNC (tr, "AsmReal::doEND vd");

	const Value v= vd.getval ();
	debout << hex4 (mainmodule.getpos ().value) << ":\t\tEND " <<
		v << endl;

	//setentrypoint (v);
	mainmodule.setstartpos (v);

	end_reached= true;
	debout << endl;
	showlisting ();
}

class MakeExtern {
public:
	MakeExtern (AsmReal & asmin_n) :
		asmin (asmin_n)
	{ }
	void operator () (const string & varname)
	{
		asmin.makeextern (stripdollar (varname) );
	}
private:
	AsmReal & asmin;
};


void AsmReal::doEXTRN (const VarnameList & varnamelist)
{
	if (pass < 2)
	{
		for_each (varnamelist.begin (), varnamelist.end (),
			MakeExtern (* this) );
	}

	debout << "\t\tEXTRN ";
	putvarnamelist (debout, varnamelist);
	showlisting ();
}

class MakeLocal {
public:
	MakeLocal (LocalLevel & loc_n) :
		loc (loc_n)
	{ }
	void operator () (const string & varname)
	{
		loc.add (varname);
	}
private:
	LocalLevel & loc;
};

void AsmReal::doLOCAL (const VarnameList & varnamelist)
{
	LocalLevel * plocal= localstack.top ();
	for_each (varnamelist.begin (), varnamelist.end (),
		MakeLocal (* plocal) );
	debout << "\t\tLOCAL ";
	putvarnamelist (debout, varnamelist);
	showlisting ();
}

void AsmReal::doPROC ()
{
	TRFUNC (tr, "AsmReal::doPROC");

	finishautolocal ();
	ProcLevel * pproc= new ProcLevel (* this, getline () );
	localstack.push (pproc);

	showlisting ();
	debout << "\t\tPROC" << endl;
}

void AsmReal::doENDP ()
{
	TRFUNC (tr, "AsmReal::doENDP");

	finishautolocal ();
	if (localstack.empty () ||
		dynamic_cast <ProcLevel *> (localstack.top () ) == NULL)
	{
		throw UnbalancedENDP;
	}
	localstack.pop ();

	showlisting ();
	debout << "\t\tENDP" << endl;
}

void AsmReal::do_ERROR (const string & msg)
{
	showlisting ();
	throw ErrorDirective (msg);
}

void AsmReal::do_WARNING (const string & msg)
{
	showlisting ();
	emitwarning (msg);
}

bool AsmReal::setequorlabel (const string & varname, address value)
{
	enterorfinishautolocal (varname);

	switch (defvar (varname) )
	{
	case NoDefined:
		if (pass > 1)
			throw UndefinedInPass1 (varname);
		// Else nothing to do.
		break;
	case DefinedLiteral:
		throw logic_error ("Unexpected literal not evaluated");
	case DefinedDEFL:
		throw RedefinedDEFL (varname);
	case PreDefined:
		throw IsPredefined;
	case DefinedPass1:
		if (pass == 1)
			throw RedefinedEQU (varname);
		// Else nothing to do (this may chnage).
		break;
	case DefinedPass2:
		ASSERT (pass > 1);
		throw RedefinedEQU (varname);
	case DefinedExtern:
		throw RedefinedExtern (varname);
	}
	Defined def;
	switch (pass)
	{
	case 0:
		def= PreDefined; break;
	case 1:
		def= DefinedPass1; break;
	case 2:
		def= DefinedPass2; break;
	default:
		throw InvalidPassValue;
	}
	return setvardef (varname, value, def);
}

bool AsmReal::setlabel (const string & varname, const Value & v)
{
	TRFUNC (tr, "AsmReal::setlabel");

	enterorfinishautolocal (varname);

	switch (defvar (varname) )
	{
	case NoDefined:
		if (pass > 1)
			throw UndefinedInPass1 (varname);
		// Else nothing to do.
		break;
	case DefinedLiteral:
		throw logic_error ("Unexpected literal not evaluated");
	case DefinedDEFL:
		throw RedefinedDEFL (varname);
	case PreDefined:
		throw IsPredefined;
	case DefinedPass1:
		if (pass == 1)
			throw RedefinedEQU (varname);
		// Else nothing to do (this may chnage).
		break;
	case DefinedPass2:
		ASSERT (pass > 1);
		throw RedefinedEQU (varname);
	case DefinedExtern:
		throw RedefinedExtern (varname);
	}
	Defined def;
	switch (pass)
	{
	case 1:
		def= DefinedPass1; break;
	case 2:
		def= DefinedPass2; break;
	default:
		throw InvalidPassValue;
	}
	return setvardef (varname, VarData (v, def), def);
}

bool AsmReal::setequ (const string & varname, const VarData & vdata)
{
	TRFUNC (tr, "AsmReal::setequ");

	enterorfinishautolocal (varname);

	switch (defvar (varname) )
	{
	case NoDefined:
		if (pass > 1)
			throw UndefinedInPass1 (varname);
		// Else nothing to do.
		break;
	case DefinedLiteral:
		throw logic_error ("Unexpected literal not evaluated");
	case DefinedDEFL:
		throw RedefinedDEFL (varname);
	case PreDefined:
		throw IsPredefined;
	case DefinedPass1:
		if (pass == 1)
			throw RedefinedEQU (varname);
		// Else nothing to do (this may chnage).
		break;
	case DefinedPass2:
		ASSERT (pass > 1);
		throw RedefinedEQU (varname);
	case DefinedExtern:
		throw RedefinedExtern (varname);
	}
	Defined def;
	switch (pass)
	{
	case 0:
		def= PreDefined; break;
	case 1:
		def= DefinedPass1; break;
	case 2:
		def= DefinedPass2; break;
	default:
		throw InvalidPassValue;
	}
	//return setvardef (varname, value, def);
	return setvardef (varname, vdata, def);
}

bool AsmReal::setdefl (const string & varname, address value)
{
	TRFUNC (tr, "AsmReal::setdefl");

	enterorfinishautolocal (varname);

	switch (defvar (varname) )
	{
	case NoDefined:
		// Fine in this case.
		break;
	case DefinedLiteral:
		throw logic_error ("Unexpected literal in DEFL");
	case DefinedDEFL:
		// Fine also.
		break;
	case PreDefined:
	case DefinedPass1:
	case DefinedPass2:
		throw RedefinedEQU (varname);
	case DefinedExtern:
		throw RedefinedExtern (varname);
	}
	return setvardef (varname, value, DefinedDEFL);
}

void AsmReal::doLabel (const string & varname)
{
	#if 0
	//const address curpos= phased (current);
	const address curpos= phasedpos ();

	//bool islocal= setlabel (varname, curpos);
	// Provisional
	Value vpos (phasing ? ValueAbsolute : ValueProgRelative, curpos);
	#endif

	const Value vpos= phasedpos ();
	const address curpos= vpos.value;

	bool islocal= setlabel (varname, vpos);

	debout << hex4 (currentpos ().value) << ":\t\t";
	if (islocal)
		debout << "LOCAL ";
	debout << varname << ": " << hex4 (curpos) << endl;
}

void AsmReal::doENDM ()
{
	TRFUNC (tr, "AsmReal::doENDM");

	MacroFrameBase * pframe= getmframe ();
	if (pframe)
	{
		--macrolevel;
		showlisting ();
		if (pframe->iterate () )
		{
			debout << "\tNext " <<pframe->name () << endl;
			setline (pframe->getexpline () );
		}
		else
		{
			debout << "\tEnd of " << pframe->name () <<
				endl;
			size_t retline= pframe->returnline ();
			delete pframe;
			if (retline != size_t (-1) )
				setline (retline);
		}
	}
	else
		throw ENDMwithoutMacro;
}

void AsmReal::doEXITM ()
{
	TRFUNC (tr, "AsmReal::doEXITM");

	MacroFrameBase * pframe= getmframe ();
	if (! pframe)
		throw runtime_error ("EXITM without macro");
	else
	{
		debout << '\t' << pframe->name () << " EXITM" << endl;
		--macrolevel;
		showlisting ();
		size_t retline= pframe->returnline ();
		delete pframe;
		if (retline != size_t (-1) )
			setline (retline);
		else
			gotoENDM ();
	}
}

void AsmReal::do_SHIFT ()
{
	MacroFrameBase * pframe= getmframe ();
	if (! pframe)
		throw ShiftOutsideMacro;
	else
		pframe->shift ();
}

void AsmReal::doMACRO (const string & name, const vector <string> & param)
{
	debout << "\tDefining MACRO " << name << endl;
	ASSERT (! name.empty () );

	if (opt.autolocal)
	{
		finishautolocal ();
		verifynoautolocal (name);
	}
	showlisting ();

	// Skip macro body.

	size_t macroline= getline ();
	gotoENDM ();
	debout << "\tEnd definition of MACRO " << name << endl;

	// Store the macro definition.
	//mapmacro.insert (name, param, macroline, getline () );
	MacroStore::insert (name, param, macroline, getline () );
}

void AsmReal::emitwarning (const string & text)
{
	showcurrentlineinfo (warnout);
	warnout << " WARNING: " << text << endl;
}

void AsmReal::warningUglyInstruction ()
{
	emitwarning ("Looks like a non existent instruction");
}

void AsmReal::parse_error (const string & errmsg)
{
	throw runtime_error (errmsg);
}

void AsmReal::no8080 ()
{
	if (opt.warn8080)
		emitwarning ("not a 8080 instruction");
}

void AsmReal::no86 ()
{
	if (opt.mode86)
		throw NotValid86;
}

void AsmReal::doByteInst (TypeByteInst ti, regbCode reg,
	byte prefix, bool hasdesp, byte desp)
{
	if (prefix != prefixNone)
	{
		no86 ();
		gencode (prefix);
	}
	if (opt.mode86)
	{
		ASSERT (! hasdesp);
		byte basecode= getbaseByteInst (ti, gen86);
		byte code;
		if (reg == reg_HL_)
		{
			basecode+= 2;
			code= 7;
		}
		else
			code= 0xC0 |  (getregb86 (reg) << 3);
		gencode (basecode, code);
	}
	else
	{
		byte code= getbaseByteInst (ti, gen80) | reg;
		gencode (code);
	}
	if (hasdesp)
		gencode (desp);

	switch (asmmode)
	{
	case AsmZ80:
		showcode (byteinstName (ti) + ' ' +
			getregbname (reg, prefix, hasdesp, desp) );
		break;
	case Asm8080:
		ASSERT (prefix == prefixNone);
		ASSERT (! hasdesp);
		showcode (byteinst8080Name (ti) + ' ' + getregb8080name (reg) );
		break;
	}
	if (prefix != prefixNone)
		no8080 ();
	showlisting ();
}

void AsmReal::doByteInmediate (TypeByteInst ti, byte bvalue)
{
	gencode (getByteInstInmediate (ti, genmode), bvalue);
	switch (asmmode)
	{
	case AsmZ80:
		showcode (byteinstName (ti) + ' ' + hex2str (bvalue) );
		break;
	case Asm8080:
		showcode (byteinst8080iName (ti) + ' ' + hex2str (bvalue) );
		break;
	}
	showlisting ();
}

void AsmReal::doByteInstCB (byte codereg, regbCode reg,
	byte prefix, bool hasdesp, byte desp)
{
	TRFUNC (tr, "AsmReal::doByteInstCB");
	TRSTREAM (tr, "Code: " << hex2str (codereg) <<
		" Reg: " << static_cast <int> (reg) );

	no86 ();

	string instrname;
	switch (codereg)
	{
	case codeRL:  instrname= "RL"; break;
	case codeRLC: instrname= "RLC"; break;
	case codeRR:  instrname= "RR"; break;
	case codeRRC: instrname= "RRC"; break;
	case codeSLA: instrname= "SLA"; break;
	case codeSRA: instrname= "SRA"; break;
	case codeSRL: instrname= "SRL"; break;
	case codeSLL: instrname= "SLL"; break;
	default:
		{
			byte codeaux= codereg & 0xC0;
			byte nbit= (codereg & 38) >> 3;
			ostringstream oss;
			switch (codeaux)
			{
			case codeBIT: oss << "BIT"; break;
			case codeRES: oss << "RES"; break;
			case codeSET: oss << "SET"; break;
			default:
				throw logic_error ("Invalid byte CD code");
			}
			oss << ' ' << static_cast <int> (nbit) << ',';
			instrname= oss.str ();
		}
	}

	if (prefix != prefixNone)
		gencode (prefix);
	gencode (0xCB);
	if (hasdesp)
		gencode (desp);
	byte code= codereg + reg;
	gencode (code);

	showcode (instrname + ' ' +
		getregbname (reg, prefix, hasdesp, desp) );
	no8080 ();
	showlisting ();
}

void AsmReal::doNoargInst (TypeToken tt)
{
	#if 0
	simpleinst_t::iterator it= simpleinst.find (tt);
	if (it == simpleinst.end () )
		throw logic_error ("Invalid no arg instruction");

	const SimpleInst & si= it->second;
	#else
	const SimpleInst & si= SimpleInst::get (tt);
	#endif

	if (opt.mode86)
	{
		#if 0
		if (! si.valid86)
			no86 ();
		if (si.code86_1 != 0)
		{
			gencode (si.code86_1, si.code86_2);
		}
		else
		{
			if (si.code86_2 != 0)
				gencode (si.code86_2);
		}
		gencode (si.code86_3);
		#else

		if (! si.is86 () )
			no86 ();
		vector <byte> code= si.getcode86 ();
		for (size_t i= 0; i < code.size (); ++i)
			gencode (code [i] );

		#endif
	}
	else
	{
		#if 0
		if (si.edprefix)
			gencodeED (si.code);
		else
			gencode (si.code);
		#else
		if (si.isED () )
			gencodeED (si.getcode () );
		else
			gencode (si.getcode () );
		#endif
	}

	showcode (gettokenname (tt) );

	//if (! si.valid8080)
	if (si.is8080 () )
		no8080 ();

	showlisting ();
}

void AsmReal::doIM (address v)
{
	byte code;
	switch (v)
	{
	case 0:
		code= codeIM_0; break;
	case 1:
		code= codeIM_1; break;
	case 2:
		code= codeIM_2; break;
	default:
		throw InvalidValueIM;
	}

	no86 ();
	gencodeED (code);

	showcode (string ("IM ") + static_cast <char> ('0' + v) );
	no8080 ();
	showlisting ();
}

void AsmReal::doRST (address addr)
{
	byte valaddr= lobyte (addr);
	switch (asmmode)
	{
	case AsmZ80:
		if (addr & ~ static_cast <address> (0x38) )
			throw InvalidValueRST;
		break;
	case Asm8080:
		if (addr > 7)
			throw InvalidValueRST;
		addr*= 8;
	}

	//no86 ();
	if (opt.mode86)
	{
		doCALL (VarData (addr, PreDefined) );
	}
	else
	{
		gencode (codeRST00 + lobyte (addr) );
	}

	showcode ("RST " + hex2str (valaddr) );
	showlisting ();
}

void AsmReal::doLD_SP_HL ()
{
	if (opt.mode86)
		gencode (0x89, 0xDC);
	else
		gencode (codeLD_SP_HL);
	showcode ("LD SP, HL");
	showlisting ();
}

void AsmReal::doLD_SP_IX ()
{
	no86 ();
	gencode (prefixIX, codeLD_SP_HL);
	showcode ("LD SP, IX");
	no8080 ();
	showlisting ();
}

void AsmReal::doLD_SP_IY ()
{
	no86 ();
	gencode (prefixIY, codeLD_SP_HL);
	showcode ("LD SP, IY");
	no8080 ();
	showlisting ();
}

//void AsmReal::doLD_SP_nn (address value)
void AsmReal::doLD_SP_nn (const VarData & value)
{
	gencode (opt.mode86 ? 0xBC : 0x31);
	gencodeword (value);
	showcode ("LD SP, " + hex4str (value.getvalue () ) );
	showlisting ();
}

//void AsmReal::doLD_SP_indexp (address value)
void AsmReal::doLD_SP_indexp (const VarData & value)
{
	if (opt.mode86)
		gencode (0x8B, 0x26);
	else
		gencodeED (0x7B);
	gencodeword (value);

	showcode ("LD SP, [" + hex4str (value.getvalue () ) + closeIndir);
	no8080 ();
	showlisting ();
}

#if 0
void AsmReal::doLD_HL_nn (address value)
{
	if (opt.mode86)
		gencode (0xBB);
	else
		gencode (codeLD_HL_nn);
	gencodeword (value);
	switch (asmmode)
	{
	case AsmZ80:  showcode ("LD HL, " + hex4str (value) ); break;
	case Asm8080: showcode ("LXI H, " + hex4str (value) );
	}
	showlisting ();
}
#endif

void AsmReal::doLD_HL_nn (const VarData & value)
{
	if (opt.mode86)
		gencode (0xBB);
	else
		gencode (codeLD_HL_nn);
	gencodeword (value);
	switch (asmmode)
	{
	case AsmZ80:
		showcode ("LD HL, " + hex4str (value.getvalue () ) );
		break;
	case Asm8080:
		showcode ("LXI H, " + hex4str (value.getvalue () ) );
		break;
	}
	showlisting ();
}

//void AsmReal::doLD_HL_indexp (address value)
void AsmReal::doLD_HL_indexp (const VarData & vd)
{
	if (opt.mode86)
		gencode (0x8B, 0x1E);
	else
		gencode (codeLD_HL_indexp);
	gencodeword (vd);

	address value= vd.getvalue ();
	switch (asmmode)
	{
	case AsmZ80:  showcode ("LD HL, [" + hex4str (value) + ']'); break;
	case Asm8080: showcode ("LHLD " + hex4str (value) );
	}
	showlisting ();
}

//void AsmReal::doLD_rr_nn (regwCode regcode, byte prefix, address value)
void AsmReal::doLD_rr_nn (regwCode regcode, const VarData & addr)
{
	TRFUNC (tr, "AsmReal::doLD_rr_nn");
	TRSTREAM (tr, "Code " << regcode <<
		" value " << hex4str (addr.getvalue () ) );

	#if 0
	if (prefix != prefixNone)
	{
		gencode (prefix);
		no86 ();
	}
	#endif

	byte code;
	if (opt.mode86)
		code= regcode + 0xB9;
	else
		code= regcode * 16 + 1;
	gencode (code);

	//gencodeword (addr.getvalue () );
	gencodeword (addr);

	switch (asmmode)
	{
	case AsmZ80:
		showcode ("LD " + regwName (regcode, nameSP) +
			", " + hex4str (addr.getvalue () ) );
		break;
	case Asm8080:
		showcode ("LXI " + regw8080Name (regcode) +
			", " + hex4str (addr.getvalue () ) );
		break;
	}

	//if (prefix != prefixNone)
	//	no8080 ();
	showlisting ();
}

//void AsmReal::doLD_rr_indexp (regwCode regcode, byte prefix, address value)
void AsmReal::doLD_rr_indexp (regwCode regcode, const VarData & addr)
{
	TRFUNC (tr, "AsmReal::doLD_rr_indexp");
	TRSTREAM (tr, "Code " << regcode <<
		" value " << hex4str (addr.getvalue () ) );

	bool valid8080= false;

	#if 0
	switch (regcode)
	{
	case regBC:
		if (opt.mode86)
			gencode (0x8B, 0x0E);
		else
			gencodeED (0x4B);
		gencodeword (addr);
		break;
	case regDE:
		if (opt.mode86)
			gencode (0x8B, 0x16);
		else
			gencodeED (0x5B);
		gencodeword (addr);
		break;
	case regHL:
		if (prefix == prefixNone)
			valid8080= true;
		else
		{
			no86 ();
			gencode (prefix);
		}
		if (opt.mode86)
			gencode (0x8B, 0x1E);
		else
			gencode (0x2A);
		gencodeword (addr);
		break;
	default:
		throw logic_error ("Invalid reg in doLD_rr_indexp");
	}
	#else
	if (opt.mode86)
	{
		switch (regcode)
		{
		case regBC:
			gencode (0x8B, 0x0E);
			break;
		case regDE:
			gencode (0x8B, 0x16);
			break;
		case regHL:
			//if (prefix == prefixNone)
				valid8080= true;
			//else
			//{
			//	no86 ();
			//	gencode (prefix);
			//}
			gencode (0x8B, 0x1E);
			break;
		default:
			throw logic_error ("Invalid reg in doLD_rr_indexp");
		}
	}
	else
	{
		switch (regcode)
		{
		case regBC:
			gencodeED (0x4B);
			break;
		case regDE:
			gencodeED (0x5B);
			break;
		case regHL:
			//if (prefix == prefixNone)
				valid8080= true;
			//else
			//{
			//	no86 ();
			//	gencode (prefix);
			//}
			gencode (0x2A);
			break;
		default:
			throw logic_error ("Invalid reg in doLD_rr_indexp");
		}
	}
	gencodeword (addr);
	#endif

	showcode ("LD " + regwName (regcode, nameSP) + ", "
		+ openIndir + hex4str (addr.getvalue () ) + closeIndir);
	if (! valid8080)
		no8080 ();
	showlisting ();
}

void AsmReal::doLD_IXY_nn (byte prefix, const VarData & addr)
{
	TRFUNC (tr, "AsmReal::doLD_IXY_nn");
	TRSTREAM (tr, "Prefix " << hex2str (prefix) <<
		" value " << hex4str (addr.getvalue () ) );

	ASSERT (prefix == prefixIX || prefix == prefixIY);

	no86 ();

	gencode (prefix);
	gencode (0x21);
	gencodeword (addr);

	showcode ("LD " + nameHLpref (prefix) +
		", " + hex4str (addr.getvalue () ) );

	no8080 ();
	showlisting ();
}

void AsmReal::doLD_IXY_indexp (byte prefix,
	const VarData & addr)
{
	TRFUNC (tr, "AsmReal::doLD_IXY_indexp_");
	TRSTREAM (tr, "Prefix " << hex2str (prefix) <<
		" value " << hex4str (addr.getvalue () ) );

	ASSERT (prefix == prefixIX || prefix == prefixIY);

	no86 ();

	gencode (prefix);
	gencode (0x2A);
	gencodeword (addr);

	showcode ("LD " + nameHLpref (prefix) + ", "
		+ openIndir + hex4str (addr.getvalue () ) + closeIndir);

	no8080 ();
	showlisting ();
}

void AsmReal::doLD_indexp_A (const VarData & vd)
{
	byte code= opt.mode86 ? 0xA2 : 0x32;
	gencode (code);
	gencodeword (vd);
	switch (asmmode)
	{
	case AsmZ80:
		showcode ("LD [" + hex4str (vd.getvalue () ) + "],A");
		break;
	case Asm8080:
		showcode ("STA " + hex4str (vd.getvalue () ) );
		break;
	}
	showlisting ();
}

void AsmReal::doLD_indexp_BC (const VarData & vd)
{
	if (opt.mode86)
		gencode (0x89, 0x0E);
	else
		gencodeED (0x43);
	gencodeword (vd);
	showcode ("LD [" + hex4str (vd.getvalue () ) + "],BC");
	no8080 ();
	showlisting ();
}

void AsmReal::doLD_indexp_DE (const VarData & vd)
{
	if (opt.mode86)
		gencode (0x89, 0x16);
	else
		gencodeED (0x53);
	gencodeword (vd);
	showcode ("LD [" + hex4str (vd.getvalue () ) + "],DE");
	no8080 ();
	showlisting ();
}

void AsmReal::doLD_indexp_HL (const VarData & vd)
{
	if (opt.mode86)
		gencode (0x89, 0x1E);
	else
		gencode (codeLD_indexp_HL);
	gencodeword (vd);
	switch (asmmode)
	{
	case AsmZ80:
		showcode ("LD [" + hex4str (vd.getvalue () ) + "],HL");
		break;
	case Asm8080:
		showcode ("SHLD " + hex4str (vd.getvalue () ) );
		break;
	}
	showlisting ();
}

void AsmReal::doLD_indexp_SP (const VarData & vd)
{
	if (opt.mode86)
		gencode (0x89, 0x26);
	else
		gencodeED (0x73);
	gencodeword (vd);
	showcode ("LD [" + hex4str (vd.getvalue () ) + "],BC");
	no8080 ();
	showlisting ();
}

void AsmReal::doLD_indexp_IX (const VarData & vd)
{
	no86 ();
	gencode (prefixIX, codeLD_indexp_HL);
	gencodeword (vd);
	showcode ("LD [" + hex4str (vd.getvalue () ) + "],IX");
	no8080 ();
	showlisting ();
}

void AsmReal::doLD_indexp_IY (const VarData & vd)
{
	no86 ();
	gencode (prefixIY, codeLD_indexp_HL);
	gencodeword (vd);
	showcode ("LD [" + hex4str (vd.getvalue () ) + "],IY");
	no8080 ();
	showlisting ();
}

void AsmReal::doLDir (byte code)
{
	string args ("LD ");
	switch (code)
	{
	case codeLD_A_I: args+= "A, I"; break;
	case codeLD_A_R: args+= "A, R"; break;
	case codeLD_I_A: args+= "I, A"; break;
	case codeLD_R_A: args+= "R, A"; break;
	default:
		throw logic_error ("Invalid LDir call");
	}
	no86 ();
	gencodeED (code);
	showcode (args);
	no8080 ();
	showlisting ();
}

void AsmReal::doLD_r_r (regbCode reg1, regbCode reg2)
{
	regbCode rr1= reg1;
	regbCode rr2= reg2;
	if (opt.mode86)
	{
		byte precode;
		byte code= 0xC0;
		if (reg2 == reg_HL_)
		{
			precode= 0x8A;
			code= 0x00;
			reg2= regH;
		}
		else if (reg1 == reg_HL_)
		{
			precode= 0x88;
			code= 0x00;
			reg1= reg2;
			reg2= regH;
		}
		else
			precode= 0x8A;

		code+= (getregb86 (reg1) << 3) +
			getregb86 (reg2);
		gencode (precode, code);
	}
	else
	{
		byte code= 0x40 + (reg1 << 3) + reg2;
		gencode (code);
	}

	string inst;
	switch (asmmode)
	{
	case AsmZ80:
		inst= "LD " + getregbname (rr1) +
			", " + getregbname (rr2);
		break;
	case Asm8080:
		inst= "MOV " + getregb8080name (rr1) +
			", " + getregb8080name (rr2);
		break;
	}
	showcode (inst);
	showlisting ();
}

void AsmReal::doLD_r_n (regbCode reg, byte n)
{
	if (opt.mode86)
	{
		switch (reg)
		{
		case reg_HL_:
			gencode (0xC6, 0x07);
			break;
		default:
			//gencode (0xC6, 0xB0 + getregb86 (reg) );
			gencode (0xB0 + getregb86 (reg) );
		}
	}
	else
		gencode ( (reg << 3) + 0x06);
	gencode (n);
	string inst;
	switch (asmmode)
	{
	case AsmZ80:
		inst= "LD " + getregbname (reg);
		break;
	case Asm8080:
		inst= "MVI " + getregb8080name (reg);
		break;
	}
	showcode (inst + ',' + hex2str (n) );
	showlisting ();
}

void AsmReal::doLD_r_undoc (regbCode reg1, regbCode reg2, byte prefix)
{
	ASSERT (prefix != prefixNone);
	no86 ();
	gencode (prefix, 0x40 + (reg1 << 3) + reg2);
	showcode ("LD " + getregbname (reg1) + ", " + getregbname (reg2, prefix) );
	showlisting ();
}

void AsmReal::doLD_r_idesp (regbCode reg1, byte prefix, byte desp)
{
	ASSERT (prefix == prefixIX || prefix == prefixIY);
	no86 ();
	gencode (prefix, 0x40 + (reg1 << 3) + reg_HL_, desp);
	showcode ("LD " + getregbname (reg1) + ", " +
		getregbname (reg_HL_, prefix, true, desp) );
	showlisting ();
}

void AsmReal::doLD_undoc_r (regbCode reg1, byte prefix, regbCode reg2)
{
	ASSERT (prefix != prefixNone);
	no86 ();
	gencode (prefix, 0x40 + (reg1 << 3) + reg2);
	showcode ("LD " + getregbname (reg1, prefix) + ", " +
		getregbname (reg2, prefix) );
	showlisting ();
}

void AsmReal::doLD_undoc_n (regbCode reg, byte prefix, byte n)
{
	ASSERT (prefix != prefixNone);
	no86 ();
	gencode (prefix, (reg << 3) + 0x06, n);
	showcode ("LD " + getregbname (reg, prefix) + ',' + hex2str (n) );
	no8080 ();
	showlisting ();
}

void AsmReal::doLD_idesp_r (byte prefix, byte desp, regbCode reg2)
{
	ASSERT (prefix == prefixIX || prefix == prefixIY);
	no86 ();
	gencode (prefix, 0x40 + (reg_HL_ << 3) + reg2, desp);
	showcode ("LD " + getregbname (reg_HL_, prefix, true, desp) +
		", " + getregbname (reg2) );
}

void AsmReal::doLD_idesp_n (byte prefix, byte desp, byte n)
{
	ASSERT (prefix == prefixIX || prefix == prefixIY);
	no86 ();

	gencode (prefix, 0x36, desp, n);
	showcode ("LD " + nameIdesp (prefix, true, desp) + ", " +
		hex2str (n) );
	no8080 ();
	showlisting ();
}

void AsmReal::doLD_A_indexp (const VarData & vd)
{
	byte code= opt.mode86 ? 0xA0 : 0x3A;
	gencode (code);
	gencodeword (vd);

	switch (asmmode)
	{
	case AsmZ80:
		showcode ("LD A, [" + hex4str (vd.getvalue () ) + closeIndir);
		break;
	case Asm8080:
		showcode ("LDA " + hex4str (vd.getvalue () ) );
		break;
	}
	showlisting ();
}

void AsmReal::doLD_A_indBC ()
{
	if (opt.mode86)
	{
		// MOV SI,CX ; MOV AL,[SI]
		gencode (0x89, 0xCE, 0x8A, 0x04);
	}
	else
		gencode (0x0A);
	switch (asmmode)
	{
	case AsmZ80:  showcode ("LD A, [BC]"); break;
	case Asm8080: showcode ("LDAX B"); break;
	}
	showlisting ();
}

void AsmReal::doLD_A_indDE ()
{
	if (opt.mode86)
	{
		// MOV SI,DX ; MOV AL,[SI]
		gencode (0x89, 0xD6, 0x8A, 0x04);
	}
	else
		gencode (0x1A);
	switch (asmmode)
	{
	case AsmZ80:  showcode ("LD [DE], A"); break;
	case Asm8080: showcode ("LDAX D"); break;
	}
	showlisting ();
}

void AsmReal::doLD_indBC_A ()
{
	if (opt.mode86)
	{
		// MOV SI,CX ; MOV [SI],AL
		gencode (0x89, 0xCE, 0x88, 0x04);
	}
	else
		gencode (0x02);
	switch (asmmode)
	{
	case AsmZ80:  showcode ("LD [BC], A"); break;
	case Asm8080: showcode ("STAX B"); break;
	}
	showlisting ();
}

void AsmReal::doLD_indDE_A ()
{
	if (opt.mode86)
	{
		// MOV SI,DX ; MOV [SI],AL
		gencode (0x89, 0xD6, 0x88, 0x04);
	}
	else
		gencode (0x12);
	switch (asmmode)
	{
	case AsmZ80:  showcode ("LD [DE], A"); break;
	case Asm8080: showcode ("STAX D"); break;
	}	
	showlisting ();
}

void AsmReal::doADDADCSBC_HL (byte basecode, regwCode reg, byte prefix)
{
	if (prefix != prefixNone)
	{
		no86 ();
		gencode (prefix);
	}
	if (opt.mode86)
	{
		byte code;
		switch (basecode)
		{
		case codeADDHL:
			code= (reg << 3) + 0xCB;
			gencode (0x01, code);
			break;
		case codeADCHL:
			code= (reg << 3) + 0xCB;
			gencode (0x11, code);
			break;
		case codeSBCHL:
			code= (reg << 3) + 0xCB;
			gencode (0x19, code);
			break;
		default:
			ASSERT (false);
			throw UnexpectedRegisterCode;
		}
	}
	else
	{
		if (basecode == codeSBCHL || basecode == codeADCHL)
			gencode (0xED);
		byte code= (reg << 4) + basecode;
		gencode (code);
	}

	string inst;
	switch (asmmode)
	{
	case AsmZ80:
		switch (basecode)
		{
		case codeADDHL: inst= "ADD"; break;
		case codeADCHL: inst= "ADC"; break;
		case codeSBCHL: inst= "SBC"; break;
		default:
			throw UnexpectedRegisterCode;
		}
		inst+= ' ';
		inst+= nameHLpref (prefix);
		inst+= ", ";
		inst+= regwName (reg, nameSP, reg == regHL ? prefix : prefixNone);
		break;
	case Asm8080:
		switch (basecode)
		{
		case codeADDHL: inst= "DAD"; break;
		case codeADCHL: inst= "ADC"; break;
		case codeSBCHL: inst= "SBC"; break;
		default:
			throw UnexpectedRegisterCode;
		}
		inst+= ' ';
		inst+= regw8080Name (reg);
		break;
	}
	showcode (inst);
	if (basecode == codeADCHL || basecode == codeSBCHL ||
		prefix != prefixNone)
	{
		no8080 ();
	}
	showlisting ();
}

// Push and pop codes:
//	push bc -> C5
//	push de -> D5
//	push hl -> E5
//	push af -> F5
//	push ix -> DD E5
//	push iy -> FD E5
//	pop bc -> C1
//	pop de -> D1
//	pop hl -> E1
//	pop af -> F1
//	pop ix -> DD E1
//	pop iy -> FD E1

void AsmReal::doPUSHPOP (regwCode reg, byte prefix, bool isPUSH)
{
	if (prefix != prefixNone)
	{
		no86 ();
		gencode (prefix);
	}

	const byte code= opt.mode86 ?
		(isPUSH ? codePUSH_AX : codePOP_AX) + ( (reg + 1) % 4) :
		(isPUSH ? 0xC5 : 0xC1) + (reg << 4);

	switch (code) {
	case codePUSH_AX:
		ASSERT (opt.mode86 && isPUSH);
		gencode (0x9F, 0x86, 0xC4); // LAHF ; XCHG AL,AH
		gencode (codePUSH_AX);
		gencode (0x86, 0xC4);       // XCHG AL,AH
		break;
	case codePOP_AX:
		ASSERT (opt.mode86 && ! isPUSH);
		gencode (codePOP_AX);
		gencode (0x86, 0xC4, 0x9E); // XCHG AL, AH ; SAHF
		break;
	default:
		gencode (code);
	}

	string inst (isPUSH ? "PUSH " : "POP ");
	switch (asmmode)
	{
	case AsmZ80:
		inst+= regwName (reg, nameAF, prefix);
		break;
	case Asm8080:
		ASSERT (prefix == prefixNone);
		inst+= regw8080NamePSW (reg);
		break;
	}
	showcode (inst);
	if (prefix != prefixNone)
		no8080 ();
	showlisting ();
}

// CALL codes
// call NN     -> CD
// call nz, NN -> C4
// call z, NN  -> CC
// call nc, NN -> D4
// call c, NN  -> DC
// call po, NN -> E4
// call pe, NN -> EC
// call p, NN  -> F4
// call m, NN  -> FC

#if 0
void AsmReal::genCALL (byte code, address addr)
{
	if (opt.mode86)
	{
		//const address curinst= phased (currentinstruction);
		const address curinst= phased (currentinstruction).value;
		if (code == 0xE8)
		{
			//address offset= addr - (currentinstruction + 3);
			address offset= addr - (curinst + 3);
			gencode (0xE8);
			gencodeword (offset);
		}
		else
		{
			// Generate a conditional jump with the
			// opposite condition to the following
			// instruction, followed by a call to
			// the destination.
			//address offset= addr - (currentinstruction + 5);
			address offset= addr - (curinst + 5);
			gencode (code, 0x03, 0xE8);
			gencodeword (offset);
		}
	}
	else
	{
		gencode (code);
		gencodeword (addr);
	}

}
#endif

void AsmReal::genCALL (byte code, const VarData & addr)
{
	TRFUNC (tr, "AsmReal::genCALL");

	if (opt.mode86)
	{
		// TODO: the linker must handle 8086

		//if (opt.getObjectType () == ObjectRel && addr.isextern () )
		//	no86 ();

		//const address curinst= phased (currentinstruction);
		const address curinst= phased (currentinstruction).value;
		if (code == 0xE8)
		{
			//address offset= addr - (currentinstruction + 3);
			//address offset= addr - (curinst + 3);
			address offset= addr.getvalue () - (curinst + 3);
			gencode (0xE8);
			gencodeword (offset);
		}
		else
		{
			// Generate a conditional jump with the
			// opposite condition to the following
			// instruction, followed by a call to
			// the destination.
			//address offset= addr - (currentinstruction + 5);
			//address offset= addr - (curinst + 5);
			address offset= addr.getvalue () - (curinst + 5);
			gencode (code, 0x03, 0xE8);
			gencodeword (offset);
		}
	}
	else
	{
		gencode (code);
		gencodeword (addr);
	}

}

//void AsmReal::doCALL (address addr)
void AsmReal::doCALL (const VarData & addr)
{
	TRFUNC (tr, "AsmReal::doCALL");

	byte code= opt.mode86 ? 0xE8 : 0xCD;
	//genCALL (code, addr.getvalue () );
	genCALL (code, addr);
	showcode ("CALL " + hex4str (addr.getvalue () ) );
	showlisting ();
}

void AsmReal::doCALL_flag (flagCode fcode, const VarData & vd)
{
	byte code;
	if (opt.mode86)
	{
		code= invertflag86 (getflag86 (fcode) ) | 0x70;
	}
	else
	{
		code= (fcode << 3) | 0xC4;
	}

	genCALL (code, vd);
	string inst (1, 'C');
	switch (asmmode)
	{
	case AsmZ80:
		inst+= "ALL ";
		inst+= getflagname (fcode);
		inst+= ',';
		break;
	case Asm8080:
		inst+= getflagname (fcode);
		break;
	}
	inst+= ' ';
	inst+= hex4str (vd.getvalue () );
	showcode (inst);
	showlisting ();
}

void AsmReal::doRET ()
{
	byte code= opt.mode86 ? codeRET_86 : codeRET;
	gencode (code);
	showcode ("RET");
	showlisting ();
}

void AsmReal::doRETflag (flagCode fcode)
{
	byte code;
	if (opt.mode86)
	{
		// Generate a conditional jump with the opposite
		// condition to the following instruction,
		// followed by a RET.
		code= invertflag86 (getflag86 (fcode) ) | 0x70;
		gencode (code, 0x01, codeRET_86);
	}
	else
	{
		code= (fcode << 3) | 0xC0;
		gencode (code);
	}
	showcode ("RET " + getflagname (fcode) );
	showlisting ();
}

// JP codes
// jp NN     -> C3
// jp (hl)   -> E9
// jp nz, NN -> C2
// jp z,  NN -> CA
// jp nc, NN -> D2
// jp c, NN  -> DA
// jp po, NN -> E2
// jp pe, NN -> EA
// jp p, NN  -> F2
// jp m, NN  -> FA

#if 0
void AsmReal::genJP (byte code, address addr)
{
	if (opt.mode86)
	{
		const address curinst= phased (currentinstruction);
		if (code == 0xE9)
		{
			address offset= addr - (curinst + 3);
			gencode (0xE9);
			gencodeword (offset);
		}
		else
		{
			// Generate a conditional jump with the
			// opposite condition to the following
			// instruction, followed by a jump to
			// the destination.
			// TODO: optimize this in cases that the
			// destination is known and is in range.
			address offset= addr - (curinst + 5);
			gencode (code, 0x03, 0xE9);
			gencodeword (offset);
		}
	}
	else
	{
		gencode (code);
		gencodeword (addr);
	}
}
#endif

void AsmReal::genJP (byte code, const VarData & addr)
{
	if (opt.mode86)
	{
		//const address curinst= phased (currentinstruction);
		const address curinst= phased (currentinstruction).value;
		if (code == 0xE9)
		{
			address offset= addr.getvalue () - (curinst + 3);
			gencode (0xE9);
			gencodeword (offset);
		}
		else
		{
			// Generate a conditional jump with the
			// opposite condition to the following
			// instruction, followed by a jump to
			// the destination.
			// TODO: optimize this in cases that the
			// destination is known and is in range.
			address offset= addr.getvalue () - (curinst + 5);
			gencode (code, 0x03, 0xE9);
			gencodeword (offset);
		}
	}
	else
	{
		gencode (code);
		//gencodeword (addr.getvalue () );
		gencodeword (addr);
	}
}

#if 0
void AsmReal::doJP (address addr)
{
	byte code= opt.mode86 ? codeJP_86 : codeJP;
	genJP (code, addr);
	string inst;
	switch (asmmode)
	{
	case AsmZ80:  inst= "JP "; break;
	case Asm8080: inst= "JMP "; break;
	}
	showcode (inst + hex4str (addr) );
	showlisting ();
}
#endif

void AsmReal::doJP (const VarData & addr)
{
	byte code= opt.mode86 ? codeJP_86 : codeJP;
	genJP (code, addr);
	string inst;
	switch (asmmode)
	{
	case AsmZ80:  inst= "JP "; break;
	case Asm8080: inst= "JMP "; break;
	}
	showcode (inst + hex4str (addr.getvalue () ) );
	showlisting ();
}

void AsmReal::doJP_indHL ()
{
	if (opt.mode86)
		gencode (0xFF, 0xE3);
	else
		gencode (codeJP_indHL);
	showcode ("JP [HL]");
	showlisting ();
}

void AsmReal::doJP_indIX ()
{
	no86 ();
	gencode (prefixIX, codeJP_indHL);
	showcode ("JP [IX]");
	showlisting ();
}

void AsmReal::doJP_indIY ()
{
	no86 ();
	gencode (prefixIY, codeJP_indHL);
	showcode ("JP [IY]");
	showlisting ();
}

#if 0
void AsmReal::doJP_flag (flagCode fcode, address addr)
{
	byte code;
	if (opt.mode86)
	{
		code= invertflag86 (getflag86 (fcode) ) | 0x70;
	}
	else
		code= (fcode << 3) | 0xC2;
	genJP (code, addr);
	string inst (1, 'J');
	switch (asmmode)
	{
	case AsmZ80:
		inst+= "P ";
		inst+= getflagname (fcode);
		inst+= ',';
		break;
	case Asm8080:
		inst+= getflagname (fcode);
		break;
	}
	inst+= ' ';
	inst+= hex4str (addr);
	showcode (inst);
	showlisting ();
}
#endif

void AsmReal::doJP_flag (flagCode fcode, const VarData & addr)
{
	byte code;
	if (opt.mode86)
	{
		code= invertflag86 (getflag86 (fcode) ) | 0x70;
	}
	else
		code= (fcode << 3) | 0xC2;
	genJP (code, addr);
	string inst (1, 'J');
	switch (asmmode)
	{
	case AsmZ80:
		inst+= "P ";
		inst+= getflagname (fcode);
		inst+= ',';
		break;
	case Asm8080:
		inst+= getflagname (fcode);
		break;
	}
	inst+= ' ';
	inst+= hex4str (addr.getvalue () );
	showcode (inst);
	showlisting ();
}

void AsmReal::doRelative (byte code, address addr, const string instrname)
{
	int dif= 0;
	if (pass >= 2)
	{
		//address curpos= phased (current);
		//address curpos= phasedpos ();
		address curpos= phasedpos ().value;
		dif= addr - (curpos + 2);
		if (dif > 127 || dif < -128)
		{
			debout << "addr= " << addr <<
				" current= " << curpos <<
				" dif= " << dif << endl;
			throw RelativeOutOfRange;
		}
	}
	signed char reldesp = static_cast <signed char> (dif);

	gencode (code, reldesp);

	showcode (instrname + ' ' + hex4str (addr) );
	no8080 ();
	showlisting ();
}

void AsmReal::doRelative (byte code, const VarData & vd,
	const string instrname)
{
	int dif= 0;
	if (pass >= 2)
	{
		//address curpos= phased (current);
		//address curpos= phasedpos ();
		//address curpos= phasedpos ().value;
		//dif= addr - (curpos + 2);

		Value v (vd.getval () );
		v-= phasedpos ();

		address vdif= v.value - 2;
		dif= vdif > 127 ? vdif - 0x10000 : static_cast <int> (vdif);

		if (dif > 127 || dif < -128)
		{
			debout << "addr= " << vd.getvalue () <<
				" current= " << phasedpos ().value <<
				" dif= " << dif << endl;
			throw RelativeOutOfRange;
		}
	}
	signed char reldesp = static_cast <signed char> (dif);

	gencode (code, reldesp);

	showcode (instrname + ' ' + hex4str (vd.getvalue () ) );
	no8080 ();
	showlisting ();
}

void AsmReal::doJR (const VarData & vd)
{
	byte code= opt.mode86 ? 0xEB : 0x18;
	doRelative (code, vd, "JR");
}

void AsmReal::doJR_flag (flagCode fcode, const VarData & vd)
{
	byte code= opt.mode86 ?
		(0x70 | getflag86 (fcode) ) :
		(0x20 | (fcode << 3) );
	doRelative (code, vd, "JR " + getflagname (fcode) + ',');
}

void AsmReal::doDJNZ (const VarData & vd)
{
	if (! opt.mode86)
	{
		doRelative (codeDJNZ, vd, "DJNZ");
	}
	else
	{
		int dif= 0;
		if (pass >= 2)
		{
			//address curpos= phased (current);
			//address curpos= phasedpos ();
			Value v (vd.getval () );
			v-= phasedpos ();
			address vdif= v.value - 4;
			dif= vdif > 127 ?
				static_cast <int> (vdif) - 0x10000 :
				static_cast <int> (vdif);

			if (dif > 127 || dif < -128)
			{
				debout << "addr= " << vd.getvalue () <<
					" current= " << phasedpos ().value <<
					" vdif= " << vdif <<
					" dif= " << dif << endl;
				throw RelativeOutOfRange;
			}
		}
		signed char reldesp = static_cast <signed char> (dif);

		// DEC CH ; JNZ  ...
		gencode (0xFE, 0xCD, 0x75, reldesp);

		showcode ("DJNZ" + hex4str (vd.getvalue () ) );
		showlisting ();
	}
}

void AsmReal::doINC_r (bool isINC, byte prefix, regbCode reg)
{
	byte code= opt.mode86 ? (isINC ? 0xC0 : 0xC8) :
		isINC ? 04 : 05;
	if (prefix != prefixNone)
	{
		no86 ();
		gencode (prefix);
	}
	if (opt.mode86)
	{
		if (reg == reg_HL_)
			code= isINC ? 0x07 : 0x0F;
		else
			code+= getregb86 (reg);
		gencode (0xFE);
	}
	else
		code+= reg << 3;

	gencode (code);

	string inst;
	switch (asmmode)
	{
	case AsmZ80: 
		showcode (incordec (isINC) + getregbname (reg, prefix) );
		break;
	case Asm8080:
		ASSERT (prefix == prefixNone);
		showcode (inrordcr (isINC) + getregb8080name (reg) );
		break;
	}
	showlisting ();
}

void AsmReal::doINC_IX (bool isINC, address adesp)
{
	byte desp= static_cast <byte> (adesp);

	no86 ();
	byte code= isINC ? 0x34 : 0x35;
	gencode (prefixIX, code, desp);

	showcode (incordec (isINC) + "[IX + " + hex2str (desp) + ']');
	no8080 ();
	showlisting ();
}

void AsmReal::doINC_IY (bool isINC, address adesp)
{
	byte desp= static_cast <byte> (adesp);

	no86 ();
	byte code= isINC ? 0x34 : 0x35;
	gencode (prefixIY, code, desp);

	showcode (incordec (isINC) + "[IY + " + hex2str (desp) + ']');
	no8080 ();
	showlisting ();
}

void AsmReal::doINC_rr (bool isINC, regwCode reg, byte prefix)
{
	ASSERT (prefix == prefixNone ||
		prefix == prefixIX || prefix == prefixIY);

	if (prefix != prefixNone)
	{
		no86 ();
		gencode (prefix);
	}

	byte code= opt.mode86 ? (isINC ? 0x41 : 0x49) :
		isINC ? 0x03 : 0x0B;
	if (opt.mode86)
		code+= reg;
	else
		code+= reg << 4;
	gencode (code);

	string inst;
	switch (asmmode)
	{
	case AsmZ80:
		inst= incordec (isINC) + regwName (reg, nameSP, prefix);
		break;
	case Asm8080:
		ASSERT (prefix == prefixNone);
		inst= inxordcx (isINC) + regw8080Name (reg);
		break;
	}
	showcode (inst);
	if (prefix != prefixNone)
		no8080 ();
	showlisting ();
}

void AsmReal::doEX_indSP_HL ()
{
	if (opt.mode86)
	{
		gencode (0x89, 0xE5);       // MOV BP,SP
		gencode (0x87, 0x5E, 0x00); // XCHG BX,[BP]
	}
	else
		gencode (0xE3);

	switch (asmmode)
	{
	case AsmZ80:  showcode ("EX [SP], HL"); break;
	case Asm8080: showcode ("XTHL"); break;
	}
	showlisting ();
}

void AsmReal::doEX_indSP_IX ()
{
	gencode (prefixIX, 0xE3);

	showcode ("EX [SP], IX");
	no8080 ();
	showlisting ();
}

void AsmReal::doEX_indSP_IY ()
{
	no86 ();
	gencode (prefixIY, 0xE3);

	showcode ("EX [SP], IY");
	no8080 ();
	showlisting ();
}

void AsmReal::doEX_AF_AFP ()
{
	no86 ();
	gencode (0x08);

	showcode ("EX AF, AF'");
	no8080 ();
	showlisting ();
}

void AsmReal::doEX_DE_HL ()
{
	if (opt.mode86)
		gencode (0x87, 0xD3);
	else
		gencode (0xEB);

	showcode ("EX DE, HL");
	showlisting ();
}

void AsmReal::doIN_A_indC ()
{
	no86 ();
	gencodeED (0x78);

	showcode ("IN A, [C]");
	no8080 ();
	showlisting ();
}

void AsmReal::doIN_A_indn (byte n)
{
	byte code= opt.mode86 ? 0xE4 : 0xDB;
	gencode (code, n);

	switch (asmmode)
	{
	case AsmZ80:  showcode ("IN A, [" + hex2str (n) + closeIndir); break;
	case Asm8080: showcode ("IN " + hex2str (n) ); break;
	}
	showlisting ();
}

void AsmReal::doINr_c_ (regbCode reg)
{
	byte code;
	switch (reg)
	{
	case regB:
		code= 0x40; break;
	case regC:
		code= 0x48; break;
	case regD:
		code= 0x50; break;
	case regE:
		code= 0x58; break;
	case regH:
		code= 0x60; break;
	case regL:
		code= 0x68; break;
	default:
		throw logic_error ("Invalid doINr_c_ call");
	}

	no86 ();
	gencodeED (code);

	showcode ("IN " + getregbname (reg) + ", [C]");
	no8080 ();
	showlisting ();
}

void AsmReal::doOUT_C_ (regbCode rcode)
{
	byte code;
	switch (rcode)
	{
	case regA:
		code= 0x79; break;
	case regB:
		code= 0x41; break;
	case regC:
		code= 0x49; break;
	case regD:
		code= 0x51; break;
	case regE:
		code= 0x59; break;
	case regH:
		code= 0x61; break;
	case regL:
		code= 0x69; break;
	default:
		throw logic_error ("Invalid OUT (C) argument");
	}

	no86 ();

	gencodeED (code);

	showcode ("OUT [C], " + getregbname (rcode) );
	no8080 ();
	showlisting ();
}

void AsmReal::doOUT_n_ (byte b)
{
	const byte code= opt.mode86 ? 0xE6 : 0xD3;
	gencode (code, b);

	switch (asmmode)
	{
	case AsmZ80:  showcode ("OUT [" + hex2str (b) + "], A"); break;
	case Asm8080: showcode ("OUT " + hex2str (b) ); break;
	}
	showlisting ();
}

void AsmReal::doDEFBliteral (const string & s)
{
	const string::size_type l= s.size ();
	for (string::size_type i= 0; i < l; ++i)
	{
		//gendata (s [i] );
		gencode (s [i] );
	}
}

void AsmReal::doDEFBnum (byte b)
{
	//gendata (b);
	gencode (b);
}

#if 0
void AsmReal::doDEFBnum (const VarData & vd)
{
	gencode (vd);
}
#endif

void AsmReal::doDEFBend ()
{
	//address count= current - currentinstruction;
	//address count= currentpos () - currentinstruction;
	address count= currentpos ().value - currentinstruction.value;

	ostringstream oss;
	oss << "DEFB of " << count << " bytes";

	showcode (oss.str () );
	showlisting ();
}

#if 0
void AsmReal::doDEFWnum (address num)
{
	gendataword (num);
}
#endif

void AsmReal::doDEFWnum (const VarData & num)
{
	//gendataword (num.getvalue () );
	//gendataword (num);
	gencodeword (num);
}

void AsmReal::doDEFWend ()
{
	//address count= (current - currentinstruction) / 2;
	//address count= (currentpos () - currentinstruction) / 2;
	address count= (currentpos ().value - currentinstruction.value) / 2;
	ostringstream oss;
	oss << "DEFW of " << count << " words";

	showcode (oss.str () );
	showlisting ();
}

void AsmReal::doDEFS (address count, byte value)
{
	for (address i= 0; i < count; ++i)
		genbyte (value);

	ostringstream oss;
	oss << "DEFS of " << count << " bytes with value " << hex2 (value);

	showcode (oss.str () );
	showlisting ();
}

void AsmReal::doINCBIN (const string & includefile)
{
	debout << "\t\tINCBIN " << includefile << endl;
	showlisting ();
	ifstream f;
	openis (f, includefile, ios::in | ios::binary);

	char buffer [1024];
	for (;;)
	{
		f.read (buffer, sizeof (buffer) );
		for (streamsize i= 0, r= f.gcount (); i < r; ++i)
		{
			genbyte (static_cast <byte> (buffer [i] ) );
		}

		if (! f)
		{
			if (f.eof () )
				break;
			else
				throw ErrorReadingINCBIN;
		}
	}
}


//*********************************************************
//		Macro expansions.
//*********************************************************


bool AsmReal::gotoENDM ()
{
	TRFUNC (tr, "AsmReal::gotoENDM");

	size_t level= 1;
	while (level > 0 && nextline () )
	{
		TRMESSAGE (tr, "next line");

		Tokenizer tz (getcurrenttz () );
		showdebnocodeline (tz);
		showlisting ();

		Token tok= tz.gettoken ();
		TypeToken tt= tok.type ();

		if (tt == TypeIdentifier)
		{
			tok= tz.gettoken ();
			tt= tok.type ();
			if (tt == TypeColon)
			{
				tok= tz.gettoken ();
				tt= tok.type ();
			}
		}
		if (tt == TypeENDM)
		{
			--level;
		}
		else
		{
			if (ismacrodirective (tt) )
				++level;
		}
	}
	return true;
}

void AsmReal::doExpandMacro (const string & name,
	const MacroArgList & arglist)
{
	TRFUNC (tr, "AsmReal::doExpandMacro");
	TRMESSAGE (tr, name);

	debout << "\tExpanding MACRO " << name << endl;

	showlisting ();

	++macrolevel;

	MacroFrameMacro * pmframe=
		new MacroFrameMacro (* this, name, arglist);
	setline (pmframe->getline () );
}

void AsmReal::doREPT (address numrep, const string & varcounter,
	address valuecounter, address step)
{
	TRFUNC (tr, "AsmReal::doREPT");

	showlisting ();

	debout << "\t\tREPT " << numrep;
	if (! varcounter.empty () )
	{
		debout << ", " << varcounter << ", " << valuecounter <<
			", " << step;
	}
	debout << endl;

	if (numrep == 0)
	{
		if (! gotoENDM () )
			throw REPTwithoutENDM;
		return;
	}

	++macrolevel;

	new MacroFrameREPT (* this, numrep, varcounter, valuecounter, step);
}

void AsmReal::doIRP (const string & varname, const MacroArgList & arglist)
{
	TRFUNC (tr, "AsmReal::doIRP");

	debout << "\t\tIRP " << varname <<
		" with " << arglist.size () << " args" << endl;

	showlisting ();
	++macrolevel;

	new MacroFrameIRP (* this, varname, arglist);
}

void AsmReal::doIRPC (const string & varname, const string & charlist)
{
	TRFUNC (tr, "AsmReal::doIRPC");
	TRSTREAM (tr, varname << '<' << charlist << '>');

	debout << "\t\tIRPC " << varname << " <" << charlist << '>' << endl;

	showlisting ();
	++macrolevel;

	new MacroFrameIRPC (* this, varname, charlist);
}


void AsmReal::parseinstruction (Tokenizer & tz)
{
	TRFUNC (tr, "AsmReal::parseinstruction");

	ParseType type= opt.bracketonly ? BracketOnly : PassedFirst;
	bool inp2= pass > 1;
	auto_ptr <Machine> pmachine (newMachine (* this, tz, type, inp2) );

	try
	{
		TRMESSAGE (tr, "Parse");
		if (yyparse (* pmachine) != 0)
			throw runtime_error ("Syntax error");

		TRMESSAGE (tr, "Exec");
		pmachine->exec ();
	}
	catch (nonfatal_error & e)
	{
		TRMESSAGE (tr, "catch nonfatal");
		showcurrentlineinfo (errout);
		errout << ' ' << e.what () << endl;
		verbout << ">>" << tz << "<<" << endl;
		if (++counterr == opt.numerr)
			throw error_already_reported ();
	}
	catch (exception & e)
	{
		showcurrentlineinfo (errout);
		errout << ' ' << e.what () << endl;
		verbout << ">>" << tz << "<<" << endl;
		throw error_already_reported ();
	}
}


void AsmReal::checkafterprocess ()
{
	for (publiciterator pit= pubbegin (); pit != pubend (); ++pit)
	{
		VarData vd= getvar (* pit);
		if (vd.def () == NoDefined)
			emitwarning ("PUBLIC " + * pit + " undefined");
	}
}


//*********************************************************
//		Object file generation.
//*********************************************************


void AsmReal::message_emit (const string & type)
{
	if (opt.debugtype != NoDebug)
	{
		debout << "Emiting " << type <<
			" from " << hex4 (getminused () ) <<
			" to " << hex4 (getmaxused () ) <<
			endl;
	}
}

address AsmReal::getminused () const
{
	return mainseg.getminused ();
}

address AsmReal::getmaxused () const
{
	return mainseg.getmaxused ();
}

size_t AsmReal::getcodesize () const
{
	const address maxused= getmaxused ();
	const address minused= getminused ();
	return (maxused >= minused) ? maxused - minused + 1 : 0;
}

void AsmReal::writebincode (ostream & out)
{
	mainseg.write (out, getminused (), getcodesize () );
}

void AsmReal::emitobject (ostream & out)
{
	TRFUNC (tr, "AsmReal::emitobject");

	message_emit ("raw binary");

	writebincode (out);
}

class Dumper {
public:
	Dumper (std::ostream & out_n, address start_n);
	~Dumper ();
	void operator () (byte code);
private:
	std::ostream & out;
	const address start;
	size_t count;
	static const size_t bytes_line= 16;
};

Dumper::Dumper (std::ostream & out_n, address start_n) :
	out (out_n),
	start (start_n),
	count (0)
{
}

Dumper::~Dumper ()
{
	if (count > 0)
		out << endl;
}

void Dumper::operator () (byte code)
{
	if (count % bytes_line == 0)
	{
		if (count > 0)
			out << endl;
		out << hex4 (start + count) << ": ";
	}
	else
		out << ' ';
	out << hex2 (code);
	++count;
}

void AsmReal::emitdump (std::ostream & out)
{
	message_emit ("hex dump");
	size_t codesize= getcodesize ();
	const address minused= getminused ();

	for_each (mainseg.iter (minused), mainseg.iter (minused + codesize),
		Dumper (out, minused) );
}

void AsmReal::emitplus3dos (ostream & out)
{
	message_emit ("PLUS3DOS");

	const size_t codesize= getcodesize ();
	const address minused= getminused ();

	spectrum::Plus3Head head;
	head.setsize (codesize);
	head.setstart (minused);
	head.write (out);

	// Write code.
	writebincode (out);

	// Write rounding to 128 byte block.
	size_t round= 128 - (codesize % 128);
	if (round != 128)
	{
		char aux [128]= {};
		out.write (aux, round);
	}

	if (! out)
		throw ErrorOutput;
}

void AsmReal::emittap (ostream & out)
{
	message_emit ("TAP");

	// Pepare data needed.
	const size_t codesize= getcodesize ();
	const address minused= getminused ();
	tap::CodeHeader headcodeblock (minused, codesize, opt.headername);

	tap::CodeBlock codeblock (codesize, mainseg.iter (minused) );

	// Write the file.
	headcodeblock.write (out);
	codeblock.write (out);

	if (! out)
		throw ErrorOutput;
}

void AsmReal::writetzxcode (ostream & out)
{
	// Preapare data needed.
	const size_t codesize= getcodesize ();
	const address minused= getminused ();
	tap::CodeHeader block1 (minused, codesize, opt.headername);

	tap::CodeBlock block2 (codesize, mainseg.iter (minused) );

	// Write the data.

	tzx::writestandardblockhead (out);
	block1.write (out);

	tzx::writestandardblockhead (out);
	block2.write (out);
	if (! out)
		throw ErrorOutput;
}

void AsmReal::emittzx (ostream & out)
{
	message_emit ("TZX");

	tzx::writefilehead (out);

	writetzxcode (out);
}

void AsmReal::writecdtcode (ostream & out)
{
	const size_t codesize= getcodesize ();
	const address minused= getminused ();
	const address entry= hasentrypoint ? entrypoint : 0;

	cpc::Header head (opt.headername);
	head.settype (cpc::Header::Binary);
	head.firstblock (true);
	head.lastblock (false);
	head.setlength (codesize);
	head.setloadaddress (minused);
	head.setentry (entry);

	address pos= minused;
	address pending= codesize;

	const address maxblock= 2048;
	const address maxsubblock= 256;
	byte blocknum= 1;

	while (pending > 0)
	{
		const address block= pending < maxblock ? pending : maxblock;

		head.setblock (blocknum);
		if (blocknum > 1)
			head.firstblock (false);
		if (pending <= maxblock)
			head.lastblock (true);
		head.setblocklength (block);

		// Size of the tzx data block: type byte, code, checksums,
		// filling of last subblock and final bytes.

		size_t tzxdatalen= static_cast <size_t> (block) +
			maxsubblock - 1;
		tzxdatalen/= maxsubblock;
		tzxdatalen*= maxsubblock + 2;
		tzxdatalen+= 5;
		
		// Write header.

		tzx::writeturboblockhead (out, 263);

		head.write (out);

		// Write code.

		tzx::writeturboblockhead (out, tzxdatalen);

		out.put (0x16);  // Data block identifier.

		address subpos= pos;
		address blockpending= block;
		while (blockpending > 0)
		{
			address subblock= blockpending < maxsubblock ?
				blockpending : maxsubblock;

			mainseg.write (out, subpos, subblock);

			for (size_t i= subblock; i < maxsubblock; ++i)
				out.put ('\0');

			address crc= cpc::crc (mainseg.iter (subpos),
				subblock);

			out.put (hibyte (crc) ); // CRC in hi-lo format.
			out.put (lobyte (crc) );
			blockpending-= subblock;
			subpos+= subblock;
		}

		out.put (0xFF);
		out.put (0xFF);
		out.put (0xFF);
		out.put (0xFF);

		pos+= block;
		pending-= block;
		++blocknum;
	}

	if (! out)
		throw ErrorOutput;
}

void AsmReal::emitcdt (ostream & out)
{
	message_emit ("CDT");

	tzx::writefilehead (out);

	writecdtcode (out);
}

string AsmReal::cpcbasicloader ()
{
	using namespace cpc;

	string basic;

	const address minused= getminused ();

	// Line: 10 MEMORY before_min_used
	string line= tokMEMORY + hexnumber (minused - 1);
	basic+= basicline (10, line);

	// Line: 20 LOAD "!", minused
	line= tokLOAD + "\"!\"," + hexnumber (minused);
	basic+= basicline (20, line);

	if (hasentrypoint)
	{
		// Line: 30 CALL entry_point
		line= tokCALL + hexnumber (minused);
		basic+= basicline (30, line);
	}

	// A line length of 0 marks the end of program.
	basic+= '\0';
	basic+= '\0';

	return basic;
}

void AsmReal::emitcdtbas (ostream & out)
{
	message_emit ("CDT");

	const string basic= cpcbasicloader ();
	const address basicsize= static_cast <address> (basic.size () );

	cpc::Header head ("LOADER");
	head.settype (cpc::Header::Basic);
	head.firstblock (true);
	head.lastblock (true);
	head.setlength (basicsize);
	head.setblock (1);
	head.setblocklength (basicsize);

	tzx::writefilehead (out);

	// Write header.

	tzx::writeturboblockhead (out, 263);

	head.write (out);

	// Write Basic.

	const address maxsubblock= 256;
	size_t tzxdatalen= static_cast <size_t> (basicsize) + maxsubblock - 1;
	tzxdatalen/= maxsubblock;
	tzxdatalen*= maxsubblock + 2;
	tzxdatalen+= 5;

	tzx::writeturboblockhead (out, tzxdatalen);

	out.put (0x16);  // Data block identifier.

	out.write (basic.data (), basicsize);
	for (address n= basicsize ; (n % maxsubblock) != 0; ++n)
		out.put ('\0');
	address crc= cpc::crc
		(reinterpret_cast <const unsigned char *> (basic.data () ),
		basicsize);
	out.put (hibyte (crc) ); // CRC in hi-lo format.
	out.put (lobyte (crc) );

	out.put (0xFF);
	out.put (0xFF);
	out.put (0xFF);
	out.put (0xFF);

	writecdtcode (out);
}


string AsmReal::spectrumbasicloader ()
{
	using namespace spectrum;

	string basic;

	const address minused= getminused ();

	// Line: 10 CLEAR before_min_used
	string line= tokCLEAR + number (minused - 1);
	basic+= basicline (10, line);

	// Line: 20 POKE 23610, 255
	// To avoid a error message when using +3 loader.
	line= tokPOKE + number (23610) + ',' + number (255);
	basic+= basicline (20, line);

	// Line: 30 LOAD "" CODE
	line= tokLOAD + "\"\"" + tokCODE;
	basic+= basicline (30, line);

	if (hasentrypoint)
	{
		// Line: 40 RANDOMIZE USR entry_point
		line= tokRANDOMIZE + tokUSR + number (entrypoint);
		basic+= basicline (40, line);
	}

	return basic;
}

void AsmReal::emittapbas (ostream & out)
{
	if (opt.debugtype != NoDebug)
		debout << "Emiting TAP basic loader" << endl;

	// Prepare the data.

	string basic (spectrumbasicloader () );
	tap::BasicHeader basicheadblock (basic);
	tap::BasicBlock basicblock (basic);

	// Write the file.

	basicheadblock.write (out);
	basicblock.write (out);

	emittap (out);
}

void AsmReal::emittzxbas (ostream & out)
{
	if (opt.debugtype != NoDebug)
		debout << "Emiting TZX with basic loader" << endl;

	// Prepare the data.

	string basic (spectrumbasicloader () );
	tap::BasicHeader basicheadblock (basic);
	tap::BasicBlock basicblock (basic);

	// Write the file.

	tzx::writefilehead (out);

	tzx::writestandardblockhead (out);
	basicheadblock.write (out);

	tzx::writestandardblockhead (out);
	basicblock.write (out);

	writetzxcode (out);
}

void AsmReal::emithex (ostream & out)
{
	message_emit ("Intel HEX");

	address end= getmaxused () + 1;
	for (address i= getminused (); i < end; i+= 16)
	{
		address len= end - i;
		if (len > 16)
			len= 16;
		out << ':' << hex2 (lobyte (len) ) << hex4 (i) << "00";
		byte sum= len + ( (i >> 8) & 0xFF) + i & 0xFF;
		for (address j= 0; j < len; ++j)
		{
			byte b= mainseg.getbyte (i + j);
			out << hex2 (b);
			sum+= b;
		}
		out << hex2 (lobyte (0x100 - sum) );
		out << "\r\n";
	}
	out << ":00000001FF\r\n";

	if (! out)
		throw ErrorOutput;
}

void AsmReal::emitamsdos (ostream & out)
{
	message_emit ("Amsdos");

	const size_t codesize= getcodesize ();
	const address minused= getminused ();

	cpc::AmsdosHeader head (opt.headername);
	head.setlength (codesize);
	head.setloadaddress (minused);
	if (hasentrypoint)
		head.setentry (entrypoint);

	head.write (out);

	// Write code.
	writebincode (out);

	if (! out)
		throw ErrorOutput;
}

void AsmReal::emitmsx (ostream & out)
{
	message_emit ("MSX");

	const address minused= getminused ();
	const address maxused= getmaxused ();

	// Header of an MSX BLOADable disk file.
	byte header [7]= { 0xFE }; // Header identification byte.
	// Start address.
	header [1]= minused & 0xFF;
	header [2]= minused >> 8;
	// End address.
	header [3]= maxused & 0xFF;
	header [4]= maxused >> 8;
	// Exec address.
	address entry= 0;
	if (hasentrypoint)
		entry= entrypoint;
	header [5]= entry & 0xFF;
	header [6]= entry >> 8;	

	// Write hader.
	out.write (reinterpret_cast <char *> (header), sizeof (header) );

	// Write code.
	writebincode (out);
}

void AsmReal::emitprl (ostream & out)
{
	message_emit ("PRL");

	// Assembly with 1 page offset to obtain the information needed
	// to create the prl relocation table.
	AsmReal asmoff (* this);
	asmoff.setbase (0x100);
	asmoff.processfile ();

	const address minused= getminused ();
	const address maxused= getmaxused ();

	if (minused - base != asmoff.getminused () - asmoff.base)
		throw OutOfSyncPRL;
	if (maxused - base != asmoff.getmaxused () - asmoff.base)
		throw OutOfSyncPRL;
	size_t len= getcodesize ();
	address off= asmoff.base - base;

	// PRL header.

	byte prlhead [256]= { 0 };
	prlhead [1]= len & 0xFF;
	prlhead [2]= len >> 8;
	out.write (reinterpret_cast <char *> (prlhead), sizeof (prlhead) );

	// Relocation bitmap.

	address reloclen= (len + 7) / 8;
	byte * reloc= new byte [reloclen];
	fill (reloc, reloc + reloclen, byte (0) );

	for (address i= minused; i <= maxused; ++i)
	{
		byte b= mainseg.getbyte (i);
		byte b2= asmoff.mainseg.getbyte (i + off);

		if (b != b2)
		{
			if (b2 - b != off / 256)
			{
				errout << "off= " << hex4 (off) <<
					", b= " << hex2 (b) <<
					", b2= " << hex2 (b2) <<
					endl;
				throw OutOfSyncPRL;
			}
			address pos= i - minused;
			static const byte mask [8]= {
				0x80, 0x40, 0x20, 0x10,
				0x08, 0x04, 0x02, 0x01
			};
			reloc [pos / 8]|= mask [pos % 8];
		}
	}

	// Write code in position 0x0100
	asmoff.writebincode (out);

	// Write relocation bitmap.
	out.write (reinterpret_cast <char *> (reloc), reloclen);

	if (! out)
		throw ErrorOutput;
}


#if 0


class MakePublicRel {
public:
	MakePublicRel (AsmReal & asmr_n, RelFileOut & rel_n) :
		asmr (asmr_n),
		rel (rel_n)
	{ }
	void operator () (const string & s);
private:
	AsmReal & asmr;
	RelFileOut & rel;
	typedef set <string> Pub;
	Pub pub;
};

void MakePublicRel::operator () (const string & s)
{
	VarData vd= asmr.getvar (s);
	if (vd.def () != NoDefined)
	{

	//cerr << "PUBLIC " << s << " as " << vd.def () << endl;

	const string exname= rel.putentrysymbol (s);
	std::pair <Pub::iterator, bool> r (pub.insert (exname) );
	if (! r.second)
		throw runtime_error ("PUBLIC '" + s + "' repeated");

	}
}


class PutEntryPoints {
public:
	PutEntryPoints (AsmReal & asmr_n, RelFileOut & rel_n) :
		asmr (asmr_n),
		rel (rel_n)
	{ }
	void operator () (const string & s);
private:
	AsmReal & asmr;
	RelFileOut & rel;
	//typedef set <string> Point;
	//Point point;
};

void PutEntryPoints::operator () (const string & s)
{
	VarData vd= asmr.getvar (s);
	if (vd.def () != NoDefined)
	{
		//cerr << "ENTRY POINT " << s << " as " << vd.def () << endl;

		rel.putdefineentrypoint (s, vd.getval () );
	}
}


#endif


class ModuleEntryPoints {
public:
	ModuleEntryPoints (AsmReal & asmr_n, Module & mod_n) :
		asmr (asmr_n),
		mod (mod_n)
	{ }
	void operator () (const string & s);
private:
	AsmReal & asmr;
	Module & mod;
};

void ModuleEntryPoints::operator () (const string & s)
{
	VarData vd= asmr.getvar (s);
	if (vd.def () != NoDefined)
	{
		mod.setentrypoint (s, vd.getval () );
	}
}



void AsmReal::emitrel (ostream & out)
{
	TRFUNC (tr, "AsmReal::emitrel");

	message_emit ("REL");

	RelFileOut rel (out);

	//rel.putprogramname (opt.headername);

	TRMESSAGE (tr, "Creating public names");

	#if 0
	for (publiciterator pit= pubbegin (); pit != pubend (); ++pit)
	{
		rel.putentrysymbol (* pit);
	}
	#else

	//for_each (pubbegin (), pubend (), MakePublicRel (* this, rel) );
	for_each (pubbegin (), pubend (),
		ModuleEntryPoints (* this, mainmodule) );

	#endif

	//rel.putdatasize (0);
	//rel.putprogramsize (len);

	TRMESSAGE (tr, "Writing code");

	//mainmodule.saverel (rel);
	mainmodule.savemodule (rel, opt.headername);

	#if 0
	TRMESSAGE (tr, "Writing public values");
	for (publiciterator pit= pubbegin (); pit != pubend (); ++pit)
	{
		rel.putdefineentrypoint (* pit, getvar (* pit).getval () );
	}
	#else

	//for_each (pubbegin (), pubend (), PutEntryPoints (* this, rel) );

	#endif

	//mainmodule.closerel (rel);
	#if 0
	if (hasentrypoint)
	{
		//rel.putendmodule (entrypoint);
		rel.putendmodule (ventrypoint);
	}
	else
	{
		rel.putendmodule ();
	}
	#endif

	rel.putendfile ();
}


class CmdGroup {
public:
	CmdGroup ();			// Create empty group
	CmdGroup (address lengthn);	// Create Code group
	void put (ostream & out) const;
private:
	byte type;
	address length;
	address base;
	address minimum;
	address maximum;

	static address para (address n);
};

address CmdGroup::para (address n)
{
	return (n + 15) / 16;
}

CmdGroup::CmdGroup () :
	type (0),
	length (0),
	base (0),
	minimum (0),
	maximum (0)
{
}

CmdGroup::CmdGroup (address lengthn) :
	type (1),
	length (para (lengthn) + 0x0010),
	base (0),
	minimum (length),
	maximum (0x0FFF)
{
}

void CmdGroup::put (ostream & out) const
{
	out.put (type);
	putword (out, length);
	putword (out, base);
	putword (out, minimum);
	putword (out, maximum);
}


void AsmReal::emitcmd (ostream & out)
{
	message_emit ("CMD");

	size_t codesize= getcodesize ();
	CmdGroup code (codesize);
	CmdGroup empty;

	// CMD header.

	// 8 group descriptors: 72 bytes in total.
	code.put (out);
	for (size_t i= 1; i < 8; ++i)
		empty.put (out);

	// Until 128 bytes: filled with zeroes (in this case).
	char fillhead [128 - 72]= { };
	out.write (fillhead, sizeof (fillhead) );

	// First 256 bytes of prefix in 8080 model.
	char prefix [256]= { };
	out.write (prefix, sizeof (prefix) );

	// Binary image.
	writebincode (out);

	if (! out)
		throw ErrorOutput;
}

void AsmReal::emitcom (ostream & out)
{
	TRFUNC (tr, "AsmReal::emitcom");

	message_emit ("COM");

	// Provisional

	writebincode (out);

	#if 0
	static const address recordsize= 128;
	const address fillsize= recordsize - (getcodesize () % recordsize);
	if (fillsize != recordsize)
	{
		for (address i= 0; i < fillsize; ++i)
			out.put ('\x1A');
	}
	#endif
}

void AsmReal::emitcode (ostream & out)
{
	TRFUNC (tr, "AsmReal::emitcode");

	switch (opt.getObjectType () )
	{
	case ObjectBin:      emitobject (out);   break;
	case ObjectDump:     emitdump (out);     break;
	case ObjectHex:      emithex (out);      break;
	case ObjectPrl:      emitprl (out);      break;
	case ObjectRel:      emitrel (out);      break;
	case ObjectCmd:      emitcmd (out);      break;
	case ObjectCom:      emitcom (out);      break;
	case ObjectTap:      emittap (out);      break;
	case ObjectTapBas:   emittapbas (out);   break;
	case ObjectTzx:      emittzx (out);      break;
	case ObjectTzxBas:   emittzxbas (out);   break;
	case ObjectCdt:      emitcdt (out);      break;
	case ObjectCdtBas:   emitcdtbas (out);   break;
	case ObjectPlus3Dos: emitplus3dos (out); break;
	case ObjectAmsDos:   emitamsdos (out);   break;
	case ObjectMsx:      emitmsx (out);      break;
	default:
		throw logic_error ("Unexpected object type");
	}
}


//*********************************************************
//		Symbol table generation.
//*********************************************************


void AsmReal::dumppublic (ostream & out)
{
	TRF;

	for (publiciterator pit= pubbegin (); pit != pubend (); ++pit)
	{
		VarData vd= getvar (* pit);
		out << tablabel (* pit) << "EQU 0" <<
			hex4 (vd.getvalue () ) << 'H' << endl;
	}
}

void AsmReal::dumpsymbol (ostream & out)
{
	TRF;

	for (mapvar_t::const_iterator it= varbegin (); it != varend (); ++it)
	{
		const VarData & vd= it->second;
		// Dump only EQU and label valid symbols.
		if (vd.def () != DefinedPass2)
			continue;

		out << tablabel (it->first) << "EQU 0" <<
			hex4 (vd.getvalue () ) << 'H'
			<< endl;
	}
}


} // namespace impl
} // namespace pasmo

// End of asmimpl.cpp
