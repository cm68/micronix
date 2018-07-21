// local.cpp
// Revision 10-jan-2007


#include "local.h"

#include "asmreal.h"

#include "trace.h"


namespace pasmo
{

namespace impl
{


pasmo_fatal LocalNotExist ("Trying to use a non existent local level");


LocalLevel::LocalLevel (AsmReal & asmin_n) :
	asmin (asmin_n)
{ }

LocalLevel::~LocalLevel ()
{
	TRFUNC (tr, "LocalLevel::~LocalLevel");

	for (mapvar_t::iterator it= saved.begin ();
		it != saved.end ();
		++it)
	{
		const string locname= it->first;
		const string & globname= globalized [locname];

		Defined d= asmin.defvar (locname);

		if (d == NoDefined)
		{
			asmin.emitwarning ("LOCAL " + locname +
				" is not used");
		}

		asmin.setvar (globname, locname);
		asmin.setvar (locname, it->second);
	}
}

bool LocalLevel::is_auto () const
{
	return false;
}

void LocalLevel::add (const string & varname)
{
	// Ignore redeclarations as LOCAL of the same identifier.
	if (saved.hasvar (varname) )
		return;

	saved.set (varname, asmin.rawgetvar (varname) );
	const string globname= asmin.genlocalname (varname);
	globalized [varname]= globname;

	switch (asmin.currentpass () )
	{
	case 1:
		asmin.setvar (varname, VarData (true) );
		break;
	case 2:
		asmin.setvar (varname, globname);
		break;
	default:
		throw InvalidPassValue;
	}
}

AutoLevel::AutoLevel (AsmReal & asmin_n) :
	LocalLevel (asmin_n)
{ }

AutoLevel::~AutoLevel ()
{
	TRFUNC (tr, "AutoLevel::~AutoLevel");
}

bool AutoLevel::is_auto () const
{
	return true;
}

ProcLevel::ProcLevel (AsmReal & asmin_n, size_t line) :
	LocalLevel (asmin_n),
	line (line)
{ }

ProcLevel::~ProcLevel ()
{
	TRFUNC (tr, "ProcLevel::~ProcLevel");
}

size_t ProcLevel::getline () const
{
	return line;
}

MacroLevel::MacroLevel (AsmReal & asmin_n) :
	LocalLevel (asmin_n)
{ }

MacroLevel::~MacroLevel ()
{
	TRFUNC (tr, "MacroLevel::~MacroLevel");
}

LocalStack::~LocalStack ()
{
	TRFUNC (tr, "LocalStack::~LocalStack");

	while (! st.empty () )
		pop ();
}

bool LocalStack::empty () const
{
	return st.empty ();
}

void LocalStack::push (LocalLevel * level)
{
	st.push (level);
}

LocalLevel * LocalStack::top () const
{
	if (st.empty () )
		throw LocalNotExist;
	return st.top ();
}

void LocalStack::pop ()
{
	if (st.empty () )
		throw LocalNotExist;
	delete st.top ();
	st.pop ();
}


} // namespace impl

} // namespace pasmo


// End of local.cpp
