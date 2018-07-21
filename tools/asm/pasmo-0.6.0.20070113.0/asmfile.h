#ifndef INCLUDE_ASMFILE_H
#define INCLUDE_ASMFILE_H

// asmfile.h
// Revision 29-aug-2005

#include "token.h"

#include <iostream>
#include <fstream>


namespace pasmo {
namespace impl {


using std::ios;
using std::ostream;
using std::ifstream;


class AsmFile {
public:
	AsmFile ();
	AsmFile (const AsmFile & af);
	~AsmFile ();
	void addincludedir (const string & dirname);
	void loadfile (const string & filename,
		ostream & outverb, ostream & outerr);
	size_t getline () const;
protected:
	void openis (ifstream & is, const string & filename,
		ios::openmode mode) const;
	void showlineinfo (ostream & os, size_t nline) const;
	void showcurrentlineinfo (ostream & os) const;
	void showline (ostream & os, size_t nline) const;
	void showcurrentline (ostream & os) const;
	bool getvalidline ();
	bool passeof () const;
	const Tokenizer getcurrentline (AsmMode asmmode);
	const string & getcurrenttext () const;

	void setline (size_t line);
	void beginline ();
	bool nextline ();
	void prevline ();
private:
	class In;
	In * pin;
	In & in ();
	const In & in () const;

	size_t currentline;
};


} // namespace impl
} // namespace pasmo


#endif

// End of asmfile.h
