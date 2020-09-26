// asmfile.cpp
// Revision 8-oct-2005

#include "asmfile.h"
#include "parser.h"

#include "trace.h"


#include <sstream>
#include <vector>
#include <deque>
#include <algorithm>
#include <stdexcept>


namespace pasmo {
namespace impl {
namespace asmfile {


using std::ios;
using std::istream;
using std::ostream;
using std::cin;
using std::endl;
using std::ifstream;
using std::vector;
using std::deque;
using std::find_if;
using std::runtime_error;

using namespace pasmo::impl;


class FileNotFound : public runtime_error {
public:
	FileNotFound (const string & filename) :
		runtime_error ("File '" + filename + "' not found")
	{ }
};

class InvalidInclude : public runtime_error {
public:
	InvalidInclude (const Token & tok) :
		runtime_error ("Unexpected " + tok.str () +
			" after INCLUDE file name")
	{ }
};


class FileLine {
public:
	FileLine (const string & text_n);

	bool empty () const;
	const string & getstrline () const;
	static string striplinenumber (const string & text);
private:
	string text;
	size_t linenum;
};


string FileLine::striplinenumber (const string & text)
{
	string::size_type pos= text.find_first_not_of ("0123456789");
	switch (pos)
	{
	case string::npos:
		return string ();
	case 0:
		return text;
	default:
		return text.substr (pos);
	}
}

FileLine::FileLine (const string & text_n) :
	text (striplinenumber (text_n) )
{
}

bool FileLine::empty () const
{
	return text.empty ();
}

const string & FileLine::getstrline () const
{
	return text;
	//return tkz.gettextline ();
}



typedef deque <FileLine> filelines_t;


class FileRef {
public:
	FileRef (const string & name, size_t linebeg);

	void load (istream & is);

	void setend (size_t n);

	size_t linebegin () const;
	size_t lineend () const;

	size_t numlines () const;
	size_t lastline () const;

	string name () const;

	bool lineempty (size_t n) const;
	//Tokenizer gettkz (size_t n);
	const string & getstrline (size_t n) const;

