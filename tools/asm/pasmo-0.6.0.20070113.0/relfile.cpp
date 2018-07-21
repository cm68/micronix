// relfile.cpp
// Revision 13-jun-2006

#include <iomanip>
#include <fstream>
#include <stdexcept>

#include "relfile.h"


namespace pasmo {


using std::logic_error;


//*********************************************************
//		class RelFileIn
//*********************************************************

RelFileIn::RelFileIn (std::istream & is_n) :
	is (is_n),
	bitread (8),
	eofpassed (false)
{
}


bool RelFileIn::eof () const
{
	return eofpassed;
}

void RelFileIn::skiptobyte ()
{
	bitread= 8;
}

address RelFileIn::getbit ()
{
	if (bitread == 8)
	{
		current= static_cast <byte> (is.get () );
		if (is.eof () )
		{
			eofpassed= true;
			return TypeEof;
		}
		bitread= 0;
	}
	bool b= (current & 0x80) != 0;
	current<<= 1;
	++bitread;
	return b;
}

address RelFileIn::getbit2 ()
{
	address b= getbit () << 1;
	b|= getbit ();
	return b;
}

address RelFileIn::getbit3 ()
{
	address b= getbit ();
	for (size_t i= 0; i < 2; ++i)
		b= (b << 1) | getbit ();
	return b;
}

address RelFileIn::getbit4 ()
{
	address b= getbit ();
	for (size_t i= 0; i < 3; ++i)
		b= (b << 1) | getbit ();
	return b;
}

byte RelFileIn::readchar ()
{
	byte b= getbit ();
	for (size_t i= 0; i < 7; ++i)
		b= (b << 1) | getbit ();
	return b;
}

string RelFileIn::readname ()
{
	address n= getbit3 ();
	string r;
	for (size_t i= 0; i < n; ++i)
		r+= static_cast <char> (readchar () );
	return r;
}

address RelFileIn::readtype ()
{
	if (eofpassed)
		return TypeEof;
	address b= getbit ();
	if (b == 0)
		return TypeByte;
	b= getbit2 ();
	if (b == 0)
	{
		b= getbit4 () | 0x0100;
	}
	else
		b|= 0x0010;
	return b;
}

address RelFileIn::readaddresstype ()
{
	return getbit2 () | 0x10;
}

address RelFileIn::readaddress ()
{
	address n= static_cast <address> (readchar () );
	n|= static_cast <address> (readchar () ) << 8;
	return n;
}

Value RelFileIn::readvalue ()
{
	address t= getbit2 ();
	ValueType type;
	switch (t)
	{
	case 0: type= ValueAbsolute;       break;
	case 1: type= ValueProgRelative;   break;
	case 2: type= ValueDataRelative;   break;
	case 3: type= ValueCommonRelative; break;
	default:
		throw logic_error
			("Something bad in value types definitions");
	}
	address v= readaddress ();
	return Value (type, v);
}


//*********************************************************
//		class RelFileOut
//*********************************************************

RelFileOut::RelFileOut (std::ostream & os_n) :
	os (os_n),
	bitwritten (0),
	current (0),
	size (0)
{
}

RelFileOut::~RelFileOut ()
{
	bytealign ();
	while (size % 128)
	{
		os.put (0x1A);
		++size;
	}
}

void RelFileOut::putbit (bool b)
{
	current= (current << 1) | b;
	if (++bitwritten == 8)
	{
		os.put (current);
		++size;
		bitwritten= 0;
	}
}

void RelFileOut::bytealign ()
{
	while (bitwritten != 0)
		putbit (0);
}

void RelFileOut::putbyte (byte b)
{
	putbit (b & 0x80);
	putbit (b & 0x40);
	putbit (b & 0x20);
	putbit (b & 0x10);
	putbit (b & 0x08);
	putbit (b & 0x04);
	putbit (b & 0x02);
	putbit (b & 0x01);
}

void RelFileOut::putword (address s)
{
	putbyte (lobyte (s) );
	putbyte (hibyte (s) );
}

void RelFileOut::puttype (address t)
{
	putbit (1);
	switch (t & 0xFFF0)
	{
	case 0x0010:
		if (t & ~ 0x0013)
			throw logic_error ("Invalid REL data type");
		putbit (t & 0x0002);
		putbit (t & 0x0001);
		break;
	case 0x0100:
		putbit (0);
		putbit (0);
		if (t & ~ 0x010F)
			throw logic_error ("Invalid REL data type");
		putbit (t & 0x0008);
		putbit (t & 0x0004);
		putbit (t & 0x0002);
		putbit (t & 0x0001);
		break;
	default:
		throw logic_error ("Invalid REL data type");
	}
}

void RelFileOut::putvalue (byte type, address value)
{
	if (type & ~ 0x13)
		throw logic_error ("Invalid REL data type");
	putbit (type & 0x0002);
	putbit (type & 0x0001);
	putword (value);
}

string RelFileOut::putname (const string & name)
{
	unsigned int l= name.size ();
	if (l > 6)
		l= 6;
	putbit (l & 0x4);
	putbit (l & 0x2);
	putbit (l & 0x1);

	const string exname (name.substr (0, l) );
	for (size_t i= 0; i < l; ++i)
		putbyte (static_cast <byte> (exname [i]) );
	return exname;
}

void RelFileOut::putbyteitem (byte b)
{
	putbit (0);
	putbyte (b);
}

void RelFileOut::putworditem (const Value & v)
{
	address value= v.value;
	switch (v.type)
	{
	case ValueAbsolute:
		putbyteitem (lobyte (value) );
		putbyteitem (hibyte (value) );
		break;
	case ValueProgRelative:
		puttype (TypeProgRelative);
		putword (value);
		break;
	case ValueDataRelative:
		puttype (TypeDataRelative);
		putword (value);
		break;
	default:
		throw logic_error ("Invalid REL data type");
	}
}

#if 0
void RelFileOut::putabsolute (address s)
{
	//puttype (TypeAbsolute);
	putbyteitem (lobyte (s) );
	putbyteitem (hibyte (s) );
}

void RelFileOut::putprogramrelative (address s)
{
	puttype (TypeProgRelative);
	putword (s);
}

void RelFileOut::putdatarelative (address s)
{
	puttype (TypeDataRelative);
	putword (s);
}
#endif

string RelFileOut::putentrysymbol (const string & name)
{
	puttype (TypeEntrySymbol);
	return putname (name);
}

void RelFileOut::putprogramname (const string & name)
{
	puttype (TypeProgramName);
	putname (name);
}

void RelFileOut::putchainexternal (address type, address value,
	const string & name)
{
	puttype (TypeChainExternal);
	putvalue (type, value);
	putname (name);
}

void RelFileOut::putdefineentrypoint (const string & name, const Value & v)
{
	puttype (TypeDefineEntryPoint);
	putvalue (v.type, v.value);
	putname (name);
}

void RelFileOut::putexternalplusoffset (address type, address value)
{
	puttype (TypeExternalPlusOffset);
	putvalue (type, value);
}

void RelFileOut::putdatasize (address s)
{
	puttype (TypeDataSize);
	putvalue (TypeAbsolute, s);
}

void RelFileOut::putlocationcounter (address type, address value)
{
	puttype (TypeLocationCounter);
	putvalue (type, value);
}

void RelFileOut::putprogramsize (address s)
{
	puttype (TypeProgramSize);
	putvalue (TypeProgRelative, s);
}

void RelFileOut::putendmodule ()
{
	puttype (TypeEndModule);
	putvalue (TypeAbsolute, 0);
	bytealign ();
}

void RelFileOut::putendmodule (const Value & v)
{
	puttype (TypeEndModule);
	putvalue (v.type, v.value);
	bytealign ();
}

void RelFileOut::putendfile ()
{
	puttype (TypeEndFile);
}


} // namespace pasmo


// End of relfile.cpp
