// value.cpp
// Revision 22-nov-2005

#include "value.h"


namespace pasmo {

Value::Value () :
	type (ValueAbsolute), value (0)
{
}

Value::Value (ValueType type_n, address value_n) :
	type (type_n), value (value_n)
{
}

Value::Value (const Value & v2)
{
	type= v2.type;
	value= v2.value;
}

void Value::operator += (const Value & v2)
{
	// Need revision.
	if (type == ValueAbsolute)
	{
		type= v2.type;
		value+= v2.value;
	}
	else if (v2.type == ValueAbsolute)
	{
		value+= v2.value;
	}
	else if (type == v2.type)
	{
		type= ValueAbsolute;
		value= v2.value;
	}
	else
	{
		value+= v2.value;
	}
}

void Value::operator -= (const Value & v2)
{
	// Need revision.
	if (type == ValueAbsolute)
	{
		type= v2.type;
		value-= v2.value;
	}
	else if (v2.type == ValueAbsolute)
	{
		value-= v2.value;
	}
	else if (type == v2.type)
	{
		type= ValueAbsolute;
		value-= v2.value;
	}
	else
	{
		value-= v2.value;
	}
}

bool operator < (const Value & v1, const Value & v2)
{
	if (v1.type < v2.type)
		return true;
	else
	{
		if (v1.type > v2.type)
			return false;
		else
			return v1.value < v2.value;
	}
}

namespace {

void puttypedesc (ostream & os, ValueType type)
{
	switch (type)
	{
	case ValueAbsolute:
		os << "ASEG"; break;
	case ValueProgRelative:
		os << "CSEG"; break;
	case ValueDataRelative:
		os << "DSEG"; break;
	case ValueCommonRelative:
		os << "COMMON"; break;
	}
}

} // namespace

ostream & operator << (ostream & os, const Value & v)
{
	puttypedesc (os, v.type);
	os << ':' << hex4 (v.value);
	return os;
}


} // namespace pasmo


// End of value.cpp
