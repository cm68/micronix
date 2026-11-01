/*
 * micronix rom register save area structure after trap
 *
 * include/sys/reg.h
 * Changed: <2021-12-23 14:32:12 curt>
 */

/*
 * The Decision trap firmware saves the trapped process' registers in the
 * following structure. The address of this structure is given in uhdr.8.
 */
struct reg {
    UINT8 task;
    UINT8 mask;
    UINT16 pc;
    UINT16 sp;
    UINT16 af;
    UINT16 bc;
    UINT16 de;
    UINT16 hl;
    UINT16 zir;
    UINT16 zix;
    UINT16 ziy;
    UINT16 zaf;
    UINT16 zbc;
    UINT16 zde;
    UINT16 zhl;
} r;

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
