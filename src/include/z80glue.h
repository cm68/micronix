/*
 * shared chips Z80 glue
 *
 * include/z80glue.h
 *
 * The register accessors the debugger, the gui and the machine drivers
 * use to reach the CPU.  Each simulator owns its chips z80 instance -
 * usersim and hwsim/d1 each do "#define CHIPS_IMPL" and "#include
 * "z80.h"" in one of their own files - and this header is how the
 * shared code in lib/ reaches that instance through the extern below.
 *
 * The accessors used to live in lib/z80_shim.c, back when the emulator
 * was a pluggable backend behind sim.h.  They are thin inline wrappers
 * over chips' accessors now, and nothing is lost by inlining them.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include "z80.h"

/*
 * This header is included after sim.h, whose byte/word typedefs and
 * reg8/reg16 enums the accessors below speak in.  It is not included
 * here because sim.h has no include guard and would redeclare them.
 */

/*
 * The CPU instance, defined by whichever simulator is being linked.
 */
extern z80_t z80;

/*
 * The CPU run/init entry points, defined by whichever simulator is
 * being linked (usersim and hwsim/d1 each own one).
 */
void z80_init();
void z80_run();

/*
 * The machine/bus state shared between the CPU and the drivers.
 */
extern unsigned long long sim_cycles;	/* the clock everything is timed by */
extern byte status;			/* S_M1 | S_HLTA | S_INTA */
extern int int_pin;			/* Z80 INT input, set by the interrupt controller */
extern int nmi_pin;			/* Z80 NMI input */
extern byte control;			/* C_NMI | C_INT | C_RESET */
extern byte unhalt;			/* leave HALT on the next tick */

static inline byte
z80_get_reg8(enum reg8 r8)
{
	switch (r8) {
	case iff_reg:
		return (z80_iff1(&z80) ? IFF1 : 0) | (z80_iff2(&z80) ? IFF2 : 0);
	case a_reg:	return z80_a(&z80);
	case f_reg:	return z80_f(&z80);
	case b_reg:	return z80_b(&z80);
	case c_reg:	return z80_c(&z80);
	case d_reg:	return z80_d(&z80);
	case e_reg:	return z80_e(&z80);
	case h_reg:	return z80_h(&z80);
	case l_reg:	return z80_l(&z80);
	case a1_reg:	return (z80_af_(&z80) >> 8) & 0xff;
	case f1_reg:	return z80_af_(&z80) & 0xff;
	case b1_reg:	return (z80_bc_(&z80) >> 8) & 0xff;
	case c1_reg:	return z80_bc_(&z80) & 0xff;
	case d1_reg:	return (z80_de_(&z80) >> 8) & 0xff;
	case e1_reg:	return z80_de_(&z80) & 0xff;
	case h1_reg:	return (z80_hl_(&z80) >> 8) & 0xff;
	case l1_reg:	return z80_hl_(&z80) & 0xff;
	case i_reg:	return z80_i(&z80);
	case r_reg:	return z80_r(&z80);
	case im_reg:	return z80_im(&z80);
	case status_reg: return status;
	default:	return 0;
	}
}

static inline void
z80_set_reg8(enum reg8 r8, byte v)
{
	switch (r8) {
	case a_reg:	z80_set_a(&z80, v);	break;
	case f_reg:	z80_set_f(&z80, v);	break;
	case b_reg:	z80_set_b(&z80, v);	break;
	case c_reg:	z80_set_c(&z80, v);	break;
	case d_reg:	z80_set_d(&z80, v);	break;
	case e_reg:	z80_set_e(&z80, v);	break;
	case h_reg:	z80_set_h(&z80, v);	break;
	case l_reg:	z80_set_l(&z80, v);	break;
	case a1_reg:	z80_set_af_(&z80, (z80_af_(&z80) & 0xff) | (v << 8));	break;
	case f1_reg:	z80_set_af_(&z80, (z80_af_(&z80) & 0xff00) | v);	break;
	case b1_reg:	z80_set_bc_(&z80, (z80_bc_(&z80) & 0xff) | (v << 8));	break;
	case c1_reg:	z80_set_bc_(&z80, (z80_bc_(&z80) & 0xff00) | v);	break;
	case d1_reg:	z80_set_de_(&z80, (z80_de_(&z80) & 0xff) | (v << 8));	break;
	case e1_reg:	z80_set_de_(&z80, (z80_de_(&z80) & 0xff00) | v);	break;
	case h1_reg:	z80_set_hl_(&z80, (z80_hl_(&z80) & 0xff) | (v << 8));	break;
	case l1_reg:	z80_set_hl_(&z80, (z80_hl_(&z80) & 0xff00) | v);	break;
	case i_reg:	z80_set_i(&z80, v);	break;
	case r_reg:	z80_set_r(&z80, v);	break;
	case status_reg:
		if (v == 0 && (status & S_HLTA))
			unhalt = 1;
		break;
	default:	break;
	}
}

static inline word
z80_get_reg16(enum reg16 r16)
{
	switch (r16) {
	case bc_reg:	return z80_bc(&z80);
	case de_reg:	return z80_de(&z80);
	case hl_reg:	return z80_hl(&z80);
	case bc1_reg:	return z80_bc_(&z80);
	case de1_reg:	return z80_de_(&z80);
	case hl1_reg:	return z80_hl_(&z80);
	case ix_reg:	return z80_ix(&z80);
	case iy_reg:	return z80_iy(&z80);
	case sp_reg:	return z80_sp(&z80);
	case pc_reg:	return z80_pc(&z80);
	default:	return 0;
	}
}

static inline void
z80_set_reg16(enum reg16 r16, word v)
{
	switch (r16) {
	case bc_reg:	z80_set_bc(&z80, v);	break;
	case de_reg:	z80_set_de(&z80, v);	break;
	case hl_reg:	z80_set_hl(&z80, v);	break;
	case bc1_reg:	z80_set_bc_(&z80, v);	break;
	case de1_reg:	z80_set_de_(&z80, v);	break;
	case hl1_reg:	z80_set_hl_(&z80, v);	break;
	case ix_reg:	z80_set_ix(&z80, v);	break;
	case iy_reg:	z80_set_iy(&z80, v);	break;
	case sp_reg:	z80_set_sp(&z80, v);	break;
	case pc_reg:	z80_set_pc(&z80, v);	break;
	default:	break;
	}
}
