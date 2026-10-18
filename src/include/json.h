/*
 * A minimal JSON DOM: parse, look up, print.
 *
 * include/json.h
 *
 * One implementation for the two host programs that read the machine
 * description - the provisioning tool (src/tools) and the simulator
 * itself (src/hwsim) - so a file means the same thing to both.  It was
 * the tool's private parser first; nothing about it is tool-specific.
 *
 * The tree is deliberately exposed: the tool builds a volume by hand and
 * then prints the whole file back out, so the nodes it makes and the
 * nodes the parser makes have to be the same shape.
 *
 * Errors do not exit.  A failed call returns NULL and leaves a message
 * behind for json_error()/json_perror(); the caller decides whether a
 * file it could not read is fatal, which for the simulator's three-deep
 * search path it is not.
 */
#ifndef JSON_H
#define JSON_H

#include <stdio.h>

#define J_OBJ  1
#define J_ARR  2
#define J_STR  3
#define J_NUM  4
#define J_BOOL 5
#define J_NULL 6

typedef struct jv jv;
struct jv {
	int type;
	char *str;		/* J_STR value */
	long num;		/* J_NUM */
	int b;			/* J_BOOL */
	jv **v;			/* object values / array items */
	char **key;		/* object keys, parallel to v (NULL for arrays) */
	int n;
};

jv *json_parse(const char *text);	/* NULL on a syntax error */
jv *json_read(const char *path);	/* NULL if unreadable or malformed */

jv *json_new(int type);
void json_push(jv *parent, char *key, jv *child);

jv *json_get(jv *o, const char *key);
const char *json_str(jv *o, const char *key);
long json_num(jv *o, const char *key);
int json_bool(jv *o, const char *key);

void json_print(FILE *f, jv *j, int indent);

const char *json_error(void);
void json_perror(const char *prog);

#endif
