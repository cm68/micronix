/*
 * pretty generic table-driven disassembler
 */
#include <string.h>
#include <stdio.h>
#include "dis.h"

char xref[8192];

void
make_operand(struct inst *ip, int index, unsigned char *pc)
{
	struct operand *o;
	int vi;
	int val;
	struct format *fmt;
	
	ip->opvals[index] = ip->vals;
	for (o = ip->ic->operand[index]; o; o = o->next) {
		vi = ip->vals++;
		val = extract(o->fld, pc);
		if ((fmt = o->fld->fmt) != 0) {
			if (fmt->flags & F_PCREL) {
				if (fmt->flags & F_SIGNED) {
					if ((o->fld->width == 8) && (val & 0x80)) {
						val |= 0xff00;
					}
					if (val & 0x8000) {
						val |= 0xffff0000;
					}
				}
				val = ip->addr + ip->ic->ilen + val;
			}
			if (fmt->flags & F_XREF) {
				set_bit(xref, val);
			}
		}
		ip->values[vi] = val;
	}
}
 
int
lookup_inst(pc, ip)
unsigned char *pc;
struct inst *ip;
{
	struct iclass *icp;
	unsigned long op;
	int i;

	op = getlong(pc);

	if (verbose & V_MATCH) {
		printf("lookup_inst 0x%08lx, 0x%x, 0x%x\n", op, 
			(unsigned long)pc, (unsigned long)ip);
	}
	for (icp = instructions; icp < &instructions[n_instructions]; icp++) {
		if (verbose & V_MATCH) {
			printf("check 0x%08lx, 0x%08lx, 0x%08lx\n",
				icp->mask, icp->val, icp->mask & op);
		}
		if ((icp->mask & op) == icp->val) {
			ip->ic = icp;
			if (icp->opi) {
				ip->op = icp->opa->values[extract(icp->opi, pc)];
			} else {
				ip->op = icp->opcode;
			}
	
			ip->vals = 0;
			for (i = 0; i < NUM_OPS; i++) {
				if (icp->operand[i]) {
					make_operand(ip, i, pc);
				}
			}
			if (verbose & V_MATCH) {
				printf("hit %s\n", icp->opcode);
			}
			return icp->ilen;
		}
	}
	if (verbose & V_MATCH) {
		printf("table miss\n");
	}
	return (0);
}

int
decode_inst(unsigned char *pc, struct inst *ip)
{
	int ilen = 0;

	if (verbose & V_MATCH) {
		printf("decode_inst 0x%x: 0x%x\n", 
			(unsigned long)pc, (unsigned long)*pc);
	}
	ilen += lookup_inst(pc, ip);
	if (!ilen) {
		ip->op = "-UNDECODED-";
		ilen = 1;
	}
	if (verbose & V_MATCH) {
		printf("decode_inst ret %d\n", ilen);
	}
	return ilen;
}

/*
 * given the semantic description of an operand, output a human readable form
 */
void
fmt_operand(char *obuf, struct inst *ip, int opindex)
{
	struct ref *ref;
	struct operand *o;
	int v = ip->opvals[opindex];
	int val;
	char fbuf[20];
	char tbuf[20];
	char *sym;

	if (verbose & V_MATCH) {
		printf("fmt_operand %d\n", opindex);
	}

	for (o = ip->ic->operand[opindex]; o; o = o->next) {
		if (verbose & V_MATCH) {
			printf("fmt_operand %s", o->fld->name);
			if (o->fld->fmt) {
				printf(" fmt: %s", o->fld->fmt->name);
			}
			if (o->fld->array) {
				printf(" array: %s", o->fld->array->name);
			}
			printf("\n");
		}

		/* if there's a prefix, send it */
		if (o->fld->fmt && o->fld->fmt->prefix) {
			if (verbose & V_MATCH) {
				printf("fmt_operand prefix %s", o->fld->fmt->prefix);
			}
			strcat(obuf, o->fld->fmt->prefix);
		}
		val = ip->values[v];
		if (o->fld->array) {
			strcat(obuf, o->fld->array->values[val]);
		}
		if (o->fld->fmt) {
			if (o->fld->fmt->width != 0) {
				if ((o->fld->fmt->flags & (F_SIGNED|F_PCREL)) == F_SIGNED) {
					if ((o->fld->fmt->width == 8) && (val & 0x80)) {
						strcat(obuf, "-");
						val |= 0xffffff00;	
						val = -val;
					} else {
						strcat(obuf, "+");
					}
				}
				if (o->fld->fmt->flags & F_HEX) {
					if ((ref = getref((ip->addr+(o->fld->bitoff/8))))) {
						if (ref->bias) {
							sprintf(tbuf, "%s+%d", ref->sym->name, ref->bias);
						} else {
							sprintf(tbuf, "%s", ref->sym->name);
						}
//						|| (sym = getsymname(val))) 
					} else {
						sprintf(fbuf, "0x%%0%dx", (o->fld->fmt->width + 3) / 4);
						sprintf(tbuf, fbuf, val);
					}
				} else {
					sprintf(tbuf, "%d", val);
				}
				strcat(obuf, tbuf);
			}
		}
		v++;
	}
	/* if there's a suffix, send it */
	for (o = ip->ic->operand[opindex]; o; o = o->next) {
		if (o->fld->fmt && o->fld->fmt->suffix) {
			if (verbose & V_MATCH) {
				printf("fmt_operand suffix %x %s", 
					(unsigned long)o, o->fld->fmt->suffix);
			}
			strcat(obuf, o->fld->fmt->suffix);
		}
	}
	if (verbose & V_MATCH) {
		printf("fmt_operand ret\n");
	}
}

