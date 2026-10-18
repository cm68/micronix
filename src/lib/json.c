/*
 * A minimal JSON DOM: parse, look up, print.
 *
 * lib/json.c
 *
 * Lifted from src/tools/hwsim.c, where it was the provisioning tool's
 * private parser.  The simulator reads the same file now, so there is one
 * parser rather than two that drift.
 *
 * The parser is a recursive descent over the text and reports failure by
 * longjmp out to json_parse(), which is what lets every helper below stay
 * a plain function returning a node instead of threading a status back up
 * through four call shapes.  A failed parse leaves the tree half-built
 * and leaks it; a bad machine description is a fatal startup error, so
 * that is a cost worth not paying attention to.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>

#include <json.h>

static char jerrmsg[512];
static jmp_buf jerrjmp;
static int jerrset;

static void
jerr(const char *p, const char *msg)
{
	snprintf(jerrmsg, sizeof jerrmsg, "json: %s near: %.20s", msg, p);
	if (jerrset)
		longjmp(jerrjmp, 1);
	fprintf(stderr, "%s\n", jerrmsg);
	exit(1);
}

const char *
json_error(void)
{
	return jerrmsg;
}

void
json_perror(const char *prog)
{
	fprintf(stderr, "%s: %s\n", prog, jerrmsg[0] ? jerrmsg : "json error");
}

jv *
json_new(int type)
{
	jv *j = calloc(1, sizeof *j);
	j->type = type;
	return j;
}

/*
 * Append a child.  An object takes its key here - the key is owned by the
 * parent, as are the parallel arrays, which is why the array case leaves
 * key NULL rather than leaving it unset.
 */
void
json_push(jv *parent, char *key, jv *child)
{
	parent->v = realloc(parent->v, (parent->n + 1) * sizeof *parent->v);
	if (parent->type == J_OBJ) {
		parent->key = realloc(parent->key, (parent->n + 1) * sizeof *parent->key);
		parent->key[parent->n] = key;
	}
	parent->v[parent->n] = child;
	parent->n++;
}

static void
skip_ws(const char **p)
{
	while (**p && isspace((unsigned char)**p))
		(*p)++;
}

/* *p is at the opening quote; returns a fresh string, *p past the close. */
static char *
parse_string(const char **p)
{
	const char *s;
	char *out, *o;

	(*p)++;
	out = malloc(strlen(*p) + 1);
	o = out;
	for (s = *p; *s && *s != '"'; s++) {
		if (*s != '\\') {
			*o++ = *s;
			continue;
		}
		s++;
		switch (*s) {
		case 'n': *o++ = '\n'; break;
		case 't': *o++ = '\t'; break;
		case 'r': *o++ = '\r'; break;
		case 'b': *o++ = '\b'; break;
		case 'f': *o++ = '\f'; break;
		case 'u': s += 4; *o++ = '?'; break;	/* unicode: config is ASCII */
		default:  *o++ = *s; break;		/* \" \\ \/ and friends */
		}
	}
	if (*s != '"')
		jerr(*p, "unterminated string");
	*p = s + 1;
	*o = '\0';
	return out;
}

static jv *parse_value(const char **p);

static jv *
parse_object(const char **p)
{
	jv *o = json_new(J_OBJ);

	(*p)++;				/* { */
	skip_ws(p);
	if (**p == '}') {
		(*p)++;
		return o;
	}
	for (;;) {
		char *key;
		jv *v;

		skip_ws(p);
		if (**p != '"')
			jerr(*p, "expected object key");
		key = parse_string(p);
		skip_ws(p);
		if (**p != ':')
			jerr(*p, "expected ':'");
		(*p)++;
		skip_ws(p);
		v = parse_value(p);
		json_push(o, key, v);
		skip_ws(p);
		if (**p == ',') {
			(*p)++;
			continue;
		}
		if (**p == '}') {
			(*p)++;
			return o;
		}
		jerr(*p, "expected ',' or '}'");
	}
}

static jv *
parse_array(const char **p)
{
	jv *a = json_new(J_ARR);

	(*p)++;				/* [ */
	skip_ws(p);
	if (**p == ']') {
		(*p)++;
		return a;
	}
	for (;;) {
		jv *v;

		skip_ws(p);
		v = parse_value(p);
		json_push(a, NULL, v);
		skip_ws(p);
		if (**p == ',') {
			(*p)++;
			continue;
		}
		if (**p == ']') {
			(*p)++;
			return a;
		}
		jerr(*p, "expected ',' or ']'");
	}
}

