/*
 * hwsim - provision and run the hardware simulator (hwsim/d1).
 *
 * One sim.json holds every simulator definition.  A sim is a collection of
 * volumes; a volume is a disk on a controller (geometry + controller), not a
 * role.  root and swap are the kernel's setdev arguments, named as device
 * tokens.  hwsim is a front end: it execs mnix to format/partition/mkfs and
 * d1/d1p to boot, and it turns the config block into one packed -c number and
 * the mediasets block into the -e manifest.
 *
 * The design is src/tools/PROVISION.md; the slice layout is the format
 * program's protoslices() (cmd/mwformat/mwformat.c), reproduced here.
 *
 * See docs/WORKORDER-provisioning.md.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>

/* ------------------------------------------------------------------ */
/* a minimal JSON DOM                                                  */

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

static void
jerr(const char *p, const char *msg)
{
	fprintf(stderr, "hwsim: json: %s near: %.20s\n", msg, p);
	exit(1);
}

static jv *
jnew(int type)
{
	jv *j = calloc(1, sizeof *j);
	j->type = type;
	return j;
}

static void
jpush(jv *parent, char *key, jv *child)
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
	jv *o = jnew(J_OBJ);

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
		jpush(o, key, v);
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
	jv *a = jnew(J_ARR);

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
		jpush(a, NULL, v);
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
		jv *j = jnew(J_STR);
		j->str = parse_string(p);
		return j;
	}
	if (strncmp(*p, "true", 4) == 0) {
		jv *j = jnew(J_BOOL);
		j->b = 1;
		*p += 4;
		return j;
	}
	if (strncmp(*p, "false", 5) == 0) {
		jv *j = jnew(J_BOOL);
		j->b = 0;
		*p += 5;
		return j;
	}
	if (strncmp(*p, "null", 4) == 0) {
		*p += 4;
		return jnew(J_NULL);
	}
	if (**p == '-' || isdigit((unsigned char)**p)) {
		char *end;
		jv *j = jnew(J_NUM);
		j->num = strtol(*p, &end, 10);
		*p = end;
		return j;
	}
	jerr(*p, "unexpected character");
	return NULL;
}

static jv *
json_parse(const char *text)
{
	const char *p = text;
	jv *root = parse_value(&p);
	skip_ws(&p);
	if (*p != '\0')
		jerr(p, "trailing garbage");
	return root;
}

/* object member lookup */
static jv *
jget(jv *o, const char *key)
{
	int i;

	if (!o || o->type != J_OBJ)
		return NULL;
	for (i = 0; i < o->n; i++)
		if (o->key[i] && strcmp(o->key[i], key) == 0)
			return o->v[i];
	return NULL;
}

static const char *
js(jv *o, const char *key)
{
	jv *x = jget(o, key);
	return (x && x->type == J_STR) ? x->str : NULL;
}

static long
jnum(jv *o, const char *key)
{
	jv *x = jget(o, key);
	return (x && x->type == J_NUM) ? x->num : 0;
}

static int
jb(jv *o, const char *key)
{
	jv *x = jget(o, key);
	return (x && x->type == J_BOOL) ? x->b : 0;
}

/* ------------------------------------------------------------------ */
/* pretty-print (also the serializer for add/del)                      */

static void
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

/* ------------------------------------------------------------------ */
/* sim.json location and the repo root                                 */

static char rootbuf[PATH_MAX];

/*
 * The repo root is three levels up from the binary (src/tools/hwsim), the
 * same way the per-card scripts walk ../../ from disks/hdinstall.  sim.json
 * lives there; image/monitor/kernel paths in sim.json are relative to it.
 */
static const char *
repo_root(const char *argv0)
{
	char bin[PATH_MAX];
	char *s;
	int i;

	if (!realpath(argv0, bin))
		strncpy(bin, argv0, sizeof bin - 1);
	for (i = 0; i < 3; i++) {
		s = strrchr(bin, '/');
		if (s)
			*s = '\0';
	}
	strncpy(rootbuf, bin, sizeof rootbuf - 1);
	return rootbuf;
}

