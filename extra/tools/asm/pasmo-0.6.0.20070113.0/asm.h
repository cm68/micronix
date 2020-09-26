#ifndef INCLUDE_ASM_H
#define INCLUDE_ASM_H

// asm.h
// Revision 13-jun-2006

#include "pasmotypes.h"

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <deque>


namespace pasmo {


enum DebugType { NoDebug, DebugSecondPass, DebugAll };

enum ObjectType {
	ObjectBin,
	ObjectDump,
	ObjectHex,
	ObjectPrl,
	ObjectRel,
	ObjectCmd,
	ObjectCom,
	ObjectTap,
	ObjectTapBas,
	ObjectTzx,
	ObjectTzxBas,
	ObjectCdt,
	ObjectCdtBas,
	ObjectPlus3Dos,
	ObjectAmsDos,
	ObjectMsx
};

class AsmOptions {
public:
	AsmOptions ();
	ObjectType getObjectType () const;
	void setObjectType (ObjectType type);
	address getLinkBase () const;
	void setLinkBase (address base);

	AsmMode asmmode;
	bool nocase;
	bool autolocal;
	bool bracketonly;
private:
	ObjectType objecttype;
	address linkbase;
public:
	string headername;
	bool mode86;
	bool warn8080;
	bool redirecterr;
	bool verbose;
	bool common_after_abs;
	DebugType debugtype;
	size_t lines_to_skip;
	size_t numerr;

	std::vector <string> module;
};


class Asm {
public:
	Asm ();
	virtual ~Asm ();

	static Asm * create (const AsmOptions & options_n);

	virtual void addincludedir (const string & dirname)= 0;
	virtual void addpredef (const string & predef)= 0;
	virtual void setfilelisting (std::ostream & out_n)= 0;

	virtual void loadfile (const string & filename)= 0;
	virtual void processfile ()= 0;
	virtual void link ()= 0;

	virtual void emitcode (std::ostream & out)= 0;

	virtual void dumppublic (std::ostream & out)= 0;
	virtual void dumpsymbol (std::ostream & out)= 0;
protected:
	Asm (const Asm & a);
private:
	void operator = (const Asm &); // Forbidden
};


} // namespace pasmo

#endif

// End of asm.h
