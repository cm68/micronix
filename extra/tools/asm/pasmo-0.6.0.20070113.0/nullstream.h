#ifndef INCLUDE_NULLSTREAM_H
#define INCLUDE_NULLSTREAM_H

// nullstream.h
// Revision 9-aug-2005


#include <iostream>


namespace pasmo {
namespace impl {


class Nullbuff : public std::streambuf
{
public:
	Nullbuff ();
protected:
	int overflow (int);
	int sync ();
};

class Nullostream : public std::ostream
{
public:
	Nullostream ();
private:
	Nullbuff buff;
};


} // namespace impl
} // namespace pasmo


#endif

// End of nullstream.h
