#ifndef INCLUDE_SPECTRUM_H
#define INCLUDE_SPECTRUM_H

// spectrum.h
// Revision 11-aug-2005

#include "pasmotypes.h"


namespace pasmo {
namespace spectrum {


class Plus3Head {
public:
	Plus3Head ();
	void setsize (address size);
	void setstart (address start);
	void write (std::ostream & out);
private:
	static const size_t headsize= 128;
	byte plus3 [headsize];
};


// Spectrum Basic generation.


extern const string tokNumPrefix;
extern const string tokEndLine;
extern const string tokCODE;
extern const string tokUSR;
extern const string tokLOAD;
extern const string tokPOKE;
extern const string tokRANDOMIZE;
extern const string tokCLEAR;

string number (address n);
string linenumber (address n);
string linelength (address n);
string basicline (address linenum, const string & line);


} // namespace spectrum
} // namespace pasmo

#endif

// End of spectrum.h
