
char           *reg8[] = {"b", "c", "d", "e", "h", "l", "mem8(hl)", "a"};
char           *rp[] = {"bc", "de", "hl", "sp"};
char           *rp1[] = {"bc", "de", "hl", "af"};
char           *op8[] = {"+=", "+= CF +", "-=", "-= CF +", "&=", "^=", "|=", "=="};
char           *ixreg[] = {"ix", "iy"};
char           *cond[] = {"!Z", "Z", "!CF", "CF", "!PV", "PV", "!M", "M"};
char           *rot[] = {"rlc", "rrc", "rl", "rr", "sla", "sra", "sll", "srl"};

extern char	getmem(unsigned short a);

extern char    *const16(unsigned short a);
extern char    *const8(unsigned short a);
extern char    *addr(unsigned short a);
extern char    *signedoff(unsigned char a);
extern char    *reladdr(unsigned short pc, unsigned char a);
extern int is_special(char *addr);

int
do_call(unsigned short pc, unsigned short dest)
{
	unsigned char extra;

	extra = is_special(dest);
	return extra;
}

int
do_instr(char *buf, int pc)
{
    unsigned char   opcode;
    unsigned char   arg8;
    unsigned short  arg16;
    unsigned char   index;

    unsigned char   r2;		/* bc de hl sp,af */
    unsigned char   sr;		/* bcdehl.a */
    unsigned char   dr;		/* bcdehl.a */
    unsigned char   ir;		/* 0 = ix, 1 = iy */

    opcode = getmem(pc);
    arg8 = getmem(pc + 1);
    arg16 = getmem(pc + 1) + (getmem(pc + 2) << 8);

    sr = opcode & 0x7;
    dr = (opcode >> 3) & 0x7;
    r2 = (opcode >> 4) & 0x3;
    ir = (opcode >> 5) & 0x1;

    switch (opcode) {
    case 0x00:
	sprintf(buf, "nop;");
	return 1;
    case 0x07:
	sprintf(buf, "rlca;");
	return 1;
    case 0x08:
	sprintf(buf, "ex af,af';");
	return 1;
    case 0x0F:
	sprintf(buf, "rrca;");
	return 1;
    case 0x10:
	sprintf(buf, "b = b - 1; if NZ { jp %s };",
		reladdr(pc, arg8));
	return 2;
    case 0x17:
	sprintf(buf, "rla;");
	return 1;
    case 0x18:
	sprintf(buf, "jp %s;", reladdr(pc, arg8));
	return 2;
    case 0x1F:
	sprintf(buf, "rlca;");
	return 1;
    case 0x20:
	sprintf(buf, "if NZ { jp %s; };", reladdr(pc, arg8));
	return 2;
    case 0x27:
	sprintf(buf, "daa;");
	return 1;
    case 0x28:
	sprintf(buf, "if Z { jp %s; };", reladdr(pc, arg8));
	return 2;
    case 0x2F:
	sprintf(buf, "A ^= 0xff;");
	return 1;
    case 0x30:
	sprintf(buf, "if NC { jp %s; };", reladdr(pc, arg8));
	return 2;
    case 0x37:
	sprintf(buf, "CF = 1;");
	return 1;
    case 0x38:
	sprintf(buf, "if C { jp %s; };", reladdr(pc, arg8));
	return 2;
    case 0x3F:
	sprintf(buf, "CF ^= 1;");
	return 1;
    case 0x22:
	sprintf(buf, "mem16(%s) = hl;", addr(arg16));
	return 3;
    case 0x2a:
	sprintf(buf, "hl = mem16(%s);", addr(arg16));
	return 3;
    case 0x32:
	sprintf(buf, "mem8(%s) = a;", addr(arg16));
	return 3;
    case 0x3a:
	sprintf(buf, "a = mem8(%s);", addr(arg16));
	return 3;
    case 0xc3:
	sprintf(buf, "jp %s;", addr(arg16));
	return 3;
    case 0xc9:
	sprintf(buf, "ret;");
	return 1;
    case 0xcd:
	sprintf(buf, "call %s;", addr(arg16));
	return 3 + do_call(pc, arg16);
    case 0x76:
	sprintf(buf, "halt;");
	return 1;
    case 0xd3:
	sprintf(buf, "ld io8(%s),a;", const8(arg8));
	return 2;
    case 0xd9:
	sprintf(buf, "exx;");
	return 1;
    case 0xdb:
	sprintf(buf, "ld a,io8(%s);", const8(arg8));
	return 2;
    case 0xe3:
	sprintf(buf, "ex (sp),hl;");
	return 1;
    case 0xe9:
	sprintf(buf, "jp hl;");
	return 1;
    case 0xeb:
	sprintf(buf, "ex de,hl;");
	return 1;
    case 0xf3:
	sprintf(buf, "di;");
	return 1;
    case 0xf9:
	sprintf(buf, "sp = hl;");
	return 1;
    case 0xfb:
	sprintf(buf, "ei;");
	return 1;
    default:
	break;
    }

    if ((opcode & 0xcf) == 0x01) {
	sprintf(buf, "%s = %s;", rp[r2], const16(arg16));
	return 3;
    }
    if ((opcode & 0xcf) == 0x02) {
	sprintf(buf, "mem8(%s) = a;", rp[r2]);
	return 1;
    }
    if ((opcode & 0xcf) == 0x03) {
	sprintf(buf, "%s += 1;", rp[r2]);
	return 1;
    }
    if ((opcode & 0xc7) == 0x04) {
	sprintf(buf, "%s += 1;", reg8[dr]);
	return 1;
    }
    if ((opcode & 0xc7) == 0x05) {
	sprintf(buf, "%s -= 1;", reg8[dr]);
	return 1;
    }
    if ((opcode & 0xc7) == 0x06) {
	sprintf(buf, "%s = %s;", reg8[sr], const8(arg8));
	return 2;
    }
    if ((opcode & 0xcf) == 0x09) {
	sprintf(buf, "hl += %s;", rp[r2]);
	return 1;
    }
    if ((opcode & 0xcf) == 0x0a) {
	sprintf(buf, "a = mem8(%s);", rp[r2]);
	return 1;
    }
    if ((opcode & 0xcf) == 0x0b) {
	sprintf(buf, "%s -= 1;", rp[r2]);
	return 1;
    }
    if ((opcode & 0xc7) == 0xc0) {
	sprintf(buf, "if %s { ret; };", cond[dr]);
	return 1;
    }
    if ((opcode & 0xcf) == 0xc1) {
	sprintf(buf, "pop %s;", rp1[r2]);
	return 1;
    }
    if ((opcode & 0xc7) == 0xc2) {
	sprintf(buf, "if %s { jp %s; };", cond[dr], addr(arg16));
	return 3;
    }
    if ((opcode & 0xc7) == 0xc4) {
	sprintf(buf, "if %s { call %s; };",
		cond[dr], addr(arg16));
	return 3;
    }
    if ((opcode & 0xcf) == 0xc5) {
	sprintf(buf, "push %s;", rp1[r2]);
	return 1;
    }
    if ((opcode & 0xc7) == 0xc6) {
	sprintf(buf, "a %s %s;", op8[dr], const8(arg8));
	return 2;
    }
    if ((opcode & 0xc7) == 0xc7) {
	sprintf(buf, "call %s;", addr(dr * 8));
	return 1 + do_call(pc, arg16);
    }
    if ((opcode & 0xc0) == 0x40) {
	sprintf(buf, "%s = %s;", reg8[dr], reg8[sr]);
	return 1;
    }
    if ((opcode & 0xc0) == 0x80) {
	sprintf(buf, "a %s %s;", op8[dr], reg8[sr]);
	return 1;
    }
    if (opcode == 0xed) {
	opcode = getmem(pc + 1);
	r2 = (opcode >> 4) & 0x3;
	arg16 = getmem(pc + 2) + (getmem(pc + 3) << 8);
	if ((opcode & 0xcf) == 0x42) {
	    sprintf(buf, "hl -= CF + %s;", rp[r2]);
	    return 2;
	}
	if ((opcode & 0xcf) == 0x43) {
	    sprintf(buf, "mem16(%s) = %s;", addr(arg16), rp[r2]);
	    return 4;
	}
	if ((opcode & 0xcf) == 0x4a) {
	    sprintf(buf, "hl += CF + %s;", rp[r2]);
	    return 2;
	}
	if ((opcode & 0xcf) == 0x4b) {
	    sprintf(buf, "%s = mem16(%s);", rp[r2], addr(arg16));
	    return 4;
	}
	switch (opcode) {
	case 0xb0:
	    sprintf(buf, "mem8(de) = mem8(hl); de += 1; hl += 1 ; bc -= 1; sf16 bc ; if PV { jp %s }", addr(pc));
	    return 2;
	case 0x44:
	    sprintf(buf, "a = -a;");
	    return 2;
	}
	sprintf(buf, "----------undef ed %x", opcode);
	return 2;
    }
    if (opcode == 0xcb) {
	opcode = getmem(pc + 1);
	sr = opcode & 0x7;
	dr = (opcode >> 3) & 0x7;
	arg8 = 1 << dr;

	switch (opcode & 0xc0) {
	case 0x00:
	    break;
	case 0x80:
	    sprintf(buf, "%s &= %s;", reg8[sr], const8(~arg8));
	    return 2;
	case 0xc0:
	    sprintf(buf, "%s |= %s;", reg8[sr], const8(arg8));
	    return 2;
	case 0x40:
	    sprintf(buf, "Z = %s & %s;", reg8[sr], const8(arg8));
	    return 2;
	}
	if (dr != 6) {
	    sprintf(buf, "%s %s;", rot[dr], reg8[sr]);
	    return 2;
	}
    }
    if ((opcode & 0xdf) == 0xdd) {
	opcode = getmem(pc + 1);
	sr = opcode & 0x7;
	dr = (opcode >> 3) & 0x7;
	r2 = (opcode >> 4) & 0x3;
	index = getmem(pc + 2);
	arg16 = getmem(pc + 2) + (getmem(pc + 3) << 8);

	switch (opcode) {
	case 0x21:
	    sprintf(buf, "%s = %s;", ixreg[ir], addr(arg16));
	    return 4;
	case 0x22:
	    sprintf(buf, "mem16(%s) = %s;", addr(arg16), ixreg[ir]);
	    return 4;
	case 0x2a:
	    sprintf(buf, "%s = mem16(%s);", ixreg[ir], addr(arg16));
	    return 4;
	case 0x23:
	    sprintf(buf, "%s = %s + 1;", ixreg[ir], ixreg[ir]);
	    return 2;
	case 0x2b:
	    sprintf(buf, "%s = %s + 1;", ixreg[ir], ixreg[ir]);
	    return 2;
	case 0x34:
	    sprintf(buf, "mem8(%s%s) = mem8(%s%s) + 1;",
		    ixreg[ir], signedoff(index),
		    ixreg[ir], signedoff(index));
	    return 3;
	case 0x35:
	    sprintf(buf, "mem8(%s%s) = mem8(%s%s) - 1;",
		    ixreg[ir], signedoff(index),
		    ixreg[ir], signedoff(index));
	    return 3;
	case 0x36:
	    sprintf(buf, "mem8(%s%s) = %s;",
		    ixreg[ir], signedoff(index),
		    const8(getmem(pc + 3)));
	    return 4;
	case 0xe1:
	    sprintf(buf, "pop %s;", ixreg[ir]);
	    return 2;
	case 0xe3:
	    sprintf(buf, "ex (sp),%s;", ixreg[ir]);
	    return 2;
	case 0xe5:
	    sprintf(buf, "push %s;", ixreg[ir]);
	    return 2;
	case 0xe9:
	    sprintf(buf, "jp iy;", ixreg[ir]);
	    return 2;
	case 0xf9:
	    sprintf(buf, "sp = iy;", ixreg[ir]);
	    return 2;
	default:
	    break;
	}
	if (opcode == 0xcb) {
	    opcode = getmem(pc + 3);
	    sr = opcode & 0x7;
	    dr = (opcode >> 3) & 0x7;
	    index = getmem(pc + 2);
	    arg8 = 1 << dr;

	    switch (opcode & 0xc0) {
	    case 0x00:
		break;
	    case 0x80:
		sprintf(buf, "mem8(%s%s) &= %s;",
			ixreg[ir], signedoff(index),
			const8(~arg8));
		return 4;
	    case 0xc0:
		sprintf(buf, "mem8(%s%s) |= %s;",
			ixreg[ir], signedoff(index),
			const8(arg8));
		return 4;
	    case 0x40:
		sprintf(buf, "Z = mem8(%s%s) & %s;",
			ixreg[ir], signedoff(index),
			const8(arg8));
		return 4;
	    }
	    if (dr != 6) {
		sprintf(buf, "%s mem8(%s%s);",
			rot[dr], ixreg[ir], signedoff(index));
		return 4;
	    }
	}
	if ((opcode & 0xc7) == 0x46) {
	    sprintf(buf, "%s = mem8(%s%s);",
		    reg8[dr], ixreg[ir], signedoff(index));
	    return 3;
	}
	if ((opcode & 0xcf) == 0x09) {
	    sprintf(buf, "%s += %s;", ixreg[ir], rp[r2]);
	    return 2;
	}
	if ((opcode & 0xf8) == 0x70) {
	    if (sr != 6) {
		sprintf(buf, "mem8(%s%s) = %s;",
			ixreg[ir], signedoff(index), reg8[sr]);
		return 3;
	    }
	}
	if ((opcode & 0xc7) == 0x86) {
	    sprintf(buf, "a %s mem8(%s%s);",
		    op8[dr], ixreg[ir], signedoff(index));
	    return 3;
	}
	sprintf(buf, "-----------undef %s %x",
		ir ? "iy" : "ix", opcode);
	return 2;
    }
    sprintf(buf, "-----------undef %x", opcode);
    return 1;

}
