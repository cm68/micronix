/*
 * process times
 *
 * sys/times.h
 *
 * The four CPU-time counters.  On the simulator they are Z80 cycle
 * counts; on the kernel they are HERTZ ticks.  The usersim fills
 * tms_utime with the calling process's own cycles and tms_cutime with
 * the shared running total, so a caller that takes the difference
 * around a fork/wait sees the child's cycles.
 */

struct tms {
    INT32 tms_utime;     /* user time */
    INT32 tms_stime;     /* system time */
    INT32 tms_cutime;    /* user time of children */
    INT32 tms_cstime;    /* system time of children */
};
