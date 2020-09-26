/*
 * reg.h 
 */

/*
 * The Decision trap firmware saves the trapped
 * process' registers in the following structure.
 * The address of this structure is given in uhdr.8.
 */
struct reg
{
    UCHAR task;
    UCHAR mask;
    UINT pc;
    UINT sp;
    UINT af;
    UINT bc;
    UINT de;
    UINT hl;
    UINT zir;
    UINT zix;
    UINT ziy;
    UINT zaf;
    UINT zbc;
    UINT zde;
    UINT zhl;
} r;

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
