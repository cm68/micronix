/*
 * file region locking
 *
 * include/sys/lock.h
 * Changed: <2021-12-23 14:27:03 curt>
 */

#define NLOCK 10

/*
 * Structure of lock table entry for system level file region locking. We
 * only implement lock against lock and only immediate return on failure (no
 * queueing yet). The file member is NULL if the lock table entry is
 * unallocated. offset is the byte number of the first locked byte of the
 * file. size is the number of bytes in this locked regions. Regions may not
 * overlap. A region may not be multiply locked. Locks are inherited on fork.
 * Ex. child may clear parent's locks.
 */

struct lock {
    struct file *file;          /* file/user identity */
    UINT32 offset;              /* location of region */
    UINT size;                  /* size of region */
} llist[];

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
