#ifndef INCLUDE_TOKEN_H
#define INCLUDE_TOKEN_H

// token.h
// Revision 1-sep-2005


#include "pasmotypes.h"
#include "pasmoimpl.h"


#include <deque>


namespace pasmo {
namespace impl {

typedef int TypeToken;

// Single char operators.
const TypeToken
	TypeComma= ',',
	TypeColon= ':',
	TypePlus= '+',
	TypeMinus= '-',
	TypeMult= '*',
	TypeDiv= '/',
	TypeEqOp= '=',
	TypeOpen= '(',
	TypeClose= ')',
	TypeOpenBracket= '[',
	TypeCloseBracket= ']',
	TypeDollar= '$',
	TypeMod= '%',
	TypeLtOp= '<',
	TypeGtOp= '>',
	TypeBitNotOp= '~',
	TypeBoolNotOp= '!',
	TypeBitAnd= '&',
	TypeBitOr= '|',
	TypeQuestion= '?',
	TypeSharp= '#';


regwCode getregw (TypeToken tt);

regbCode getregb (TypeToken tt);

flagCode getflag86 (flagCode fcode);
flagCode invertflag86 (flagCode fcode);
flagCode getflag (TypeToken tt);
string getflagname (flagCode f);


string gettokenname (TypeToken tt);


class Token {
public:
	Token ();
	Token (TypeToken ttn);
	Token (address n);
	Token (address n, const string & sn);
	Token (char c);
	Token (TypeToken ttn, const string & sn, address style= 0);

	TypeToken type () const;
	bool isliteral () const;
	string raw () const;
	string str () const;
	address num () const;
	string numstr () const;
	string literal () const;
	string identifier () const;
	string macroname () const;
	string macroarg () const;
private:
	TypeToken tt;
	string s;
	address number;
};

ostream & operator << (ostream & oss, const Token & tok);


class Tokenizer {
public:
	Tokenizer ();
	Tokenizer (TypeToken ttok);
	Tokenizer (const Tokenizer & tz);
	Tokenizer (const string & line, AsmMode asmmode);
	~Tokenizer ();

	Tokenizer & operator = (const Tokenizer &);

	void push_back (const Token & tok);

	bool empty () const;

	void reset ();

	Token getrawtoken ();
	Token gettoken ();
	Token getmacroarg ();
	Token getincludefile ();

	void ungettoken ();


	friend ostream & operator << (ostream & oss,
		const Tokenizer & tz);

	class Internal;
private:
	Internal * pin;
	Internal & in ();
	const Internal & in () const;
};


} // namespace impl
} // namespace pasmo


#endif

// End of token.h
