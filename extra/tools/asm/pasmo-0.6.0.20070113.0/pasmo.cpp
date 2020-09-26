// pasmo.cpp
// Revision 13-jun-2006

#include "asm.h"

#include "config_version.h"

#include "trace.h"


#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <algorithm>


namespace pasmo {
namespace main {

using std::ios;
using std::streambuf;
using std::ostream;
using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using std::vector;
using std::map;
using std::auto_ptr;
using std::exception;
using std::runtime_error;
using std::logic_error;
using std::for_each;

const string pasmoversion ("Pasmo " VERSION);

class DoNothing { };
class Usage { };

class ErrorInOptions : public runtime_error {
public:
	ErrorInOptions (const string & s) :
		runtime_error (s)
	{ }
};

class NeedArgument : public ErrorInOptions {
public:
	NeedArgument (const string & option) :
		ErrorInOptions ("Option " + option + " requires argument")
	{ }
};

class InvalidArgument : public ErrorInOptions {
public:
	InvalidArgument (const string & option, const string & arg) :
		ErrorInOptions ("Invalid argument '" + arg +
			"' to option " + option)
	{ }
};

class InvalidOption : public ErrorInOptions {
public:
	InvalidOption (const string & option) :
		ErrorInOptions ("Invalid option: " + option)
	{ }
};

runtime_error LinkButFile
	("link option and input file can't be used together");

runtime_error CreateObjectError ("Error creating object file");
runtime_error SymbolFileError ("Error creating symbols file");
runtime_error PublicFileError ("Error creating public symbols file");
runtime_error ListingFileError ("Error creating listing file");


class Options : public AsmOptions {
public:
	Options (int argc_n, char * * argv_n);

	bool publiconly () const { return emitpublic; }
	string getfilein () const { return filein; }
	string getfileout () const { return fileout; }
	string getfilesymbol () const { return filesymbol; }
	string getfilepublic () const;
	string getfilelisting () const;
	void apply (Asm & assembler) const;
private:
	Options (const Options &); // Forbidden
	void operator = (const Options &); // Forbidden

	int argc;
	char * * const argv;
	bool end_options;

	typedef void (Options::*handle_option_t) (const string &, int &);

	typedef map <string, handle_option_t> mapoption_t;
	static mapoption_t mapoption;

	typedef map <string, string> descoption_t;
	static descoption_t descoption;

	typedef map <string, string> shortoption_t;
	static shortoption_t shortoption;

	static void initmap ();
	static handle_option_t gethandler (const string & optname);

	#define DECLARE_HANDLER(option) \
		void handle_option_ ## option \
			(const string & optname, int & argpos)

	DECLARE_HANDLER (dash);
	DECLARE_HANDLER (doubledash);
	DECLARE_HANDLER (endoptions);

	DECLARE_HANDLER (86);
	DECLARE_HANDLER (after);
	DECLARE_HANDLER (alocal);
	DECLARE_HANDLER (amsdos);
	DECLARE_HANDLER (asm);
	DECLARE_HANDLER (base);
	DECLARE_HANDLER (bin);
	DECLARE_HANDLER (bracket);
	DECLARE_HANDLER (cdt);
	DECLARE_HANDLER (cdtbas);
	DECLARE_HANDLER (cmd);
	DECLARE_HANDLER (com);
	DECLARE_HANDLER (debinfo);
	DECLARE_HANDLER (debinfo1);
	DECLARE_HANDLER (dump);
	DECLARE_HANDLER (err);
	DECLARE_HANDLER (equ);
	DECLARE_HANDLER (help);
	DECLARE_HANDLER (hex);
	DECLARE_HANDLER (include_dir);
	DECLARE_HANDLER (input);
	DECLARE_HANDLER (link);
	DECLARE_HANDLER (listing);
	DECLARE_HANDLER (module);
	DECLARE_HANDLER (msx);
	DECLARE_HANDLER (name);
	DECLARE_HANDLER (nocase);
	DECLARE_HANDLER (numerr);
	DECLARE_HANDLER (output);
	DECLARE_HANDLER (plus3dos);
	DECLARE_HANDLER (prl);
	DECLARE_HANDLER (public);
	DECLARE_HANDLER (rel);
	DECLARE_HANDLER (skiplines);
	DECLARE_HANDLER (tap);
	DECLARE_HANDLER (tapbas);
	DECLARE_HANDLER (tzx);
	DECLARE_HANDLER (tzxbas);
	DECLARE_HANDLER (verbose);
	DECLARE_HANDLER (version);
	DECLARE_HANDLER (w8080);
	#undef DECLARE_HANDLER 

