#ifndef INCLUDE_VAR_H
#define INCLUDE_VAR_H

// var.h
// Revision 24-jun-2006


#include "value.h"

#include <map>
#include <set>


namespace pasmo {


enum Defined {
	NoDefined,
	DefinedLiteral,
	DefinedDEFL,
	PreDefined,
	DefinedPass1,
	DefinedPass2,
	DefinedExtern
};


namespace impl {


using std::map;
using std::set;


class VarData {
public:
	explicit VarData (bool makelocal= false);
	VarData (address value_n, Defined defined_n,
		ValueType type_n= ValueAbsolute);
	VarData (Value value_n, Defined defined_n);
	VarData (const string & literal);

	void clear ();
	void set (address value_n, Defined defined_n);
	void set (Value value_n, Defined defined_n);
	void set (Value value_n);
	void setdefined ();

	Value getval () const;
	address getvalue () const;
	string getname () const;
	void setname (string name_n);

	Defined def () const;
	ValueType gettype () const;
	bool isliteral () const;
	bool islocal () const;
	bool isdefined () const;
	bool isextern () const;
	bool isabsolute () const;
	bool errordiv () const;
	void noextern () const;

	void updateflags (const VarData & vd);

	void add (const VarData & vd);
	void mul (const VarData & vd);
	void sub (const VarData & vd);
	void div (const VarData & vd);
	void mod (const VarData & vd);
	void shl (const VarData & vd);
	void shr (const VarData & vd);
	void equal (const VarData & vd);
	void notequal (const VarData & vd);
	void lessthan (const VarData & vd);
	void greaterthan (const VarData & vd);
	void lessequal (const VarData & vd);
	void greaterequal (const VarData & vd);
	void do_not ();
	void do_boolnot ();
	void do_minus ();
	void booland (const VarData & vd);
	void boolor (const VarData & vd);
	void do_and (const VarData & vd);
	void do_or (const VarData & vd);
	void do_xor (const VarData & vd);
	void do_high ();
	void do_low ();
	void cond (const VarData & first, const VarData & second);
private:
	Value val;
	//address value;
	string name;
	Defined defined;
	//ValueType type;
	bool local;
	bool divzero;
};


class mapvar_t {
public:
	typedef map <string, VarData> m_t;
	typedef m_t::iterator iterator;
	typedef m_t::const_iterator const_iterator;
	typedef m_t::value_type value_type;

	iterator begin ();
	const_iterator begin () const;
	iterator end ();
	const_iterator end () const;

	iterator find (const string & varname);
	const_iterator find (const string & varname) const;

	bool hasvar (const string & varname) const;

	VarData get (const string & varname) const;
	void set (const string & varname, const VarData & data);
	void set (const string & varname, const string & varfrom);
	bool set (const string & varname, address value, Defined defined);
	bool set (const string & varname,
		const Value & value, Defined defined);

	Defined def (const string & varname) const;

	void clearDEFL ();
private:
	m_t mvar;
	static const VarData varnodefined;
};


class Vars {
public:
	VarData getvar (const string & varname);
	void setvar (const string & varname, const VarData & data);
	void setvar (const string & varname, const string & varfrom);
	bool setvar (const string & varname, address value, Defined defined);
	bool setvar (const string & varname,
		const Value & value, Defined defined);

	Defined defvar (const string & varname) const;

	mapvar_t::const_iterator varbegin () const;
	mapvar_t::const_iterator varend () const;

	mapvar_t::const_iterator varfind (const string & varname) const;

	void clearDEFL ();

	bool ispublic (const string & varname) const;
	void makepublic (const string & varname);

	void makeextern (const string & varname);

	typedef set <string> setpublic_t;
	typedef setpublic_t::const_iterator publiciterator;

	publiciterator pubbegin () const;
	publiciterator pubend () const;
private:
	// ********** Symbol tables ************

	mapvar_t mapvar;
	setpublic_t setpublic;
};


} // namespace impl
} // namespace pasmo


#endif

// End of var.h
