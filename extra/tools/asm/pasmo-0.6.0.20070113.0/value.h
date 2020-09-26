#ifndef INCLUDE_VALUE_H
#define INCLUDE_VALUE_H

// value.h
// Revision 22-nov-2005


#include "pasmotypes.h"


namespace pasmo {


enum ValueType {
	ValueAbsolute=       0x00,
	ValueProgRelative=   0x01,
	ValueDataRelative=   0x02,
	ValueCommonRelative= 0x03
};

struct Value {
	ValueType type;
	address value;
	Value ();
	Value (ValueType type_n, address value_n);
	Value (const Value & v2);
	void operator += (const Value & v2);
	void operator -= (const Value & v2);
};


bool operator < (const Value & v1, const Value & v2);

ostream & operator << (ostream & os, const Value & v);


} // namespace pasmo


#endif

// End of value.h