void
fmt_inst(char *obuf, struct inst *ip)
{
	int i;

	if (verbose & V_MATCH) {
		printf("fmt_inst 0x%08x %s\n", (unsigned long)ip, ip->op);
	}

	strcpy(obuf, ip->op);
	strcat(obuf, " ");
	if (ip->ic) {
		for (i = 0; i < NUM_OPS; i++) {
			if (ip->ic->operand[i]) {
				if (i > 0) {
					strcat(obuf, ",");
				}
				fmt_operand(obuf, ip, i);
			}
		}
	}
}

void
disas(unsigned char *buf, int startaddr, int length)
{
	struct ref *ref;
	char *sym;
	char hex[80];
	char inst[80];
	char ascii[10];
	int i;
	int op;
	struct inst ip;
	int offset = startaddr;

	if (verbose) {
		printf("dis: %x for %d\n", startaddr, length);
	}

	if (!verbose) {
		/* run through once to do cross-reference */
		for (offset = startaddr; offset < length; ) {
			bzero(&ip, sizeof(ip));
			ip.op = "-";
			ip.addr = offset;
			ip.len = decode_inst(&buf[offset], &ip);
			offset += ip.len;
		}
	}

	/* and again to print */
	for (offset = startaddr; offset < length; ) {
		char len;

		sym = getsymname(offset);
		if (sym) {
			printf("%s:\n", sym);
		}

		if (verbose || bit_set(xref, offset)) {
			sprintf(hex, "%04x: ", offset);
		} else {
			sprintf(hex, "      ");
		}

		if (is_code(offset)) {
			bzero(&ip, sizeof(ip));
			ip.op = "-";
			ip.addr = offset;
			ip.len = decode_inst(&buf[offset], &ip);
			len = ip.len;
			fmt_inst(inst, &ip);
		} else {
			ref = getref(offset);
			if (ref) {
				if (ref->bias) {
					sprintf(inst, "DW %s+%d", ref->sym, ref->bias);
				} else {
					sprintf(inst, "DW %s", ref->sym);
				}
				len = 2;
			} else {
				sprintf(inst, "DB 0x%x", buf[offset]);
				len = 1;
			}
		}

		/* opcode bytes */
		for (i = 0; i < len; i++) {
			op = buf[offset + i];
			sprintf(&hex[6 + i * 3], "%02x ", op);
			ascii[i] = ((op > ' ') && (op < 0x7f))  ? op : '.';
			ascii[i+1] = '\0';
		}

		/* fills */
		for (i = len; i < 6; i++) {
			hex[6 + i * 3] = ' ';
			hex[7 + i * 3] = ' ';
			hex[8 + i * 3] = ' ';
			hex[9 + i * 3] = '\0';
			ascii[i] = ' ';
			ascii[i+1] = '\0';
		}
		for (i = strlen(inst); i < 30; i++) {
			strcat(inst, " ");
		}

		// printf("%s %s %s\n", hex, ascii, inst);
		printf("\t%s ; %s %s\n", inst, hex, ascii);
		offset += len;
	}
}
