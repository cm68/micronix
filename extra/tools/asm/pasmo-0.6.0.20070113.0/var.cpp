// var.cpp
// Revision 12-dec-2006


#include "var.h"
#include "trace.h"

#include <algorithm>

using std::for_each;


namespace pasmo {
namespace impl {


runtime_error InvalidExternOperation ("Invalid operation with extern");
runtime_error BadLiteralEval
	("Invalid operation with literal, lenght must be 1 or 2");


//**************************************************************
//			VarData
//**************************************************************


VarData::VarData (bool makelocal) :
	//value (0),
	val (ValueAbsolute, 0),
	defined (NoDefined),
	//type (ValueAbsolute),
	local (makelocal),
	divzero (false)
{
}

VarData::VarData (address value_n, Defined defined_n, ValueType type_n) :
	//value (value_n),
	val (type_n, value_n),
	defined (defined_n),
	//type (type_n),
	local (false),
	divzero (false)
{
}

VarData::VarData (Value value_n, Defined defined_n) :
	//value (value_n.value),
	val (value_n),
	defined (defined_n),
	//type (value_n.type),
	local (false),
	divzero (false)
{
}

VarData::VarData (const string & literal) :
	//value (0),
	val (ValueAbsolute, 0),
	name (literal),
	defined (DefinedLiteral),
	//type (ValueAbsolute),
	local (false),
	divzero (false)
{
}

void VarData::clear ()
{
	//value= 0;
	val.value= 0;
	defined= NoDefined;
}

void VarData::set (address value_n, Defined defined_n)
{
	//value= value_n;
	val.value= value_n;
	defined= defined_n;
}

void VarData::set (Value value_n, Defined defined_n)
{
	val= value_n;
	defined= defined_n;
}

void VarData::set (Value value_n)
{
	val= value_n;
}

void VarData::setdefined ()
{
	//value= defined ? addrTRUE : addrFALSE;
	val.value= defined ? addrTRUE : addrFALSE;
	val.type= ValueAbsolute;
	defined= PreDefined;
	ASSERT (! divzero);
}

Value VarData::getval () const
{
	switch (defined)
	{
	case DefinedLiteral:
		{
			const string::size_type l= name.size ();
			address v= 0;
			switch (l)
			{
			case 2:
				v= static_cast <unsigned char> (name [0] )
					* 256 + static_cast <unsigned char>
					(name [1] );
				break;
			case 1:
				v= static_cast <unsigned char>
					(name [0] );
				break;
			default:
				throw BadLiteralEval;
			}
			return Value (ValueAbsolute, v);
		}
	default:
		return val;
	}
}

address VarData::getvalue () const
{
	switch (defined)
	{
	case DefinedLiteral:
		{
			const string::size_type l= name.size ();
			switch (l)
			{
			case 1:
				return static_cast <unsigned char>
					(name [0] );
			case 2:
				return static_cast <unsigned char> (name [0] )
					* 256 + static_cast <unsigned char>
					(name [1] );
			default:
				throw BadLiteralEval;
			}
		}
	default:
		return val.value;
	}
}

string VarData::getname () const
{
	return name;
}

void VarData::setname (string name_n)
{
	name= name_n;
}

Defined VarData::def () const
{
	return defined;
}

ValueType VarData::gettype () const
{
	return val.type;
}

bool VarData::isliteral () const
{
	return defined == DefinedLiteral;
}

bool VarData::islocal () const
{
	return local;
}

bool VarData::isdefined () const
{
	return defined != NoDefined;
}

bool VarData::isextern () const
{
	return defined == DefinedExtern;
}

bool VarData::isabsolute () const
{
	return val.type == ValueAbsolute;
}

bool VarData::errordiv () const
{
	return false;
}

void VarData::noextern () const
{
	if (defined == DefinedExtern)
		throw InvalidExternOperation;
}

void VarData::updateflags (const VarData & vd)
{
	if (vd.divzero)
		divzero= true;
	if (! divzero)
	{
		switch (defined)
		{
		case DefinedExtern:
			throw InvalidExternOperation;
		case NoDefined:
			vd.noextern ();
			break;
		default:
			switch (vd.defined)
			{
			case DefinedExtern:
				throw InvalidExternOperation;
			case NoDefined:
				defined= NoDefined;
				break;
			default:
				defined= PreDefined;
			}
		}
	}
}

void VarData::add (const VarData & vd)
{
	//value+= vd.value;

	//const Defined vdef= vd.defined;
	//if (defined == DefinedExtern)
	if (this->isextern () )
	{
		//if (vdef == DefinedExtern || vd.gettype () != ValueAbsolute)
		if (vd.isextern () || ! vd.isabsolute () )
			throw InvalidExternOperation;
		//val.value+= vd.getvalue ();
		val+= vd.val;
	}
	else if (vd.isextern () )
	{
		Value vnew (vd.val);
		vnew+= val;
		val= vnew;
		defined= DefinedExtern;
		name= vd.name;
	}
	else
	{
		//value= getvalue () + vd.getvalue ();
		//val.value+= vd.getvalue ();
		val.value= getvalue () + vd.getvalue ();
		updateflags (vd);
	}
}

void VarData::sub (const VarData & vd)
{
	//value-= vd.value;

	//const Defined vdef= vd.defined;
	//if (defined == DefinedExtern)
	if (this->isextern () )
	{
		//if (vdef == DefinedExtern || vd.gettype () != ValueAbsolute)
		if (vd.isextern () || ! vd.isabsolute () )
			throw InvalidExternOperation;
		//val.value-= vd.getvalue ();
		val-= vd.val;
	}
	else
	{
		//val-= vd.val;
		val.value= getvalue () - vd.getvalue ();
		updateflags (vd);
	}
}

void VarData::mul (const VarData & vd)
{
	val.value*= vd.val.value;
	updateflags (vd);
}

void VarData::div (const VarData & vd)
{
	if (vd.defined)
	{
		if (vd.divzero)
		{
			val.value= 0;
			divzero= true;
		}
		else
		{
			address v= vd.val.value;
			if (v == 0)
			{
				val.value= 0;
				divzero= true;
			}
			else
				val.value/= vd.val.value;
		}
	}
	else
	{
		val.value= 0;
		defined= NoDefined;
	}
}

void VarData::mod (const VarData & vd)
{
	if (vd.defined)
	{
		if (vd.divzero)
		{
			val.value= 0;
			divzero= true;
		}
		else
		{
			address v= vd.val.value;
			if (v == 0)
			{
				val.value= 0;
				divzero= true;
			}
			else
				val.value%= vd.val.value;
		}
	}
	else
	{
		val.value= 0;
		defined= NoDefined;
	}
}

void VarData::shl (const VarData & vd)
{
	val.value<<= vd.val.value;
	updateflags (vd);
}

void VarData::shr (const VarData & vd)
{
	val.value>>= vd.val.value;
	updateflags (vd);
}

void VarData::equal (const VarData & vd)
{
	val.value= (val.value == vd.val.value) ? addrTRUE : addrFALSE;
	val.type= ValueAbsolute;
	updateflags (vd);
}

void VarData::notequal (const VarData & vd)
{
	val.value= (val.value != vd.val.value) ? addrTRUE : addrFALSE;
	val.type= ValueAbsolute;
	updateflags (vd);
}

void VarData::lessthan (const VarData & vd)
{
	val.value= (val.value < vd.val.value) ? addrTRUE : addrFALSE;
	val.type= ValueAbsolute;
	updateflags (vd);
}

void VarData::greaterthan (const VarData & vd)
{
	val.value= (val.value > vd.val.value) ? addrTRUE : addrFALSE;
	val.type= ValueAbsolute;
	updateflags (vd);
}

void VarData::lessequal (const VarData & vd)
{
	val.value= (val.value <= vd.val.value) ? addrTRUE : addrFALSE;
	val.type= ValueAbsolute;
	updateflags (vd);
}

void VarData::greaterequal (const VarData & vd)
{
	val.value= (val.value >= vd.val.value) ? addrTRUE : addrFALSE;
	val.type= ValueAbsolute;
	updateflags (vd);
}

void VarData::do_not ()
{
	val.value= ~ val.value;
}

void VarData::do_minus ()
{
	val.value= - val.value;
}

void VarData::do_boolnot ()
{
	val.value= ! val.value;
}

void VarData::do_and (const VarData & vd)
{
	val.value&= vd.val.value;
	updateflags (vd);
}

void VarData::do_or (const VarData & vd)
{
	val.value|= vd.val.value;
	updateflags (vd);
}

void VarData::do_xor (const VarData & vd)
{
	val.value^= vd.val.value;
	updateflags (vd);
}

void VarData::booland (const VarData & vd)
{
	if (defined && ! divzero)
	{
		if (val.value == 0)
			val.value= addrFALSE;
		else
		{
			val.value= (vd.val.value != 0) ? addrTRUE : addrFALSE;
			if (! vd.defined)
				defined= NoDefined;
			if (vd.divzero)
				divzero= true;
		}
	}
}

void VarData::boolor (const VarData & vd)
{
	if (defined && ! divzero)
	{
		if (val.value != 0)
			val.value= addrTRUE;
		else
		{
			val.value= (vd.val.value != 0) ? addrTRUE : addrFALSE;
			if (! vd.defined)
				defined= NoDefined;
			if (vd.divzero)
				divzero= true;
		}
	}
}

void VarData::do_high ()
{
	val.value= hibyte (val.value);
}

void VarData::do_low ()
{
	val.value= lobyte (val.value);
}

void VarData::cond (const VarData & first, const VarData & second)
{
	bool usefirst= val.value != 0;
	this->operator = (usefirst ? first : second);
}


//**************************************************************
//			mapvar_t
//**************************************************************


const VarData mapvar_t::varnodefined;

mapvar_t::iterator mapvar_t::begin ()
{
	return mvar.begin ();
}

mapvar_t::const_iterator mapvar_t::begin () const
{
	return mvar.begin ();
}

mapvar_t::iterator mapvar_t::end ()
{
	return mvar.end ();
}

mapvar_t::const_iterator mapvar_t::end () const
{
	return mvar.end ();
}

mapvar_t::iterator mapvar_t::find (const string & varname)
{
	//return mvar.find (varname);
	return mvar.find (stripdollar (varname) );
}

mapvar_t::const_iterator mapvar_t::find (const string & varname) const
{
	//return mvar.find (varname);
	return mvar.find (stripdollar (varname) );
}

bool mapvar_t::hasvar (const string & varname) const
{
	//return mvar.find (varname) != mvar.end ();
	return mvar.find (stripdollar (varname) ) != mvar.end ();
}

VarData mapvar_t::get (const string & varname) const
{
	//const_iterator it= mvar.find (varname);
	const_iterator it= mvar.find (stripdollar (varname) );

	if (it != mvar.end () )
		return it->second;
	else
		return varnodefined;
}

void mapvar_t::set (const string & varname, const VarData & data)
{
	//mvar [varname]= data;
	mvar [stripdollar (varname) ]= data;
}

void mapvar_t::set (const string & varname, const string & varfrom)
{
	//mvar [varname]= mvar [varfrom];
	mvar [stripdollar (varname) ]= mvar [stripdollar (varfrom) ];
}

bool mapvar_t::set (const string & varname, address value, Defined defined)
{
	//iterator it= mvar.find (varname);
	iterator it= mvar.find (stripdollar (varname) );
	if (it != mvar.end () )
	{
		it->second.set (value, defined);
		return it->second.islocal ();
	}
	else
	{
		//mvar.insert (make_pair (varname, VarData (value, defined) ) );
		mvar.insert (make_pair
			(stripdollar (varname), VarData (value, defined) ) );
		return false;
	}
}

bool mapvar_t::set (const string & varname,
	const Value & value, Defined defined)
{
	//iterator it= mvar.find (varname);
	iterator it= mvar.find (stripdollar (varname) );
	if (it != mvar.end () )
	{
		it->second.set (value, defined);
		return it->second.islocal ();
	}
	else
	{
		//mvar.insert (make_pair (varname, VarData (value, defined) ) );
		mvar.insert (make_pair
			(stripdollar (varname), VarData (value, defined) ) );
		return false;
	}
}

Defined mapvar_t::def (const string & varname) const
{
	//const_iterator it= mvar.find (varname);
	const_iterator it= mvar.find (stripdollar (varname) );
	if (it != mvar.end () )
		return it->second.def ();
	else
		return NoDefined;
}

class ClearDefl {
public:
	void operator () (mapvar_t::value_type & vardef)
	{
		VarData & vd= vardef.second;
		if (vd.def () == DefinedDEFL)
			vd.clear ();
	}
};

void mapvar_t::clearDEFL ()
{
	for_each (mvar.begin (), mvar.end (), ClearDefl () );
}


//**************************************************************
//			Vars
//**************************************************************


VarData Vars::getvar (const string & varname)
{
	TRFDEB (varname);

	return mapvar.get (stripdollar (varname) );
}

void Vars::setvar (const string & varname, const VarData & data)
{
	TRFDEB (varname);

	mapvar.set (stripdollar (varname), data);
}

void Vars::setvar (const string & varname, const string & varfrom)
{
	TRFDEB (varname);

	mapvar.set (stripdollar (varname), varfrom);
}

bool Vars::setvar (const string & varname, address value, Defined defined)
{
	TRFDEB (varname);

	return mapvar.set (stripdollar (varname), value, defined);
}

bool Vars::setvar (const string & varname,
	const Value & value, Defined defined)
{
	TRFDEB (varname);

	bool r= mapvar.set (stripdollar (varname), value, defined);
	TRDEBS (r);
	return r;
}

Defined Vars::defvar (const string & varname) const
{
	TRFDEB (varname);

	Defined r= mapvar.def (stripdollar (varname) );
	TRDEBS (r);
	return r;
}

mapvar_t::const_iterator Vars::varbegin () const
{
	return mapvar.begin ();
}

mapvar_t::const_iterator Vars::varend () const
{
	return mapvar.end ();
}

mapvar_t::const_iterator Vars::varfind (const string & varname) const
{
	return mapvar.find (stripdollar (varname) );
}

void Vars::clearDEFL ()
{
	mapvar.clearDEFL ();
}

bool Vars::ispublic (const string & varname) const
{
	return setpublic.find (stripdollar (varname) ) != setpublic.end ();
}

void Vars::makepublic (const string & varname)
{
	setpublic.insert (stripdollar (varname) );
}

void Vars::makeextern (const string & varname)
{
	//Defined def= mapvar.def (varname);
	Defined def= mapvar.def (stripdollar (varname) );

	if (def != NoDefined && def != DefinedExtern)
		throw runtime_error ("Can't define '" + varname +
			" as extern, is already defined");
	mapvar.set (stripdollar (varname), 0, DefinedExtern);
}

Vars::publiciterator Vars::pubbegin () const
{
	return setpublic.begin ();
}

Vars::publiciterator Vars::pubend () const
{
	return setpublic.end ();
}


} // namespace impl
} // namespace pasmo

// End of var.cpp
