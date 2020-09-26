#ifndef INCLUDE_MACRO_H
#define INCLUDE_MACRO_H

// macro.h
// Revision 20-nov-2005

#include "pasmotypes.h"

#include <vector>
#include <map>


namespace pasmo {
namespace impl {


using std::vector;
using std::map;


class MacroBase {
protected:
	MacroBase ();
	explicit MacroBase (const vector <string> & param);
	explicit MacroBase (const string & sparam);
	~MacroBase ();
public:
	size_t getparam (const string & name) const;
	string getparam (size_t n) const;
	static const size_t noparam= size_t (-1);
private:
	vector <string> param;
};


class Macro : public MacroBase {
public:
	Macro (const vector <string> & param,
			size_t linen, size_t endlinen);
	~Macro ();
	size_t getline () const;
	size_t getendline () const;
private:
	const size_t line;
	const size_t endline;
};


class MacroIRPbase : public MacroBase {
public:
	MacroIRPbase (const string & sparam);
};


class MacroIRP : public MacroIRPbase {
public:
	MacroIRP (const string & sparam);
};


class MacroIRPC : public MacroIRPbase {
public:
	MacroIRPC (const string & sparam);
};


class MacroREPT : public MacroBase {
public:
	MacroREPT (const string & sparam);
};


class MapMacro {
private:
	typedef map <string, Macro> mapmacro_t;
	mapmacro_t mapmacro;
public:
	typedef mapmacro_t::iterator iterator;
	typedef mapmacro_t::const_iterator const_iterator;

	void clear ();
	void erase (const string & name);
	void insert (const string & name,
		const vector <string> & param,
		size_t beginline, size_t endline);

	const_iterator begin () const;
	const_iterator end () const;
	const_iterator find (const string & name) const;
};


class MacroStore {
protected:
	void clear ();
	const Macro & getmacro (const string & name);
	bool ismacro (const string & name) const;
	void insert (const string & name,
		const vector <string> & param,
		size_t beginline, size_t endline);
private:
	MapMacro mapmacro;
};


} // namespace impl
} // namespace pasmo


#endif

// End of macro.h
