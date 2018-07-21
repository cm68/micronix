#ifndef INCLUDE_PASMOTYPES_H
#define INCLUDE_PASMOTYPES_H

// pasmotypes.h
// Revision 17-dec-2006


#include "config_ostream.h"
#include "config_types.h"


// If ostream is available, use it. Otherwise the assumption
// is that a pre-standard compiler is used and iostream
// contains all ostream declarations required.
#if HAVE_OSTREAM
#include <ostream>
#else
#include <iostream>
#endif

#include <string>

#include <limits.h>
#include <stdlib.h>
#include <stdexcept>

#if HAVE_STDINT_H
#include <stdint.h>
#endif


namespace pasmo {


using std::ostream;
using std::string;
using std::runtime_error;


class nonfatal_error : public runtime_error {
public:
	nonfatal_error (const string & message) :
		runtime_error (message)
	{ }
};

class error_already_reported { };

bool ischarspace (char c);
bool ischardigit (char c);
bool ischarhex (char c);
bool ischaralnum (char c);
char charupper (char c);
string upper (const string & str);
string stripdollar (const string & str);


#if HAVE_STDINT_H

typedef uint8_t byte;
typedef int8_t sbyte;
typedef uint16_t address;

#else

#if UCHAR_MAX == 255

typedef unsigned char byte;
typedef signed char sbyte;

#else

#error Unable to find a 8 bit unsigned type.

#endif

#if USHRT_MAX == 65535

typedef unsigned short address;

#else

#error Unable to find a 16 bit unsigned type.

#endif

#endif


const address addrTRUE= 0xFFFFU;
const address addrFALSE= 0;


enum AsmMode { AsmZ80, Asm8080 };

inline byte lobyte (address n)
{
	return static_cast <byte> (n & 0xFF);
}

inline byte hibyte (address n)
{
	return static_cast <byte> (n >> 8);
}

inline void putword (ostream & os, address word)
{
	os.put (lobyte (word) );
	os.put (hibyte (word) );
}

string hex2str (byte b);
string hex4str (address n);
string hex8str (size_t nn);

inline string hex2str (sbyte b)
{ return hex2str (static_cast <byte> (b) ); }
inline string hex2str (char b)
{ return hex2str (static_cast <byte> (b) ); }


class Hex2 {
public:
	explicit Hex2 (byte b) : b (b)
	{ }
	byte getb () const
	{ return b; }
	string str () const;
private:
	byte b;
};

class Hex4 {
public:
	explicit Hex4 (address n) : n (n)
	{ }
	address getn () const
	{ return n; }
	string str () const;
private:
	address n;
};

class Hex8 {
public:
	explicit Hex8 (size_t nn) : nn (nn)
	{ }
	size_t getnn () const
	{ return nn; }
	string str () const;
private:
	size_t nn;
};

inline Hex2 hex2 (byte b) { return Hex2 (b); }
inline Hex2 hex2 (sbyte b) { return Hex2 (static_cast <byte> (b) ); }
inline Hex2 hex2 (char b) { return Hex2 (static_cast <byte> (b) ); }
inline Hex4 hex4 (address n) { return Hex4 (n); }
inline Hex8 hex8 (size_t nn) { return Hex8 (nn); }

ostream & operator << (ostream & os, const Hex2 & h2);
ostream & operator << (ostream & os, const Hex4 & h4);
ostream & operator << (ostream & os, const Hex8 & h8);


bool ischarbeginidentifier (char c);

bool ischaridentifier (char c);


const byte codeADDHL= 0x09;
const byte codeADCHL= 0x4A;
const byte codeSBCHL= 0x42;
const byte codeLDIA= 0x47;
const byte codeLDRA= 0x4F;

const byte codeRST00= 0xC7;

const byte codeDJNZ= 0x10;

const byte codeIM_0= 0x46;
const byte codeIM_1= 0x56;
const byte codeIM_2= 0x5E;

const byte
	codeJP=           0xC3,
	codeJP_86=        0xE9,
	codeJP_indHL=     0xE9,
	codeLD_HL_nn=     0x21,
	codeLD_HL_indexp= 0x2A,
	codeLD_indexp_HL= 0x22,
	codeLD_SP_HL=     0xF9,
	codeRET=          0xC9,
	codeRET_86=       0xC3,
	codePUSH_AX=      0x50,
	codePOP_AX=       0x58;

enum flagCode {
	flagNZ= 0, flagZ=  1,
	flagNC= 2, flagC=  3,
	flagPO= 4, flagPE= 5,
	flagP=  6, flagM=  7,

	flag86NZ= 0x05, flag86Z= 0x04,
	flag86NC= 0x03, flag86C= 0x02,
	flag86NP= 0x0B, flag86P= 0x0A,
	flag86NS= 0x09, flag86S= 0x08,

	flagInvalid= 8
};

} // namespace pasmo

#endif

// End of pasmotypes.h
