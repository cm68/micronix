#ifndef INCLUDE_SIMPLEINST_H
#define INCLUDE_SIMPLEINST_H

// simpleinst.h
// Revision 17-dec-2006

#include "pasmotypes.h"
#include "token.h"

#include <vector>
#include <map>

namespace pasmo
{
namespace impl
{

class SimpleInst
{
private:
	byte code;
	bool edprefix;
	bool valid8080;
	bool valid86;

	byte code86_1;
	byte code86_2;
	byte code86_3;
	std::vector <byte> code86;
public:
	SimpleInst ();
	SimpleInst (byte code_n);
	SimpleInst (byte code_n, bool edprefix_n, bool valid8080_n= false);
	SimpleInst (byte code_n, bool edprefix_n, bool valid8080_n,
		unsigned long code86);
	SimpleInst (byte code_n, const string & code86_n);

	bool isED () const;
	bool is86 () const;
	bool is8080 () const;
	byte getcode () const;
	std::vector <byte> getcode86 () const;

	static const SimpleInst & get (TypeToken tt);

	typedef std::map <TypeToken, SimpleInst> simpleinst_t;
	static simpleinst_t simpleinst;
	class Initializer
	{
	public:
		Initializer (simpleinst_t & si);
	};
	static Initializer initializer;
};

} // namespace impl
} // namespace pasmo

#endif

// End of simpleinst.h
