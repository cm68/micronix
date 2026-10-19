/*
 * hwsim/d1/vimap.h
 *
 * The S-100 vectored-interrupt wiring, in one place, so a different card
 * set is a re-jumper here rather than an edit in every driver.  Two 8259s
 * share the bus: the master on the base Wunderbus (0x48) and the slave on
 * the second MultIO (0x40).  The master owns SINTA and the slave is polled,
 * so the wiring is: VI0..VI1 are the master's IR0..IR1, VI3..VI5 are the
 * slave's IR0..IR2, and VI2 is the slave's cascade line.
 *
 * The slave's INT is wired to the master's cascade line
 * (SLAVE_CASCADE_IR), so the master vectors the slave's interrupt and the
 * cascade handler finds the actual slave line by polling its ISR.
 */

/* which S-100 VI line each simulated controller drives (the -w profiles in
 * s100.c are the live copy; these names document the default two-PIC wiring) */
#define VI_HDDMA  0
#define VI_DJDMA  1
#define VI_HDCA   0               /* shares VI0 with the HDDMA */
#define VI_NCR    3
#define VI_IDE    4
#define VI_DISK3  5               /* CompuPro DISK 3 / nic, the spare slave line */

/*
 * which VI lines each PIC monitors.  The master's IR0..IR2 are the low
 * three lines, the slave's are the next three - so a VI line's bit (0..7)
 * in vi_lines tells which PIC's irr it feeds: bits under MASTER_VI_MASK
 * are the master, bits under SLAVE_VI_MASK the slave.
 */
#define MASTER_VI_MASK  0x03   /* VI0..VI1, the master's IR0..IR1 (VI2 is the cascade) */
#define SLAVE_VI_MASK   0x38   /* VI3..VI5, the slave's IR0..IR2 */

/*
 * The master IR line the slave's INT is wired to.  The slave yanks on
 * VI2, which is the master's IR2; it cascades there in both the polled
 * and the CAS-cascaded arrangement.
 */
#define SLAVE_CASCADE_IR  2