	void pushline (const string & text);
private:
	string filename;
	bool notloaded;
	filelines_t line;
	size_t l_begin;
	size_t l_end;
};

FileRef::FileRef (const string & name, size_t linebeg) :
	filename (name),
	notloaded (true),
	l_begin (linebeg)
{ }

void FileRef::load (istream & input)
{
	string line;
	while (std::getline (input, line) )
	{
		pushline (line);
	}
}

void FileRef::setend (size_t n)
{
	l_end= n;
}

size_t FileRef::linebegin () const
{
	return l_begin;
}

size_t FileRef::lineend () const
{
	return l_end;
}

size_t FileRef::numlines () const
{
	return line.size ();
}

size_t FileRef::lastline () const
{
	return line.size () - 1;
}

string FileRef::name () const
{
	return filename == "-" ? string ("(standard input)") : filename;
}

bool FileRef::lineempty (size_t n) const
{
	ASSERT (n < line.size () );

	return line [n].empty ();
}

#if 0
Tokenizer FileRef::gettkz (size_t n)
{
	ASSERT (n < line.size () );

	//return line [n].gettkz ();
	return Tokenizer (line [n].getstrline () );
}
#endif

const string & FileRef::getstrline (size_t n) const
{
	TRFUNC (tr, "FileRef::getstrline");

	ASSERT (n < line.size () );

	return line [n].getstrline ();
}

void FileRef::pushline (const string & text)
{
	TRFUNC (tr, "FileRef::pushline");
	line.push_back (FileLine (text) );
}


class RefNameIs {
public:
	RefNameIs (const string & name_n) : name (name_n) { }
	bool operator () (const FileRef & fr) { return fr.name () == name; }
private:
	const string name;
};


class FileContainer {
public:
	static const size_t NotFound= size_t (-1);
	size_t size () const;
	const FileRef & operator [] (size_t n) const;
	FileRef & operator [] (size_t n);
	size_t push_back (const FileRef & fr);
	FileRef & back ();
	size_t find (const string & name) const;
private:
	typedef deque <FileRef> Container;
	Container container;
};


size_t FileContainer::size () const
{
	return container.size ();
}

const FileRef & FileContainer::operator [] (size_t n) const
{
	ASSERT (n < size () );
	return container [n];
}


FileRef & FileContainer::operator [] (size_t n)
{
	ASSERT (n < size () );
	return container [n];
}


size_t FileContainer::push_back (const FileRef & fr)
{
	container.push_back (fr);
	return container.size () - 1;
}


FileRef & FileContainer::back ()
{
	ASSERT (! container.empty () );
	return container.back ();
}

size_t FileContainer::find (const string & name) const
{
	Container::const_iterator it=
		find_if (container.begin (), container.end (), RefNameIs (name) );
	if (it == container.end () )
		return NotFound;
	else
	{
		return std::distance (container.begin (), it);
	}
}


class LineContent {
public:
	LineContent (size_t linenumn, size_t filenumn);
	LineContent (size_t linenumn, size_t filenumn,
		const Tokenizer & tz_n);
	size_t getfileline () const;
	size_t getfilenum () const;
	const Tokenizer & gettkz () const;
	bool hastoken () const;
	bool empty () const;
private:
	size_t filenum;
	size_t linenum;
	bool hastokenizer;
	Tokenizer tz;
};

LineContent::LineContent (size_t filenumn, size_t linenumn) :
	filenum (filenumn),
	linenum (linenumn),
	hastokenizer (false)
{ }

LineContent::LineContent (size_t filenumn, size_t linenumn,
		const Tokenizer & tz_n) :
	filenum (filenumn),
	linenum (linenumn),
	hastokenizer (true),
	tz (tz_n)
{ }

size_t LineContent::getfileline () const
{
	return linenum;
}

size_t LineContent::getfilenum () const
{
	return filenum;
}

const Tokenizer & LineContent::gettkz () const
{
	return tz;
}

bool LineContent::hastoken () const
{
	return hastokenizer;
}

bool LineContent::empty () const
{
	return tz.empty ();
}


class LineContainer {
public:
	size_t size () const;
	const LineContent & operator [] (size_t n) const;
	LineContent & operator [] (size_t n);

	void push_back (size_t filenum, size_t fileline);
	void push_back (size_t filenum, size_t fileline, const Tokenizer & tz);

	bool lineempty (size_t n) const;
private:
	typedef deque <LineContent> Container;
	Container container;
};


size_t LineContainer::size () const
{
	return container.size ();
}


const LineContent & LineContainer::operator [] (size_t n) const
{
	ASSERT (n < container.size () );
	return container [n];
}

LineContent & LineContainer::operator [] (size_t n)
{
	ASSERT (n < container.size () );
	return container [n];
}

void LineContainer::push_back (size_t filenum, size_t fileline)
{
	container.push_back (LineContent (filenum, fileline) );
}

void LineContainer::push_back (size_t filenum, size_t fileline,
	const Tokenizer & tz)
{
	container.push_back (LineContent (filenum, fileline, tz) );
}

bool LineContainer::lineempty (size_t n) const
{
	ASSERT (n < container.size () );
	return container [n].empty ();
}

} // namespace asmfile


using namespace asmfile;


class AsmFile::In {
public:
	In ();
	~In ();
	void addref ();
	void delref ();

	size_t numlines () const;
	size_t numfiles () const;

	bool lineempty (size_t n) const;
	const Tokenizer gettkz (size_t n, AsmMode asmmode);
	const string & getstrline (size_t n) const;

	void addincludedir (const string & dirname);
	bool openis (ifstream & is, const string & filename,
		ios::openmode mode) const;
	//void copyfile (FileRef & fr, ostream & outverb);
	bool openfile (const string & Filename,
		istream & input, ifstream & ifile,
		ostream & outverb);

	void includefile (Tokenizer & tz,
		size_t filenum, size_t fileline,
		 ostream & outverb, ostream & outerr);

	//void loadfile (istream & input, size_t filenum, bool nocase,
	//	ostream & outverb, ostream & outerr);

