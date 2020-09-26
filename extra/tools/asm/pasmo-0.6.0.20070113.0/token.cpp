// token.cpp
// Revision 9-oct-2006


#include "token.h"
#include "parser.h"

#include "trace.h"


#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <deque>
#include <map>
#include <algorithm>
#include <stdexcept>


namespace pasmo {
namespace impl {


using std::istream;
using std::cout;
using std::cerr;
using std::endl;
using std::istringstream;
using std::ostringstream;
using std::ostream_iterator;
using std::deque;
using std::map;
using std::for_each;
using std::copy;
using std::runtime_error;
using std::logic_error;


namespace token {

//*********************************************************
//		class ScanString
//*********************************************************

class ScanString {
public:
	ScanString (const string & str);
	char get ();
	void unget ();
	operator void * const () const;

	typedef string::size_type pos_type;
	pos_type getpos () const;
	void setpos (pos_type newpos);
private:
	const string & s;
	const string::size_type len;
	string::size_type pos;
};


ScanString::ScanString (const string & str) :
	s (str),
	len (str.size () ),
	pos (0)
{
}

char ScanString::get ()
{
	const char c ( (pos < len) ? s [pos] : '\0');
	++pos;
	return c;
}

void ScanString::unget ()
{
	ASSERT (pos > 0);
	--pos;
}

ScanString::operator void * const () const
{
	// Return a guaranteed no null as true, null as false.
	// Is false after first reading out of the string.
	static char truevalue [1]= {};
	return pos <= len ?
		static_cast <void *> (truevalue) :
		static_cast <void *> (0);
}

ScanString::pos_type ScanString::getpos () const
{
	return pos;
}

void ScanString::setpos (pos_type newpos)
{
	pos= newpos;
}

//*********************************************************
//		Auxiliary functions and constants.
//*********************************************************

template <class T, size_t N>
size_t dim_array (const T (&) [N] )
{ return N; }

template <class T, size_t N>
const T * end_array (const T (& t) [N] )
{ return t + dim_array (t); }

const address LiteralRaw=      0;
const address LiteralStyleAsm= 1;
const address LiteralStyleC=   2;


logic_error InvalidFlagConvert ("Inalid flag specified for conversion");
logic_error tokenizerunderflow ("Tokenizer underflowed");
logic_error missingfilename ("Missing filename");

runtime_error unclosed ("Unclosed literal");
logic_error bad_encode ("Bad internal string encoding");
runtime_error invalidhex ("Invalid hexadecimal number");
runtime_error invalidnumber ("Invalid numeric format");
runtime_error outofrange ("Number out of range");
runtime_error needfilename ("Filename required");
runtime_error invalidfilename ("Invalid file name");


typedef map <string, TypeToken> tmap_t;

tmap_t tmapZ80;
tmap_t tmap8080;

typedef map <TypeToken, string> invmap_t;

invmap_t invmapZ80;
invmap_t invmap8080;

struct NameType {
	const char * const str;
	const TypeToken type;
	NameType (const char * str, TypeToken type) :
		str (str), type (type)
	{ }
};

class MapInserter {
public:
	MapInserter (tmap_t & dirmap_n, invmap_t & invmap_n);
	void operator () (const NameType & nt);
private:
	tmap_t & dirmap;
	invmap_t & invmap;
};

MapInserter::MapInserter (tmap_t & dirmap_n, invmap_t & invmap_n) :
	dirmap (dirmap_n),
	invmap (invmap_n)
{
}

void MapInserter::operator () (const NameType & nt)
{
	TypeToken tt= nt.type;
	const char * const str= nt.str;
	dirmap [str]= tt;
	invmap [tt]= str;
}


#define NT(n) NameType (#n, Type ## n)

#define NT_(n) NameType ("." #n, Type_ ## n)

#define NT8080(n) NameType (#n, Type ## n ## _8080)

const NameType ntCommonOpers []= {
	NT (MOD),
	NT (SHL),
	NameType ("<<", TypeShlOp),
	NT (SHR),
	NameType (">>", TypeShrOp),
	NT (NOT),
	NT (EQ),
	NT (LT),
	NT (LE),
	NameType ("<=", TypeLeOp),
	NT (GT),
	NT (GE),
	NameType (">=", TypeGeOp),
	NT (NE),
	NameType ("!=", TypeNeOp),
	NT (NUL),
	NT (DEFINED),
	NT (HIGH),
	NT (LOW),
	NameType ("&&", TypeBoolAnd),
	NameType ("||", TypeBoolOr),
	NameType ("##", TypeSharpSharp)
};

const NameType ntCommonDirectives []= {
	NT (DEFB),
	NT (DB),
	NT (DEFM),
	NT (DEFW),
	NT (DW),
	NT (DEFS),
	NT (DS),
	NT (EQU),
	NT (ASEG),
	NT (CSEG),
	NT (DSEG),
	NT (COMMON),
	NT (ORG),
	NT (INCLUDE),
	NT (INCBIN),
	NT (IF),
	NT (IF1),
	NT (IF2),
	NT (IFDEF),
	NT (IFNDEF),
	NT (ELSE),
	NT (ENDIF),
	NT (EXTRN),
	NT (PUBLIC),
	NT (END),
	NT (LOCAL),
	NT (PROC),
	NT (ENDP),
	NT (MACRO),
	NT (ENDM),
	NT (EXITM),
	NT (REPT),
	NT (IRP),
	NT (IRPC),
	// Directives with .
	NT_ (8080),
	NT_ (8086),
	NT_ (ERROR),
	NT_ (WARNING),
	NT_ (SHIFT),
	NT_ (PHASE),
	NT_ (DEPHASE),
	NT_ (Z80)
};

const NameType ntCommonRegs []= {
	NT (A),
	NT (B),
	NT (C),
	NT (D),
	NT (E),
	NT (H),
	NT (L),
	NT (SP)
};

const NameType ntNemCommon []= {
	NT (DAA),
	NT (DI),
	NT (EI),
	NT (NOP),
};

const NameType ntNemZ80 []= {
	NT (ADC),
	NT (ADD),
	NT (AND),
	NT (BIT),
	NT (CALL),
	NT (CCF),
	NT (CP),
	NT (CPD),
	NT (CPDR),
	NT (CPI),
	NT (CPIR),
	NT (CPL),
	NT (DEC),
	NT (DJNZ),
	NT (EX),
	NT (EXX),
	NT (HALT),
	NT (IM),
	NT (IN),
	NT (INC),
	NT (IND),
	NT (INDR),
	NT (INI),
	NT (INIR),
	NT (JP),
	NT (JR),
	NT (LD),
	NT (LDD),
	NT (LDDR),
	NT (LDI),
	NT (LDIR),
	NT (NEG),
	NT (OR),
	NT (OTDR),
	NT (OTIR),
	NT (OUT),
	NT (OUTD),
	NT (OUTI),
	NT (POP),
	NT (PUSH),
	NT (RES),
	NT (RET),
	NT (RETI),
	NT (RETN),
	NT (RL),
	NT (RLA),
	NT (RLC),
	NT (RLCA),
	NT (RLD),
	NT (RR),
	NT (RRA),
	NT (RRC),
	NT (RRCA),
	NT (RRD),
	NT (RST),
	NT (SBC),
	NT (SCF),
	NT (SET),
	NT (SLA),
	NT (SLL),
	NT (SRA),
	NT (SRL),
	NT (SUB),
	NT (XOR)
};

const NameType ntOtherZ80 []= {
	// Registers.
	NT (AF),
	NameType ("AF'", TypeAFp),
	NT (BC),
	NT (DE),
	NT (HL),
	NT (IX),
	NT (IXH),
	NT (IXL),
	NT (IY),
	NT (IYH),
	NT (IYL),
	NT (I),
	NT (R),
	// Flags (C is listed as register)
	NT (NZ),
	NT (Z),
	NT (NC),
	NT (PO),
	NT (PE),
	NT (P),
	NT (M),
	// Directives
	NT (DEFL)
};

const NameType ntNem8080 []= {
	NT8080 (ADC),
	NT8080 (ADD),
	NT8080 (ACI),
	NT8080 (ADI),
	NT8080 (ANA),
	NT8080 (ANI),
	NT8080 (CALL),
	NT8080 (CC),
	NT8080 (CM),
	NT8080 (CMA),
	NT8080 (CMC),
	NT8080 (CMP),
	NT8080 (CNC),
	NT8080 (CNZ),
	NT8080 (CP),
	NT8080 (CPE),
	NT8080 (CPI),
	NT8080 (CPO),
	NT8080 (CZ),
	NT8080 (DAD),
	NT8080 (DCR),
	NT8080 (DCX),
	NT8080 (HLT),
	NT8080 (IN),
	NT8080 (INR),
	NT8080 (INX),
	NT8080 (JC),
	NT8080 (JM),
	NT8080 (JMP),
	NT8080 (JNC),
	NT8080 (JNZ),
	NT8080 (JP),
	NT8080 (JPE),
	NT8080 (JPO),
	NT8080 (JZ),
	NT8080 (LDA),
	NT8080 (LDAX),
	NT8080 (LHLD),
	NT8080 (LXI),
	NT8080 (MVI),
	NT8080 (MOV),
	NT8080 (ORA),
	NT8080 (ORI),
	NT8080 (OUT),
	NT8080 (PCHL),
	NT8080 (POP),
	NT8080 (PUSH),
	NT8080 (RAL),
	NT8080 (RAR),
	NT8080 (RLC),
	NT8080 (RC),
	NT8080 (RRC),
	NT8080 (RET),
	NT8080 (RM),
	NT8080 (RNC),
	NT8080 (RNZ),
	NT8080 (RP),
	NT8080 (RPE),
	NT8080 (RPO),
	NT8080 (RST),
	NT8080 (RZ),
	NT8080 (SBB),
	NT8080 (SBI),
	NT8080 (SHLD),
	NT8080 (SPHL),
	NT8080 (STA),
	NT8080 (STAX),
	NT8080 (STC),
	NT8080 (SUB),
	NT8080 (SUI),
	NT8080 (XCHG),
	NT8080 (XRA),
	NT8080 (XRI),
	NT8080 (XTHL)
};

const NameType ntOther8080 []= {
	// Operators.
	NT8080 (AND),
	NT8080 (OR),
	NT8080 (XOR),
	// Registers.
	NT8080 (M),
	NT8080 (PSW),
	// Directives
	NT8080 (SET)
};

class mapiniter {
	mapiniter ();
	static mapiniter instance;
};

mapiniter mapiniter::instance;

mapiniter::mapiniter ()
{
	const NameType * endCommonOpers= end_array (ntCommonOpers);
	const NameType * endCommonRegs= end_array (ntCommonRegs);
	const NameType * endCommonDirectives= end_array (ntCommonDirectives);
	const NameType * endNemCommom= end_array (ntNemCommon);

	const NameType * endNemZ80= end_array (ntNemZ80);
	const NameType * endNem8080= end_array (ntNem8080);
	const NameType * endOtherZ80= end_array (ntOtherZ80);
	const NameType * endOther8080= end_array (ntOther8080);

	MapInserter mapsZ80 (tmapZ80, invmapZ80);
	MapInserter maps8080 (tmap8080, invmap8080);

	// Common parts.
	for_each (ntCommonOpers, endCommonOpers, mapsZ80);
	for_each (ntCommonOpers, endCommonOpers, maps8080);
	for_each (ntCommonRegs, endCommonRegs, mapsZ80);
	for_each (ntCommonRegs, endCommonRegs, maps8080);
	for_each (ntCommonDirectives, endCommonDirectives, mapsZ80);
	for_each (ntCommonDirectives, endCommonDirectives, maps8080);
	for_each (ntNemCommon, endNemCommom, mapsZ80);
	for_each (ntNemCommon, endNemCommom, maps8080);

	// Z80 parts
	for_each (ntNemZ80, endNemZ80, mapsZ80);
	for_each (ntOtherZ80, endOtherZ80, mapsZ80);

	// 8080 parts
	for_each (ntNem8080, endNem8080, maps8080);
	for_each (ntOther8080, endOther8080, maps8080);
}

TypeToken getliteraltokeninmap (const string & str, const tmap_t & map)
{
	tmap_t::const_iterator it= map.find (upper (str) );
	if (it != map.end () )
		return it->second;
	else
		return TypeUndef;
}

TypeToken getliteraltoken (const string & str, AsmMode asmmode)
{
	switch (asmmode)
	{
	case AsmZ80:
		return getliteraltokeninmap (str, tmapZ80);
	case Asm8080:
		return getliteraltokeninmap (str, tmap8080);
	default:
		throw logic_error ("Unexpected assembly type");
	}
}

class addescaped {
public:
	addescaped (string & str) : str (str)
	{ }
	void operator () (char c);
private:
	string & str;
};

void addescaped::operator () (char c)
{
	unsigned char c1= c;
	if (c1 >= ' ' && c1 < 128)
		str+= c;
	else
	{
		str+= "\\x";
		str+= hex2str (c1);
	}
}

string escapestr (const string & str)
{
	string result;
	for_each (str.begin (), str.end (), addescaped (result) );
	return result;
}

string unescapestringasm (const string & s)
{
	TRFUNC (tr, "unescapestringasm");
	TRMESSAGE (tr, s);

	string result (s);
	string::size_type pos= 0;
	while ( (pos= result.find ('\'', pos) ) != string::npos)
	{
		result.erase (pos, 1);
		ASSERT (result [pos] == '\'');
		++pos;
	}
	TRMESSAGE (tr, result);
	return result;
}

string unescapestringc (const string & s)
{
	ScanString iss (s);
	string str;
	for (;;)
	{
		char c= iss.get ();
		if (! iss)
			return str;
		if (c == '"')
			throw bad_encode;
		if (c == '\\')
		{
			c= iss.get ();
			if (! iss)
				throw bad_encode;
			switch (c)
			{
			case '\\': break;
			case 'n':  c= '\n'; break;
			case 'r':  c= '\r'; break;
			case 't':  c= '\t'; break;
			case 'a':  c= '\a'; break;
			case '"':  break;
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				c-= '0';
				for (size_t i= 0; i < 2; ++i)
				{
					char c2= iss.get ();
					if (! iss)
						break;
					if (c2 < '0' || c2 > '7')
					{
						iss.unget ();
						break;
					}
					c*= 8;
					c+= c2 - '0';
				}
				break;
			case 'x': case 'X':
				{
					char x [3]= { 0 };
					for (size_t i= 0; i < 2; ++i)
					{
						c= iss.get ();
						if (! iss)
							break;
						if (! ischarhex (c) )
						{
							iss.unget ();
							break;
						}
						x [i]= c;
					}
					c= strtoul (x, NULL, 16);
				}
				break;
			}
			str+= c;
		}
		else
			str+= c;
	}
}


} // namespace token


using namespace token;


//*********************************************************
//			Extern functions.
//*********************************************************


string gettokenname (TypeToken tt)
{
	invmap_t::iterator it= invmapZ80.find (tt);

	if (it != invmapZ80.end () )
	{
		return it->second;
	}
	else
	{
		it= invmap8080.find (tt);
		if (it != invmap8080.end () )
		{
			return it->second;
		}
		else
		{
			switch (tt)
			{
			case TypeEndLine:
				return "(end of line)";
			case TypeComment:
				return "(comment)";
			case TypeIdentifier:
				return "(identifier)";
			case TypeLiteral:
				return "(literal)";
			case TypeNoFileINCLUDE:
				return "INCLUDE (no file)";
			default:
				return string (1, static_cast <char> (tt) );
			}
		}
	}
}


//*********************************************************
//			class Token
//*********************************************************


Token::Token () :
	tt (TypeUndef)
{
}

Token::Token (TypeToken ttn) :
	tt (ttn)
{
}

Token::Token (address n) :
	tt (TypeNumber),
	number (n)
{
}

Token::Token (address n, const string & sn) :
	tt (TypeNumber),
	s (sn),
	number (n)
{
}

Token::Token (char c) :
	tt (TypeLiteral),
	s (string (1, c) )
{
}

Token::Token (TypeToken ttn, const string & sn, address style) :
	tt (ttn),
	s (sn),
	number (style)
{
	ASSERT (ttn == TypeLiteral || style == LiteralRaw);
}

TypeToken Token::type () const
{
	return tt;
}

bool Token::isliteral () const
{
	return tt == TypeLiteral;
}

string Token::raw () const
{
	switch (tt)
	{
	case TypeIdentifier:
	case TypeMacroName:
	case TypeMacroArg:
		return s;
	case TypeNumber:
		return numstr ();
	case TypeLiteral:
		switch (number)
		{
		case LiteralRaw:
			return s;
		case LiteralStyleAsm:
			return '\'' + s + '\'';
		case LiteralStyleC:
			return '\"' + s + '\"';
		default:
			throw logic_error ("Unexpected type of literal");
		}
	case TypeWhiteSpace:
		if (s.empty () )
			return " ";
		else
			return s;
	case TypeComment:
		return ';' + s;
	default:
		if (s.empty () )
			return str ();
		else
			return s;
	}
}

string Token::str () const
{
	switch (tt)
	{
	case TypeUndef:
		return "(undef)";
	case TypeEndLine:
		return "(end of line)";
	case TypeComment:
		return s;
	case TypeIdentifier:
		return s;
	case TypeLiteral:
		return literal ();
	case TypeMacroName:
		return s;
	case TypeMacroArg:
		return s;
	case TypeNumber:
		if (s.empty () )
			return "0" + hex4str (number) + "h";
		else
			return s;
	case TypeWhiteSpace:
		return " ";
	default:
		return gettokenname (tt);
	}
}

address Token::num () const
{
	ASSERT (tt == TypeNumber);

	return number;
}

string Token::numstr () const
{
	ASSERT (tt == TypeNumber);

	if (s.empty () )
	{
		ostringstream oss;
		oss << number;
		return oss.str ();
	}
	else
		return s;
}

string Token::literal () const
{
	ASSERT (tt == TypeLiteral);
	switch (number)
	{
	case LiteralRaw:
		return s;
	case LiteralStyleAsm:
		return unescapestringasm (s);
	case LiteralStyleC:
		return unescapestringc (s);
	default:
		throw logic_error ("Unexpected type of literal");
	}
}

string Token::identifier () const
{
	ASSERT (tt == TypeIdentifier);
	return s;
}

string Token::macroname () const
{
	ASSERT (tt == TypeMacroName);
	return s;
}

string Token::macroarg () const
{
	ASSERT (tt == TypeMacroArg);
	return s;
}

ostream & operator << (ostream & oss, const Token & tok)
{
	oss << tok.raw ();
	return oss;
}

//*********************************************************
//		Tokenizer auxiliary functions.
//*********************************************************

namespace tokenizer {

class UnexpectedChar : public runtime_error {
public:
	UnexpectedChar (char c) :
		runtime_error ("Unexpected character: " + chartostr (c) )
	{ }
private:
	static string chartostr (char c)
	{
		string r;
		if (c < 32 || c >= 127)
			r= '&' + hex2str (c);
		else
		{
			r= '\'';
			r+= c;
			r+= '\'';
		}
		return r;
	}
};

Token parseidentifier (ScanString & iss, char c, AsmMode asmmode)
{
	//TRFUNC (tr, "parseidentifier");

	string str;

	// Check conditional operator.
	if (c == '?')
	{
		str+= '?';
		c= iss.get ();
		if (! ischaridentifier (c) )
		{
			iss.unget ();
			return Token (TypeQuestion);
		}
	}

	do
	{
		str+= c;
		c= iss.get ();
	} while (ischaridentifier (c) );
	iss.unget ();

	const string rawstr (str);
	TypeToken tt= getliteraltoken (str, asmmode);

	if (tt == TypeUndef)
	{
		//stripdollar (str);
		//TRMESSAGE (tr, "Identifier: " + str);
		return Token (TypeIdentifier, str);
	}
	else
	{
		if (tt == TypeAF && c == '\'')
		{
			iss.get ();
			tt= TypeAFp;
		}
		return Token (tt, rawstr);
	}
}

Token parseless (ScanString & iss)
{
	//TRFUNC (tr, "parseless");

	char c= iss.get ();
	switch (c)
	{
	case '=':
		return Token (TypeLeOp);
	case '<':
		return Token (TypeShlOp);
	default:
		iss.unget ();
		return Token (TypeLtOp);
	}
}

Token parsegreat (ScanString & iss)
{
	//TRFUNC (tr, "parsegreat");

	char c= iss.get ();
	switch (c)
	{
	case '=':
		return Token (TypeGeOp);
	case '>':
		return Token (TypeShrOp);
	default:
		iss.unget ();
		return Token (TypeGtOp);
	}
}

Token parsenot (ScanString & iss)
{
	//TRFUNC (tr, "parsenot");

	char c= iss.get ();

	switch (c)
	{
	case '=':
		return Token (TypeNeOp);
	default:
		iss.unget ();
		return Token (TypeBoolNotOp);
	}
}

Token parsestringc (ScanString & iss)
{
	//TRFUNC (tr, "parsestringc");

	string str;
	for (;;)
	{
		char c= iss.get ();
		if (! iss)
			throw unclosed;
		if (c == '"')
		{
			return Token (TypeLiteral, str, LiteralStyleC);
		}
		if (c == '\\')
		{
			str+= c;
			c= iss.get ();
			if (! iss)
				throw unclosed;
			str+= c;
			switch (c)
			{
			case '\\': break;
			case 'n': break;
			case 'r': break;
			case 't': break;
			case 'a': break;
			case '"': break;
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				{
					c= iss.get ();
					if (! iss)
						throw unclosed;
					if (c < '0' || c > '7')
						iss.unget ();
					else
					{
						str+= c;
						c= iss.get ();
						if (! iss)
							throw unclosed;
						if (c < '0' || c > '7')
							iss.unget ();
						else
							str+= c;
					}
				}
				break;
			case 'x': case 'X':
				{
					c= iss.get ();
					if (! iss)
						throw unclosed;
					if (! ischarhex (c) )
						iss.unget ();
					else
					{
						str+= c;
						c= iss.get ();
						if (! iss)
							throw unclosed;
						if (! ischarhex (c) )
							iss.unget ();
						else
							str+= c;
					}
				}
				break;
			default:
				str+= c;
			}
		}
		else
		{
			str+= c;
		}
	}
}

Token parsestringasm (ScanString & iss)
{
	//TRFUNC (tr, "parsestringasm");

	string str;
	for (;;)
	{
		char c= iss.get ();
		if (! iss)
			throw unclosed;
		if (c == '\'')
		{
			c= iss.get ();
			if (! iss)
				return Token (TypeLiteral,
					str, LiteralStyleAsm);
			if (c != '\'')
			{
				iss.unget ();
				return Token (TypeLiteral,
					str, LiteralStyleAsm);
			}
			else
				str+= "''";
		}
		else
		{
			str+= c;
		}
	}
}

Token parsedollar (ScanString & iss)
{
	//TRFUNC (tr, "parsedollar");

	char c= iss.get ();
	#if 0
	if (iss && ischarhex (c) )
	{
		string rawstr (1, '$');
		string str;
		do
		{
			rawstr+= c;
			if (c != '$')
				str+= c;
			c= iss.get ();
		} while (iss && (ischarhex (c) || c == '$') );
		if (str.empty () )
			throw invalidhex;
		if (iss)
			iss.unget ();

		unsigned long n;
		char * aux;
		n= strtoul (str.c_str (), & aux, 16);
		if (* aux != '\0')
			throw invalidnumber;
		if (n > 0xFFFFUL)
			throw outofrange;
		return Token (static_cast <address> (n), rawstr);
		iss.unget ();
	}
	#else
	if (ischaridentifier (c) )
	{
		string rawstr (1, '$');
		string str;
		do
		{
			rawstr+= c;
			str+= c;
			c= iss.get ();
		} while (ischaridentifier (c) );
		iss.unget ();
		unsigned long n;
		char * aux;
		n= strtoul (str.c_str (), & aux, 16);
		if (* aux == '\0')
		{
			if (n > 0xFFFFUL)
				throw outofrange;
			return Token (static_cast <address> (n), rawstr);
		}
		else
		{
			return Token (TypeIdentifier, rawstr);
		}
	}
	#endif
	else
	{
		iss.unget ();
		return Token (TypeDollar);
	}
}

Token parsesharp (ScanString & iss)
{
	//TRFUNC (tr, "parsesharp");

	char c= iss.get ();
	if (c == '#')
		return Token (TypeSharpSharp);
	string str;
	string rawstr (1, '#');
	while (iss && (ischarhex (c) || c == '$') )
	{
		rawstr+= c;
		if (c != '$')
			str+= c;
		c= iss.get ();
	}
	if (str.empty () )
		return Token (TypeSharp);

	iss.unget ();

	unsigned long n;
	char * aux;
	n= strtoul (str.c_str (), & aux, 16);
	if (* aux != '\0')
		throw invalidnumber;
	if (n > 0xFFFFUL)
		throw outofrange;
	return Token (static_cast <address> (n), rawstr);
}

Token parseampersand (ScanString & iss)
{
	//TRFUNC (tr, "parseampersand");

	const ScanString::pos_type savepos= iss.getpos ();
	char c= iss.get ();

	if (! iss)
		return Token (TypeBitAnd);

	string rawstr (1, '&');
	string str;
	int base= 16;
	switch (c)
	{
	case '&':
		return Token (TypeBoolAnd);
	case 'h': case 'H':
		break;
	case 'o': case 'O':
		base= 8;
		break;
	case 'x': case 'X':
		base= 2;
		break;
	default:
		if (ischarhex (c) )
			str= c;
		else
		{
			iss.unget ();
			return Token (TypeBitAnd);
		}
	}
	rawstr+= c;
	for (c= iss.get (); ischaridentifier (c); c= iss.get () )
	{
		rawstr+= c;
		str+= c;
	}

	if (str.empty () )
	{
		iss.setpos (savepos);
		return Token (TypeBitAnd);
	}
	iss.unget ();

	string strnum (stripdollar (str) );
	char * aux;
	unsigned long n= strtoul (strnum.c_str (), & aux, base);
	if (* aux != '\0')
	{
		iss.setpos (savepos);
		return Token (TypeBitAnd);
	}
	//if (n > 0xFFFFUL)
	//	throw outofrange;
	return Token (static_cast <address> (n), rawstr);
}

Token parseor (ScanString & iss)
{
	//TRFUNC (tr, "parseor");

	char c= iss.get ();
	if (iss && c == '|')
		return Token (TypeBoolOr);
	else
	{
		if (iss)
			iss.unget ();
		return Token (TypeBitOr);
	}
}

Token parsepercent (ScanString & iss)
{
	//TRFUNC (tr, "parsepercent");

	char c= iss.get ();
	if (! iss || (c != '0' && c != '1') )
	{
		// Mod operator.
		iss.unget ();
		return Token (TypeMod);
	}

	// Binary number.

	string rawstr (1, '%');
	string str;
	do {
		rawstr+= c;
		if (c != '$')
			str+= c;
		c= iss.get ();
	} while (iss && (c == '0' || c == '1' || c == '$') );
	if (iss)
		iss.unget ();

	unsigned long n;
	char * aux;
	n= strtoul (str.c_str (), & aux, 2);
	if (* aux != '\0')
		throw invalidnumber;
	//if (n > 0xFFFFUL)
	//	throw outofrange;
	return Token (static_cast <address> (n), rawstr);
}

string getdigits (ScanString & iss, char c)
{
	string str;
	do {
		str+= c;
		c= iss.get ();
	} while (ischaralnum (c) || c == '$');
	iss.unget ();
	return str;
}

Token parsedigit (ScanString & iss, char c)
{
	//TRFUNC (tr, "parsedigit");

	const string rawstr= getdigits (iss, c);
	string str= stripdollar (rawstr);

	const string::size_type l= str.size ();
	unsigned long n;
	char * aux;

	// Hexadecimal with 0x prefix.
	if (str [0] == '0' && l > 1 &&
		(str [1] == 'x' || str [1] == 'X') )
	{
		str.erase (0, 2);
		if (str.empty () )
			throw invalidhex;
		n= strtoul (str.c_str (), & aux, 16);
	}
	else
	{
		// Decimal, hexadecimal, octal and binary
		// with or without suffix.
		switch (charupper (str [l - 1]) )
		{
		case 'H': // Hexadecimal.
			str.erase (l - 1);
			n= strtoul (str.c_str (), & aux, 16);
			break;
		case 'O': // Octal.
		case 'Q': // Octal.
			str.erase (l - 1);
			n= strtoul (str.c_str (), & aux, 8);
			break;
		case 'B': // Binary.
			str.erase (l - 1);
			n= strtoul (str.c_str (), & aux, 2);
			break;
		case 'D': // Decimal
			str.erase (l - 1);
			n= strtoul (str.c_str (), & aux, 10);
			break;
		default: // Decimal
			n= strtoul (str.c_str (), & aux, 10);
		}
	}
	if (* aux != '\0')
		throw invalidnumber;

	return Token (static_cast <address> (n), rawstr);
}

Token parsespecialmacroarg (ScanString & iss)
{
	//TRFUNC (tr, "parsespecialmacroarg");

	string str;
	char c;
	size_t level= 1;
	while (level > 0 && (c= iss.get () ) )
	{
		switch (c)
		{
		case '>':
			if (--level > 0)
				str+= c;
			break;
		case '<':
			++level;
			str+= c;
			break;
		default:
			str+= c;
		}
	}
	if (! iss)
		throw unclosed;
	return Token (TypeMacroArg, str);
}

Token parsecomment (ScanString & iss)
{
	string s;
	while (char c= iss.get () )
		s+= c;
	return Token (TypeComment, s);
}

Token parsetoken (ScanString & iss, AsmMode asmmode)
{
	//TRFUNC (tr, "parsetoken");

	char c= iss.get ();
	if (! iss)
		return Token (TypeEndLine);

	if (ischarspace (c) )
	{
		string str (1, c);
		while (ischarspace ( (c= iss.get () ) ) )
			str+= c;
		iss.unget ();
		return Token (TypeWhiteSpace, str);
	}

	switch (c)
	{
	case ';':
		//return Token (TypeEndLine);
		return parsecomment (iss);
	case ',':
		return Token (TypeComma);
	case ':':
		return Token (TypeColon);
	case '+':
		return Token (TypePlus);
	case '-':
		return Token (TypeMinus);
	case '*':
		return Token (TypeMult);
	case '/':
		return Token (TypeDiv);
	case '=':
		return Token (TypeEqOp);
	case '<':
		// Less or less equal.
		return parseless (iss);
	case '>':
		// Greater or greter equal.
		return parsegreat (iss);
	case '~':
		return Token (TypeBitNotOp);
	case '!':
		// Not or not equal.
		return parsenot (iss);
	case '(':
		return Token (TypeOpen);
	case ')':
		return Token (TypeClose);
	case '[':
		return Token (TypeOpenBracket);
	case ']':
		return Token (TypeCloseBracket);
	case '$':
		// Hexadecimal number.
		return parsedollar (iss);
	case '\'':
		// Classic assembler string literal.
		return parsestringasm (iss);
	case '"':
		// C style string literal.
		return parsestringc (iss);
	case '#':
		// Hexadecimal number.
		return parsesharp (iss);
	case '&':
		// Hexadecimal, octal or binary number,
		// or and operators.
		return parseampersand (iss);
	case '|':
		// Or operators.
		return parseor (iss);
	case '%':
		// Binary number or mod operator.
		return parsepercent (iss);
	default:
		; // Nothing
	}

	if (ischardigit (c) )
		return parsedigit (iss, c);

	if (ischarbeginidentifier (c) )
		return parseidentifier (iss, c, asmmode);

	// Any other case, invalid character.

	throw UnexpectedChar (c);
}

Token parsemacroarg (ScanString & iss, AsmMode asmmode)
{
	//TRFUNC (tr, "parsemacroarg");

	char c= iss.get ();
	if (! iss)
		return Token (TypeEndLine);

	if (ischarspace (c) )
	{
		do {
			c= iss.get ();
		} while (iss && ischarspace (c) );
		if (iss)
			iss.unget ();
		return Token (TypeWhiteSpace);
	}
	if (c == '<')
		return parsespecialmacroarg (iss);
	else
	{
		iss.unget ();
		return parsetoken (iss, asmmode);
	}
}

string parseincludefile (ScanString & iss)
{
	char c;
	do {
		c= iss.get ();
	} while (ischarspace (c) );

	if (! iss)
		throw needfilename;
	string r;
	switch (c)
	{
	case '"':
		do
		{
			c= iss.get ();
			if (! iss)
				throw invalidfilename;
			if (c != '"')
				r+= c;
		} while (c != '"');
		break;
	case '\'':
		do
		{
			c= iss.get ();
			if (! iss)
				throw invalidfilename;
			if (c != '\'')
				r+= c;
		} while (c != '\'');
		break;
	default:
		do
		{
			r+= c;
			c= iss.get ();
		} while (iss && ! ischarspace (c) );
		if (iss)
			iss.unget ();
	}
	return r;
}

string parsemessage (ScanString & iss)
{
	string result;
	char c;
	do {
		c= iss.get ();
	} while (ischarspace (c) );
	
	if (iss)
	{
		do {
			result+= c;
			c= iss.get ();
		} while (iss);
	}
	return result;
}

} // namespace tokenizer

using namespace tokenizer;

//*********************************************************
//		class Tokenizer::Internal
//*********************************************************

class Tokenizer::Internal {
public:
	virtual ~Internal ();
	virtual Internal * clone () const= 0;

