/*
 * The simulator's own configuration file.
 *
 * hwsim/config.c
 *
 * Three layers, in this order, each overriding the one before:
 *
 *   the compiled defaults             - every caller passes its own
 *   the file's "default" section      - the machine, spelled out
 *   the file's named profile          - the machine with something changed
 *   the command line                  - by construction, since the option
 *                                       loop runs after conf_load()
 *
 * The file is z80sim.json, looked for in $HWSIM_CONF, then the current
 * directory, then /usr/local/etc.  An environment variable is set on
 * purpose, so it is taken as an answer rather than a hint; the working
 * directory beats the installed copy so a tree being worked on is the
 * tree that runs.  Finding nothing at all is not an error - it is the
 * compiled defaults, which is a whole machine.
 *
 * The profile is always the first bare argument, never guessed at.  See
 * profile_arg() in hwsim.c for how it is picked out of the argument list.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sim.h"
#include "hwsim.h"
#include "config.h"

#define CONFFILE	"z80sim.json"
#define SYSCONF		"/usr/local/etc/" CONFFILE

extern char *progname;

static jv *conf_def;			/* the "default" section */
static jv *conf_prof;			/* the named profile, or NULL */
static char *conf_name;			/* its name, or NULL */
static char *conf_path;			/* the file it came from, or NULL */

const char *
conf_profile(void)
{
	return conf_name;
}

const char *
conf_source(void)
{
	return conf_path;
}

static const char *
conf_find(void)
{
	char *env = getenv("HWSIM_CONF");

	if (env && *env)
		return env;
	if (access(CONFFILE, R_OK) == 0)
		return CONFFILE;
	if (access(SYSCONF, R_OK) == 0)
		return SYSCONF;
	return NULL;
}

/* the names a profile could have been, for the error that says so */
static void
conf_names(jv *root)
{
	int i;

	fprintf(stderr, "%s: profiles:", progname);
	for (i = 0; i < root->n; i++)
		fprintf(stderr, " %s", root->key[i]);
	fprintf(stderr, "\n");
}

void
conf_load(const char *profile)
{
	const char *path = conf_find();
	jv *root;

	if (profile)
		conf_name = strdup(profile);

	if (!path)
		return;

	root = json_read(path);
	if (!root) {
		json_perror(progname);
		exit(1);
	}
	conf_path = strdup(path);
	conf_def = json_get(root, "default");

	if (profile) {
		conf_prof = json_get(root, profile);
		if (!conf_prof) {
			fprintf(stderr, "%s: no profile %s in %s\n", progname,
			    profile, path);
			conf_names(root);
			exit(1);
		}
	}
}

/*
 * Walk a dotted path down one tree.  Note that the two trees are walked
 * separately rather than merged node by node: the profile wins wherever
 * it has the whole path, and the default section answers wherever the
 * profile's path runs out.  That is the right rule for scalars, which is
 * what everything in the file is once the boards list has been resolved -
 * a profile that says "boot": {"level1": x} leaves boot.monitor to the
 * default section, as it should.
 */
static jv *
conf_walk(jv *o, const char *dotted)
{
	char buf[256];
	char *seg, *save;
	jv *v = o;

	if (!v)
		return NULL;
	snprintf(buf, sizeof buf, "%s", dotted);
	for (seg = strtok_r(buf, ".", &save); seg; seg = strtok_r(NULL, ".", &save)) {
		if (v->type != J_OBJ)
			return NULL;
		v = json_get(v, seg);
		if (!v)
			return NULL;
	}
	return v;
}

jv *
conf_get(const char *dotted)
{
	jv *v = conf_walk(conf_prof, dotted);

	return v ? v : conf_walk(conf_def, dotted);
}

const char *
conf_str(const char *dotted, const char *def)
{
	jv *v = conf_get(dotted);

	return (v && v->type == J_STR) ? v->str : def;
}

long
conf_int(const char *dotted, long def)
{
	jv *v = conf_get(dotted);

	return (v && v->type == J_NUM) ? v->num : def;
}

int
conf_bool(const char *dotted, int def)
{
	jv *v = conf_get(dotted);

	return (v && v->type == J_BOOL) ? v->b : def;
}

long
conf_hex(const char *dotted, long def)
{
	jv *v = conf_get(dotted);
	const char *s;
	char *end;
	long n;

	if (!v)
		return def;
	if (v->type != J_STR) {
		fprintf(stderr, "%s: %s: an address is a string like \"0x48\"\n",
		    progname, dotted);
		exit(1);
	}
	s = v->str;
	if (s[0] != '0' || (s[1] != 'x' && s[1] != 'X') || !s[2]) {
		fprintf(stderr, "%s: %s: \"%s\" is not a 0x address\n", progname,
		    dotted, s);
		exit(1);
	}
	n = strtol(s, &end, 16);
	if (*end) {
		fprintf(stderr, "%s: %s: \"%s\" is not a 0x address\n", progname,
		    dotted, s);
		exit(1);
	}
	return n;
}

/*
 * One board out of a "boards" list.  The two layers are searched in turn
 * rather than merged: an instance is a whole thing, and a profile that
 * gives a card an address has given it an address.
 *
 * The knockout rules of the plan - a profile removing an instance, an
 * instance that does not exist being an error - belong to the pass that
 * resolves the list once, into the machine's board table; this lookup is
 * what a driver uses in the meantime, and it is the same question.
 */
static jv *
board_in(jv *list, const char *card, int instance)
{
	int i;

	if (!list || list->type != J_ARR)
		return NULL;
	for (i = 0; i < list->n; i++) {
		jv *b = list->v[i];
		const char *c;

		if (b->type != J_OBJ)
			continue;
		c = json_str(b, "card");
		if (!c || strcmp(c, card) != 0)
			continue;
		if ((int)json_num(b, "instance") == instance)
			return b;
	}
	return NULL;
}

jv *
conf_board(const char *card, int instance)
{
	jv *b = board_in(conf_walk(conf_prof, "boards"), card, instance);

	return b ? b : board_in(conf_walk(conf_def, "boards"), card, instance);
}

const char *
conf_board_str(const char *card, int instance, const char *key,
    const char *def)
{
	jv *b = conf_board(card, instance);
	const char *s = b ? json_str(b, key) : NULL;

	return s ? s : def;
}

long
conf_board_int(const char *card, int instance, const char *key, long def)
{
	jv *b = conf_board(card, instance);

	return b ? json_num(b, key) : def;
}

long
conf_board_hex(const char *card, int instance, const char *key, long def)
{
	jv *b = conf_board(card, instance);
	const char *s;
	char *end;
	long n;

	if (!b)
		return def;
	s = json_str(b, key);
	if (!s)
		return def;
	if (s[0] != '0' || (s[1] != 'x' && s[1] != 'X') || !s[2]) {
		fprintf(stderr, "%s: %s base %s: \"%s\" is not a 0x address\n",
		    progname, card, key, s);
		exit(1);
	}
	n = strtol(s, &end, 16);
	if (*end) {
		fprintf(stderr, "%s: %s base %s: \"%s\" is not a 0x address\n",
		    progname, card, key, s);
		exit(1);
	}
	return n;
}
