/*
 * format of the mount file managed by mount(1) and umount(1).
 * 
 * include/mtab.h
 * Changed: <2021-12-23 15:09:10 curt>
 */
struct mtab {
	char directory[32],
	char special[32];
};

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
