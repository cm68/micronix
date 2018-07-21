// macroframe.cpp
// Revision 27-jun-2006

#include "macroframe.h"

#include "asmreal.h"
#include "parser.h"

#include "trace.h"


namespace pasmo {

namespace impl {


MacroFrameBase::MacroFrameBase (AsmReal & asmin_n,
		const MacroBase & macro_n, const MacroArgList & arglist_n) :
	asmin (asmin_n),
	nocase (asmin.getnocase () ),
	expandline (asmin.getline () ),
	previflevel (asmin.getiflevel () ),
	macro (macro_n),
	arglist (arglist_n),
	pprevmframe (asmin.getmframe () )
{
	MacroLevel * pproc= new MacroLevel (asmin);
	asmin.pushlocal (pproc);

	// Ensure that an IF opened before is not closed
	// inside the macro expansion.
	//asmin.iflevel= 0;
	asmin.setiflevel (0);

	asmin.setmframe (this);
}

MacroFrameBase::~MacroFrameBase ()
{
	// Clear the local frame, including unclosed PROCs and autolocals.
	while (dynamic_cast <MacroLevel *> (asmin.toplocal () ) == NULL)
	{
		asmin.poplocal ();
	}
	asmin.poplocal ();

	// IF whitout ENDIF inside a macro are valid.
	while (asmin.getiflevel () > 0)
	{
		asmin.deciflevel ();
	}
	asmin.setiflevel (previflevel);

	asmin.setmframe (pprevmframe);
}


void MacroFrameBase::setarguments (const MacroArgList & arglist_n)
{
	TRFUNC (tr, "MacroFrameBase::setarguments");

	arglist= arglist_n;
}

size_t MacroFrameBase::getexpline () const
{
	return expandline;
}

void MacroFrameBase::shift ()
{
	throw ShiftOutsideMacro;
}

void MacroFrameBase::do_shift ()
{
	arglist.erase (arglist.begin () );
}

string MacroFrameBase::expandarg (size_t argnum)
{
	string result;
	if (argnum < arglist.size () )
	{
		const MacroArg & arg= arglist [argnum];
		for (MacroArg::size_type i= 0; i < arg.size (); ++i)
		{
			const Token & tok= arg [i];
			switch (tok.type () )
			{
			case TypeNumber:
				result+= tok.numstr ();
				break;
			case TypeLiteral:
				result+= tok.raw ();
				break;
			case TypeComment:
				// TODO: perhaps make this illegal.
				//result+= tok.raw ();
				result+= tok.str ();
				break;
			default:
				result+= tok.str ();
			}
		}
	}
	return result;
}

string MacroFrameBase::expandarginlit (size_t argnum)
{
	string result;
	if (argnum < arglist.size () )
	{
		const MacroArg & arg= arglist [argnum];
		for (MacroArg::size_type i= 0; i < arg.size (); ++i)
		{
			switch (arg [i].type () )
			{
			case TypeNumber:
				result+= arg [i].numstr ();
				break;
			default:
				result+= arg [i].str ();
			}
		}
	}
	return result;
}

size_t MacroFrameBase::getparam (const string & parname)
{
	return macro.getparam (nocase ? upper (parname) : parname);
}

string MacroFrameBase::substparam (const string & parname)
{
	TRFUNC (tr, "MacroFrameBase::substparam");
	TRMESSAGE (tr, parname);

	size_t n= getparam (parname);
	if (n == Macro::noparam)
		return parname;
	else
	{
		TRMESSAGE (tr, "is parameter");
		return expandarg (n);
	}
}

string MacroFrameBase::substparamafter_amp (const string & parname)
{
	TRFUNC (tr, "MacroFrameBase::substparamafer_amp");
	TRMESSAGE (tr, parname);

	size_t n= getparam (parname);
	if (n == Macro::noparam)
		return '&' + parname;
	else
	{
		TRMESSAGE (tr, "is parameter");
		return expandarg (n);
	}
}

string MacroFrameBase::substparaminlit (const string & parname)
{
	TRFUNC (tr, "MacroFrameBase::substparaminlit");

	size_t n= getparam (parname);
	if (n == Macro::noparam)
		return '&' + parname;
	else
	{
		TRMESSAGE (tr, "is parameter");
		return expandarginlit (n);
	}
}

string getidentifier (const string & s)
{
	string result;
	const string::size_type l (s.size () );
	string::size_type i;
	for (i= 0; i < l && ischaridentifier (s [i] ); ++i)
	{
		result+= s [i];
	}
	return result;
}

string MacroFrameBase::substliteral (const string & literal)
{
	TRFUNC (tr, "MacroFrameBase::substliteral");

	string s (literal);
	string result;
	string::size_type pos= 0;
	while ( (pos= s.find ('&') ) != string::npos)
	{
		result+= s.substr (0, pos);
		s.erase (0, pos + 1);
		string param= getidentifier (s);
		const string::size_type psize (param.size () );
		if (psize > 0)
		{
			result+= substparaminlit (param);
			s.erase (0, psize);
		}
		else
			result+= '&';
	}
	result+= s;

	TRSTREAM (tr, '\'' << literal << "' -> '" << result << '\'');

	return '"' + result + '"';
}

string MacroFrameBase::substparams (Tokenizer & tz)
{
	TRFUNC (tr, "MacroFrameBase::substparams");

	string expanded;
	TypeToken tt;
	bool skipblank= false;
	for (Token tok= tz.getrawtoken ();
		(tt= tok.type () ) != TypeEndLine;
		tok= tz.getrawtoken () )
	{
		if (skipblank)
		{
			if (tt == TypeWhiteSpace)
				continue;
			skipblank= false;
		}
		switch (tt)
		{
		case TypeLiteral:
			expanded+= substliteral (tok.str () );
			break;
		case TypeIdentifier:
			expanded+= substparam (tok.str () );
			break;
		case TypeNumber:
			expanded+= tok.numstr ();
			break;
		case TypeWhiteSpace:
			expanded+= tok.raw ();
			break;
		case TypeBitAnd:
			{
				Token tok2= tz.getrawtoken ();
				switch (tok2.type () )
				{
				case TypeWhiteSpace:
					expanded+= tok.str ();
					expanded+= ' ';
					break;
				case TypeIdentifier:
					expanded+= substparamafter_amp
						(tok2.str () );
					break;
				default:
					expanded+= tok.str ();
					expanded+= tok2.str ();
				}
			}
			break;
		case TypeComment:
			{
				const string comment= tok.str ();
				if (! comment.empty () && comment [0] != ';')
				{
					expanded+= ";";
					expanded+= comment;
				}
			}
			break;
		case TypeSharpSharp:
			{
				string::size_type pos=
					expanded.find_last_not_of (" ");
				if (pos != string::npos)
					expanded.erase (pos + 1);
				else
					expanded.erase ();
			}
			skipblank= true;
			break;
		case TypeMacroArg:
			expanded+= tok.str ();
			break;
		default:
			expanded+= substparam (tok.raw () );
		}
	}
	TRMESSAGE (tr, expanded);
	return expanded;
}

string MacroFrameBase::substparams (const string & line)
{
	Tokenizer tz (line, asmin.getasmmode () );
	return MacroFrameBase::substparams (tz);
}


MacroFrameChild::MacroFrameChild (AsmReal & asmin_n,
		const MacroBase & macro_n, const MacroArgList & arglist_n) :
	MacroFrameBase (asmin_n, macro_n, arglist_n)
{
}

void MacroFrameChild::shift ()
{
	MacroFrameBase * prev= getprevframe ();
	if (prev)
		prev->shift ();
	else
		throw ShiftOutsideMacro;
}

string MacroFrameChild::substparams (Tokenizer & tz)
{
	TRFUNC (tr, "MacroFrameChild::substparams");

	MacroFrameBase * prev= getprevframe ();
	if (prev)
	{
		const string newline= prev->substparams (tz);
		return MacroFrameBase::substparams (newline);
	}
	else
		return MacroFrameBase::substparams (tz);
}

MacroFrameIRPbase::MacroFrameIRPbase (AsmReal & asmin_n,
		const MacroIRPbase & macro_n, const MacroArgList & arglist_n) :
	MacroFrameChild (asmin_n, macro_n, arglist_n)
{
}


MacroFrameIRP::MacroFrameIRP (AsmReal & asmin_n,
		const string & varname, const MacroArgList & arglist_n) :
	MacroFrameIRPbase (asmin_n, macroirp, MacroArgList () ),
	macroirp (varname),
	arglist (arglist_n),
	item (0)
{
	MacroArgList actualarg;
	actualarg.push_back (arglist [0] );
	setarguments (actualarg);
}

string MacroFrameIRP::name () const
{
	return "IRP";
}

bool MacroFrameIRP::iterate ()
{
	++item;
	if (item >= arglist.size () )
		return false;
	else
	{
		MacroArgList actualarg;
		actualarg.push_back (arglist [item] );
		setarguments (actualarg);
		return true;
	}
}


MacroFrameIRPC::MacroFrameIRPC (AsmReal & asmin_n,
		const string & varname, const string & charlist_n) :
	MacroFrameIRPbase (asmin_n, macroirpc, MacroArgList () ),
	macroirpc (varname),
	charlist (charlist_n),
	end_items (charlist.size () ),
	item (0)
{
	updatearg ();
}

string MacroFrameIRPC::name () const
{
	return "IRPC";
}

void MacroFrameIRPC::updatearg ()
{
	ASSERT (item < end_items);

	MacroArgList actualarg;
	MacroArg args;
	args.push_back
		(Token (TypeIdentifier, string (1, charlist [item] ) ) );
	actualarg.push_back (args);
	setarguments (actualarg);
}

bool MacroFrameIRPC::iterate ()
{
	++item;
	if (item >= end_items)
		return false;
	else
	{
		updatearg ();
		return true;
	}
}


MacroFrameREPT::MacroFrameREPT (AsmReal & asmin_n,
		address numrep_n, const string & varcounter_n,
		address valuecounter_n, address step_n) :
	MacroFrameChild (asmin_n, macrorept, MacroArgList () ),
	macrorept (varcounter_n),
	numrep (numrep_n),
	varcounter (varcounter_n),
	valuecounter (valuecounter_n),
	step (step_n)
{
	if (! varcounter.empty () )
		updatearg ();
}

string MacroFrameREPT::name () const
{
	return "REPT";
}

void MacroFrameREPT::updatearg ()
{
	MacroArgList actualarg;
	MacroArg args;
	args.push_back (Token (valuecounter) );
	actualarg.push_back (args);
	setarguments (actualarg);
}

bool MacroFrameREPT::iterate ()
{
	if (--numrep > 0)
	{
		if (! varcounter.empty () )
		{
			valuecounter+= step;
			updatearg ();
		}
		return true;
	}
	else
		return false;
}


MacroFrameMacro::MacroFrameMacro (AsmReal & asmin_n,
		const string & macroname_n, const MacroArgList & arglist_n) :
	MacroFrameBase (asmin_n, macro, arglist_n),
	macro (asmin_n.getmacro (macroname_n) ),
	macroname (macroname_n)
{
}

string MacroFrameMacro::name () const
{
	return "Macro " + macroname;
}

size_t MacroFrameMacro::getline () const
{
	return macro.getline ();
}

size_t MacroFrameMacro::getendline () const
{
	return macro.getendline ();
}

size_t MacroFrameMacro::returnline () const
{
	return getexpline ();
}

void MacroFrameMacro::shift ()
{
	do_shift ();
}

string MacroFrameMacro::substparams (Tokenizer & tz)
{
	TRFUNC (tr, "MacroFrameMacro::substparams");

	return MacroFrameBase::substparams (tz);
}


} // namespace impl

} // namespace pasmo


// End of macroframe.cpp
