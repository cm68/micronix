/*
 * extract a field value from the instruction
 */
#include "dis.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/errno.h>
#include <string.h>

int line;
char lbuf[255];
FILE *archfile;

int
gripe(char *f, int i, char *err)
{
	fprintf(stderr, "file: %s line: %d: %s\n", f, i, err);
	return (-1);
}

/*
 * advance to the next significant thing
 */
void
bump(char **s)
{
	while (**s) {
		switch (**s) {
		case ' ': case '\t':
			break;
		case '\\':
			if ((*s)[1] == '\0') {
				if (fgets(lbuf, sizeof(lbuf), archfile) == 0) {
					(*s)++;
					return;
				}
				*s = lbuf;
				lbuf[strlen(lbuf) - 1] = 0;
				line++;
			}
			continue;
		default:
			return;
		}	
		(*s)++;
	}
}

void
getstuff(char **s, char *d)
{
	char quotechar = 0;

	bump(s);
	if (**s == '\"') quotechar = *(*s)++;

	while (**s) {
		if (quotechar) {
			if (**s == quotechar) {
				(*s)++;
				break;
			}
		} else if (**s == ',' || **s == '\t' || **s == '\n' || **s == ' ') {
			break;
		}
		*d++ = *(*s)++;
		*d = 0;
	}
	bump(s);
}

struct format *
lookup_format(char *name, int required)
{
	int i;
	struct format *fp = 0;

	for (i = 0; i < n_formats; i++) {
		if (strcmp(formats[i].name, name) == 0) {
			fp = &formats[i];
			break;
		}
	}
	if (required && !fp) {
		fprintf(stderr, "lookup_format: failed %s\n", name);
		fprintf(stderr, "line: %d %s\n", line, lbuf);
	}
	return fp;
}

struct field *
lookup_field(char *name, int required)
{
	int i;
	struct field *fp = 0;

	for (i = 0; i < n_fields; i++) {
		if (strcmp(fields[i].name, name) == 0) {
			fp = &fields[i];
			break;
		}
	}
	if (required && !fp) {
		fprintf(stderr, "lookup_field: failed %s\n", name);
		fprintf(stderr, "line: %d %s\n", line, lbuf);
	}
	return fp;
}

struct array *
lookup_array(char *name, int required)
{
	int i;
	struct array *ap = 0;

	for (i = 0; i < n_arrays; i++) {
		if (strcmp(arrays[i].name, name) == 0) {
			ap = &arrays[i];
			break;
		}
	}
	if (required && !ap) {
		fprintf(stderr, "lookup_array: failed %s\n", name);
		fprintf(stderr, "line: %d %s\n", line, lbuf);
	}
	return ap;
}

struct operand *
getoperands(char **s)
{
	char obuf[30];	
	struct operand *o;
	struct operand *oh = 0, *ot = 0;
	struct field *f;

	while (**s) {
		getstuff(s, obuf);
		f = lookup_field(obuf, 1);
		o = malloc(sizeof(*o));
		o->fld = f;
		if (oh) {
			ot->next = o;
			ot = o;
		} else {
			oh = ot = o;
		}
		if (**s != ',') break;
		(*s)++;
	}
	return (oh);
}