static jv *
parse_value(const char **p)
{
	skip_ws(p);
	if (**p == '{')
		return parse_object(p);
	if (**p == '[')
		return parse_array(p);
	if (**p == '"') {
		jv *j = json_new(J_STR);
		j->str = parse_string(p);
		return j;
	}
	if (strncmp(*p, "true", 4) == 0) {
		jv *j = json_new(J_BOOL);
		j->b = 1;
		*p += 4;
		return j;
	}
	if (strncmp(*p, "false", 5) == 0) {
		jv *j = json_new(J_BOOL);
		j->b = 0;
		*p += 5;
		return j;
	}
	if (strncmp(*p, "null", 4) == 0) {
		*p += 4;
		return json_new(J_NULL);
	}
	if (**p == '-' || isdigit((unsigned char)**p)) {
		char *end;
		jv *j = json_new(J_NUM);
		j->num = strtol(*p, &end, 10);
		*p = end;
		return j;
	}
	jerr(*p, "unexpected character");
	return NULL;
}

jv *
json_parse(const char *text)
{
	const char *p = text;
	jv *root;

	jerrmsg[0] = '\0';
	jerrset = 1;
	if (setjmp(jerrjmp)) {
		jerrset = 0;
		return NULL;
	}
	root = parse_value(&p);
	skip_ws(&p);
	if (*p != '\0')
		jerr(p, "trailing garbage");
	jerrset = 0;
	return root;
}

jv *
json_read(const char *path)
{
	FILE *f;
	long sz;
	char *text;
	jv *root;

	f = fopen(path, "r");
	if (!f) {
		snprintf(jerrmsg, sizeof jerrmsg, "cannot open %s: %s", path,
		    strerror(errno));
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	rewind(f);
	text = malloc(sz + 1);
	if (fread(text, 1, sz, f) != (size_t)sz) {
		fclose(f);
		free(text);
		snprintf(jerrmsg, sizeof jerrmsg, "short read on %s", path);
		return NULL;
	}
	text[sz] = '\0';
	fclose(f);
	root = json_parse(text);
	free(text);
	return root;
}

/* object member lookup */
jv *
json_get(jv *o, const char *key)
{
	int i;

	if (!o || o->type != J_OBJ)
		return NULL;
	for (i = 0; i < o->n; i++)
		if (o->key[i] && strcmp(o->key[i], key) == 0)
			return o->v[i];
	return NULL;
}

const char *
json_str(jv *o, const char *key)
{
	jv *x = json_get(o, key);
	return (x && x->type == J_STR) ? x->str : NULL;
}

long
json_num(jv *o, const char *key)
{
	jv *x = json_get(o, key);
	return (x && x->type == J_NUM) ? x->num : 0;
}

int
json_bool(jv *o, const char *key)
{
	jv *x = json_get(o, key);
	return (x && x->type == J_BOOL) ? x->b : 0;
}

/*
 * Pretty-print, and the serializer the tool writes sim.json back with.
 * An object or array of one member stays on one line, which is why the
 * single-volume case reads as it was typed.
 */
void
json_print(FILE *f, jv *j, int indent)
{
	int i;

	switch (j->type) {
	case J_NULL:
		fprintf(f, "null");
		break;
	case J_BOOL:
		fprintf(f, "%s", j->b ? "true" : "false");
		break;
	case J_NUM:
		fprintf(f, "%ld", j->num);
		break;
	case J_STR:
		fputc('"', f);
		for (i = 0; j->str[i]; i++) {
			unsigned char c = j->str[i];
			switch (c) {
			case '"': fputs("\\\"", f); break;
			case '\\': fputs("\\\\", f); break;
			case '\n': fputs("\\n", f); break;
			case '\t': fputs("\\t", f); break;
			default: fputc(c, f); break;
			}
		}
		fputc('"', f);
		break;
	case J_ARR:
		fputc('[', f);
		for (i = 0; i < j->n; i++) {
			if (i)
				fputc(',', f);
			if (j->n > 1)
				fprintf(f, "\n%*s", indent + 2, "");
			json_print(f, j->v[i], indent + 2);
		}
		if (j->n > 1)
			fprintf(f, "\n%*s", indent, "");
		fputc(']', f);
		break;
	case J_OBJ:
		fputc('{', f);
		for (i = 0; i < j->n; i++) {
			if (i)
				fputc(',', f);
			if (j->n > 1)
				fprintf(f, "\n%*s", indent + 2, "");
			fprintf(f, "\"%s\": ", j->key[i]);
			json_print(f, j->v[i], indent + 2);
		}
		if (j->n > 1)
			fprintf(f, "\n%*s", indent, "");
		fputc('}', f);
		break;
	}
}
