// pasmotypes.cpp
// Revision 24-sep-2005


#include "pasmotypes.h"


#include <ctype.h>


namespace pasmo {

namespace {


const char hexdigit []= { '0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

} // namespace

bool ischarspace (char c)
{
	//return c == ' ' || c == '\t';
	return isspace (static_cast <unsigned char> (c) );
}

bool ischardigit (char c)
{
	return isdigit (static_cast <unsigned char> (c) );
}

bool ischarhex (char c)
{
	return isxdigit (static_cast <unsigned char> (c) );
}

bool ischaralnum (char c)
{
	return isalnum (static_cast <unsigned char> (c) );
}

char charupper (char c)
{
	return toupper (static_cast <unsigned char> (c) );
}

string upper (const string & str)
{
	string r;
	const string::size_type l= str.size ();
	for (string::size_type i= 0; i < l; ++i)
		r+= toupper (str [i] );
	return r;
}

string stripdollar (const string & str)
{
	string::size_type n= str.find ('$');
	if (n == string::npos)
		return str;
	else
	{
		string aux (str);
		do
		{
			aux.erase (n, 1);
			n= aux.find ('$', n);
		} while (n != string::npos);
		return aux;
	}
}

string hex2str (byte b)
{
	return string (1, hexdigit [ (b >> 4) & 0x0F] ) +
		hexdigit [b & 0x0F];
}

string hex4str (address n)
{
	return hex2str (hibyte (n) ) + hex2str (lobyte (n) );
}

string hex8str (size_t nn)
{
	return hex4str ( (nn >> 16) & 0xFFFF) + hex4str (nn & 0xFFFF);
}

string Hex2::str () const
{
	return hex2str (b);
}

string Hex4::str () const
{
	return hex4str (n);
}

string Hex8::str () const
{
	return hex8str (nn);
}

ostream & operator << (ostream & os, const Hex2 & h2)
{
	os << h2.str ();
	return os;
}

ostream & operator << (ostream & os, const Hex4 & h4)
{
	os << h4.str ();
	return os;
}

ostream & operator << (ostream & os, const Hex8 & h8)
{
	os << h8.str ();
	return os;
}

bool ischarbeginidentifier (char c)
{
	return isalpha (static_cast <unsigned char> (c) ) ||
		c == '_' || c == '?' || c == '@' || c == '.';
}

bool ischaridentifier (char c)
{
	return isalnum (static_cast <unsigned char> (c) ) ||
		c == '_' || c == '$' || c == '?' || c == '@' || c == '.';
}


} // namespace pasmo

// End of pasmotypes.cpp
