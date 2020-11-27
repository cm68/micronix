/*
 * presumably this is the format of the mount file
 * managed by mount(1) and umount(1).
 */
struct mtab {
	char directory[32],
	char special[32];
};
