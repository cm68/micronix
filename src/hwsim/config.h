/*
 * The simulator's own configuration file.
 *
 * hwsim/config.h
 *
 * The machine description the simulator reads at startup: which cards are
 * in the machine, where each one answers, how the interrupts are wired,
 * which roms it boots.  It is an overlay on the compiled-in defaults, so
 * a key the file does not mention is the code's own value, and an option
 * on the command line beats both.
 *
 * conf_get() and friends are the whole interface.  A driver asks for what
 * it needs by a dotted path - "boot.monitor", "boards" - and gets the
 * compiled default it passed in if the file says nothing about it.  That
 * is why there is no registration step: the default lives at the point of
 * use, which is where it already was.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <json.h>

/*
 * Find, parse and select.  profile may be NULL, which is the default
 * section alone.  A named profile that the file does not have is fatal.
 * No file anywhere is not: it means the compiled defaults.
 */
void conf_load(const char *profile);

const char *conf_profile(void);		/* the name, or NULL */
const char *conf_source(void);		/* the file it came from, or NULL */

/* the merged view: the profile over the default section */
jv *conf_get(const char *dotted);

const char *conf_str(const char *dotted, const char *def);
long conf_int(const char *dotted, long def);
int conf_bool(const char *dotted, int def);

/*
 * A board, out of the "boards" list.  That list is a list of instances and
 * not a table keyed by card name, because what a card *is* here is the
 * pair (card, instance) - which is what lets two of the same card exist
 * at two addresses.  A driver asks for the one it is; the instance number
 * defaults to 0, so the common machine says nothing about it.
 *
 * Returns NULL when neither layer names that instance, and then the
 * caller's compiled value stands.
 */
jv *conf_board(const char *card, int instance);

const char *conf_board_str(const char *card, int instance, const char *key,
    const char *def);
long conf_board_int(const char *card, int instance, const char *key, long def);
long conf_board_hex(const char *card, int instance, const char *key, long def);

/*
 * An address: the file spells every one "0x.." and nothing else, because
 * JSON has no hex literal and a decimal address in a machine file is a
 * trap.  A value that is not 0x followed by hex digits is refused here,
 * by name, rather than silently decoding to something wrong.
 */
long conf_hex(const char *dotted, long def);

#endif
