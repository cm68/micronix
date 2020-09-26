// machine.cpp
// Revision 22-nov-2006

#include "machine.h"
#include "asmimpl.h"
#include "var.h"
#include "token.h"
#include "parser.h"
#include "parsertypes.h"
#include "trace.h"

#include <sstream>
#include <deque>
#include <stdexcept>


namespace pasmo {
namespace impl {


using std::ostringstream;
using std::deque;
using std::logic_error;
using std::runtime_error;


namespace {


string showtoken (const Token & tok)
{
	const string stok ('\'' + tok.str () + '\'');
	string s;
	TypeToken tt= tok.type ();
	switch (tt)
	{
	case TypeEndLine:    s= "(end of line)";      break;
	case TypeIdentifier: s= "Identifier " + stok; break;
	case TypeMacroName:  s= "Macro " + stok;      break;
	case TypeLiteral:    s= "Literal " + stok;    break;
	case TypeNumber:     s= "Number " + stok;     break;
	default:             s= stok;
	}
	return s;
}

const string messexpected (" expected but ");
const string messfound (" found");

string showfound (const Token & tokfound)
{
	return messexpected + showtoken (tokfound) + messfound;
}

string showexpfound (const Token & tokexp, const Token & tokfound)
{
	return showtoken (tokexp) + showfound (tokfound);
}

string name_of (const VarData & vd)
{
	TRF;

	const string & name= vd.getname ();
	if (name.size () > 0)
		return " of " + name + ' ';
	else
		return string (1, ' ');
}


} // namespace


//**************************************************************
//			Assembler errors
//**************************************************************


nonfatal_error DivisionByZero ("Division by zero");
nonfatal_error Lenght1Required ("Invalid literal, length 1 required");
nonfatal_error InvalidOperand ("Invalid operand");


class Expected : public nonfatal_error {
public:
	Expected (const Token & tokexp, const Token & tokfound) :
		nonfatal_error (showexpfound (tokexp, tokfound) )
	{ }
	Expected (const string & expected, const Token & tok) :
		nonfatal_error (expected + showfound (tok) )
	{ }
	Expected (const string & expected, const string & found) :
		nonfatal_error (expected + messexpected + found + messfound)
	{ }
};

class Unexpected : public nonfatal_error {
public:
	Unexpected (const Token & tokfound, const string & str) :
		nonfatal_error ("Unexpected " +
			showtoken (tokfound) + ' ' + str)
	{ }
};


class Undefined : public nonfatal_error {
public:
	Undefined (const VarData & vd) :
		nonfatal_error ("Value" + name_of (vd) + "undefined")
	{ }
};


void check_defined (const VarData & vd, bool required)
{
	TRF;

	if (required && ! vd.isdefined () )
	{
		//throw nonfatal_error ("Value undefined");
		throw Undefined (vd);
	}
}

void check_defined_noextern (const VarData & vd, bool required)
{
	vd.noextern ();
	check_defined (vd, required);
}


//**************************************************************


typedef VarData MachValueType;


class MachStack {
public:
	bool empty () const;

	MachValueType result ();
	MachValueType pop ();
	void push (address n);
	void push (const Value & v);
	void push (const VarData & vd);

	void defined ();
	void add ();
	void sub ();
	void mul ();
	void div ();
	void mod ();
	void shl ();
	void shr ();
	void equal ();
	void notequal ();
	void lessthan ();
	void greaterthan ();
	void lessequal ();
	void greaterequal ();
	void do_not ();
	void do_boolnot ();
	void do_minus ();
	void do_and ();
	void do_or ();
	void do_xor ();
	void booland ();
	void boolor ();
	void do_high ();
	void do_low ();
	void cond ();
private:
	//typedef deque <MachValue> st_t;
	typedef deque <MachValueType> st_t;
	st_t st;

	//MachValue & back ();
	//MachValue & get (size_t n);
	MachValueType & back ();
	MachValueType & get (size_t n);
	void drop ();
	void drop (size_t n);
};


bool MachStack::empty () const
{
	return st.empty ();
}

VarData & MachStack::back ()
{
	ASSERT (! st.empty () );
	return st.back ();
}

void MachStack::drop ()
{
	st.pop_back ();
}

void MachStack::drop (size_t n)
{
	ASSERT (st.size () >= n);

	for (size_t i= 0; i < n; ++i)
		st.pop_back ();
}

VarData & MachStack::get (size_t n)
{
	ASSERT (n > 0);

	st_t::size_type s= st.size ();
	ASSERT (n < s);
	return st [s - n - 1];
}

MachValueType MachStack::result ()
{
	TRF;

	MachValueType v= back ();
	drop ();
	if (! st.empty () )
		throw logic_error ("Evaluator stack unbalanced");
	return v;
}

MachValueType MachStack::pop ()
{
	TRF;

	MachValueType v= back ();
	if (v.errordiv () )
		throw DivisionByZero;
	drop ();
	return v;
}

void MachStack::push (address n)
{
	st.push_back (MachValueType (n, PreDefined) );
}

void MachStack::push (const Value & v)
{
	st.push_back (MachValueType (v, PreDefined) );
}

void MachStack::push (const VarData & vd)
{
	st.push_back (vd);
}

void MachStack::defined ()
{
	back ().setdefined ();
}

void MachStack::add ()
{
	TRFUNC (tr, "MachStack::add");

	//get (1).add (back () );
	VarData & op1= get (1);
	op1.add (back () );

	drop ();
}

void MachStack::sub ()
{
	get (1).sub (back () );
	drop ();
}

void MachStack::mul ()
{
	TRFUNC (tr, "MachStack::mul");

	get (1).mul (back () );
	drop ();
}

void MachStack::div ()
{
	get (1).div (back () );
	drop ();
}

void MachStack::mod ()
{
	get (1).mod (back () );
	drop ();
}

void MachStack::shl ()
{
	get (1).shl (back () );
	drop ();
}

void MachStack::shr ()
{
	get (1).shr (back () );
	drop ();
}

void MachStack::equal ()
{
	get (1).equal (back () );
	drop ();
}

void MachStack::notequal ()
{
	get (1).notequal (back () );
	drop ();
}

void MachStack::lessthan ()
{
	get (1).lessthan (back () );
	drop ();
}

void MachStack::greaterthan ()
{
	get (1).greaterthan (back () );
	drop ();
}

void MachStack::lessequal ()
{
	get (1).lessequal (back () );
	drop ();
}

void MachStack::greaterequal ()
{
	get (1).greaterequal (back () );
	drop ();
}

void MachStack::do_not ()
{
	back ().do_not ();
}

void MachStack::do_boolnot ()
{
	back ().do_boolnot ();
}

void MachStack::do_minus ()
{
	back ().do_minus ();
}

void MachStack::do_and ()
{
	TRFUNC (tr, "MachStack::and");

	get (1).do_and (back () );
	drop ();
}

void MachStack::do_or ()
{
	get (1).do_or (back () );
	drop ();
}

void MachStack::do_xor ()
{
	get (1).do_xor (back () );
	drop ();
}

void MachStack::booland ()
{
	get (1).booland (back () );
	drop ();
}

void MachStack::boolor ()
{
	get (1).boolor (back () );
	drop ();
}

void MachStack::do_high ()
{
	TRFUNC (tr, "MachStack::high");

	back ().do_high ();
}

void MachStack::do_low ()
{
	TRFUNC (tr, "MachStack::low");

	back ().do_low ();
}

void MachStack::cond ()
{
	get (2).cond (get (1), back () );
	drop (2);
}


class Instruc {
public:
	Instruc (Oper op_n);
	Instruc (Oper op_n, const string & lit);
	Instruc (address value_n);
	Instruc (const string & ident_n);
	Instruc (const Instruc & instruc);
	~Instruc ();
	void operator = (const Instruc & instruc);

