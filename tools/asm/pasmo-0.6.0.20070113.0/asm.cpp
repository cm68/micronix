// asm.cpp
// Revision 14-dec-2006

#include "asm.h"
#include "asmimpl.h"

namespace pasmo {


AsmOptions::AsmOptions () :
	asmmode (AsmZ80),
	nocase (false),
	autolocal (false),
	bracketonly (false),
	objecttype (ObjectBin),
	linkbase (0),
	mode86 (false),
	warn8080 (false),
	redirecterr (false),
	verbose (false),
	common_after_abs (false),
	debugtype (NoDebug),
	lines_to_skip (0),
	numerr (1)
{
}

ObjectType AsmOptions::getObjectType () const
{
	return objecttype;
}

void AsmOptions::setObjectType (ObjectType type)
{
	objecttype= type;
	if (objecttype == ObjectCom || objecttype == ObjectCmd)
		linkbase= 0x100;
}

address AsmOptions::getLinkBase () const
{
	return linkbase;
}

void AsmOptions::setLinkBase (address base)
{
	linkbase= base;
}


Asm::Asm ()
{
}

Asm::Asm (const Asm &)
{
}

Asm::~Asm ()
{
}

Asm * Asm::create (const AsmOptions & options_n)
{
	return pasmo::impl::AsmImpl::create (options_n);
}


} // namespace pasmo

// End of asm.cpp
