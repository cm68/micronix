#ifndef INCLUDE_MACROFRAME_H
#define INCLUDE_MACROFRAME_H

// macroframe.h
// Revision 27-jun-2006


#include "token.h"
#include "macro.h"


#include <string>
#include <vector>


namespace pasmo {

namespace impl {


class AsmReal;

typedef std::vector <Token> MacroArg;
typedef std::vector <MacroArg> MacroArgList;


class MacroFrameBase {
protected:
	MacroFrameBase (AsmReal & asmin_n,
		const MacroBase & macro_n, const MacroArgList & arglist_n);
public:
	virtual ~MacroFrameBase ();
	virtual std::string name () const = 0;
	void setarguments (const MacroArgList & arglist_n);
	size_t getexpline () const;
	virtual void shift ();
	virtual std::string substparams (Tokenizer & tz);
	std::string substparams (const std::string & line);

	virtual bool iterate () { return false; }
	virtual size_t returnline () const { return size_t (-1); }
protected:
	AsmReal & asmin;
	bool nocase;
	void do_shift ();
	MacroFrameBase * getprevframe () { return pprevmframe; }
private:
	const size_t expandline;
	const size_t previflevel;
	const MacroBase & macro;
	MacroArgList arglist;
	MacroFrameBase * pprevmframe;

	std::string expandarg (size_t argnum);
	std::string expandarginlit (size_t argnum);
	size_t getparam (const std::string & parname);
	std::string substparam (const std::string & parname);
	std::string substparamafter_amp (const std::string & parname);
	std::string substparaminlit (const std::string & parname);
	std::string substliteral (const std::string & literal);
};


class MacroFrameChild : public MacroFrameBase {
protected:
	MacroFrameChild (AsmReal & asmin_n,
		const MacroBase & macro_n, const MacroArgList & arglist_n);
public:
	void shift ();
	std::string substparams (Tokenizer & tz);
};

class MacroFrameIRPbase : public MacroFrameChild {
protected:
	MacroFrameIRPbase (AsmReal & asmin_n,
		const MacroIRPbase & macro_n, const MacroArgList & arglist_n);
};

class MacroFrameIRP : public MacroFrameIRPbase {
public:
	MacroFrameIRP (AsmReal & asmin_n,
		const std::string & varname, const MacroArgList & arglist_n);
	std::string name () const;
	bool iterate ();
private:
	MacroIRP macroirp;
	MacroArgList arglist;
	size_t item;
};

class MacroFrameIRPC : public MacroFrameIRPbase {
public:
	MacroFrameIRPC (AsmReal & asmin_n,
		const std::string & varname, const std::string & charlist_n);
	std::string name () const;
	bool iterate ();
private:
	MacroIRP macroirpc;
	const std::string charlist;
	const size_t end_items;
	size_t item;

	void updatearg ();
};

class MacroFrameREPT : public MacroFrameChild {
public:
	MacroFrameREPT (AsmReal & asmin_n,
		address numrep_n, const std::string & varcounter_n,
		address valuecounter_n, address step_n);
	std::string name () const;
	bool iterate ();
private:
	MacroREPT macrorept;
	address numrep;
	const std::string varcounter;
	address valuecounter;
	address step;

	void updatearg ();
};


class MacroFrameMacro : public MacroFrameBase {
public:
	MacroFrameMacro (AsmReal & asmin_n,
		const std::string & macroname_n,
		const MacroArgList & arglist_n);
	std::string name () const;
	size_t getline () const;
	size_t getendline () const;
	size_t returnline () const;
	void shift ();
	std::string substparams (Tokenizer & tz);
private:
	const Macro macro;
	const std::string macroname;
};


} // namespace impl

} // namespace pasmo


#endif

// End of macroframe.h