	virtual bool empty () const= 0;

	virtual void push_back (const Token & tok);

	virtual Token getrawtoken ()= 0;
	virtual Token gettoken ()= 0;
	virtual Token getmacroarg ()= 0;
	virtual Token getincludefile ()= 0;
	virtual void ungettoken ()= 0;

	virtual void tostream (ostream & oss) const= 0;
};

Tokenizer::Internal::~Internal ()
{ }

void Tokenizer::Internal::push_back (const Token &)
{
	throw logic_error ("Unexpected call");
}

class Internal2 : public Tokenizer::Internal {
public:
	Internal2 ();
	Internal2 (const Internal2 & in);
	Tokenizer::Internal * clone () const;

	bool empty () const;

	void push_back (const Token & tok);

	Token getrawtoken ();
	virtual Token gettoken ();
	virtual Token getmacroarg ();
	virtual Token getincludefile ();

	virtual void ungettoken ();

	void tostream (ostream & oss) const;
private:
	size_t current;
	typedef deque <Token> tokenlist_t;
	mutable tokenlist_t tokenlist;
};

Internal2::Internal2 () :
	current (0)
{
}

Internal2::Internal2 (const Internal2 & in) :
	Tokenizer::Internal (),
	current (0),
	tokenlist (in.tokenlist)
{
}

Tokenizer::Internal * Internal2::clone () const
{
	return new Internal2 (* this);
}

bool Internal2::empty () const
{
	return tokenlist.empty ();
}

void Internal2::push_back (const Token & tok)
{
	tokenlist.push_back (tok);
}

Token Internal2::getrawtoken ()
{
	if (current >= tokenlist.size () )
	{
		++current;
		return Token (TypeEndLine);
	}
	else
		return tokenlist [current++];
}

Token Internal2::gettoken ()
{
	Token tok;
	do
	{
		tok= getrawtoken ();
	} while (tok.type () == TypeWhiteSpace);
	if (tok.type () == TypeComment)
		return Token (TypeEndLine);
	return tok;
}

Token Internal2::getmacroarg ()
{
	return gettoken ();
}

Token Internal2::getincludefile ()
{
	return gettoken ();
}

void Internal2::ungettoken ()
{
	--current;
}

void Internal2::tostream (ostream & oss) const
{
	copy (tokenlist.begin (), tokenlist.end (),
		ostream_iterator <Token> (oss, "") );
}


class Internal1 : public Tokenizer::Internal {
public:
	Internal1 (const string & line, AsmMode asmmode_n);
	Internal1 (const Internal1 & tz);
	~Internal1 ();
	Tokenizer::Internal * clone () const;

