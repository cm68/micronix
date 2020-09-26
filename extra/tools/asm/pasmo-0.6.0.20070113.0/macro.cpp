// macro.cpp
// Revision 23-jan-2006

#include "macro.h"

#include "trace.h"

#include <algorithm>
#include <stdexcept>

namespace pasmo {
namespace impl {


using std::transform;
using std::runtime_error;


namespace {

vector <string> normalizeparam (const vector <string> & param)
{
	vector <string> result;
	transform (param.begin (), param.end (),
		back_inserter (result), stripdollar);
	return result;
}

class Expected : public runtime_error {
public:
	#if 0
	Expected (const Token & tokexp, const Token & tokfound) :
		runtime_error ("'" + tokexp.str () + "' expected but '" +
			tokfound.str () + "' found")
	{ }
	Expected (const string & expected, const Token & tok) :
		runtime_error (expected + " expected but '" +
			tok.str () + "' found")
	{ }
	#endif
	Expected (const string & expected, const string & found) :
		runtime_error (expected + " expected but '" +
			found + "' found")
	{ }
};


class MacroExpected : public Expected {
public:
	MacroExpected (const string & name) :
		Expected ("Macro name", name)
	{ }
};

} // namespace


MacroBase::MacroBase ()
{
	TRFUNC (tr, "MacroBase::MacroBase");
}

MacroBase::MacroBase (const vector <string> & param) :
	param (normalizeparam (param) )
{
	TRFUNC (tr, "MacroBase::MacroBase");
}

MacroBase::MacroBase (const string & sparam) :
	param (1)
{
	TRFUNC (tr, "MacroBase::MacroBase");
	TRMESSAGE (tr, sparam);

	param [0]= stripdollar (sparam);
}

MacroBase::~MacroBase ()
{
	TRFUNC (tr, "MacroBase::~MacroBase");
}

size_t MacroBase::getparam (const string & name) const
{
	const string normname= stripdollar (name);
	for (size_t i= 0; i < param.size (); ++i)
		if (normname == param [i])
			return i;
	return noparam;
}

string MacroBase::getparam (size_t n) const
{
	if (n >= param.size () )
		return "(none)";
	return param [n];
}


Macro::Macro (const vector <string> & param,
		size_t linen, size_t endlinen) :
	MacroBase (param),
	line (linen),
	endline (endlinen)
{ }

Macro::~Macro ()
{
	TRFUNC (tr, "Macro::~Macro");
}

size_t Macro::getline () const
{ return line; }

size_t Macro::getendline () const
{ return endline; }


MacroIRPbase::MacroIRPbase (const string & sparam) :
	MacroBase (sparam)
{ }

MacroIRP::MacroIRP (const string & sparam) :
	MacroIRPbase (sparam)
{ }

MacroIRPC::MacroIRPC (const string & sparam) :
	MacroIRPbase (sparam)
{ }

MacroREPT::MacroREPT (const string & sparam) :
	MacroBase (sparam)
{ }


void MapMacro::clear ()
{
	mapmacro.clear ();
}

void MapMacro::erase (const string & name)
{
	mapmacro_t::iterator it= mapmacro.find (name);
	if (it != mapmacro.end () )
		mapmacro.erase (it);
}

void MapMacro::insert (const string & name,
	const vector <string> & param,
	size_t beginline, size_t endline)
{
	erase (name);
	mapmacro.insert (make_pair (stripdollar (name),
		Macro (param, beginline, endline) ) );
}

MapMacro::const_iterator MapMacro::begin () const
{
	return mapmacro.begin ();
}

MapMacro::const_iterator MapMacro::end () const
{
	return mapmacro.end ();
}

MapMacro::const_iterator MapMacro::find (const string & name) const
{
	return mapmacro.find (stripdollar (name) );
}


void MacroStore::clear ()
{
	mapmacro.clear ();
}

const Macro & MacroStore::getmacro (const string & name)
{
	MapMacro::const_iterator it= mapmacro.find (name);
	if (it == mapmacro.end () )
		throw MacroExpected (name);
	else
		return it->second;
}

bool MacroStore::ismacro (const string & name) const
{
	return mapmacro.find (name) != mapmacro.end ();
}

void MacroStore::insert (const string & name,
	const vector <string> & param,
	size_t beginline, size_t endline)
{
	mapmacro.insert (name, param, beginline, endline);
}


} // namespace impl
} // namespace pasmo


// End of macro.cpp
