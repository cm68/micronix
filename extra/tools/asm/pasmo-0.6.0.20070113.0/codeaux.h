#ifndef INCLUDE_CODEAUX_H
#define INCLUDE_CODEAUX_H

// codeaux.h
// Revision 20-nov-2005

#include "pasmoimpl.h"
#include "asmimpl.h"


namespace pasmo {
namespace impl {


byte getregb86 (regbCode rb);
string nameHLpref (byte prefix);
string regwName (regwCode code, bool useSP, byte prefix= prefixNone);
string regw8080Name (regwCode code);
string regw8080NamePSW (regwCode code);
string byteinstName (TypeByteInst ti);
string byteinst8080Name (TypeByteInst ti);
string byteinst8080iName (TypeByteInst ti);
byte getbaseByteInst (TypeByteInst ti, GenCodeMode genmode);
byte getByteInstInmediate (TypeByteInst ti, GenCodeMode genmode);
string nameIdesp (byte prefix, bool hasdesp, byte desp);
string getregbname (regbCode rb, byte prefix= prefixNone,
	bool hasdesp= false, byte desp= 0);
string getregb8080name (regbCode rb);


} // namespace impl
} // namespace pasmo


#endif

// End of codeaux.h