	void startscan ();

	bool checkend () const;
	bool empty () const;
	bool endswithparen () const;

	void reset ();

	Token getrawtoken ();
	Token gettoken ();
	Token getmacroarg ();
	Token getincludefile ();

	void ungettoken ();

	void tostream (ostream & oss) const;
private:
	AsmMode asmmode;
	string textline;
	mutable ScanString * ptoscan;
	typedef deque <Token> tokenlist_t;
	mutable tokenlist_t tokenlist;
	mutable tokenlist_t::size_type current;

	Internal1 & operator = (const Internal1 &); // Forbidden.

	void addtoken (const Token & tok) const;
	bool checkendscan () const;
	void parsenexttoken () const;
	bool checkmacroarg () const;
	void parsenextmacroarg () const;

	class AvoidWarningPrivateDestructor { };
	friend class AvoidWarningPrivateDestructor;
};


Internal1::Internal1 (const string & line, AsmMode asmmode_n) :
	Tokenizer::Internal (),
	asmmode (asmmode_n),
	textline (line),
	ptoscan (0),
	current (0)
{
	startscan ();
}

Internal1::Internal1 (const Internal1 & tz) :
	Tokenizer::Internal (),
	asmmode (tz.asmmode),
	textline (tz.textline),
	ptoscan (0),
	current (0)
{
	startscan ();
}

Internal1::~Internal1 ()
{
	delete ptoscan;
}

Tokenizer::Internal * Internal1::clone () const
{
	return new Internal1 (* this);
}

void Internal1::startscan ()
{
	delete ptoscan;
	ptoscan= 0;
	ptoscan= new ScanString (textline);
	current= 0;
}

void Internal1::addtoken (const Token & tok) const
{
	bool empty= tokenlist.empty ();
	tokenlist.push_back (tok);
	if (empty)
		current= 0;
}

bool Internal1::checkendscan () const
{
	return ! ptoscan;
}

void Internal1::parsenexttoken () const
{
	//TRFUNC (tr, "Internal1::parsenexttoken");

	if (checkendscan () )
		return;

	Token tok= parsetoken (* ptoscan, asmmode);
	TypeToken tt= tok.type ();
	if (tt == TypeEndLine)
	{
		//TRMESSAGE (tr, "end line");
		ptoscan= 0;
		return;
	}

	addtoken (tok);
	switch (tt)
	{
	case TypeINCLUDE:
	case TypeINCBIN:
		addtoken (Token (TypeLiteral,
			parseincludefile (* ptoscan) ) );
		break;
	case Type_ERROR:
	case Type_WARNING:
		addtoken (Token (TypeLiteral, parsemessage (* ptoscan) ) );
		break;
	default:
		// Nothing special.
		break;
	}
}

void Internal1::parsenextmacroarg () const
{
	if (checkendscan () )
		return;
	Token tok= parsemacroarg (* ptoscan, asmmode);
	TypeToken tt= tok.type ();
	if (tt == TypeEndLine)
	{
		ptoscan= 0;
		return;
	}
	addtoken (tok);
}

void Internal1::tostream (ostream & oss) const
{
	Internal1 tzaux (* this);
	Token tok=tzaux.getrawtoken ();
	while (tok.type () != TypeEndLine)
	{
		oss << tok.raw ();
		tok= tzaux.getrawtoken ();
	}
}

bool Internal1::checkend () const
{
	//TRFUNC (tr, "Internal1::checkend");

	if (current < tokenlist.size () )
		return false;
	else
	{
		parsenexttoken ();
		return current >= tokenlist.size ();
	}
}

bool Internal1::checkmacroarg () const
{
	if (current < tokenlist.size () )
		return false;
	else
	{
		parsenextmacroarg ();
		return current >= tokenlist.size ();
	}
}

bool Internal1::empty () const
{
	//TRFUNC (tr, "Internal1::empty");

	if (current > 0)
		return false;
	else
	{
		checkend ();
		return tokenlist.empty ();
	}
}

bool Internal1::endswithparen () const
{
	ASSERT (! tokenlist.empty () );

	return tokenlist.back ().type () == TypeClose;
}


void Internal1::reset ()
{
	current= 0;
}

Token Internal1::getrawtoken ()
{
	//TRFUNC (tr, "Internal1::getrawtoken");

	Token tok;
	if (checkend () )
	{
		++current;
		//TRMESSAGE (tr, "(end line)");
		return Token (TypeEndLine);
	}
	ASSERT (current < tokenlist.size () );
	tok= tokenlist [current];
	++current;
	//TRMESSAGE (tr, tok.str () );
	return tok;
}

Token Internal1::gettoken ()
{
	//TRFUNC (tr, "Internal1::gettoken");

	Token tok;
	do
	{
		tok= getrawtoken ();
	} while (tok.type () == TypeWhiteSpace);
	//TRMESSAGE (tr, tok.str () );
	if (tok.type () == TypeComment)
		return Token (TypeEndLine);
	return tok;
}

Token Internal1::getmacroarg ()
{
	//TRFUNC (tr, "Internal1::getmacroarg");

	Token tok;
	do {
		if (checkmacroarg () )
		{
			++current;
			//TRMESSAGE (tr, "(endline)");
			return Token (TypeEndLine);
		}
		tok= tokenlist [current];
		++current;
	} while (tok.type () == TypeWhiteSpace);
	//TRMESSAGE (tr, tok.str () );
	if (tok.type () == TypeComment)
		return Token (TypeEndLine);
	return tok;
}

void Internal1::ungettoken ()
{
	//TRFUNC (tr, "Internal1::ungettoken");

	if (current > tokenlist.size () )
	{
		--current;
	}
	else
	{
		do
		{
			if (current == 0)
				throw tokenizerunderflow;
			--current;
		} while (tokenlist [current].type () == TypeWhiteSpace);
	}
}

Token Internal1::getincludefile ()
{
	Token tok= gettoken ();
	if (tok.type () != TypeLiteral)
		throw missingfilename;
	return tok;
}


//*********************************************************
//			class Tokenizer
//*********************************************************


Tokenizer::Tokenizer () :
	pin (new Internal2)
{
}

Tokenizer::Tokenizer (TypeToken ttok) :
	pin (new Internal2)
{
	pin->push_back (Token (ttok) );
}

Tokenizer::Tokenizer (const Tokenizer & tz) :
	pin (tz.pin-> clone () )
{
}

Tokenizer::Tokenizer (const string & line, AsmMode asmmode) :
	pin (new Internal1 (line, asmmode) )
{
}

Tokenizer::~Tokenizer ()
{
	delete pin;
}

Tokenizer::Internal & Tokenizer::in ()
{
	return * pin;
}

const Tokenizer::Internal & Tokenizer::in () const
{
	return * pin;
}

Tokenizer & Tokenizer::operator = (const Tokenizer & tz)
{
	Tokenizer::Internal * paux= pin;
	pin= tz.pin->clone ();
	delete paux;
	return * this;
}

void Tokenizer::push_back (const Token & tok)
{
	in ().push_back (tok);
}

bool Tokenizer::empty () const
{
	return in ().empty ();
}

void Tokenizer::reset ()
{
}

Token Tokenizer::gettoken ()
{
	//TRFUNC (tr, "Tokenizer::gettoken");

	return in ().gettoken ();
}

Token Tokenizer::getrawtoken ()
{
	return in ().getrawtoken ();
}

Token Tokenizer::getmacroarg ()
{
	return in ().getmacroarg ();
}

void Tokenizer::ungettoken ()
{
	in ().ungettoken ();
}

Token Tokenizer::getincludefile ()
{
	return in ().getincludefile ();
}

ostream & operator << (ostream & oss, const Tokenizer & tz)
{
	tz.in ().tostream (oss);
	return oss;
}


//*********************************************************
//			Auxiliar functions.
//*********************************************************


regwCode getregw (TypeToken tt)
{
	switch (tt)
	{
	case TypeBC: return regBC;
	case TypeDE: return regDE;
	case TypeHL: return regHL;
	case TypeAF: return regAF;
	case TypeSP: return regSP;
	default:     return regwInvalid;
	}
}

regbCode getregb (TypeToken tt)
{
	switch (tt)
	{
	case TypeA: return regA;
	case TypeB: return regB;
	case TypeC: return regC;
	case TypeD: return regD;
	case TypeE: return regE;
	case TypeH: return regH;
	case TypeL: return regL;
	default:    return regbInvalid;
	}
}

flagCode getflag86 (flagCode fcode)
{
	switch (fcode)
	{
	case flagNZ: return flag86NZ;
	case flagZ:  return flag86Z;
	case flagNC: return flag86NC;
	case flagC:  return flag86C;
	case flagPO: return flag86NP;
	case flagPE: return flag86P;
	case flagP:  return flag86NS;
	case flagM:  return flag86S;
	default:
		throw InvalidFlagConvert;
	}
}

flagCode invertflag86 (flagCode fcode)
{
	return static_cast <flagCode>
		( (fcode & 1) ? fcode & ~ 1 : fcode | 1);
}

flagCode getflag (TypeToken tt)
{
	switch (tt)
	{
	case TypeNZ: return flagNZ;
	case TypeZ:  return flagZ;
	case TypeNC: return flagNC;
	case TypeC:  return flagC;
	case TypePO: return flagPO;
	case TypePE: return flagPE;
	case TypeP:  return flagP;
	case TypeM:  return flagM;
	default:     return flagInvalid;
	}
}

string getflagname (flagCode f)
{
	switch (f)
	{
	case flagNZ: return "NZ";
	case flagZ:  return "Z";
	case flagNC: return "NC";
	case flagC:  return "C";
	case flagPO: return "PO";
	case flagPE: return "PE";
	case flagP:  return "P";
	case flagM:  return "M";
	default:
		throw logic_error ("Invalid flag code");
	}
}


} // namespace impl
} // namespace pasmo


// End of token.cpp
