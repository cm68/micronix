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

#include <json.h>

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
	jv *root = json_read(path);

	if (!root) {
		json_perror("hwsim");
		exit(1);
	}
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

/*
 * The boot chain's roms.  They live under "boot" in the current schema;
 * the first schema kept the monitor at the top level, so fall back to
 * that for a file that has not been rewritten.
 */
static const char *
boot_field(jv *sim, const char *key)
{
	jv *boot = json_get(sim, "boot");
	const char *s;

	if (boot && (s = json_str(boot, key)) != NULL)
		return s;
	if (strcmp(key, "monitor") == 0)
		return json_str(sim, "monitor");
	return NULL;
}

/*
 * The monitor roms that decode the boot device from switches 5-7: the 375
 * and 447.  mon500 reads only switch 6 (monitor or boot, and boot is the
 * level 1 rom), so its boot bits choose level 1, not a card, and packing
 * a device name into them would mean nothing.
 */
static int
monitor_has_bootdev(const char *mon)
{
	const char *base;

	if (!mon)
		return 0;
	base = strrchr(mon, '/');
	base = base ? base + 1 : mon;
	return strstr(base, "375") != NULL || strstr(base, "447") != NULL;
}

/* the -c switch, packed from the named config fields (PROVISION.md) */
static int
pack_config(jv *sim)
{
	jv *config = json_get(sim, "config");
	const char *mon = boot_field(sim, "monitor");
	const char *boot = boot_field(sim, "device");
	const char *console = json_str(config, "console");
	const char *clock = json_str(config, "clock");
	jv *xterm = json_get(config, "xterm");
	int c = 0;
	int i;

	/* the older schema kept the device in config.boot */
	if (!boot)
		boot = json_str(config, "boot");
	if (boot && monitor_has_bootdev(mon)) {
		if (!strcmp(boot, "hdcdma"))
			c |= 0x08;
		else if (!strcmp(boot, "djdma"))
			c |= 0x10;
		else if (strcmp(boot, "hdca") != 0 && strcmp(boot, "ide") != 0) {
			fprintf(stderr, "hwsim: unknown boot device: %s\n", boot);
			exit(1);
		}
		/* hdca / ide are 0x00 */
		c |= 0x04;		/* the boot, not the monitor */
	}
	if (json_bool(config, "nomon"))
		c |= 0x04;
	/*
	 * On the mon500 class switch 5 (0x08) says which console to find
	 * first: set, the Wunderbus; clear, the SS1.  A 375/447 reads the
	 * same bit as the HD-DMA boot device, so there the boot field above
	 * already owns it.
	 */
	if (console && !monitor_has_bootdev(mon)) {
		if (!strcmp(console, "wb"))
			c |= 0x08;
		else if (strcmp(console, "ss1") != 0) {
			fprintf(stderr, "hwsim: unknown console: %s\n", console);
			exit(1);
		}
	}
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
	jv *sims = json_get(db, "sims");
	jv *sim;

	if (!sims || !(sim = json_get(sims, name))) {
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
	jv *ms = json_get(db, "mediasets");
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
		jv *vols = json_get(ms->v[i], "volumes");
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
	jv *sims = json_get(db, "sims");
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
	jv *sims = json_get(db, "sims");
	jv *sim;
	const char *root, *swap;
	const char *mon, *l1, *j2;
	int major, minor;

	if (!sims || !(sim = json_get(sims, name))) {
		fprintf(stderr, "hwsim: no such sim: %s\n", name);
		exit(1);
	}
	json_print(stdout, sim, 0);
	fputc('\n', stdout);

	mon = boot_field(sim, "monitor");
	l1 = boot_field(sim, "level1");
	j2 = boot_field(sim, "j2");
	root = json_str(sim, "root");
	swap = json_str(sim, "swap");
	printf("# root %s", root ? root : "-");
	if (root && parse_dev(root, &major, &minor) == 0)
		printf(" = %d/%d", major, minor);
	printf("\n# swap %s", swap ? swap : "-");
	if (swap && parse_dev(swap, &major, &minor) == 0)
		printf(" = %d/%d", major, minor);
	printf("\n");
	if (mon)
		printf("# -b %s\n", mon);
	if (l1)
		printf("# -M %s\n", l1);
	if (j2)
		printf("# -j %s\n", j2);
	printf("# -c 0x%x\n", pack_config(sim));
	if (json_str(sim, "wiring"))
		printf("# -w %s\n", json_str(sim, "wiring"));
}

static void
v_format(const char *root, const char *name)
{
	jv *sim = sim_get(name);
	jv *vols = json_get(sim, "volumes");
	char *mnix = resolve_path(root, "src/tools/mnix");
	int i;

	for (i = 0; vols && i < vols->n; i++) {
		jv *v = vols->v[i];
		const char *geom = json_str(v, "geometry");
		const char *img = json_str(v, "image");
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
	jv *vols = json_get(sim, "volumes");
	char *mnix = resolve_path(root, "src/tools/mnix");
	int i;

	for (i = 0; vols && i < vols->n; i++) {
		jv *v = vols->v[i];
		const char *geom = json_str(v, "geometry");
		const char *img = json_str(v, "image");
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
apply_files(const char *root, jv *v);

static void
v_mkfs(const char *root, const char *name)
{
	jv *sim = sim_get(name);
	jv *vols = json_get(sim, "volumes");
	char *mnix = resolve_path(root, "src/tools/mnix");
	char *bootimg = resolve_path(root, "src/micronix/stand/boot/bootimg");
	int i;

	for (i = 0; vols && i < vols->n; i++) {
		jv *v = vols->v[i];
		const char *img = json_str(v, "image");
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
		apply_files(root, v);
	}
}

/*
 * A volume's file overrides.  Each names a path in the image and the
 * source file to place there, from anywhere in the tree, so a machine
 * can be built with a particular .login or /etc/rc instead of whatever
 * the tree installs.  mnix write creates or replaces the destination.
 */
static void
apply_files(const char *root, jv *v)
{
	const char *img = json_str(v, "image");
	jv *files = json_get(v, "files");
	char *mnix = resolve_path(root, "src/tools/mnix");
	int i;

	if (!img)
		return;
	for (i = 0; files && i < files->n; i++) {
		jv *f = files->v[i];
		const char *path = json_str(f, "path");
		const char *src = json_str(f, "source");
		char *av[8];

		if (!path || !src) {
			fprintf(stderr,
			    "hwsim: %s: file override %d needs path and source\n",
			    img, i);
			exit(1);
		}
		av[0] = mnix;
		av[1] = "-f";
		av[2] = resolve_path(root, img);
		av[3] = "write";
		av[4] = resolve_path(root, src);
		av[5] = (char *) path;
		av[6] = NULL;
		runv(av);
	}
}

static void
v_files(const char *root, const char *name)
{
	jv *sim = sim_get(name);
	jv *vols = json_get(sim, "volumes");
	int i;

	for (i = 0; vols && i < vols->n; i++)
		apply_files(root, vols->v[i]);
}

static void
v_boot(const char *root, const char *name, int debug)
{
	jv *sim = sim_get(name);
	jv *vols = json_get(sim, "volumes");
	const char *emu = json_str(sim, "emulator");
	const char *mon = boot_field(sim, "monitor");
	const char *level1 = boot_field(sim, "level1");
	const char *j2 = boot_field(sim, "j2");
	const char *kernel = json_str(sim, "kernel");
	const char *wiring = json_str(sim, "wiring");
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

	if (!mon) {
		fprintf(stderr, "hwsim: %s: no monitor rom (boot.monitor)\n", name);
		exit(1);
	}
	av[n++] = simbin;
	if (!debug)
		av[n++] = "-H";			/* headless, like dist_boot */
	av[n++] = "-b";
	av[n++] = resolve_path(root, mon);
	if (level1) {
		av[n++] = "-M";
		av[n++] = resolve_path(root, level1);
	}
	if (j2) {
		if (strcmp(j2, "ab") != 0 && strcmp(j2, "bc") != 0) {
			fprintf(stderr, "hwsim: %s: boot.j2 is ab or bc\n", name);
			exit(1);
		}
		av[n++] = "-j";
		av[n++] = (char *)j2;
	}
	av[n++] = "-c";
	av[n++] = itoh(pack_config(sim));
	if (wiring) {
		av[n++] = "-w";
		av[n++] = (char *)wiring;
	}
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
		const char *ctl = json_str(v, "controller");
		const char *img = json_str(v, "image");
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
		sprintf(wiring, "%s%ld:%s", ctl, json_num(v, "unit"), imgpath);
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
	jv *vols = json_get(sim, "volumes");
	jv *vol, *x;

	if (!vols || vols->type != J_ARR) {
		vols = json_new(J_ARR);
		json_push(sim, strdup("volumes"), vols);
	}
	vol = json_new(J_OBJ);
	x = json_new(J_STR);
	x->str = strdup(args[0]);
	json_push(vol, strdup("controller"), x);
	x = json_new(J_NUM);
	x->num = atol(args[1]);
	json_push(vol, strdup("unit"), x);
	x = json_new(J_STR);
	x->str = strdup(args[2]);
	json_push(vol, strdup("geometry"), x);
	x = json_new(J_STR);
	x->str = strdup(args[3]);
	json_push(vol, strdup("image"), x);
	json_push(vols, NULL, vol);
	save_db();
	printf("hwsim: added %s:%s to %s\n", args[0], args[1], name);
}

static void
v_del(const char *name, const char *tok)
{
	jv *sim = sim_get(name);
	jv *vols = json_get(sim, "volumes");
	char ctl[32];
	int unit;
	int i, found = -1;

	if (sscanf(tok, "%31[^:]:%d", ctl, &unit) != 2) {
		fprintf(stderr, "hwsim: bad volume: %s (want controller:unit)\n",
		    tok);
		exit(1);
	}
	for (i = 0; vols && i < vols->n; i++) {
		const char *c = json_str(vols->v[i], "controller");
		if (c && strcmp(c, ctl) == 0 && json_num(vols->v[i], "unit") == unit) {
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
	    "  files <sim>        place each volume's file overrides\n"
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
	if (strcmp(verb, "files") == 0) {
		if (!name)
			usage();
		v_files(root, name);
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
