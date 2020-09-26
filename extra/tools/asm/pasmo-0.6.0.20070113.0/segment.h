#ifndef INCLUDE_SEGMENT_H
#define INCLUDE_SEGMENT_H

// segment.h
// Revision 11-oct-2006


#include "value.h"


//*********************************************************
//			class Segment
//*********************************************************


namespace pasmo {
namespace impl {


class Segment {
public:
	Segment ();

	void clear ();

	bool empty () const;
	void setbase (address base_n);
	void setsize (address size_n);
	address getbase () const;
	address getsize () const;
	address getminused () const;
	address getmaxused () const;

	void setbyte (address pos, byte b);
	void setword (address pos, address value);
	void offsetword (address pos, address value);
	byte getbyte (address pos) const;
	address getword (address pos) const;

	void setpos (address pos);
	void setorg (address pos);
	address getpos () const;
	bool hasorg () const;
	address getorg () const;
	void putbyte (byte b);
	void putword (address value);

	void evallimits ();

	typedef byte * byte_iterator;
	typedef const byte * byte_const_iterator;
	byte_iterator iter (address pos);
	byte_const_iterator iter (address pos) const;

	void write (ostream & out, address pos, size_t size);
	void copy (Segment & dest);
private:
	static const size_t max_seg_size= 65536;
	byte mem [max_seg_size];
	address minused;
	address maxused;

	address curpos;
	address base;

	//address size;
	size_t size;

	address org_addr;
	bool has_org;
};


} // namespace impl
} // namespace pasmo


#endif

// End of segment.h
