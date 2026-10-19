/*
 * The interrupt topology and the interrupt-line names.
 *
 * include/sys/ints.h
 *
 * A driver claims its interrupt with a name - INT_<device> - and hands
 * that name to inton()/intoff() and in its ovlhdr's line field.  Which
 * S-100 vectored line and which 8259 that name lands on is this header's
 * business, not the driver's, so a driver never has to know the topology.
 *
 * N8259 is the number of 8259 PICs:
 *   1 - one board, Wunderbus or MultIO, no slave (three lines, VI0-VI2)
 *   2 - a master and a slave (five lines: VI0-1 and VI3-5, VI2 the cascade)
 *
 * When N8259 == 2, N8259_POLLED says how the slave attaches:
 *   1 - Wunderbus + MultIO: the Wunderbus has no CAS pins, so the slave
 *       is polled - the cascade handler reads its ISR and EOIs it.  This
 *       is the only case that needs the polling.
 *   0 - two MultIOs: the slave CAS-cascades, its vector arrives through
 *       the master's acknowledge directly, and there is no polling.
 *
 * The build overrides the defaults with -DN8259=2 and -DN8259_POLLED=0.
 */
#ifndef N8259
#define N8259       1
#endif
#ifndef N8259_POLLED
#define N8259_POLLED 1
#endif

#if N8259 == 2
#define NOVLINE     5
/* The master IR line the slave's INT is wired to: IR2 (VI2), which is why
 * the five VI lines are VI0-1 on the master and VI3-5 on the slave. */
#define CASCADE_IR  2
#else
#define NOVLINE     3
#endif

/*
 * The interrupt-line names.  Under two PICs the five driver lines are
 * VI0-1 on the master and VI3-5 on the slave (VI2 is the slave's cascade);
 * under one PIC the historical sharing applies (HDCA rides VI0 with the
 * HD-DMA, NCR and IDE share VI2).
 */
#if N8259 == 2
#define INT_HDDMA   0       /* VI0, the master's IR0 */
#define INT_DJDMA   1       /* VI1, the master's IR1 */
#define INT_HDCA    0       /* VI0, shared with the HD-DMA (VI2 is the cascade) */
#define INT_NCR     2       /* VI3, the slave's IR0 */
#define INT_IDE     3       /* VI4, the slave's IR1 */
#define INT_NIC     4       /* VI5, the slave's IR2 (the spare DISK3 line) */
#else
#define INT_HDDMA   0
#define INT_DJDMA   1
#define INT_HDCA    0
#define INT_NCR     2
#define INT_IDE     2
#endif