static const char *
sim_json_path(const char *root, const char *override)
{
	static char path[PATH_MAX];

	if (override)
		return override;
	snprintf(path, sizeof path, "%s/sim.json", root);
	if (access(path, F_OK) == 0)
		return path;
	return "sim.json";	/* fall back to the current directory */
}

static jv *
load_sim(const char *path)
{
	FILE *f;
	long sz;
	char *text;
	jv *root;

	f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "hwsim: cannot open %s: %s\n", path, strerror(errno));
		exit(1);
	}
	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	rewind(f);
	text = malloc(sz + 1);
	if (fread(text, 1, sz, f) != (size_t)sz) {
		fprintf(stderr, "hwsim: short read on %s\n", path);
		exit(1);
	}
	text[sz] = '\0';
	fclose(f);
	root = json_parse(text);
	return root;
}

/* ------------------------------------------------------------------ */
/* device naming, -c packing, the default slice layout                 */

struct medium {
	const char *name;
	int cyl, heads, spt;
};

/* A copy of mnix.c's mediums[] — the geometry the tool needs to lay out
 * slices, which mnix itself uses only for format.  The same duplication
 * mnix and mwformat already share. */
static struct medium mediums[] = {
	{ "m5", 153, 4, 17 },
	{ "m10", 306, 4, 17 },
	{ "m16", 306, 6, 17 },
	{ "m32", 640, 6, 17 },
	{ "m40", 512, 8, 17 },
	{ "m26", 202, 8, 32 },
	{ "xt1065", 918, 7, 17 },
	{ "xt1085", 1024, 8, 17 },
	{ "xt1105", 918, 11, 17 },
	{ "xt1140", 918, 15, 17 },
	{ "xt2085", 1224, 7, 17 },
	{ "xt2140", 1224, 11, 17 },
	{ "xt2190", 1224, 15, 17 },
	{ "wren36", 697, 5, 17 },
	{ "wren48", 925, 5, 17 },
	{ "wren51", 989, 5, 17 },
	{ "wren57", 925, 6, 17 },
	{ "wren67", 925, 7, 17 },
	{ "wren77", 925, 8, 17 },
	{ "wren85", 1024, 8, 17 },
	{ "wren86", 925, 9, 17 },
	{ "wren96", 1024, 9, 17 },
	{ "st225", 615, 4, 17 },
	{ "st251", 820, 6, 17 },
	{ "st4038", 733, 5, 17 },
	{ "st4051", 977, 5, 17 },
	{ "st4053", 1024, 5, 17 },
	{ "st4096", 1024, 9, 17 },
	{ "ms8425", 615, 4, 17 },
	{ "ms6053", 1024, 5, 17 },
	{ "ms6085", 1024, 8, 17 },
	{ "tm501", 306, 2, 17 },
	{ "tm502", 306, 4, 17 },
	{ "tm503", 306, 6, 17 },
	{ "mp1323a", 1024, 5, 17 },
	{ "mp1325", 1024, 8, 17 },
	{ "mp1335", 1024, 8, 17 },
	{ NULL, 0, 0, 0 }
};

static struct medium *
medium_of(const char *name)
{
	struct medium *m;

	for (m = mediums; m->name; m++)
		if (strcmp(m->name, name) == 0)
			return m;
	return NULL;
}

/* major numbers are fixed (PROVISION.md); the JSON never stores one */
static int
major_of(const char *ctl)
{
	if (!strcmp(ctl, "hdca"))
		return 1;
	if (!strcmp(ctl, "djdma"))
		return 2;
	if (!strcmp(ctl, "hdcdma"))
		return 3;
	if (!strcmp(ctl, "ide"))
		return 4;
	if (!strcmp(ctl, "ncr"))
		return 5;
	return -1;
}

/* "hdcdma:0a" -> major/minor; minor = (slice<<5)|unit, a=0..h=7. */
static int
parse_dev(const char *tok, int *major, int *minor)
{
	char ctl[32];
	int unit;
	char slice;
	int m;

	if (sscanf(tok, "%31[^:]:%d%c", ctl, &unit, &slice) != 3)
		return -1;
	if (slice < 'a' || slice > 'h')
		return -1;
	m = major_of(ctl);
	if (m < 0)
		return -1;
	*major = m;
	*minor = ((slice - 'a') << 5) | unit;
	return 0;
}

