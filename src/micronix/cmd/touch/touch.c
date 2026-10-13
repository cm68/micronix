/*
 * touch - update the modification time for a file
 *
 * cmd/touch/touch.c
 *
 * Micronix has no utime(2), so the page's own recipe is the whole
 * program: read one character and write it back.  The write updates
 * the file's modification time without changing its data, because the
 * character written is the one that was read.  A file too short to
 * hold a character is left as it is - there is nothing to read back.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

main(argc, argv)
	int argc;
	char **argv;
{
	int i, fd;
	char c;
	int err;

	if (argc < 2) {
		fprintf(stderr, "usage: touch file ...\n");
		exit(1);
	}
	err = 0;
	for (i = 1; i < argc; i++) {
		fd = open(argv[i], O_RDWR);
		if (fd < 0) {
			fprintf(stderr, "touch: can't open %s\n", argv[i]);
			err = 1;
			continue;
		}
		if (read(fd, &c, 1) == 1) {
			lseek(fd, 0L, 0);
			write(fd, &c, 1);
		}
		close(fd);
	}
	exit(err);
}
