#ifndef INCLUDE_LOCAL_H
#define INCLUDE_LOCAL_H

// local.h
// Revision 10-jan-2007


#include <string>
#include <map>
#include <stack>

#include "var.h"


namespace pasmo
{

namespace impl
{


class AsmReal;


class LocalLevel
{
public:
	LocalLevel (AsmReal & asmin_n);
	virtual ~LocalLevel ();
	virtual bool is_auto () const;
	void add (const std::string & varname);
private:
	AsmReal & asmin;
	mapvar_t saved;
	std::map <std::string, std::string> globalized;
};


class AutoLevel : public LocalLevel
{
public:
	AutoLevel (AsmReal & asmin_n);
	~AutoLevel ();
	bool is_auto () const;
};


class ProcLevel : public LocalLevel
{
public:
	ProcLevel (AsmReal & asmin_n, size_t line);
	~ProcLevel ();
	size_t getline () const;
private:
	size_t line;
};


class MacroLevel : public LocalLevel
{
public:
	MacroLevel (AsmReal & asmin_n);
	~MacroLevel ();
};


class LocalStack
{
public:
	~LocalStack ();
	bool empty () const;
	void push (LocalLevel * level);
	LocalLevel * top () const;
	void pop ();
private:
	typedef std::stack <LocalLevel *> st_t;
	st_t st;
};


} // namespace impl

} // namespace pasmo


#endif

// End of local.h