/* the -c switch, packed from the named config fields (PROVISION.md) */
static int
pack_config(jv *config)
{
	const char *boot = js(config, "boot");
	const char *clock = js(config, "clock");
	jv *xterm = jget(config, "xterm");
	int c = 0;
	int i;

	if (boot) {
		if (!strcmp(boot, "hdcdma"))
			c |= 0x08;
		else if (!strcmp(boot, "djdma"))
			c |= 0x10;
		/* hdca / ide are 0x00 */
	}
	if (jb(config, "nomon"))
		c |= 0x04;
	if (xterm && xterm->type == J_ARR) {
		for (i = 0; i < xterm->n; i++) {
			const char *u =
			    (xterm->v[i]->type == J_STR) ? xterm->v[i]->str : NULL;
			if (!u)
				continue;
			if (!strcmp(u, "uart0"))
				c |= 0x100;
			else if (!strcmp(u, "ttyB"))
				c |= 0x200;
			else if (!strcmp(u, "ttyC"))
				c |= 0x400;
		}
	}
	if (clock) {
		if (!strcmp(clock, "1hz"))
			c |= 0x800;
		else if (!strcmp(clock, "noclk"))
			c |= 0x1000;
		/* "normal" is 0 */
	}
	return c;
}

/* the default slice table the format program writes (mwformat.c
 * protoslices): a = biggest filesystem up to 65535 blocks, b = 1M swap,
 * d..h = 32M each, the last a runt.  Fills sl[i] (i = slice letter index
 * a..h) with the inclusive cylinder range; sl[i].set is 0 when empty. */
struct slice {
	int start, end, set;
};

static void
protoslices(int cyl, int heads, int spt, struct slice *sl)
{
	int spc = heads * spt;
	int off = 1, left = cyl - 1;
	int maxfs = 0xffff / spc;
	int bsz = (2048 + spc - 1) / spc;
	int i;

	for (i = 0; i < 8; i++)
		sl[i].set = 0;

	if (left > bsz) {
		int sz = left - bsz;
		if (maxfs < sz)
			sz = maxfs;
		sl[0].start = off;
		sl[0].end = off + sz - 1;
		sl[0].set = 1;
		off += sz;
		left -= sz;
	}
	if (bsz) {
		sl[1].start = off;
		sl[1].end = off + bsz - 1;
		sl[1].set = 1;
		off += bsz;
		left -= bsz;
	}
	for (i = 3; i < 8; i++) {
		int sz;
		if (left == 0)
			break;
		sz = (maxfs < left) ? maxfs : left;
		sl[i].start = off;
		sl[i].end = off + sz - 1;
		sl[i].set = 1;
		off += sz;
		left -= sz;
	}
}

/* ------------------------------------------------------------------ */
/* running the existing tools                                          */

static void
runv(char **av)
{
	pid_t pid = fork();
	int st;

	if (pid < 0) {
		perror("hwsim: fork");
		exit(1);
	}
	if (pid == 0) {
		execvp(av[0], av);
		fprintf(stderr, "hwsim: exec %s: %s\n", av[0], strerror(errno));
		_exit(127);
	}
	if (waitpid(pid, &st, 0) < 0) {
		perror("hwsim: waitpid");
		exit(1);
	}
	if (WIFEXITED(st) && WEXITSTATUS(st) != 0)
		exit(WEXITSTATUS(st));
	if (WIFSIGNALED(st))
		exit(1);
}

/* repo-relative path (absolute paths pass through); result is owned */
static char *
resolve_path(const char *root, const char *p)
{
	char *b;

	if (!p || p[0] == '/')
		return (char *)p;
	b = malloc(strlen(root) + strlen(p) + 2);
	sprintf(b, "%s/%s", root, p);
	return b;
}

static char *
itos(int v)
{
	char *b = malloc(16);
	snprintf(b, 16, "%d", v);
	return b;
}

static char *
slice_name(int k)
{
	char *b = malloc(2);
	b[0] = 'a' + k;
	b[1] = '\0';
	return b;
}

static jv *db;			/* the parsed sim.json */
static const char *g_json;	/* the sim.json path, for add/del to rewrite */

