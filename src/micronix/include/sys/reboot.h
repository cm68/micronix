/*
 * Arguments to the reboot system call.
 *
 * include/sys/reboot.h
 *
 * Mirrors the BSD RB_* flags (see 4.0BSD sys/reboot.h).  Only RB_HALT
 * is implemented; there is no bootstrap to jump back to yet, so an
 * RB_AUTOBOOT request still halts rather than pretending to reboot.
 */
#define RB_AUTOBOOT  0    /* boot normally */
#define RB_HALT      0x8  /* don't reboot, just halt */