int
load_arch(char *arch)
{
	int i;	
	int state = 0;
	char name[32];
	int vals;
	char val[64][64];
	char archfilename[50];
	char *s;

	struct array *ap;
	struct field *fp;
	struct iclass *ip;
	struct format *fmp;
	struct operand *op;

	sprintf(archfilename, "%s.arch", arch);

	archfile = fopen(archfilename, "r");
	if (!archfile) {
		perror(archfilename);
		return (errno);
	}       

	/* count things so we can allocate fixed arrays */
	while (fgets(lbuf, sizeof(lbuf), archfile)) {
		s = lbuf;

		lbuf[strlen(lbuf) - 1] = 0;
		line++;

		bump(&s);

		if (*s == '#' || *s == '\n' || *s == '\0') {
			continue;
		}
		if (strcmp(s, "END") == 0) {
			break;
		}
		switch (state) {
		default:
		case 0:
			if (strcmp(s, "ARRAY") == 0) {
				state = 1;
				continue;
			}
			if (strcmp(s, "BIGENDIAN") == 0) {
				bigendian = 4;
				continue;
			}
			printf("line: %s\n", lbuf);
			return (gripe(archfilename, line, "malformed arch file"));
			break;

		case 1:		/* arrays */
			if (strcmp(s, "FORMAT") == 0) {
				state = 2;
				continue;
			}
			if (*s == '"') break;
			n_arrays++;
			break;

		case 2:		/* formats */
			if (strcmp(s, "FIELDS") == 0) {
				state = 3;
				continue;
			}
			n_formats++;
			break;

		case 3:		/* fields */
			if (strcmp(s, "INSTRUCTIONS") == 0) {
				state = 4;
				continue;
			}
			n_fields++;
			break;

		case 4:		/* instructions */
			n_instructions++;
			break;
		}
	}

	rewind(archfile);
	state = 0;

	/*
	 * let's allocate space for the architecture definition
	 */
	fmp = formats = malloc(n_formats * sizeof(struct format));
	fp = fields = malloc(n_fields * sizeof(struct field));
	ip = instructions = malloc(n_instructions * sizeof(struct iclass));
	ap = arrays = malloc(n_arrays * sizeof(struct array));
		
	/*
	 * now let's actually read in the architecture
	 */
	while (fgets(lbuf, sizeof(lbuf), archfile)) {
		s = lbuf;

		lbuf[strlen(lbuf) - 1] = 0;
		line++;

		bump(&s);

		if (*s == '#' || *s == '\n' || *s == '\0') {
			continue;
		}
		if (strcmp(s, "END") == 0) {
			break;
		}
		switch (state) {
			default:
			case 0:
				if (strcmp(s, "ARRAY") == 0) {
					state = 1;
					continue;
				}
				if (strcmp(s, "BIGENDIAN") == 0) {
					bigendian = 4;
					continue;
				}
				printf("line: %s\n", lbuf);
				return (gripe(archfilename, line, "malformed arch file"));
				break;

			case 1:		/* arrays */
				if (strcmp(s, "FORMAT") == 0) {
					state = 2;
					continue;
				}
				vals = 0;
				for (i = 0; i < 64; i++) { val[i][0] = 0; }
				getstuff(&s, name);
				while (*s) {
					getstuff(&s, val[vals++]);
					if (*s != ',') {
						break;
					}
					s++;
				}
				ap->name = strdup(name);
				ap->values = (char **)malloc(sizeof(char *) * vals);
				ap->count = vals;
				for (i = 0; i < vals; i++) {
					ap->values[i] = strdup(val[i]);
				}
				ap++;
				break;

			case 2:		/* formats */
				if (strcmp(s, "FIELDS") == 0) {
					state = 3;
					continue;
				}
				for (i = 0; i < 64; i++) { val[i][0] = 0; }
				getstuff(&s, name);
				getstuff(&s, val[0]);
				getstuff(&s, val[1]);
				getstuff(&s, val[2]);

				fmp->flags = 0;
				fmp->name = strdup(name);
				fmp->prefix = strdup(val[0]);
				fmp->suffix = strdup(val[1]);
				fmp->width = strtol(val[2], 0, 0);
				
				while (*s) {
					getstuff(&s, name);
					if (strcmp(name, "signed") == 0) {
						fmp->flags |= F_SIGNED;
					} else if (strcmp(name, "pcrel") == 0) {
						fmp->flags |= F_PCREL;
					} else if (strcmp(name, "hex") == 0) {
						fmp->flags |= F_HEX;
					} else if (strcmp(name, "xref") == 0) {
						fmp->flags |= F_XREF;
					} else {
						printf("unknown format option %s\n", name);
					}
					if (*s != ',') break;
					s++;
				}
				fmp++;
				break;

			case 3:		/* fields */
				if (strcmp(s, "INSTRUCTIONS") == 0) {
					state = 4;
					continue;
				}
				for (i = 0; i < 64; i++) { val[i][0] = 0; }
				if (sscanf(s, "%s %s %s %s %s %s", 
					name, val[0], val[1], val[2], val[3], val[4]) < 4) {
					return (gripe(archfilename, line, "malformed field"));
				}
				fp->name = strdup(name);
				fp->width = strtol(val[0], 0, 0);
				fp->bitoff = strtol(val[1], 0, 0);
				fp->destbit = strtol(val[2], 0, 0);
				fp->array = lookup_array(val[3], 0);
				fp->fmt = lookup_format(val[3], 0);
				if ((fp->array == 0) && (fp->fmt == 0)) {
					printf("field class %s unknown\n", val[3]);
				}
				fp->value = strtol(val[4], 0, 0);
				fp++;
				break;

			case 4:		/* instructions */
				getstuff(&s, name);
				ip->mask = strtol(name, 0, 0);
				getstuff(&s, name);
				ip->val = strtol(name, 0, 0);
				getstuff(&s, name);
				ip->ilen = strtol(name, 0, 0);
				if (*s == '\"') {
					getstuff(&s, name);
					ip->opcode = strdup(name);
					ip->opi = 0;
				} else {
					getstuff(&s, name);
					ip->opi = lookup_field(name, 1);
					ip->opa = ip->opi->array;
				}
				for (i = 0; i < NUM_OPS; i++) {
					ip->operand[i] = getoperands(&s);
				}
				ip++;
				break;
		}
	}
	fclose(archfile);

	if (!(verbose & V_TABLE)) {
		return (0);
	}

	printf("ARRAYS\n");
	for (ap = arrays; ap < &arrays[n_arrays]; ap++) {
		printf("%s:\n", ap->name);
		for (i = 0; i < ap->count; i++) {
			printf("\t%d: %s\n", i, ap->values[i]);
		}
	}
	printf("FIELDS\n");
	for (fp = fields; fp < &fields[n_fields]; fp++) {
		char *a, *f;
		a = "none";
		f = "none";
		if (fp->array) a = fp->array->name;
		if (fp->fmt) f = fp->fmt->name;
		printf("%s: %d(%d) %d %d %s %s\n", 
			fp->name, fp->bitoff, fp->width, fp->destbit, fp->value, a, f);
	}
	printf("FORMATS\n");
	for (fmp = formats; fmp < &formats[n_formats]; fmp++) {
		printf("%s: \"%s\" \"%s\" %d %s %s %s %s\n", 

			fmp->name, fmp->prefix, fmp->suffix, 
			fmp->width, 
			(fmp->flags & F_SIGNED) ? "signed " : "", 
			(fmp->flags & F_HEX) ? "hex " : "", 
			(fmp->flags & F_XREF) ? "xref " : "",
			(fmp->flags & F_PCREL) ? "pcrel" : "");
	}
	printf("INSTRUCTIONS\n");
	for (ip = instructions; ip < &instructions[n_instructions]; ip++) {
		printf("0x%08lx 0x%08lx %d\n", ip->mask, ip->val, ip->ilen);
		if (ip->opi) {
			printf("\t%s indexed by %s\n", ip->opa->name, ip->opi->name);
		} else {
			printf("\t%s\n", ip->opcode);
		}
		for (i = 0; i < NUM_OPS; i++) {
			printf("\toperand %d:\n", i);
			for (op = ip->operand[i]; op; op = op->next) {
				printf("\t\t%s\n", op->fld->name);
			}
		}
	}
	return (0);
}

