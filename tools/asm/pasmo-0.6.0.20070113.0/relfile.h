#ifndef INCLUDE_RELFILE_H
#define INCLUDE_RELFILE_H

// relfile.h
// Revision 13-jun-2006

#include "pasmotypes.h"

#include "var.h"


namespace pasmo {

class RelFile {
public:
	static const address TypeEof=                0xFFFF;
	static const address TypeByte=                    0;

	static const address TypeAbsolute=           0x0010;
	static const address TypeProgRelative=       0x0011;
	static const address TypeDataRelative=       0x0012;
	static const address TypeCommonRelative=     0x0013;

	static const address TypeEntrySymbol=        0x0100;
	static const address TypeSelectCommon=       0x0101;
	static const address TypeProgramName=        0x0102;
	static const address TypeUnused1=            0x0103;
	static const address TypeExtensionItem=      0x0104;
	static const address TypeCommonSize=         0x0105;
	static const address TypeChainExternal=      0x0106;
	static const address TypeDefineEntryPoint=   0x0107;
	static const address TypeUnused3=            0x0108;
	static const address TypeExternalPlusOffset= 0x0109;
	static const address TypeDataSize=           0x010A;
	static const address TypeLocationCounter=    0x010B;
	static const address TypeChainAddress=       0x010C;
	static const address TypeProgramSize=        0x010D;
	static const address TypeEndModule=          0x010E;
	static const address TypeEndFile=            0x010F;
};

class RelFileIn : public RelFile {
public:
	RelFileIn (std::istream & is_n);
	bool eof () const;
	void skiptobyte ();
	byte readchar ();
	string readname ();
	address readtype ();
	address readaddresstype ();
	address readaddress ();
	Value readvalue ();

private:
	std::istream & is;
	unsigned int bitread;
	byte current;
	bool eofpassed;

	address getbit ();
	address getbit2 ();
	address getbit3 ();
	address getbit4 ();
};

class RelFileOut : public RelFile {
public:
	RelFileOut (std::ostream & os_n);
	~RelFileOut ();

	void putbyteitem (byte b);
	void putworditem (const Value & v);
	//void putabsolute (address s);
	//void putprogramrelative (address s);
	//void putdatarelative (address s);

	string putentrysymbol (const string & name);
	void putprogramname (const string & name);
	void putchainexternal (address type, address value,
		const string & name);
	//void putdefineentrypoint_prog (const string & name, address s);
	void putdefineentrypoint (const string & name, const Value & v);
	void putexternalplusoffset (address type, address value);
	void putdatasize (address s);
	void putlocationcounter (address type, address value);
	void putprogramsize (address s);

	void putendmodule ();
	//void putendmodule (address s);
	void putendmodule (const Value & v);
	void putendfile ();
private:
	std::ostream & os;
	unsigned int bitwritten;
	byte current;
	size_t size;

	void putbit (bool b);
	void bytealign ();
	void putbyte (byte b);
	void putword (address s);
	void puttype (address t);
	void putvalue (byte type, address value);
	string putname (const string & name);
};

} // namespace pasmo

#endif

// End of relfile.h