	void processfile (size_t filenum,
		ostream & outverb, ostream & outerr);
	void processfile (ostream & outverb, ostream & outerr);
	bool loadfile (const string & filename,
		ostream & outverb, ostream & outerr);
	void showlineinfo (ostream & os, size_t nline) const;
	void showline (ostream & os, size_t nline) const;
private:
	In (const In &); // Forbidden.
	In & operator = (const In &); // Forbidden.

	const FileRef & getfile (size_t n) const;
	FileRef & getfile (size_t n);

	const LineContent & getline (size_t n) const;
	LineContent & getline (size_t n);

	size_t numrefs;

	LineContainer vlinecont;

	FileContainer vfileref;

	// ******** Paths for include ************

	vector <string> includepath;
};

AsmFile::In::In ()
{
	TRFUNC (tr, "AsmFile::In::In");

	numrefs= 1;
}

AsmFile::In::~In ()
{
	TRFUNC (tr, "AsmFile::In::~In");

	numrefs= 1;
}

void AsmFile::In::addref ()
{
	++numrefs;
}

void AsmFile::In::delref ()
{
	--numrefs;
	if (numrefs == 0)
		delete this;
}

size_t AsmFile::In::numlines () const
{
	return vlinecont.size ();
}

size_t AsmFile::In::numfiles () const
{
	return vfileref.size ();
}

const FileRef & AsmFile::In::getfile (size_t n) const
{
	ASSERT (n < numfiles () );
	return vfileref [n];
}

FileRef & AsmFile::In::getfile (size_t n)
{
	ASSERT (n < numfiles () );
	return vfileref [n];
}

const LineContent & AsmFile::In::getline (size_t n) const
{
	ASSERT (n < numlines () );
	return vlinecont [n];
}

LineContent & AsmFile::In::getline (size_t n)
{
	ASSERT (n < numlines () );
	return vlinecont [n];
}

bool AsmFile::In::lineempty (size_t n) const
{
	ASSERT (n < numlines () );

	//return vlinecont [n].empty ();

	//const LineContent & lc= getline (n);
	//return getfile (lc.getfilenum () ).lineempty (lc.getfileline () );

	return vlinecont.lineempty (n);
	//return false;
}

const Tokenizer AsmFile::In::gettkz (size_t n, AsmMode asmmode)
{
	TRFUNC (tr, "AsmFile::In::gettkz");

	ASSERT (n < numlines () );

	//return vlinecont [n].gettkz ();

	LineContent & lc= getline (n);

	//return getfile (lc.getfilenum () ).gettkz (lc.getfileline () );
	if (lc.hastoken () )
		return lc.gettkz ();
	else
		return Tokenizer (getstrline (n), asmmode);
}

const string & AsmFile::In::getstrline (size_t n) const
{
	ASSERT (n < numlines () );

	//return vlinecont [n].getstrline ();
	const LineContent & lc= getline (n);
	return getfile (lc.getfilenum () ).getstrline (lc.getfileline () );
}

void AsmFile::In::addincludedir (const string & dirname)
{
	string aux (dirname);
	string::size_type l= aux.size ();
	if (l == 0)
		return;
	char c= aux [l - 1];
	if (c != '\\' && c != '/')
		aux+= '/';
	includepath.push_back (aux);
}

bool AsmFile::In::openis (ifstream & is, const string & filename,
	ios::openmode mode) const
{
	ASSERT (! is.is_open () );
	is.open (filename.c_str (), mode);
	if (is.is_open () )
		return true;
	for (size_t i= 0; i < includepath.size (); ++i)
	{
		string path (includepath [i] );
		path+= filename;
		is.clear ();
		is.open (path.c_str (), mode);
		if (is.is_open () )
			return true;
	}
	//throw FileNotFound (filename);
	return false;
}

#if 0
void AsmFile::In::copyfile (FileRef & fr, ostream & outverb)
{
	outverb << "Reloading file: " << fr.name () <<
		" in " << numlines () << endl;

	const size_t linebegin= fr.linebegin ();
	const size_t lineend= fr.lineend ();

	for (size_t i= linebegin; i < lineend; ++i)
	{
		//LineContent l= getline (i);
		//vlinecont.push_back (l);
		//vlinecontent.push_back (fr, i);
	}

	outverb << "Finished reloading file: " << fr.name () <<
		" in " << numlines () << endl;
}
#endif

bool AsmFile::In::openfile (const string & filename,
	istream & input, ifstream & ifile,
	ostream & outverb)
{
	TRFUNC (tr, "AsmFile::In::openfile");

	bool isstdin= filename == "-";

	if (! isstdin)
	{
		for (size_t i= 0; i < vfileref.size (); ++i)
		{
			if (vfileref [i].name () == filename)
			{
				//copyfile (vfileref [i], outverb);
				return true;
			}
		}
	}

	// Load the file in memory.

	outverb << "Loading file: " << filename <<
		" in " << numlines () << endl;

	//istream input (cin.rdbuf () );
	//ifstream ifile;
	if (! isstdin)
	{
		if (! openis (ifile, filename, ios::in) )
		{
			outverb << "Can't open" << endl;
			return false;
		}
		input.rdbuf (ifile.rdbuf () );
	}
	return true;
}

void AsmFile::In::includefile (Tokenizer & tz,
	size_t filenum, size_t fileline,
	ostream & outverb, ostream & outerr)
{
	TRFUNC (tr, "AsmFile::In::includefile");

	Token tokfile= tz.getincludefile ();
	Token tok= tz.gettoken ();
	if (tok.type () != TypeEndLine)
		throw InvalidInclude (tok);
	const string filename= tokfile.str ();

	if (! loadfile (filename, outverb, outerr) )
	{
		Tokenizer tzaux;
		tzaux.push_back (Token (TypeNoFileINCLUDE) );
		tzaux.push_back (tokfile);
		vlinecont.push_back (filenum, fileline, tzaux);
	}
	else
	{
		size_t filenum= vfileref.find (filename);
		ASSERT (filenum != FileContainer::NotFound);
		processfile (filenum, outverb, outerr);
	}
}

void AsmFile::In::processfile (size_t filenum,
	ostream & outverb, ostream & outerr)
{
	TRFUNC (tr, "AsmFile::In::processfile");

	const FileRef & fr= vfileref [filenum];
	const size_t numlines= fr.numlines ();
	for (size_t i= 0; i < numlines; ++i)
	{
		TRSTREAM (tr, "line " << i);
		vlinecont.push_back (filenum, i);
		string line= fr.getstrline (i);
		Tokenizer tz (line, AsmZ80); // Type unimportant here
		Token tok= tz.gettoken ();
		if (tok.type () == TypeINCLUDE)
		{
			includefile (tz, filenum, i, outverb, outerr);
		}
	}
}

void AsmFile::In::processfile (ostream & outverb, ostream & outerr)
{
	processfile (0, outverb, outerr);
}

bool AsmFile::In::loadfile (const string & filename,
	ostream & outverb, ostream & outerr)
{
	TRFUNC (tr, "AsmFile::In::loadfile filename");
	TRMESSAGE (tr, filename);

	size_t filenum= vfileref.find (filename);

	if (filenum == FileContainer::NotFound)
	{
		outverb << "Loading " << filename << endl;
		istream input (cin.rdbuf () );
		ifstream ifile;
		bool good= openfile (filename, input, ifile, outverb);
		if (! good)
		{
			outerr << "File not found" << endl;
			return false;
		}

		filenum= vfileref.push_back
			(FileRef (filename, numlines () ) );
		FileRef & fr= vfileref [filenum];
		fr.load (input);
	}

	return true;
}

void AsmFile::In::showlineinfo (ostream & os, size_t nline) const
{
	ASSERT (nline < numlines () );

	const LineContent & linf= getline (nline);
	const FileRef & fileref= getfile (linf.getfilenum () );

	//const size_t numline= fileref.numline (linf.getfileline () );
	const size_t numline= linf.getfileline ();

	/*
	os << " on line " << numline + 1 <<
		" of file " << fileref.name () << endl <<
		">>" << fileref.getstrline (numline) << "<<" << endl;
	*/
	os << fileref.name () << ':' << numline + 1;
}

void AsmFile::In::showline (ostream & os, size_t nline) const
{
	ASSERT (nline < numlines () );

	const LineContent & linf= getline (nline);
	const FileRef & fileref= getfile (linf.getfilenum () );

	//const size_t numline= fileref.numline (linf.getfileline () );
	const size_t numline= linf.getfileline ();

	os << ">>" << fileref.getstrline (numline) << "<<" << endl;
}

//*******************************************************************

namespace {

const size_t LINE_BEGIN= static_cast <size_t> (-1);

} // namespace

AsmFile::AsmFile () :
	pin (new In)
{
	TRFUNC (tr, "AsmFile::AsmFile");
}

AsmFile::AsmFile (const AsmFile & af) :
	pin (af.pin)
{
	TRFUNC (tr, "AsmFile::AsmFile");

	pin->addref ();
}

AsmFile::~AsmFile ()
{
	TRFUNC (tr, "AsmFile::~AsmFile");

	pin->delref ();
}

// These functions are for propagate constness to the internal class.

inline AsmFile::In & AsmFile::in ()
{
	return * pin;
}

inline const AsmFile::In & AsmFile::in () const
{
	return * pin;
}

void AsmFile::addincludedir (const string & dirname)
{
	in ().addincludedir (dirname);
}

void AsmFile::openis (ifstream & is, const string & filename,
	ios::openmode mode) const
{
	if (! in ().openis (is, filename, mode) )
		throw FileNotFound (filename);

}

void AsmFile::loadfile (const string & filename,
	ostream & outverb, ostream & outerr)
{
	TRFUNC (tr, "AsmFile::loadfile");

	if (! in ().loadfile (filename, outverb, outerr) )
		throw FileNotFound (filename);
	in ().processfile (outverb, outerr);
}

bool AsmFile::getvalidline ()
{
	TRFUNC (tr, "AsmFile::getvalidline");

	for (;;)
	{
		if (currentline >= in ().numlines () )
			return false;

		//if (! in ().getlinecont (currentline).empty () )
		//if (! in ().lineempty (currentline) )
		//	return true;
		//++currentline;
		return true;
	}
}

bool AsmFile::passeof () const
{
	return currentline >= in ().numlines ();
}

size_t AsmFile::getline () const
{
	return currentline;
}

const Tokenizer AsmFile::getcurrentline (AsmMode asmmode)
{
	TRFUNC (tr, "AsmFile::getcurrentline");

	ASSERT (! passeof () );

	//Tokenizer & tz= in ().getlinecont (currentline).gettkz ();

	//Tokenizer & tz= in ().gettkz (currentline);
	//tz.reset ();
	//return tz;

	return in ().gettkz (currentline, asmmode);
}

const string & AsmFile::getcurrenttext () const
{
	ASSERT (! passeof () );
	//return in ().getlinecont (currentline).getstrline ();
	return in ().getstrline (currentline);
}

void AsmFile::setline (size_t line)
{
	currentline= line;
}

void AsmFile::beginline ()
{
	TRFUNC (tr, "AsmFile::beginline");

	currentline= LINE_BEGIN;
}

bool AsmFile::nextline ()
{
	TRFUNC (tr, "AsmFile::nextline");

	if (currentline == LINE_BEGIN)
		currentline= 0;
	else
	{
		if (passeof () )
			return false;
		++currentline;
	}
	if (! getvalidline () )
		return false;
	return true;
}

void AsmFile::prevline ()
{
	ASSERT (currentline > 0);
	--currentline;
}

void AsmFile::showlineinfo (ostream & os, size_t nline) const
{
	in ().showlineinfo (os, nline);
}

void AsmFile::showcurrentlineinfo (ostream & os) const
{
	if (passeof () )
	{
		//os << " detected after end of file";
		os << ':';
	}
	else
		in ().showlineinfo (os, getline () );
}

void AsmFile::showline (ostream & os, size_t nline) const
{
	in ().showline (os, nline);
}

void AsmFile::showcurrentline  (ostream & os) const
{
	if (! passeof () )
		in ().showline (os, getline () );
}

} // namespace impl
} // namespace pasmo


// End of asmfile.cpp
