#ifndef INCLUDE_MACHINE_H
#define INCLUDE_MACHINE_H

// machine.h
// Revision 17-oct-2005

#include "token.h"

namespace pasmo {
namespace impl {

class ExecMachine {
public:
	virtual void exec ()= 0;
	virtual ~ExecMachine () { }
};

} // namespace impl
} // namespace pasmo

#endif

// End of machine.h
