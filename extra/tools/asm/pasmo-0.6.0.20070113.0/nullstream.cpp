// nullstream.cpp
// Revision 9-aug-2005


#include "nullstream.h"


namespace pasmo {
namespace impl {


Nullbuff::Nullbuff ()
{
	setbuf (0, 0);
}

int Nullbuff::overflow (int)
{
	setp (pbase (), epptr () );
	return 0;
}

int Nullbuff::sync ()
{
	return 0;
}


Nullostream::Nullostream () :
	std::ostream (& buff)
{ }


} // namespace impl
} // namespace pasmo

// End of nullstream.cpp