static jv *
sim_get(const char *name)
{
	jv *sims = jget(db, "sims");
	jv *sim;

	if (!sims || !(sim = jget(sims, name))) {
		fprintf(stderr, "hwsim: no such sim: %s\n", name);
		exit(1);
	}
	return sim;
}

static char *
join_path(const char *a, const char *b)
{
	char *s = malloc(strlen(a) + strlen(b) + 2);
	sprintf(s, "%s/%s", a, b);
	return s;
}

static char *
itoh(int v)
{
	char *b = malloc(16);
	snprintf(b, 16, "0x%x", v);
	return b;
}

/*
 * The -e manifest the media changer reads: one line per set, name then the
 * set's image paths (absolute), the format dist/distsets prints.
 */
static char *
write_manifest(const char *root)
{
	jv *ms = jget(db, "mediasets");
	char tmp[] = "/tmp/hwsim-manifest-XXXXXX";
	int fd = mkstemp(tmp);
	FILE *f;
	int i, k;

	if (fd < 0) {
		perror("hwsim: mkstemp");
		exit(1);
	}
	f = fdopen(fd, "w");
	for (i = 0; ms && i < ms->n; i++) {
		jv *vols = jget(ms->v[i], "volumes");
		fprintf(f, "%s", ms->key[i]);
		for (k = 0; vols && k < vols->n; k++)
			if (vols->v[k]->type == J_STR)
				fprintf(f, " %s", resolve_path(root, vols->v[k]->str));
		fprintf(f, "\n");
	}
	fclose(f);
	return strdup(tmp);
}

/* ------------------------------------------------------------------ */
/* verbs                                                               */

static void
v_list(void)
{
	jv *sims = jget(db, "sims");
	int i;

	if (!sims) {
		fprintf(stderr, "hwsim: sim.json has no \"sims\" object\n");
		exit(1);
	}
	for (i = 0; i < sims->n; i++)
		printf("%s\n", sims->key[i]);
}

static void
v_show(const char *name)
{
	jv *sims = jget(db, "sims");
	jv *sim;
	jv *config;
	const char *root, *swap;
	int major, minor;

	if (!sims || !(sim = jget(sims, name))) {
		fprintf(stderr, "hwsim: no such sim: %s\n", name);
		exit(1);
	}
	json_print(stdout, sim, 0);
	fputc('\n', stdout);

	config = jget(sim, "config");
	root = js(sim, "root");
	swap = js(sim, "swap");
	printf("# root %s", root ? root : "-");
	if (root && parse_dev(root, &major, &minor) == 0)
		printf(" = %d/%d", major, minor);
	printf("\n# swap %s", swap ? swap : "-");
	if (swap && parse_dev(swap, &major, &minor) == 0)
		printf(" = %d/%d", major, minor);
	printf("\n# -c 0x%x\n", pack_config(config));
}

static void
v_format(const char *root, const char *name)
{
	jv *sim = sim_get(name);
	jv *vols = jget(sim, "volumes");
	char *mnix = resolve_path(root, "src/tools/mnix");
	int i;

	for (i = 0; vols && i < vols->n; i++) {
		jv *v = vols->v[i];
		const char *geom = js(v, "geometry");
		const char *img = js(v, "image");
		char *av[8];

		if (!geom || !img) {
			fprintf(stderr, "hwsim: volume %d missing geometry/image\n", i);
			exit(1);
		}
		av[0] = mnix;
		av[1] = "initialize";
		av[2] = (char *)geom;
		av[3] = resolve_path(root, img);
		av[4] = NULL;
		runv(av);
	}
}

