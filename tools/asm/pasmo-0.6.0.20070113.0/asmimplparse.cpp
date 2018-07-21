// asmimplparse.cpp
// Revision 26-aug-2005

#include "asmimpl.h"
#include "parsertypes.h"
#include "trace.h"

#include <memory>

namespace pasmo {
namespace impl {

using std::endl;
using std::auto_ptr;
using std::exception;


#if 0
void AsmImpl::parseinstruction (Tokenizer & tz)
{
	TRFUNC (tr, "AsmImpl::parseinstruction");

	ParseType type= bracketonlymode ? BracketOnly : PassedFirst;
	bool inp2= pass > 1;
	auto_ptr <Machine> pmachine (newMachine (* this, tz, type, inp2) );

	try
	{
		TRMESSAGE (tr, "Parse");
		if (yyparse (* pmachine) != 0)
			throw runtime_error ("Syntax error");

		TRMESSAGE (tr, "Exec");
		pmachine->exec ();
	}
	catch (nonfatal_error & e)
	{
		TRMESSAGE (tr, "catch nonfatal");
		showcurrentlineinfo (* perr);
		* perr << ' ' << e.what () << endl;
		* pverb << ">>" << tz << "<<" << endl;
		if (++counterr == numerr)
			throw error_already_reported ();
	}
	catch (exception & e)
	{
		showcurrentlineinfo (* perr);
		* perr << ' ' << e.what () << endl;
		* pverb << ">>" << tz << "<<" << endl;
		throw error_already_reported ();
	}
}
#endif


} // namespace impl
} // namespace pasmo

// End of asmimplparse.cpp