	Oper getop () const;
	address getvalue () const;
	string getident () const;
	string getliteral () const;
private:
	Oper op;
	union V {
		address value;
		string * pident;
	};
	V v;
};


Instruc::Instruc (Oper op_n) :
	op (op_n)
{ }

Instruc::Instruc (Oper op_n, const string & lit) :
	op (op_n)
{
	ASSERT (op == OpLiteral);

	v.pident= new string (lit);
}

Instruc::Instruc (address value_n) :
	op (OpNumber)
{
	v.value= value_n;
}

Instruc::Instruc (const string & ident_n) :
	op (OpIdentifier)
{
	v.pident= new string (ident_n);
}

Instruc::Instruc (const Instruc & instruc) :
	op (instruc.op)
{
	switch (op)
	{
	case OpIdentifier:
	case OpLiteral:
		v.pident= new string (* (instruc.v.pident) );
		break;
	case OpNumber:
		v.value= instruc.v.value;
		break;
	default:
		; // Nothing.
	}
}

Instruc::~Instruc ()
{
	switch (op)
	{
	case OpIdentifier:
	case OpLiteral:
		delete v.pident;
		break;
	default:
		; // nothing
	}
}

void Instruc::operator = (const Instruc & instruc)
{
	if (this != & instruc)
	{
		if (op == OpIdentifier)
			delete v.pident;
		op= instruc.op;
		if (op == OpIdentifier)
			v.pident= new string (* (instruc.v.pident) );
	}
}

Oper Instruc::getop () const
{
	return op;
}

address Instruc::getvalue () const
{
	switch (op)
	{
	case OpNumber:
		return v.value;
	case OpLiteral:
		{
			const string & s= * v.pident;
			const string::size_type l= s.size ();
			switch (l)
			{
			case 1:
				return static_cast <unsigned char>
					(s [0] );
			case 2:
				return static_cast <unsigned char> (s [0] )
					* 256 +
					static_cast <unsigned char> (s [1] );
			default:
				throw Lenght1Required;
			}
		}
	default:
		throw logic_error ("Invalid parser machine operation");
	}
}

string Instruc::getident () const
{
	switch (op)
	{
	case OpIdentifier:
		return * (v.pident);
	default:
		throw logic_error ("Invalid parser machine operation");
	}
}

string Instruc::getliteral () const
{
	switch (op)
	{
	case OpLiteral:
		return * (v.pident);
	default:
		throw logic_error ("Invalid parser machine operation");
	}
}

class AsmMachine : public Machine {
public:
	AsmMachine (AsmImpl & asmin_n, Tokenizer & tz_n,
		ParseType pt_n, bool inpass2_n);
	virtual ~AsmMachine () { }

	void syntax (const char * s);
	void ugly ();
	Token gettoken ();
	void expectmacro ();

	void addcode (Oper op);
	void addcode (address value);
	void addcode (const string & ident);
	void addcodeliteral (const string & lit);
	void addlabel (const string & labelname);
	void addmacroname (const Token & tok);
	void cancelmacroname ();
	void redefmacro ();

	void macroargs ();
	void nomacroargs ();
	void addmacroitem (const Token & tok);

	void errmsg (const string & errmsg);
	void invalid ();
	void expected (const Token & tokexp, const Token & tokfound);
	void unexpected (const Token & tokfound, const string & str);

	void exec ();
private:
	AsmImpl & asmin;
	bool nocase;

	typedef deque <Instruc> inst_t;
	inst_t inst;
	MachStack st;

	Tokenizer & tz;
	ParseType pt;
	bool require_defined;

	bool expectingmacro;

	VarnameList varlist;
	string label;

	bool stopped;
	inst_t::size_type progcounter;

	string macro;
	//MacroArg macroarg;
	MacroArg macroitemlist;
	MacroArgList macroarglist;
	bool inmacroargs;
	size_t itembegin;
	size_t itemend;

	AsmMachine (const AsmMachine &); // Forbidden
	void operator = (const AsmMachine &); // Forbidden.

	string getnocase (const string & str) const;
	Token getmacroname ();

	void run ();

	void push (address n);
	void push (const Value & v);
	void push (const string & varname);
	void pushliteral (const string & literal);

	address popvalue ();
	VarData popvar ();
	address popfixvalue ();
	byte popbyte ();
	byte popfixbyte ();
	bool popbool ();
	byte popcode ();
	regbCode popregb ();
	regwCode popregw ();
	byte popdesp ();
	byte popprefix ();
	TypeByteInst poptypeinst ();
	TypeToken poptypetoken ();
	flagCode popflag ();
	flagCode popflag_val ();

	void start ();
	void stop ();

	void opDollar ();

	void doEmpty ();
	void doGenLabel ();
	void doAddVarItem ();
	void doMacroItem ();
	void doMacroArg ();
	void doMacroValue ();
	void doExpandMacro ();

	void doASEG ();
	void doCSEG ();
	//void doDEFBliteral ();
	void doDEFBnum ();
	void doDEFBend ();
	void doDEFL ();
	void doDEFS ();
	void doDEFSvalue ();
	void doDEFWnum ();
	void doDEFWend ();
	void doDSEG ();
	void doELSE ();
	void doEND ();
	void doENDn ();
	void doENDIF ();
	void doENDM ();
	void doENDP ();
	void doEQU ();
	void doEXITM ();
	void doEXTRN ();
	void doIF ();
	void doIF1 ();
	void doIF2 ();
	void doIFDEF ();
	void doIFNDEF ();
	void doINCBIN ();
	void doINCLUDE ();
	void doEndINCLUDE ();
	void doIRP ();
	void doIRPC ();
	void doLOCAL ();
	void doMACRO ();
	void doORG ();
	void doPROC ();
	void doPUBLIC ();
	void doREPT ();

	void do_8080 ();
	void do_DEPHASE ();
	void do_ERROR ();
	void do_PHASE ();
	void do_SHIFT ();
	void do_WARNING ();
	void do_Z80 ();

	void doNoargInst ();
	void doEvalBitInst ();