static void
v_partition(const char *root, const char *name)
{
	jv *sim = sim_get(name);
	jv *vols = jget(sim, "volumes");
	char *mnix = resolve_path(root, "src/tools/mnix");
	int i;

	for (i = 0; vols && i < vols->n; i++) {
		jv *v = vols->v[i];
		const char *geom = js(v, "geometry");
		const char *img = js(v, "image");
		struct medium *m;
		struct slice sl[8];
		char *av[64];
		int n = 0, k;

		if (!img) {
			fprintf(stderr, "hwsim: volume %d missing image\n", i);
			exit(1);
		}
		if (!geom || !(m = medium_of(geom))) {
			fprintf(stderr, "hwsim: volume %d: unknown geometry %s\n",
			    i, geom ? geom : "(none)");
			exit(1);
		}
		protoslices(m->cyl, m->heads, m->spt, sl);
		av[n++] = mnix;
		av[n++] = "label";
		av[n++] = resolve_path(root, img);
		for (k = 0; k < 8; k++) {
			if (!sl[k].set)
				continue;
			av[n++] = "-s";
			av[n++] = slice_name(k);
			av[n++] = itos(sl[k].start);
			av[n++] = itos(sl[k].end);
		}
		av[n] = NULL;
		runv(av);
	}
}

static void
v_mkfs(const char *root, const char *name)
{
	jv *sim = sim_get(name);
	jv *vols = jget(sim, "volumes");
	char *mnix = resolve_path(root, "src/tools/mnix");
	char *bootimg = resolve_path(root, "src/micronix/stand/boot/bootimg");
	int i;

	for (i = 0; vols && i < vols->n; i++) {
		jv *v = vols->v[i];
		const char *img = js(v, "image");
		char *av[8];

		if (!img) {
			fprintf(stderr, "hwsim: volume %d missing image\n", i);
			exit(1);
		}
		av[0] = mnix;
		av[1] = "mkfs";
		av[2] = resolve_path(root, img);
		av[3] = "-i";
		av[4] = bootimg;
		av[5] = "-f";
		av[6] = NULL;
		runv(av);
	}
}

static void
v_boot(const char *root, const char *name, int debug)
{
	jv *sim = sim_get(name);
	jv *config = jget(sim, "config");
	jv *vols = jget(sim, "volumes");
	const char *emu = js(sim, "emulator");
	const char *mon = js(sim, "monitor");
	const char *kernel = js(sim, "kernel");
	char *simbin;
	char *manifest;
	char *logpath;
	char *av[128];
	int n = 0, i;

	if (!emu)
		emu = "hwsim/d1";
	simbin = join_path(resolve_path(root, emu), debug ? "d1" : "d1p");
	manifest = write_manifest(root);
	logpath = malloc(strlen(name) + 32);
	snprintf(logpath, strlen(name) + 32, "/tmp/hwsim-%s.log", name);

	av[n++] = simbin;
	if (!debug)
		av[n++] = "-H";			/* headless, like dist_boot */
	av[n++] = "-b";
	av[n++] = resolve_path(root, mon);
	av[n++] = "-c";
	av[n++] = itoh(pack_config(config));
	if (debug && kernel) {
		av[n++] = "-S";
		av[n++] = resolve_path(root, kernel);
	}
	av[n++] = "-e";
	av[n++] = manifest;
	av[n++] = "-L";
	av[n++] = logpath;
	for (i = 0; vols && i < vols->n; i++) {
		jv *v = vols->v[i];
		const char *ctl = js(v, "controller");
		const char *img = js(v, "image");
		char *imgpath, *wiring;

		if (!ctl || !img) {
			fprintf(stderr, "hwsim: volume %d missing controller/image\n", i);
			exit(1);
		}
		imgpath = resolve_path(root, img);
		if (access(imgpath, F_OK) != 0) {
			fprintf(stderr, "hwsim: %s: not found - build it first\n",
			    imgpath);
			exit(1);
		}
		wiring = malloc(strlen(ctl) + strlen(imgpath) + 16);
		sprintf(wiring, "%s%ld:%s", ctl, jnum(v, "unit"), imgpath);
		av[n++] = wiring;
	}
	av[n] = NULL;
	runv(av);
}

static void
save_db(void)
{
	FILE *f = fopen(g_json, "w");

	if (!f) {
		fprintf(stderr, "hwsim: cannot write %s: %s\n", g_json,
		    strerror(errno));
		exit(1);
	}
	json_print(f, db, 0);
	fprintf(f, "\n");
	fclose(f);
}

