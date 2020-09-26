// segment.cpp
// Revision 11-oct-2006


#include "segment.h"

#include "trace.h"


#include <sstream>
#include <algorithm>

using std::fill_n;


namespace pasmo {
namespace impl {


//*********************************************************
//			class Segment
//*********************************************************


const size_t Segment::max_seg_size;


Segment::Segment () :
	minused (65535),
	maxused (0),
	curpos (0),
	base (0),
	size (0),
	org_addr (65535),
	has_org (false)
{
	TRFUNC (tr, "Segment::Segment");

	fill_n (mem, 0, max_seg_size);
}

void Segment::clear ()
{
	if (! empty () )
	{
		size_t s= maxused - minused + 1;
		fill_n (mem + minused, 0, s);
	}
	minused= 65535;
	maxused= 0;
	curpos= 0;
	base= 0;
	size= 0;
}

bool Segment::empty () const
{
	return minused > maxused;
}

void Segment::setbase (address base_n)
{
	base= base_n;
}

void Segment::setsize (address size_n)
{
	size= size_n;
}

address Segment::getsize () const
{
	return size;
}

address Segment::getminused () const
{
	return minused;
}

address Segment::getmaxused () const
{
	return maxused;
}

address Segment::getbase () const
{
	return base;
}

void Segment::setbyte (address pos, byte b)
{
	TRFUNC (tr, "Segment::setbyte");
	TRSTREAM (tr, "At " << hex4 (pos) << " value " << hex2 (b) );

	mem [pos]= b;
	if (pos < minused)
		minused= pos;
	if (pos > maxused)
		maxused= pos;

	TRSTREAM (tr, "Min: " << hex4 (minused) <<
		" max: " << hex4 (maxused) );
}

void Segment::setword (address pos, address value)
{
	TRFUNC (tr, "Segment::setword");
	TRSTREAM (tr, "At " << hex4 (pos) << " value " << hex4 (value) );

	#if 0

	ASSERT (pos < max_seg_size - 1);

	mem [pos + 0]= lobyte (value);
	mem [pos + 1]= hibyte (value);
	if (pos < minused)
		minused= pos;
	++pos;
	if (pos  > maxused)
		maxused= pos;

	#else

	setbyte (pos, lobyte (value) );
	++pos;
	setbyte (pos, hibyte (value) );

	#endif
}

void Segment::offsetword (address pos, address value)
{
	TRFUNC (tr, "Segment::offsetword");
	TRSTREAM (tr, "At " << hex4 (pos) << " value " << hex4 (value) );

	value+= getword (pos);
	mem [pos + 0]= lobyte (value);
	mem [pos + 1]= hibyte (value);
}

byte Segment::getbyte (address pos) const
{
	return mem [pos];
}

address Segment::getword (address pos) const
{
	ASSERT (pos < max_seg_size - 1);

	return mem [pos] + static_cast <address> (mem [pos + 1]) * 256;
}

void Segment::setpos (address pos)
{
	curpos= pos;
}

void Segment::setorg (address pos)
{
	curpos= pos;
	if (has_org)
	{
		if (pos < org_addr)
			org_addr= pos;
	}
	else
	{
		has_org= true;
		org_addr= pos;
	}
}

address Segment::getpos () const
{
	return curpos;
}

address Segment::getorg () const
{
	return org_addr;
}

bool Segment::hasorg () const
{
	return has_org;
}

void Segment::putbyte (byte b)
{
	setbyte (curpos++, b);
}

void Segment::putword (address value)
{
	//setword (curpos, value);
	//curpos+= 2;
	setbyte (curpos++, lobyte (value) );
	setbyte (curpos++, hibyte (value) );
}

void Segment::evallimits ()
{
	TRFUNC (tr, "Segment::evallimits");

	ASSERT (size == 0);

	if (! empty () )
	{
		size= maxused;
		++size;
	}
}

Segment::byte_iterator Segment::iter (address pos)
{
	return mem + pos;
}

Segment::byte_const_iterator Segment::iter (address pos) const
{
	return mem + pos;
}

void Segment::write (ostream & out, address pos, size_t size)
{
	TRFUNC (tr, "Segment::write");
	TRSTREAM (tr, "pos: " << hex4 (pos) << ", size: " << hex4 (size) );

	//ASSERT (pos < max_seg_size - size);
	ASSERT (pos <= max_seg_size - size);

	out.write (reinterpret_cast <char *> (mem + pos), size);
}

void Segment::copy (Segment & dest)
{
	if (! empty () )
	{
		#if 0
		address realsize= size - minused;
		#else
		size_t realsize= size - minused;
		#endif
		for (address i= minused; realsize > 0; --realsize, ++i)
		{
			dest.setbyte (base + i, mem [i] );
		}
	}
}


} // namespace impl
} // namespace pasmo


// End of segment.cpp
