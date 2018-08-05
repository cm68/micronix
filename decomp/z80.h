typedef unsigned char reg8_t;
typedef unsigned short reg16_t;
typedef union rp_u {
	reg16_t rp;
	struct {
		reg8_t hi;
		reg8_t lo;
	} rp;
} rp_t;

/*
 * the flags word
 */
#define	F_S	0x80
#define	F_Z	0x40
#define	F_X5	0x20
#define	F_H	0x10
#define	F_X3	0x08
#define	F_PV	0x04
#define	F_N	0x02
#define	F_C	0x01

/* this is the z80 cpu's state */
struct state {

	rp_t af;

	rp_t bc;
	rp_t de;
	rp_t hl

	/* alternate registers */
	rp_t af1;
	rp_t bc1;
	rp_t de1;
	rp_t hl1;

	rp_t ix;
	rp_t iy;
	reg16_t sp;
	reg8 ireg;
	reg8 rreg;

	char abank;
	char rbank;
	char iff1;
	char iff2;
	reg16_t pc;
};