	bool linkonly;
	bool emitpublic;

	vector <string> includedir;
	vector <string> labelpredef;

	string filein;
	string fileout;
	string filesymbol;
	string filepublic;
	string filelisting;
	friend void showlongshort (Options::shortoption_t::value_type & sop);
	friend void showdescopt (Options::descoption_t::value_type & desc);
};

Options::mapoption_t Options::mapoption;
Options::descoption_t Options::descoption;
Options::shortoption_t Options::shortoption;

void Options::initmap ()
{
	#define MAP_SHORT_OPTION(option, longoption) \
		shortoption ["-" #option]= \
			"--" #longoption
	#define MAP_LONG_OPTION(option,desc) \
		mapoption ["--" #option]= \
			& Options::handle_option_ ## option; \
		descoption ["--" #option]= \
			desc
	#define MAP_LONG_NAME_OPTION(option,name,desc) \
		mapoption ["--" #name]= \
			& Options::handle_option_ ## option; \
		descoption ["--" #option]= \
			desc
	#define MAP_SPECIAL_OPTION(option, handler) \
		mapoption [option]= \
			& Options::handle_option_ ## handler

	MAP_SHORT_OPTION (1, debinfo1);
	MAP_SHORT_OPTION (8, w8080);
	MAP_SHORT_OPTION (B, bracket);
	MAP_SHORT_OPTION (E, equ);
	MAP_SHORT_OPTION (I, include-dir);
	MAP_SHORT_OPTION (S, skiplines);
	MAP_SHORT_OPTION (d, debinfo);
	MAP_SHORT_OPTION (h, help);
	MAP_SHORT_OPTION (m, module);
	MAP_SHORT_OPTION (o, output);
	MAP_SHORT_OPTION (v, verbose);

	MAP_LONG_OPTION (86,        "8086 code generation");
	MAP_LONG_OPTION (after,     "place common segments after absolute");
	MAP_LONG_OPTION (alocal,    "autolocal mode");
	MAP_LONG_OPTION (amsdos,    "amsdos binary output format");
	MAP_LONG_OPTION (asm,       "assembly type: 8080 or Z80 (default)");
	MAP_LONG_OPTION (base,      "base address for linkage");
	MAP_LONG_OPTION (bin,       "raw binary output format");
	MAP_LONG_OPTION (bracket,   "bracket only mode");
	MAP_LONG_OPTION (cdt,       "cdt output format");
	MAP_LONG_OPTION (cdtbas,    "cdt with basic output format");
	MAP_LONG_OPTION (cmd,       "cmd output format");
	MAP_LONG_OPTION (com,       "com output format");
	MAP_LONG_OPTION (debinfo,   "show debug information in second pass");
	MAP_LONG_OPTION (debinfo1,  "show debug information in both passes");
	MAP_LONG_OPTION (dump,      "readable hex dump output format");
	MAP_LONG_OPTION (equ,       "predefine a variable");
	MAP_LONG_OPTION (err,       "error messages to std output");
	MAP_LONG_OPTION (help,      "show this help");
	MAP_LONG_OPTION (hex,       "Intel HEX output format");
	MAP_LONG_NAME_OPTION (include_dir, include-dir,
		 "add directory to include search list");
	MAP_LONG_OPTION (input,     "file to assembly");
	MAP_LONG_OPTION (link,      "link only, no input assembly file"),
	MAP_LONG_OPTION (listing,   "listing output file");
	MAP_LONG_OPTION (module,    "link with module");
	MAP_LONG_OPTION (msx,       "BLOAD msx basic output format");
	MAP_LONG_OPTION (name,      "name to put in headers");
	MAP_LONG_OPTION (nocase,    "use case insensitive mode");
	MAP_LONG_OPTION (numerr,    "num of nonfatal errors allowed (default 1)");
	MAP_LONG_OPTION (output,    "output file name");
	MAP_LONG_OPTION (plus3dos,  "spectrum plus 3 disc file output format");
	MAP_LONG_OPTION (prl,       "cp/m PRL output format");
	MAP_LONG_OPTION (public,    "put only publics in symbol listing");
	MAP_LONG_OPTION (rel,       "REL relocatable ooutput format");
	MAP_LONG_OPTION (skiplines, "skip lines at begin of file");
	MAP_LONG_OPTION (tap,       "tap output format");
	MAP_LONG_OPTION (tapbas,    "tap with basic output format");
	MAP_LONG_OPTION (tzx,       "tzx output format");
	MAP_LONG_OPTION (tzxbas,    "tzx with basic output format");
	MAP_LONG_OPTION (verbose,   "show some information during assembly");
	MAP_LONG_OPTION (version,   "show pasmo version number");
	MAP_LONG_OPTION (w8080,     "warn if non-8080 instructions are used");

	MAP_SPECIAL_OPTION ("-", dash);
	MAP_SPECIAL_OPTION ("--", doubledash);

	#undef MAP_SPECIAL_OPTION
	#undef MAP_LONG_OPTION
	#undef MAP_SHORT_OPTION
}

Options::handle_option_t Options::gethandler (const string & optname)
{
	shortoption_t::iterator its= shortoption.find (optname);
	const string oplong= (its != shortoption.end () ) ? its->second : optname;

	mapoption_t::iterator it= mapoption.find (oplong);
	if (it != mapoption.end () )
		return it->second;
	else
	{
		if (! optname.empty () && optname [0] == '-')
			throw InvalidOption (optname);
		else
			return & Options::handle_option_endoptions;
	}
}

Options::Options (int argc_n, char * * argv_n) :
	argc (argc_n),
	argv (argv_n),
	end_options (false),
	linkonly (false),
	emitpublic (false)
{
	// Static initialized here, because is assumed that
	// only one instance will be used.
	initmap ();

	int argpos;
	for (argpos= 1; argpos < argc; ++argpos)
	{
		const string arg (argv [argpos] );
		(this->*gethandler (arg) ) (arg, argpos);
		if (end_options)
			break;
	}

	// File parameters.

	if (filein.empty () && ! linkonly)
	{
		if (argpos >= argc)
			throw Usage ();
		filein= argv [argpos];
		++argpos;
	}

	if (fileout.empty () )
	{
		if (argpos >= argc)
			throw Usage ();
		fileout= argv [argpos];
		++argpos;
	}

	if (argpos < argc)
	{
		filesymbol= argv [argpos];
		++argpos;

		if (! emitpublic && argpos < argc)
		{
			filepublic= argv [argpos];
			++argpos;
		}

		if (argpos < argc)
			cerr << "WARNING: Extra arguments ignored" << endl;
	}

	if (headername.empty () )
		headername= fileout;
}

inline void unused (const string &, int)
{
	// Avoid warning about unused parameters
}

inline void unused (const string &)
{
	// Avoid warning about unused parameters
}

#define DEFINE_HANDLER(option) \
	void Options::handle_option_ ## option \
		(const string & optname, int & argpos)

DEFINE_HANDLER (dash)
{
	unused (optname, argpos);
	end_options= true;
}

DEFINE_HANDLER (doubledash)
{
	unused (optname);
	++argpos;
	end_options= true;
}

DEFINE_HANDLER (endoptions)
{
	unused (optname, argpos);
	end_options= true;
}

DEFINE_HANDLER (86)
{
	unused (optname, argpos);
	mode86= true;
}

DEFINE_HANDLER (after)
{
	unused (optname, argpos);
	common_after_abs= true;
}

DEFINE_HANDLER (alocal)
{
	unused (optname, argpos);
	autolocal= true;
}

DEFINE_HANDLER (amsdos)
{
	unused (optname, argpos);
	//objecttype= ObjectAmsDos;
	setObjectType (ObjectAmsDos);
}

DEFINE_HANDLER (asm)
{
	if (++argpos >= argc)
		throw NeedArgument (optname);
	string type (argv [argpos] );
	if (type == "Z80")
		asmmode= AsmZ80;
	else if (type == "8080")
		asmmode= Asm8080;
	else throw InvalidArgument (optname, type);
}

DEFINE_HANDLER (base)
{
	if (++argpos >= argc)
		throw NeedArgument (optname);
	string base (argv [argpos] );
	char * aux;
	unsigned long numbase= strtoul (base.c_str (), & aux, 0);
	if (! aux || * aux != '\0')
		throw InvalidArgument (optname, base);
	setLinkBase (numbase);
}

DEFINE_HANDLER (bin)
{
	unused (optname, argpos);
	//objecttype= ObjectBin;
	setObjectType (ObjectBin);
}

DEFINE_HANDLER (bracket)
{
	unused (optname, argpos);
	bracketonly= true;
}

DEFINE_HANDLER (cdt)
{
	unused (optname, argpos);
	//objecttype= ObjectCdt;
	setObjectType (ObjectCdt);
}

DEFINE_HANDLER (cdtbas)
{
	unused (optname, argpos);
	//objecttype= ObjectCdtBas;
	setObjectType (ObjectCdtBas);
}

DEFINE_HANDLER (cmd)
{
	unused (optname, argpos);
	//objecttype= ObjectCmd;
	setObjectType (ObjectCmd);
}

DEFINE_HANDLER (com)
{
	unused (optname, argpos);
	//objecttype= ObjectCom;
	setObjectType (ObjectCom);
}

DEFINE_HANDLER (debinfo)
{
	unused (optname, argpos);
	debugtype= DebugSecondPass;
}

DEFINE_HANDLER (debinfo1)
{
	unused (optname, argpos);
	debugtype= DebugAll;
}

DEFINE_HANDLER (dump)
{
	unused (optname, argpos);
	//objecttype= ObjectDump;
	setObjectType (ObjectDump);
}

DEFINE_HANDLER (equ)
{
	if (++argpos >= argc)
		throw NeedArgument (optname);
	labelpredef.push_back (argv [argpos] );
}

DEFINE_HANDLER (err)
{
	unused (optname, argpos);
	redirecterr= true;
}

void showdescopt (Options::descoption_t::value_type & desc)
{
	//cout.width (14);
	//cout << std::left << desc.first << desc.second << endl;
	// Changed to work with older compilers.
	cout.width (14);
	cout.setf (ios::left);
	cout << desc.first << desc.second << endl;
}

void showlongshort (Options::shortoption_t::value_type & sop)
{
	//cout.width (6);
	//cout << std::left << sop.first << sop.second << endl;
	// Changed to work with older compilers.
	cout.width (6);
	cout.setf (ios::left);
	cout << sop.first << sop.second << endl;
}

DEFINE_HANDLER (help)
{
	unused (optname, argpos);
	cout << pasmoversion << endl <<
		"\tLong options with description:" << endl;
	for_each (descoption.begin (), descoption.end (), showdescopt);
	cout << "\tShort options correspondig to long:" << endl;
	for_each (shortoption.begin (), shortoption.end (), showlongshort);
	#ifdef PACKAGE_BUGREPORT
	cout << "Report bugs to " PACKAGE_BUGREPORT << endl;
	#endif
	throw DoNothing ();
}

DEFINE_HANDLER (hex)
{
	unused (optname, argpos);
	//objecttype= ObjectHex;
	setObjectType (ObjectHex);
}

DEFINE_HANDLER (include_dir)
{
	if (++argpos >= argc)
		throw NeedArgument (optname);
	includedir.push_back (argv [argpos] );
}

DEFINE_HANDLER (input)
{
	if (++argpos >= argc)
		throw NeedArgument (optname);
	if (linkonly)
		throw LinkButFile;
	filein= argv [argpos];
}

DEFINE_HANDLER (link)
{
	unused (optname, argpos);
	if (! filein.empty () )
		throw LinkButFile;
	linkonly= true;
}

DEFINE_HANDLER (listing)
{
	++argpos;
	if (argpos >= argc)
		throw NeedArgument (optname);
	filelisting = argv [argpos];
}

DEFINE_HANDLER (module)
{
	++argpos;
	if (argpos >= argc)
		throw NeedArgument (optname);

	module.push_back (argv [argpos] );
}

DEFINE_HANDLER (msx)
{
	unused (optname, argpos);
	//objecttype= ObjectMsx;
	setObjectType (ObjectMsx);
}

DEFINE_HANDLER (name)
{
	++argpos;
	if (argpos >= argc)
		throw NeedArgument (optname);
	headername= argv [argpos];
}

DEFINE_HANDLER (numerr)
{
	++argpos;
	if (argpos >= argc)
		throw NeedArgument (optname);
	numerr= strtoul (argv [argpos], 0, 0);
}

DEFINE_HANDLER (nocase)
{
	unused (optname, argpos);
	nocase= true;
}

DEFINE_HANDLER (output)
{
	if (++argpos >= argc)
		throw NeedArgument (optname);
	fileout= argv [argpos];
}

DEFINE_HANDLER (plus3dos)
{
	unused (optname, argpos);
	//objecttype= ObjectPlus3Dos;
	setObjectType (ObjectPlus3Dos);
}

DEFINE_HANDLER (prl)
{
	unused (optname, argpos);
	//objecttype= ObjectPrl;
	setObjectType (ObjectPrl);
}

DEFINE_HANDLER (public)
{
	unused (optname, argpos);
	emitpublic= true;
}

DEFINE_HANDLER (rel)
{
	unused (optname, argpos);
	//objecttype= ObjectRel;
	setObjectType (ObjectRel);
}


DEFINE_HANDLER (skiplines)
{
	if (++argpos >= argc)
		throw NeedArgument (optname);
	lines_to_skip= strtoul (argv [argpos], 0, 0);
}

DEFINE_HANDLER (tap)
{
	unused (optname, argpos);
	//objecttype= ObjectTap;
	setObjectType (ObjectTap);
}

DEFINE_HANDLER (tapbas)
{
	unused (optname, argpos);
	//objecttype= ObjectTapBas;
	setObjectType (ObjectTapBas);
}

DEFINE_HANDLER (tzx)
{
	unused (optname, argpos);
	//objecttype= ObjectTzx;
	setObjectType (ObjectTzx);
}

DEFINE_HANDLER (tzxbas)
{
	unused (optname, argpos);
	//objecttype= ObjectTzxBas;
	setObjectType (ObjectTzxBas);
}

DEFINE_HANDLER (verbose)
{
	unused (optname, argpos);
	verbose= true;
}

DEFINE_HANDLER (version)
{
	unused (optname, argpos);
	cout << pasmoversion << endl <<
		"Copyright (C) 2004-2006 Julian Albo\n"
		"Pasmo comes with NO WARRANTY,\n"
		"to the extent permitted by law.\n"
		"You may redistribute copies of Pasmo\n"
		"under the terms of the GNU General Public License.\n"
		"For more information about these matters,\n"
		"see the files named COPYING." << endl;
	throw DoNothing ();
}

DEFINE_HANDLER (w8080)
{
	unused (optname, argpos);
	warn8080= true;
}

#undef DEFINE_HANDLER

string Options::getfilepublic () const
{
	if (emitpublic)
		return filesymbol;
	else
		return filepublic;
}

string Options::getfilelisting () const
{
	return filelisting;
}

void Options::apply (Asm & assembler) const
{
	for (size_t i= 0; i < includedir.size (); ++i)
		assembler.addincludedir (includedir [i] );

	for (size_t i= 0; i < labelpredef.size (); ++i)
		assembler.addpredef (labelpredef [i] );
}


int doit (int argc, char * * argv, ostream * & perr)
{
	TRFUNC (tr, "doit");

	// Process command line options.

	Options option (argc, argv);

	if (option.redirecterr)
		perr= & cout;

	// Assemble.

	auto_ptr <Asm> pass (Asm::create (option) );
	Asm & assembler= * pass.get ();

	option.apply (assembler);

	string filelisting= option.getfilelisting ();
	ofstream lout;
	if (! filelisting.empty () )
	{
		lout.open (filelisting.c_str () );
		if (! lout.is_open () )
			throw ListingFileError;
		assembler.setfilelisting (lout);
	}

	const string filein= option.getfilein ();

	if (! filein.empty () )
	{
		assembler.loadfile (filein);
		assembler.processfile ();
	}
	else
	{
		assembler.link ();
	}

	// Generate ouptut file.


	string fileout= option.getfileout ();
	ofstream fout;
	ostream out (cout.rdbuf () );
	if (fileout != "-")
	{
		fout.open (fileout.c_str (), ios::out | ios::binary);
		if (! fout.is_open () )
			throw CreateObjectError;
		out.rdbuf (fout.rdbuf () );
	}

	assembler.emitcode (out);

	fout.close ();

	// Generate symbol table and public symbol table if required.

	string filesymbol= option.getfilesymbol ();
	if (! option.publiconly () && ! filesymbol.empty () )
	{
		ofstream sout;
		streambuf * cout_buf= 0;
		if (filesymbol != "-")
		{
			sout.open (filesymbol.c_str () );
			if (! sout.is_open () )
				throw SymbolFileError;
			cout_buf= cout.rdbuf ();
			cout.rdbuf (sout.rdbuf () );
		}
		assembler.dumpsymbol (cout);
		if (cout_buf)
		{
			cout.rdbuf (cout_buf);
			sout.close ();
		}
	}

	string filepublic= option.getfilepublic ();
	if (! filepublic.empty () )
	{
		ofstream sout;
		streambuf * cout_buf= 0;
		if (filepublic != "-")
		{
			sout.open (filepublic.c_str () );
			if (! sout.is_open () )
				throw PublicFileError;
			cout_buf= cout.rdbuf ();
			cout.rdbuf (sout.rdbuf () );
		}
		assembler.dumppublic (cout);
		if (cout_buf)
		{
			cout.rdbuf (cout_buf);
			sout.close ();
		}
	}

	return 0;
}

void use_message ()
{
	cerr << "Usage:\n"
		"\tpasmo [options] source object [symbol]\n"
		"Use --help to see a short description of options.\n"
		"See the documentation file for details.\n";
}

} // namespace main
} // namespace pasmo


int main (int argc, char * * argv)
{
	using namespace pasmo::main;

	TRFUNC (tr, "main");

	// Just call doit and show possible errors.

	ostream * perr= & cerr;
	try
	{
		return doit (argc, argv, perr);
	}
	catch (DoNothing &)
	{
		return 0;
	}
	catch (Usage &)
	{
		cerr <<	pasmoversion <<
			" (C) 2004-2005 Julian Albo\n\n";
		use_message ();
	}
	catch (ErrorInOptions & e)
	{
		cerr << "ERROR: " << e.what () << endl;
		use_message ();
	}
	catch (pasmo::error_already_reported &)
	{
	}
	catch (logic_error & e)
	{
		* perr << "ERROR: " << e.what () << endl <<
			"This error is unexpected, please "
			"send a bug report." << endl;
	}
	catch (exception & e)
	{
		* perr << "ERROR: " << e.what () << endl;
	}
	catch (...)
	{
		cerr << "ERROR: Unexpected exception.\n"
			"Please send a bug report.\n";
	}
	return 1;
}

// End of pasmo.cpp