	void doADC_HL ();
	void doADD_HL ();
	void doADD_IX ();
	void doADD_IY ();
	void doCALL ();
	void doCALL_flag ();
	void doCP_r ();
	void doCP_undoc ();
	void doCP_indHL ();
	void doCP_idesp ();
	void doCP_n ();
	void doIM ();
	void doIN_A_indC ();
	void doIN_A_indn ();
	void doIN_r_indC ();
	void doDJNZ ();
	void doEX_indSP_HL ();
	void doEX_indSP_IX ();
	void doEX_indSP_IY ();
	void doEX_AF_AFP ();
	void doEX_DE_HL ();
	void doINC_r ();
	void doINC_undoc ();
	void doINC_indHL ();
	void doINC_idesp ();
	void doINC_rr ();
	void doINC_IX ();
	void doINC_IY ();
	void doINC_r (bool isINC, byte prefix);
	void doINC_rr (bool isINC);
	void doJP ();
	void doJP_flag ();
	void doJP_indHL ();
	void doJP_indIX ();
	void doJP_indIY ();
	void doJR ();
	void doJR_flag ();
	void doLD_A_I ();
	void doLD_A_R ();
	void doLD_I_A ();
	void doLD_R_A ();
	void doLD_r_r ();
	void doLD_r_undoc ();
	void doLD_r_indHL ();
	void doLD_r_idesp ();
	void doLD_r_n ();
	void doLD_undoc_r ();
	void doLD_undoc_n ();
	void doLD_A_r ();
	void doLD_A_undoc ();
	void doLD_A_n ();
	void doLD_A_indexp ();
	void doLD_A_indBC ();
	void doLD_A_indDE ();
	void doLD_A_indHL ();
	void doLD_A_idesp ();
	void doLD_indHL_r ();
	void doLD_indHL_n ();
	void doLD_indBC ();
	void doLD_indDE ();
	void doLD_indexp_A ();
	void doLD_indexp_BC ();
	void doLD_indexp_DE ();
	void doLD_indexp_HL ();
	void doLD_indexp_SP ();
	void doLD_indexp_IX ();
	void doLD_indexp_IY ();
	void doLD_idesp_r ();
	void doLD_idesp_n ();
	void doLD_SP_HL ();
	void doLD_SP_IX ();
	void doLD_SP_IY ();
	void doLD_SP_nn ();
	void doLD_SP_indexp ();
	void doLD_HL_nn ();
	void doLD_HL_indexp ();
	void doLD_rr_nn ();
	void doLD_rr_indexp ();
	void doLD_IXY_nn ();
	void doLD_IXY_indexp ();
	void doOUT_C_ ();
	void doOUT_n_ ();
	void doPUSH_rr ();
	void doPUSH_IX ();
	void doPUSH_IY ();
	void doRET ();
	void doRET_flag ();
	void doRL_r ();
	void doRL_undoc ();
	void doRL_indhl ();
	void doRL_idesp ();
	void doRST ();
	void doSBC_HL ();
};


AsmMachine::AsmMachine (AsmImpl & asmin_n, Tokenizer & tz_n,
		ParseType pt_n, bool inpass2_n) :
	asmin (asmin_n),
	nocase (asmin.getnocase () ),
	tz (tz_n),
	pt (pt_n),
	require_defined (inpass2_n),
	expectingmacro (pt == PassedFirst || pt == BracketOnly),
	inmacroargs (false)
{
}

Machine * newMachine (AsmImpl & asmin_n, Tokenizer & tz_n,
	ParseType pt_n, bool inpass2_n)
{
	return new AsmMachine (asmin_n, tz_n, pt_n, inpass2_n);
}

string AsmMachine::getnocase (const string & str) const
{
	return nocase ? upper (str) : str;
}

Token AsmMachine::getmacroname ()
{
	Token tok= tz.gettoken ();
	if (tok.type () == TypeIdentifier)
	{
		const string name (getnocase (tok.str () ) );
		if (asmin.ismacro (name) )
			tok= Token (TypeMacroName, name);
	}
	return tok;
}

void AsmMachine::expectmacro ()
{
	macro.erase ();
	expectingmacro= true;
}

void AsmMachine::syntax (const char * /*s*/)
{
	//if (! inmacroargs)
	//	throw logic_error (string ("Unexpectd parsing error: ") + s);
}

void AsmMachine::ugly ()
{
	asmin.warningUglyInstruction ();
}

Token AsmMachine::gettoken ()
{
	TRFUNC (tr, "AsmMachine::gettoken");

	if (pt == PassedFirst)
	{
		if (expectingmacro)
		{
			expectingmacro= false;
			return getmacroname ();
		}
		else
		{
			if (inmacroargs)
				return tz.getmacroarg ();
			else
				return tz.gettoken ();
		}
	}
	else
	{
		ParseType inipt= pt;
		pt= PassedFirst;
		switch (inipt)
		{
		case BracketOnly:
			return Token (TypeToken (TOK_BracketOnly) );
		default:
			throw logic_error ("Invalid parse type");
		}
	}
}

void AsmMachine::addcode (Oper op)
{
	TRFUNC (tr, "AsmMachine::addcode (op)");

	inst.push_back (Instruc (op) );
}

void AsmMachine::addcode (address value)
{
	TRFUNC (tr, "AsmMachine::addcode (value)");

	inst.push_back (Instruc (value) );
}

void AsmMachine::addcode (const string & ident)
{
	TRFUNC (tr, "AsmMachine::addcode");
	TRMESSAGE (tr, ident);

	inst.push_back (Instruc (ident) );
}

void AsmMachine::addcodeliteral (const string & lit)
{
	TRFUNC (tr, "AsmMachine::addcodeliteral");
	TRMESSAGE (tr, lit);

	inst.push_back (Instruc (OpLiteral, lit) );
}

void AsmMachine::addlabel (const string & labelname)
{
	TRFUNC (tr, "AsmMachine::addlabel");

	ASSERT (label.empty () );
	label= getnocase (labelname);
}

void AsmMachine::addmacroname (const Token & tok)
{
	TRFUNC (tr, "AsmMachine::addmacroname");

	const string macroname (tok.macroname () );
	ASSERT (! macroname.empty () );
	ASSERT (macro.empty () );
	macro= getnocase (macroname);
	macroargs ();
}

void AsmMachine::cancelmacroname ()
{
	nomacroargs ();
	ASSERT (! macro.empty () );
	label= macro;
	macro.erase ();
	addcode (OpGenLabel);
	expectmacro ();
}

void AsmMachine::redefmacro ()
{
	ASSERT (! macro.empty () );
	ASSERT (label.empty () );
	label= macro;
	macro.erase ();
	nomacroargs ();
	addcode (OpMACRO);
}

void AsmMachine::macroargs ()
{
	TRFUNC (tr, "AsmMachine::macroargs");

	inmacroargs= true;
}

void AsmMachine::nomacroargs ()
{
	TRFUNC (tr, "AsmMachine::nomacroargs");

	inmacroargs= false;
}

void AsmMachine::addmacroitem (const Token & tok)
{
	TRFUNC (tr, "AsmMachine::addmacroitem");
	TRSTREAM (tr, tok);

	//macroarg.push_back (tok);
	macroitemlist.push_back (tok);
	addcode (OpMacroItem);
}

void AsmMachine::errmsg (const string & errmsg)
{
	TRFUNC (tr, "AsmMachine::errmsg");

	throw nonfatal_error (errmsg);
}

void AsmMachine::invalid ()
{
	errmsg ("Invalid instruction");
}

void AsmMachine::expected (const Token & tokexp, const Token & tokfound)
{
	TRF;

	throw Expected (tokexp, tokfound);
}

void AsmMachine::unexpected (const Token & tokfound, const string & str)
{
	TRF;

	throw Unexpected (tokfound, str);
}

void AsmMachine::push (address n)
{
	TRFDEBS ("Value: " << std::hex << n);

	st.push (n);
}

void AsmMachine::push (const Value & v)
{
	TRF;

	st.push (v);
}

void AsmMachine::push (const string & varname)
{
	TRFDEB (varname);

	const string name (getnocase (varname) );
	VarData v= asmin.getvar (name);
	v.setname (name);

	//TESTING
	#if 0
	bool defined= false;
	switch (v.def () )
	{
	case NoDefined:
		break;
	default:
		defined= true;
	}
	st.push (v.getvalue (), defined);

	#else

	bool defined= false;
	switch (v.def () )
	{
	case NoDefined:
		TRDEB ("No defined");
		break;
	default:
		TRDEB ("Defined");
		defined= true;
	}
	#endif

	st.push (v);
}

void AsmMachine::pushliteral (const string & literal)
{
	TRFDEBS ('\'' << literal << '\'');

	VarData v (literal);
	ASSERT (v.isliteral () );
	st.push (v);
}

address AsmMachine::popvalue ()
{
	TRF;

	VarData vd= st.pop ();
	//vd.noextern ();
	//if (require_defined && ! vd.isdefined () )
	//	throw nonfatal_error ("Value undefined");
	check_defined_noextern (vd, require_defined);
	return vd.getvalue ();
}

VarData AsmMachine::popvar ()
{
	TRF;

	#if 1

	VarData vd= st.pop ();
	//if (require_defined && ! vd.isdefined () )
	//	throw nonfatal_error ("Value undefined");
	check_defined (vd, require_defined);
	return vd;

	#else
	return st.pop ();
	#endif
}

address AsmMachine::popfixvalue ()
{
	TRF;

	VarData vd= st.pop ();
	//vd.noextern ();
	//if (! vd.isdefined () )
	//	throw nonfatal_error ("Value undefined");
	check_defined_noextern (vd, require_defined);
	return vd.getvalue ();
}

byte AsmMachine::popbyte ()
{
	return static_cast <byte> (popvalue () );
}

byte AsmMachine::popfixbyte ()
{
	return static_cast <byte> (popfixvalue () );
}

bool AsmMachine::popbool ()
{
	address addr= popfixvalue ();
	ASSERT (addr == 0 || addr == 1);
	return addr;
}

byte AsmMachine::popcode ()
{
	address v= popfixvalue ();
	ASSERT (v <= 0xFF);
	return static_cast <byte> (v);
}

regbCode AsmMachine::popregb ()
{
	return static_cast <regbCode> (popfixvalue () );
}

regwCode AsmMachine::popregw ()
{
	return static_cast <regwCode> (popfixvalue () );
}

byte AsmMachine::popdesp ()
{
	return popbyte ();
}

byte AsmMachine::popprefix ()
{
	byte prefix= popfixbyte ();
	ASSERT (isvalidprefix (prefix) );
	return prefix;
}

TypeByteInst AsmMachine::poptypeinst ()
{
	return static_cast <TypeByteInst> (popfixvalue () );
}

TypeToken AsmMachine::poptypetoken ()
{
	return static_cast <TypeToken> (popfixvalue () );
}

flagCode AsmMachine::popflag ()
{
	return getflag (poptypetoken () );	
}

flagCode AsmMachine::popflag_val ()
{
	return static_cast <flagCode> (popfixvalue () );	
}

void AsmMachine::start ()
{
	TRF;

	itembegin= 0;
	itemend= 0;
	progcounter= 0;
	stopped= false;
}

void AsmMachine::stop ()
{
	TRF;

	stopped= true;
}

void AsmMachine::opDollar ()
{
	TRF;

	//push (asmin.getcurrentinstruction () );
	Value v (asmin.getcurrentinstruction () );
	TRDEBS (v);
	push (v);
}

void AsmMachine::doEmpty ()
{
	asmin.doEmpty ();
	stop ();
}

void AsmMachine::doGenLabel ()
{
	ASSERT (! label.empty () );
	asmin.doLabel (label);
}

void AsmMachine::doAddVarItem ()
{
	Instruc & in= inst [progcounter++];
	const string varname= getnocase (in.getliteral () );
	varlist.push_back (varname);
}

void AsmMachine::doMacroItem ()
{
	++itemend;
}

void AsmMachine::doMacroArg ()
{
	MacroArg macroarg;
	for ( ; itembegin < itemend; ++itembegin)
		macroarg.push_back (macroitemlist [itembegin] );
	macroarglist.push_back (macroarg);
}

void AsmMachine::doMacroValue ()
{
	address value= popvalue ();
	MacroArg macroarg;
	macroarg.push_back (value);
	macroarglist.push_back (macroarg);
}

void AsmMachine::doExpandMacro ()
{
	ASSERT (! macro.empty () );
	ASSERT (itembegin == itemend);
	ASSERT (itemend == macroitemlist.size () );
	asmin.doExpandMacro (macro, macroarglist);
	stop ();
}

void AsmMachine::doASEG ()
{
	asmin.doASEG ();
	stop ();
}

void AsmMachine::doCSEG ()
{
	asmin.doCSEG ();
	stop ();
}

#if 0
void AsmMachine::doDEFBliteral ()
{
	TRF;

	Instruc & in= inst [progcounter++];
	asmin.doDEFBliteral (in.getliteral () );
}
#endif

void AsmMachine::doDEFBnum ()
{
	TRF;

	//address n= popvalue ();
	//asmin.doDEFBnum (static_cast <byte> (n) );

	//VarData vd= st.pop ();
	VarData vd= popvar ();
	if (vd.isliteral () )
	{
		asmin.doDEFBliteral (vd.getname () );
	}
	else
	{
		#if 1
		asmin.doDEFBnum (static_cast <byte> (vd.getvalue () ) );
		#else
		asmin.doDEFBnum (vd);
		#endif
	}
}

void AsmMachine::doDEFBend ()
{
	TRF;

	asmin.doDEFBend ();
	stop ();
}

void AsmMachine::doDEFL ()
{
	TRF;

	address n= popvalue ();
	ASSERT (! label.empty () );
	asmin.doDEFL (label, n);
	stop ();
}

void AsmMachine::doDEFS ()
{
	TRF;

	asmin.doDEFS (popfixvalue (), 0);
	stop ();
}

void AsmMachine::doDEFSvalue ()
{
	TRF;

	address n= popvalue ();
	address v= popfixvalue ();
	asmin.doDEFS (v, static_cast <byte> (n) );
	stop ();
}

void AsmMachine::doDEFWnum ()
{
	TRF;

	//asmin.doDEFWnum (popvalue () );
	asmin.doDEFWnum (popvar () );
}

void AsmMachine::doDEFWend ()
{
	TRF;

	asmin.doDEFWend ();
	stop ();
}

void AsmMachine::doDSEG ()
{
	asmin.doDSEG ();
	stop ();
}

void AsmMachine::doELSE ()
{
	asmin.doELSE ();
	stop ();
}

void AsmMachine::doEND ()
{
	//asmin.doEND (0, false);
	asmin.doEND ();
	stop ();
}

void AsmMachine::doENDn ()
{
	//asmin.doEND (popfixvalue (), true);
	asmin.doEND (popvar () );
	stop ();
}

void AsmMachine::doENDIF ()
{
	asmin.doENDIF ();
	stop ();
}

void AsmMachine::doENDM ()
{
	asmin.doENDM ();
	stop ();
}

void AsmMachine::doENDP ()
{
	asmin.doENDP ();
	stop ();
}

void AsmMachine::doEQU ()
{
	TRF;

	ASSERT (! label.empty () );

	//address n= popvalue ();
	//asmin.doEQU (label, n);
	asmin.doEQU (label, popvar () );
	stop ();
}

void AsmMachine::doEXITM ()
{
	asmin.doEXITM ();
	stop ();
}

void AsmMachine::doEXTRN ()
{
	TRF;

	asmin.doEXTRN (varlist);
	stop ();
}

void AsmMachine::doIF ()
{
	address v= popfixvalue ();
	asmin.doIF (v);
	stop ();
}

void AsmMachine::doIF1 ()
{
	asmin.doIF1 ();
	stop ();
}

void AsmMachine::doIF2 ()
{
	asmin.doIF2 ();
	stop ();
}

void AsmMachine::doIFDEF ()
{
	Instruc & in= inst [progcounter++];
	asmin.doIFDEF (in.getident () );
	stop ();
}

void AsmMachine::doIFNDEF ()
{
	Instruc & in= inst [progcounter++];
	asmin.doIFNDEF (in.getident () );
	stop ();
}

void AsmMachine::doINCBIN ()
{
	Instruc & in= inst [progcounter++];
	asmin.doINCBIN (in.getliteral () );
	stop ();
}

void AsmMachine::doINCLUDE ()
{
	asmin.doINCLUDE ();
	stop ();
}

void AsmMachine::doEndINCLUDE ()
{
	asmin.doEndOfINCLUDE ();
	stop ();
}

void AsmMachine::doIRP ()
{
	TRFUNC (tr, "AsmMachine::doIRP");

	const string varname= getnocase (inst [progcounter++].getliteral () );
	asmin.doIRP (varname, macroarglist);
	stop ();
}

void AsmMachine::doIRPC ()
{
	const string varname= getnocase (inst [progcounter++].getliteral () );
	const string charlist= inst [progcounter++].getliteral ();
	asmin.doIRPC (varname, charlist);
	stop ();
}

void AsmMachine::doLOCAL ()
{
	asmin.doLOCAL (varlist);
	stop ();
}

void AsmMachine::doMACRO ()
{
	TRFUNC (tr, "AsmMachine::doMACRO");

	ASSERT (! label.empty () );
	vector <string> param;
	for ( ;
		inst [progcounter].getop () != OpMACROend;
		++progcounter)
	{
		const string varname=
			getnocase (inst [progcounter].getliteral () );
		param.push_back (varname);
	}
	++progcounter;
	asmin.doMACRO (label, param);
	stop ();
}

void AsmMachine::doORG ()
{
	TRF;

	address neworg= popfixvalue ();
	asmin.doORG (neworg);
	stop ();
}

void AsmMachine::doPROC ()
{
	asmin.doPROC ();
	stop ();
}

void AsmMachine::doPUBLIC ()
{
	asmin.doPUBLIC (varlist);
	stop ();
}

void AsmMachine::doREPT ()
{
	const address step= popfixvalue ();
	const address initial= popfixvalue ();
	const address counter= popfixvalue ();
	Instruc & in= inst [progcounter++];
	const string varname= getnocase (in.getliteral () );
	asmin.doREPT (counter, varname, initial, step);
	stop ();
}

void AsmMachine::do_8080 ()
{
	asmin.do_8080 ();
	stop ();
}

void AsmMachine::do_DEPHASE ()
{
	asmin.do_DEPHASE ();
	stop ();
}

void AsmMachine::do_ERROR ()
{
	TRF;

	Instruc & in= inst [progcounter++];
	const string msg= in.getliteral ();
	asmin.do_ERROR (msg);
	stop ();
}

void AsmMachine::do_PHASE ()
{
	const address value= popfixvalue ();
	asmin.do_PHASE (value);
	stop ();
}

void AsmMachine::do_SHIFT ()
{
	asmin.do_SHIFT ();
	stop ();
}

void AsmMachine::do_WARNING ()
{
	Instruc & in= inst [progcounter++];
	const string msg= in.getliteral ();
	asmin.do_WARNING (msg);
	stop ();
}

void AsmMachine::do_Z80 ()
{
	asmin.do_Z80 ();
	stop ();
}

void AsmMachine::doNoargInst ()
{
	asmin.doNoargInst (poptypetoken () );
	stop ();
}

void AsmMachine::doEvalBitInst ()
{
	address bitvalue= popvalue ();
	if (bitvalue > 7)
		throw nonfatal_error ("Invalid bit value");
	byte inst= popfixbyte ();
	byte codeinst= inst + (static_cast <byte> (bitvalue) << 3);
	push (codeinst);
}

void AsmMachine::doADC_HL ()
{
	regwCode reg= popregw ();
	asmin.doADDADCSBC_HL (codeADCHL, reg, prefixNone);
	stop ();
}

void AsmMachine::doADD_HL ()
{
	regwCode reg= popregw ();
	asmin.doADDADCSBC_HL (codeADDHL, reg, prefixNone);
	stop ();
}

void AsmMachine::doADD_IX ()
{
	regwCode reg= popregw ();
	asmin.doADDADCSBC_HL (codeADDHL, reg, prefixIX);
	stop ();
}

void AsmMachine::doADD_IY ()
{
	regwCode reg= popregw ();
	asmin.doADDADCSBC_HL (codeADDHL, reg, prefixIY);
	stop ();
}

void AsmMachine::doCALL ()
{
	//asmin.doCALL (popvalue () );
	asmin.doCALL (popvar () );

	stop ();
}

void AsmMachine::doCALL_flag ()
{
	//address addr= popvalue ();
	VarData vd (popvar () );
	flagCode fcode= popflag_val ();
	//asmin.doCALL_flag (fcode, addr);
	asmin.doCALL_flag (fcode, vd);
	stop ();
}

void AsmMachine::doCP_r ()
{
	regbCode reg= popregb ();
	TypeByteInst ti= poptypeinst ();
	asmin.doByteInst (ti, reg);
	stop ();
}

void AsmMachine::doCP_undoc ()
{
	byte prefix= popprefix ();
	regbCode reg= popregb ();
	TypeByteInst ti= poptypeinst ();
	asmin.doByteInst (ti, reg, prefix);
	stop ();
}

void AsmMachine::doCP_n ()
{
	byte n= popbyte ();
	TypeByteInst ti= poptypeinst ();
	asmin.doByteInmediate (ti, n);
	stop ();
}

void AsmMachine::doCP_indHL ()
{
	TypeByteInst ti= poptypeinst ();
	asmin.doByteInst (ti, reg_HL_);
	stop ();
}

void AsmMachine::doCP_idesp ()
{
	byte desp= popdesp ();
	byte prefix= popprefix ();
	TypeByteInst ti= poptypeinst ();
	asmin.doByteInst (ti, reg_HL_, prefix, true, desp);
	stop ();
}

void AsmMachine::doIM ()
{
	asmin.doIM (popvalue () );
	stop ();
}

void AsmMachine::doIN_A_indC ()
{
	asmin.doIN_A_indC ();
	stop ();
}

void AsmMachine::doIN_A_indn ()
{
	asmin.doIN_A_indn (popbyte () );
	stop ();
}

void AsmMachine::doIN_r_indC ()
{
	asmin.doINr_c_ (popregb () );
	stop ();
}

void AsmMachine::doDJNZ ()
{
	asmin.doDJNZ (popvar () );
	stop ();
}

void AsmMachine::doEX_indSP_HL ()
{
	asmin.doEX_indSP_HL ();
	stop ();
}

void AsmMachine::doEX_indSP_IX ()
{
	asmin.doEX_indSP_IX ();
	stop ();
}

void AsmMachine::doEX_indSP_IY ()
{
	asmin.doEX_indSP_IY ();
	stop ();
}

void AsmMachine::doEX_AF_AFP ()
{
	asmin.doEX_AF_AFP ();
	stop ();
}

void AsmMachine::doEX_DE_HL ()
{
	asmin.doEX_DE_HL ();
	stop ();
}

void AsmMachine::doINC_r ()
{
	regbCode reg= popregb ();
	bool isINC= popbool ();
	asmin.doINC_r (isINC, prefixNone, reg);
	stop ();
}

void AsmMachine::doINC_undoc ()
{
	byte prefix= popprefix ();
	regbCode reg= popregb ();
	bool isINC= popbool ();
	asmin.doINC_r (isINC, prefix, reg);
	stop ();
}

void AsmMachine::doINC_indHL ()
{
	bool isINC= popbool ();
	asmin.doINC_r (isINC, prefixNone, reg_HL_);
	stop ();
}

void AsmMachine::doINC_idesp ()
{
	byte desp= popdesp ();
	byte prefix= popprefix ();
	bool isINC= popbool ();
	if (prefix == prefixIX)
		asmin.doINC_IX (isINC, desp);
	else
		asmin.doINC_IY (isINC, desp);
	stop ();
}

void AsmMachine::doINC_rr ()
{
	regwCode reg= popregw ();
	bool isINC= popbool ();
	asmin.doINC_rr (isINC, reg, prefixNone);
	stop ();
}

void AsmMachine::doINC_IX ()
{
	bool isINC= popbool ();
	asmin.doINC_rr (isINC, regHL, prefixIX);
	stop ();
}

void AsmMachine::doINC_IY ()
{
	bool isINC= popbool ();
	asmin.doINC_rr (isINC, regHL, prefixIY);
	stop ();
}

void AsmMachine::doINC_r (bool isINC, byte prefix)
{
	asmin.doINC_r (isINC, prefix, popregb () );
	stop ();
}

void AsmMachine::doINC_rr (bool isINC)
{
	asmin.doINC_rr (isINC, popregw (), prefixNone);
	stop ();
}

void AsmMachine::doJP ()
{
	//asmin.doJP (popvalue () );
	asmin.doJP (popvar () );
	stop ();
}

void AsmMachine::doJP_flag ()
{
	//address dest= popvalue ();

	VarData vd= st.pop ();

	flagCode fcode= popflag_val ();

	//asmin.doJP_flag (fcode, dest);
	asmin.doJP_flag (fcode, vd);
	stop ();
}

void AsmMachine::doJP_indHL ()
{
	asmin.doJP_indHL ();
	stop ();
}

void AsmMachine::doJP_indIX ()
{
	asmin.doJP_indIX ();
	stop ();
}

void AsmMachine::doJP_indIY ()
{
	asmin.doJP_indIY ();
	stop ();
}

void AsmMachine::doJR ()
{
	asmin.doJR (popvar () );
	stop ();
}

void AsmMachine::doJR_flag ()
{
	//address dest= popvalue ();
	VarData vd= popvar ();
	flagCode fcode= popflag_val ();
	asmin.doJR_flag (fcode, vd);
	stop ();
}

void AsmMachine::doLD_A_I ()
{
	asmin.doLDir (codeLD_A_I);
	stop ();
}

void AsmMachine::doLD_A_R ()
{
	asmin.doLDir (codeLD_A_R);
	stop ();
}

void AsmMachine::doLD_I_A ()
{
	asmin.doLDir (codeLD_I_A);
	stop ();
}

void AsmMachine::doLD_R_A ()
{
	asmin.doLDir (codeLD_R_A);
	stop ();
}

void AsmMachine::doLD_r_r ()
{
	regbCode code2= popregb ();
	regbCode code1= popregb ();
	asmin.doLD_r_r (code1, code2);
	stop ();
}

void AsmMachine::doLD_r_undoc ()
{
	byte prefix= popprefix ();
	regbCode code2= popregb ();
	regbCode code1= popregb ();
	//asmin.doLDrr (code1, code2, prefix);
	asmin.doLD_r_undoc (code1, code2, prefix);
	stop ();
}

void AsmMachine::doLD_r_indHL ()
{
	regbCode code1= popregb ();
	asmin.doLD_r_r (code1, reg_HL_);
	stop ();
}

void AsmMachine::doLD_r_idesp ()
{
	byte desp= popdesp ();
	byte prefix= popprefix ();
	regbCode code1= popregb ();
	//asmin.doLDrr (code1, reg_HL_, prefix, desp);
	asmin.doLD_r_idesp (code1, prefix, desp);
	stop ();
}

void AsmMachine::doLD_r_n ()
{
	byte n= popbyte ();
	regbCode reg= popregb ();
	asmin.doLD_r_n (reg, n);
	stop ();
}

void AsmMachine::doLD_undoc_r ()
{
	regbCode code2= popregb ();
	byte prefix= popprefix ();
	regbCode code1= popregb ();
	//asmin.doLDrr (code1, code2, prefix);
	asmin.doLD_undoc_r (code1, prefix, code2);
	stop ();
}

void AsmMachine::doLD_undoc_n ()
{
	byte n= popbyte ();
	byte prefix= popprefix ();
	regbCode reg= popregb ();
	//asmin.doLDrn (reg, prefix, n);
	asmin.doLD_undoc_n (reg, prefix, n);
	stop ();
}

void AsmMachine::doLD_A_r ()
{
	regbCode code2= popregb ();
	asmin.doLD_r_r (regA, code2);
	stop ();
}

void AsmMachine::doLD_A_undoc ()
{
	byte prefix= popprefix ();
	regbCode code2= popregb ();
	//asmin.doLDrr (regA, code2, prefix);
	asmin.doLD_r_undoc (regA, code2, prefix);
	stop ();
}

void AsmMachine::doLD_A_n ()
{
	byte n= popbyte ();
	asmin.doLD_r_n (regA, n);
	stop ();
}

void AsmMachine::doLD_A_indexp ()
{
	asmin.doLD_A_indexp (popvar () );
	stop ();
}

void AsmMachine::doLD_A_indBC ()
{
	asmin.doLD_A_indBC ();
	stop ();
}

void AsmMachine::doLD_A_indDE ()
{
	asmin.doLD_A_indDE ();
	stop ();
}

void AsmMachine::doLD_A_indHL ()
{
	asmin.doLD_r_r (regA, reg_HL_);
	stop ();
}

void AsmMachine::doLD_A_idesp ()
{
	byte desp= popdesp ();
	byte prefix= popprefix ();
	//asmin.doLDrr (regA, reg_HL_, prefix, desp);
	asmin.doLD_r_idesp (regA, prefix, desp);
	stop ();
}

void AsmMachine::doLD_indHL_r ()
{
	regbCode code2= popregb ();
	asmin.doLD_r_r (reg_HL_, code2);
	stop ();
}

void AsmMachine::doLD_indHL_n ()
{
	byte n= popbyte ();
	asmin.doLD_r_n (reg_HL_, n);
	stop ();
}

void AsmMachine::doLD_indBC ()
{
	asmin.doLD_indBC_A ();
	stop ();
}

void AsmMachine::doLD_indDE ()
{
	asmin.doLD_indDE_A ();
	stop ();
}

void AsmMachine::doLD_indexp_A ()
{
	asmin.doLD_indexp_A (popvar () );
	stop ();
}

void AsmMachine::doLD_indexp_BC ()
{
	asmin.doLD_indexp_BC (popvar () );
	stop ();
}

void AsmMachine::doLD_indexp_DE ()
{
	asmin.doLD_indexp_DE (popvar () );
	stop ();
}

void AsmMachine::doLD_indexp_HL ()
{
	asmin.doLD_indexp_HL (popvar () );
	stop ();
}

void AsmMachine::doLD_indexp_SP ()
{
	asmin.doLD_indexp_SP (popvar () );
	stop ();
}

void AsmMachine::doLD_indexp_IX ()
{
	asmin.doLD_indexp_IX (popvar () );
	stop ();
}

void AsmMachine::doLD_indexp_IY ()
{
	asmin.doLD_indexp_IY (popvar () );
	stop ();
}

void AsmMachine::doLD_idesp_r ()
{
	regbCode code2= popregb ();
	byte desp= popdesp ();
	byte prefix= popprefix ();
	//asmin.doLDrr (reg_HL_, code1, prefix, desp);
	asmin.doLD_idesp_r (prefix, desp, code2);
	stop ();
}

void AsmMachine::doLD_idesp_n ()
{
	TRFUNC (tr, "AsmMachine::doLD_idesp_n");

	byte n= popbyte ();
	byte desp= popdesp ();
	byte prefix= popprefix ();
	asmin.doLD_idesp_n (prefix, desp, n);
	TRSTREAM (tr, "n: " << hex2 (n) << "prefix: " << hex2 (prefix) <<
		"desp: " << hex2 (desp) );
	stop ();
}


void AsmMachine::doLD_SP_HL ()
{
	asmin.doLD_SP_HL ();
	stop ();
}

void AsmMachine::doLD_SP_IX ()
{
	asmin.doLD_SP_IX ();
	stop ();
}

void AsmMachine::doLD_SP_IY ()
{
	asmin.doLD_SP_IY ();
	stop ();
}

void AsmMachine::doLD_SP_nn ()
{
	//asmin.doLD_SP_nn (popvalue () );
	asmin.doLD_SP_nn (popvar () );
	stop ();
}

void AsmMachine::doLD_SP_indexp ()
{
	//asmin.doLD_SP_indexp (popvalue () );
	asmin.doLD_SP_indexp (popvar () );
	stop ();
}

void AsmMachine::doLD_HL_nn ()
{
	//asmin.doLD_HL_nn (popvalue () );
	asmin.doLD_HL_nn (popvar () );
	stop ();
}

void AsmMachine::doLD_HL_indexp ()
{
	//address n= popvalue ();
	//asmin.doLD_HL_indexp (n);
	asmin.doLD_HL_indexp (popvar () );
	stop ();
}

void AsmMachine::doLD_rr_nn ()
{
	TRFUNC (tr, "AsmMachine::doLD_rr_nn");

	//address n= popvalue ();
	VarData n= popvar ();
	TRMESSAGE (tr, n.getname () );

	//byte prefix= popprefix ();
	regwCode rcode= popregw ();
	asmin.doLD_rr_nn (rcode, n);
	stop ();
}

void AsmMachine::doLD_rr_indexp ()
{
	//address n= popvalue ();
	VarData n= popvar ();

	//byte prefix= popprefix ();
	regwCode rcode= popregw ();
	asmin.doLD_rr_indexp (rcode, n);
	stop ();
}

void AsmMachine::doLD_IXY_nn ()
{
	VarData n= popvar ();

	byte prefix= popprefix ();
	//regwCode rcode= popregw ();
	asmin.doLD_IXY_nn (prefix, n);
	stop ();
}

void AsmMachine::doLD_IXY_indexp ()
{
	//address n= popvalue ();
	VarData n= popvar ();

	byte prefix= popprefix ();
	//regwCode rcode= popregw ();
	asmin.doLD_IXY_indexp (prefix, n);
	stop ();
}

void AsmMachine::doOUT_C_ ()
{
	TRFUNC (tr, "AsmMachine::out_C_");

	regbCode rcode= popregb ();
	asmin.doOUT_C_ (rcode);
	stop ();
}

void AsmMachine::doOUT_n_ ()
{
	TRFUNC (tr, "AsmMachine::out_n_");

	const address addr= popvalue ();
	const byte b= static_cast <byte> (addr);
	asmin.doOUT_n_ (b);
	stop ();
}

void AsmMachine::doPUSH_rr ()
{
	regwCode reg= popregw ();
	bool isPUSH= popbool ();
	asmin.doPUSHPOP (reg, prefixNone, isPUSH);
	stop ();
}

void AsmMachine::doPUSH_IX ()
{
	bool isPUSH= popbool ();
	asmin.doPUSHPOP (regHL, prefixIX, isPUSH);
	stop ();
}

void AsmMachine::doPUSH_IY ()
{
	bool isPUSH= popbool ();
	asmin.doPUSHPOP (regHL, prefixIY, isPUSH);
	stop ();
}

void AsmMachine::doRET ()
{
	asmin.doRET ();
	stop ();
}

void AsmMachine::doRET_flag ()
{
	asmin.doRETflag (popflag_val () );
	stop ();
}

void AsmMachine::doRL_r ()
{
	regbCode reg= popregb ();
	byte codeinst= popfixbyte ();
	asmin.doByteInstCB (codeinst, reg);
	stop ();
}

void AsmMachine::doRL_undoc ()
{
	byte prefix= popprefix ();
	regbCode reg= popregb ();
	byte codeinst= popfixbyte ();
	asmin.doByteInstCB (codeinst, reg, prefix);
	stop ();
}

void AsmMachine::doRL_indhl ()
{
	byte codeinst= popfixbyte ();
	asmin.doByteInstCB (codeinst, reg_HL_);
	stop ();
}

void AsmMachine::doRL_idesp ()
{
	byte desp= popdesp ();
	byte prefix= popprefix ();
	byte codeinst= popfixbyte ();
	asmin.doByteInstCB (codeinst, reg_HL_, prefix, true, desp);
	stop ();
}

void AsmMachine::doRST ()
{
	asmin.doRST (popvalue () );
	stop ();
}

void AsmMachine::doSBC_HL ()
{
	asmin.doADDADCSBC_HL (codeSBCHL, popregw (), prefixNone);
	stop ();
}

void AsmMachine::run ()
{
	TRF;

	const inst_t::size_type instsize= inst.size ();
	start ();
	while (! stopped)
	{
		if (progcounter >= instsize)
			throw logic_error ("Machine exahusted");
		Instruc & in= inst [progcounter ++];
		switch (in.getop () )
		{

		case OpNumber:        push (in.getvalue () ); break;
		case OpIdentifier:    push (in.getident () ); break;
		//case OpLiteral:       push (in.getvalue () ); break;
		case OpLiteral:       pushliteral (in.getliteral () ); break;
		case OpDollar:
			//TRDEB ("'$'");
			//push (asmin.getcurrentinstruction () );
			opDollar ();
			break;
		case OpDefined:       st.defined ();      break;
		case OpAdd:           st.add ();          break;
		case OpSub:           st.sub ();          break;
		case OpMul:           st.mul ();          break;
		case OpDiv:           st.div ();          break;
		case OpMod:           st.mod ();          break;
		case OpShl:           st.shl ();          break;
		case OpShr:           st.shr ();          break;
		case OpEqual:         st.equal ();        break;
		case OpNotEqual:      st.notequal ();     break;
		case OpLessThan:      st.lessthan ();     break;
		case OpGreaterThan:   st.greaterthan ();  break;
		case OpLessEqual:     st.lessequal ();    break;
		case OpGreaterEqual:  st.greaterequal (); break;
		case OpNot:           st.do_not ();       break;
		case OpBoolNot:       st.do_boolnot ();   break;
		case OpUnPlus:                            break;
		case OpUnMinus:       st.do_minus ();     break;
		case OpAnd:           st.do_and ();       break;
		case OpOr:            st.do_or ();        break;
		case OpXor:           st.do_xor ();       break;
		case OpBoolAnd:       st.booland ();      break;
		case OpBoolOr:        st.boolor ();       break;
		case OpHigh:          st.do_high ();      break;
		case OpLow:           st.do_low ();       break;
		case OpConditional:   st.cond ();         break;

		case OpStop:          stop ();            break;
		case OpEmpty:         doEmpty ();         break;
		case OpGenLabel:      doGenLabel ();      break;
		case OpAddVarItem:    doAddVarItem ();    break;
		case OpMacroItem:     doMacroItem ();     break;
		case OpMacroArg:      doMacroArg ();      break;
		case OpMacroValue:    doMacroValue ();    break;
		case OpExpandMacro:   doExpandMacro ();   break;

		case OpASEG:          doASEG ();          break;
		case OpCSEG:          doCSEG ();          break;
		case OpDSEG:          doDSEG ();          break;

		//case OpDEFBliteral:   doDEFBliteral ();   break;
		case OpDEFBnum:       doDEFBnum ();       break;
		case OpDEFBend:       doDEFBend ();       break;
		case OpDEFL:          doDEFL ();          break;
		case OpDEFS:          doDEFS ();          break;
		case OpDEFSvalue:     doDEFSvalue ();     break;
		case OpDEFWnum:       doDEFWnum ();       break;
		case OpDEFWend:       doDEFWend ();       break;
		case OpELSE:          doELSE ();          break;
		case OpEND:           doEND ();           break;
		case OpENDn:          doENDn ();          break;
		case OpENDIF:         doENDIF ();         break;
		case OpENDM:          doENDM ();          break;
		case OpENDP:          doENDP ();          break;
		case OpEQU:           doEQU ();           break;
		case OpEXITM:         doEXITM ();         break;
		case OpEXTRN:         doEXTRN ();         break;
		case OpIF:            doIF ();            break;
		case OpIF1:           doIF1 ();           break;
		case OpIF2:           doIF2 ();           break;
		case OpIFDEF:         doIFDEF ();         break;
		case OpIFNDEF:        doIFNDEF ();        break;
		case OpINCBIN:        doINCBIN ();        break;
		case OpINCLUDE:       doINCLUDE ();       break;
		case OpEndINCLUDE:    doEndINCLUDE ();    break;
		case OpIRP:           doIRP ();           break;
		case OpIRPC:          doIRPC ();          break;
		case OpLOCAL:         doLOCAL ();         break;
		case OpMACRO:         doMACRO ();         break;
		case OpORG:           doORG ();           break;
		case OpPROC:          doPROC ();          break;
		case OpPUBLIC:        doPUBLIC ();        break;
		case OpREPT:          doREPT ();          break;
		case Op_8080:         do_8080 ();         break;
		case Op_DEPHASE:      do_DEPHASE ();      break;
		case Op_ERROR:        do_ERROR ();        break;
		case Op_PHASE:        do_PHASE ();        break;
		case Op_SHIFT:        do_SHIFT ();        break;
		case Op_WARNING:      do_WARNING ();      break;
		case Op_Z80:          do_Z80 ();          break;

		case OpNoargInst:     doNoargInst ();     break;
		case OpEvalBitInst:   doEvalBitInst ();   break;

		case OpADC_HL:        doADC_HL ();        break;
		case OpADD_HL:        doADD_HL ();        break;
		case OpADD_IX:        doADD_IX ();        break;
		case OpADD_IY:        doADD_IY ();        break;
		case OpCALL:          doCALL ();          break;
		case OpCALL_flag:     doCALL_flag ();     break;
		case OpCP_r:          doCP_r ();          break;
		case OpCP_undoc:      doCP_undoc ();      break;
		case OpCP_indHL:      doCP_indHL ();      break;
		case OpCP_idesp:      doCP_idesp ();      break;
		case OpCP_n:          doCP_n ();          break;
		case OpDJNZ:          doDJNZ ();          break;
		case OpEX_indSP_HL:   doEX_indSP_HL ();   break;
		case OpEX_indSP_IX:   doEX_indSP_IX ();   break;
		case OpEX_indSP_IY:   doEX_indSP_IY ();   break;
		case OpEX_AF_AFP:     doEX_AF_AFP ();     break;
		case OpEX_DE_HL:      doEX_DE_HL ();      break;
		case OpIM:            doIM ();            break;
		case OpIN_A_indC_:    doIN_A_indC ();     break;
		case OpIN_A_indn:     doIN_A_indn ();     break;
		case OpIN_r_indC:     doIN_r_indC ();     break;
		case OpINC_r:         doINC_r ();         break;
		case OpINC_undoc:     doINC_undoc ();     break;
		case OpINC_indHL:     doINC_indHL ();     break;
		case OpINC_idesp:     doINC_idesp ();     break;
		case OpINC_rr:        doINC_rr ();        break;
		case OpINC_IX:        doINC_IX ();        break;
		case OpINC_IY:        doINC_IY ();        break;
		case OpJP:            doJP ();            break;
		case OpJP_flag:       doJP_flag ();       break;
		case OpJP_indHL:      doJP_indHL ();      break;
		case OpJP_indIX:      doJP_indIX ();      break;
		case OpJP_indIY:      doJP_indIY ();      break;
		case OpJR:            doJR ();            break;
		case OpJR_flag:       doJR_flag ();       break;

		case OpLD_A_I:        doLD_A_I ();        break;
		case OpLD_A_R:        doLD_A_R ();        break;
		case OpLD_I_A:        doLD_I_A ();        break;
		case OpLD_R_A:        doLD_R_A ();        break;
		case OpLD_r_r:        doLD_r_r ();        break;
		case OpLD_r_n:        doLD_r_n ();        break;
		case OpLD_r_undoc:    doLD_r_undoc ();    break;
		case OpLD_r_indHL:    doLD_r_indHL ();    break;
		case OpLD_r_idesp:    doLD_r_idesp ();    break;
		case OpLD_undoc_r:    doLD_undoc_r ();    break;
		case OpLD_undoc_n:    doLD_undoc_n ();    break;
		case OpLD_A_r:        doLD_A_r ();        break;
		case OpLD_A_undoc:    doLD_A_undoc ();    break;
		case OpLD_A_n:        doLD_A_n ();        break;
		case OpLD_A_indexp:   doLD_A_indexp ();   break;
		case OpLD_A_indBC:    doLD_A_indBC ();    break;
		case OpLD_A_indDE:    doLD_A_indDE ();    break;
		case OpLD_A_indHL:    doLD_A_indHL ();    break;
		case OpLD_A_idesp:    doLD_A_idesp ();    break;
		case OpLD_indHL_r:    doLD_indHL_r ();    break;
		case OpLD_indHL_n:    doLD_indHL_n ();    break;
		case OpLD_indBC:      doLD_indBC ();      break;
		case OpLD_indDE:      doLD_indDE ();      break;
		case OpLD_indexp_A:   doLD_indexp_A ();   break;
		case OpLD_indexp_BC:  doLD_indexp_BC ();  break;
		case OpLD_indexp_DE:  doLD_indexp_DE ();  break;
		case OpLD_indexp_HL:  doLD_indexp_HL ();  break;
		case OpLD_indexp_SP:  doLD_indexp_SP ();  break;
		case OpLD_indexp_IX:  doLD_indexp_IX ();  break;
		case OpLD_indexp_IY:  doLD_indexp_IY ();  break;
		case OpLD_idesp_r:    doLD_idesp_r ();    break;
		case OpLD_idesp_n:    doLD_idesp_n ();    break;
		case OpLD_SP_HL:      doLD_SP_HL ();      break;
		case OpLD_SP_IX:      doLD_SP_IX ();      break;
		case OpLD_SP_IY:      doLD_SP_IY ();      break;
		case OpLD_SP_nn:      doLD_SP_nn ();      break;
		case OpLD_SP_indexp:  doLD_SP_indexp ();  break;
		case OpLD_HL_nn:      doLD_HL_nn ();      break;
		case OpLD_HL_indexp:  doLD_HL_indexp ();  break;
		case OpLD_IXY_nn:     doLD_IXY_nn ();     break;
		case OpLD_IXY_indexp: doLD_IXY_indexp (); break;
		case OpLD_rr_nn:      doLD_rr_nn ();      break;
		case OpLD_rr_indexp:  doLD_rr_indexp ();  break;

		case OpOUT_C_:        doOUT_C_ ();        break;
		case OpOUT_n_:        doOUT_n_ ();        break;
		case OpPUSH_rr:       doPUSH_rr ();       break;
		case OpPUSH_IX:       doPUSH_IX ();       break;
		case OpPUSH_IY:       doPUSH_IY ();       break;
		case OpRET:           doRET ();           break;
		case OpRET_flag:      doRET_flag ();      break;
		case OpRL_r:          doRL_r ();          break;
		case OpRL_undoc:      doRL_undoc ();      break;
		case OpRL_indhl:      doRL_indhl ();      break;
		case OpRL_idesp:      doRL_idesp ();      break;
		case OpRST:           doRST ();           break;
		case OpSBC_HL:        doSBC_HL ();        break;

		default:
			throw logic_error ("Invalid parse machine operator");
		}
	}
}

void AsmMachine::exec ()
{
	TRFUNC (tr, "AsmMachine::exec");

	if (inst.size () == 0)
		throw logic_error ("empty parser result");

	run ();

	if (! st.empty () )
		throw logic_error ("parse machine stack not exhausted");
	if (progcounter < inst.size () )
		throw logic_error ("parse machine with unused codes");
}

} // namespace impl
} // namespace pasmo

int pasmo::impl::yylex (YYSTYPE * lvalp, Machine & machine)
{
	TRFUNC (tr, "pasmo_impl::yylex");

	* lvalp= machine.gettoken ();
	//cerr << "Token: " << * lvalp << endl;
	return lvalp->type ();
}

void pasmo::impl::yyerror (Machine & machine, const char *s)
{
	machine.syntax (s);
}

// End of machine.cpp
