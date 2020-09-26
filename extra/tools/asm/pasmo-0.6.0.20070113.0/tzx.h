#ifndef INCLUDE_TZX_H
#define INCLUDE_TZX_H

// tzx.h
// Revision 9-aug-2005

#include <iostream>

#include <stdlib.h>

namespace pasmo {
namespace tzx {

void writefilehead (std::ostream & out);

void writestandardblockhead (std::ostream & out);

void writeturboblockhead (std::ostream & out, size_t len);

} // namespace tzx
} // namespace pasmo

#endif

// End of tzx.h
