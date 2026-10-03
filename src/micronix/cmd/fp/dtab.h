/*
 * the dump table, /etc/dtab - one record per dumped directory, holding
 * the device, inode and date of the last incremental dump.
 *
 * cmd/fp/dtab.h
 */

struct dtab {
    int device, inode;
    long date;
};