static void
v_add(const char *name, char **args)
{
	jv *sim = sim_get(name);
	jv *vols = jget(sim, "volumes");
	jv *vol, *x;

	if (!vols || vols->type != J_ARR) {
		vols = jnew(J_ARR);
		jpush(sim, strdup("volumes"), vols);
	}
	vol = jnew(J_OBJ);
	x = jnew(J_STR);
	x->str = strdup(args[0]);
	jpush(vol, strdup("controller"), x);
	x = jnew(J_NUM);
	x->num = atol(args[1]);
	jpush(vol, strdup("unit"), x);
	x = jnew(J_STR);
	x->str = strdup(args[2]);
	jpush(vol, strdup("geometry"), x);
	x = jnew(J_STR);
	x->str = strdup(args[3]);
	jpush(vol, strdup("image"), x);
	jpush(vols, NULL, vol);
	save_db();
	printf("hwsim: added %s:%s to %s\n", args[0], args[1], name);
}

static void
v_del(const char *name, const char *tok)
{
	jv *sim = sim_get(name);
	jv *vols = jget(sim, "volumes");
	char ctl[32];
	int unit;
	int i, found = -1;

	if (sscanf(tok, "%31[^:]:%d", ctl, &unit) != 2) {
		fprintf(stderr, "hwsim: bad volume: %s (want controller:unit)\n",
		    tok);
		exit(1);
	}
	for (i = 0; vols && i < vols->n; i++) {
		const char *c = js(vols->v[i], "controller");
		if (c && strcmp(c, ctl) == 0 && jnum(vols->v[i], "unit") == unit) {
			found = i;
			break;
		}
	}
	if (found < 0) {
		fprintf(stderr, "hwsim: no volume %s in %s\n", tok, name);
		exit(1);
	}
	free(vols->v[found]);
	for (i = found; i < vols->n - 1; i++)
		vols->v[i] = vols->v[i + 1];
	vols->n--;
	save_db();
	printf("hwsim: removed %s from %s\n", tok, name);
}

/* ------------------------------------------------------------------ */

static void
usage(void)
{
	fprintf(stderr,
	    "usage: hwsim [-f sim.json] <verb> [sim] [args]\n"
	    "  format <sim>       mnix initialize (geometry label)\n"
	    "  partition <sim>    mnix label (default slice table)\n"
	    "  mkfs <sim>         mnix mkfs (filesystem + boot)\n"
	    "  boot <sim>         launch d1/d1p from config + volumes\n"
	    "  add / del <sim>    a volume in or out\n"
	    "  list / show <sim>  manage sim.json\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	const char *progname = argv[0];
	const char *json_override = NULL;
	const char *verb;
	const char *name;
	const char *root;
	int debug = 0;

	while (argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		if (strcmp(argv[1], "-f") == 0) {
			if (argc < 4)
				usage();
			json_override = argv[2];
			argv += 2;
			argc -= 2;
		} else if (strcmp(argv[1], "--debug") == 0 ||
		    strcmp(argv[1], "-g") == 0) {
			debug = 1;
			argv += 1;
			argc -= 1;
		} else {
			break;
		}
	}
	if (argc < 2)
		usage();
	verb = argv[1];
	name = (argc > 2) ? argv[2] : NULL;

	root = repo_root(progname);
	g_json = sim_json_path(root, json_override);
	db = load_sim(g_json);

	if (strcmp(verb, "list") == 0) {
		v_list();
		return 0;
	}
	if (strcmp(verb, "show") == 0) {
		if (!name)
			usage();
		v_show(name);
		return 0;
	}
	if (strcmp(verb, "format") == 0) {
		if (!name)
			usage();
		v_format(root, name);
		return 0;
	}
	if (strcmp(verb, "partition") == 0) {
		if (!name)
			usage();
		v_partition(root, name);
		return 0;
	}
	if (strcmp(verb, "mkfs") == 0) {
		if (!name)
			usage();
		v_mkfs(root, name);
		return 0;
	}
	if (strcmp(verb, "boot") == 0) {
		if (!name)
			usage();
		v_boot(root, name, debug);
		return 0;
	}
	if (strcmp(verb, "add") == 0) {
		if (!name || argc < 7)
			usage();
		v_add(name, argv + 3);
		return 0;
	}
	if (strcmp(verb, "del") == 0) {
		if (!name || argc < 4)
			usage();
		v_del(name, argv[3]);
		return 0;
	}
	usage();
	return 0;
}