/*
 * sign extend
 */
unsigned short
bias(unsigned short addr, unsigned short off)
{
    if (off & 0x80) {
        off |= 0xff00;
    }
    return (addr + off);
}

/*
 * grab a contiguous bit field from a possibly multibyte instruction
 * this is endian dependent, clearly.
 */
unsigned long
extract(struct field *fp, unsigned char *pc)
{
	unsigned long v = 0;
	char bitoff;
	char width;
	char bits;
	char destbit;
	int pcoff;
	static unsigned char mask[] = {0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f,0xff};
	
	v |= fp->value;

	destbit = fp->destbit;
	width = fp->width;
	bitoff = fp->bitoff;

	while (width) {
		bits = 8 - (bitoff % 8);
		if (width < bits)
			bits = width;

		pcoff = bitoff / 8;
		if (bigendian) {
			pcoff = bigendian - pcoff - 1;
		}
		v |= ((pc[pcoff] >> (bitoff % 8)) & mask[bits - 1]) << destbit;
		destbit += bits;
		width -= bits;
		bitoff += bits;
	}
	return (v);
}

unsigned long
getlong(unsigned char *p)
{
	unsigned long v = 0;
	int i;

	for (i = 0; i < 4; i++) {
		if (bigendian) {
			v |= p[i] << ((bigendian - i - 1) * 8);
		} else {
			v |= p[i] << (i * 8);
		}
	}
	return v;
}
