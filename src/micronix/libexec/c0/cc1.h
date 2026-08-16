/*
 * all of pass1's world, for a source that has not been narrowed
 *
 * This was 590 lines that every source included for everything.  The
 * parts of it are the p1*.h below; a source that wants the type
 * system and not the switch bookkeeping includes p1core.h and
 * p1type.h and leaves cpp the room to hold the source itself.
 *
 * Kept so that including it still means what it always did.
 */
#ifndef _CC1_H
#define _CC1_H

#include "p1core.h"
#include "p1expr.h"
#include "p1type.h"
#include "p1name.h"
#include "p1stmt.h"
#include "p1pblk.h"
#include "p1swcnt.h"
#include "p1outh.h"
#include "p1lex.h"

#endif
