#include <fcntl.h>

#pragma pack(1)

struct head {
	unsigned short magic;
	unsigned char thing1;
	unsigned int bytesex;
	unsigned short thing2;
	char arch[4];
};

struct seg {
	unsigned short len;
	unsigned char flag;
	unsigned int addr;
	char name[5];
};

int
dump_head(char *s)
{
	struct head *h = (struct head *)s;

	printf("magic: 0x%04x thing1: 0x%04x bytesex: 0x%08x thing2: 0x%04x arch: %s\n", 
		h->magic, h->thing1, h->bytesex, h->thing2, h->arch);

	return (sizeof(*h));
}

int
dump_seg(unsigned char *s, int limit)
{
	struct seg *h = (struct seg *)s;
	int i;

	printf("name: %s len: %d flag: %x addr: %x\n", h->name, h->len, h->flag, h->addr);
	s += sizeof(*h);
	if (h->len > limit) h->len = limit;

	for (i = 0; i <= 2 + h->len - sizeof(*h); i++) {
		printf("%02x ", *s++);
		if ((i % 16) == 15) printf("\n");
	}
	printf("\n");

	return (sizeof(*h));
}

unsigned char buf[65536];

do_object(char *fn)
{
	int fd;
	int i;
	int o;

	fd = open(fn, O_RDONLY);
	if (fd < 0) {
		perror(fn);
		exit(-1);
	}
	i = read(fd, buf, sizeof(buf));

	printf("%s:\n", fn);

	o = 0;
	o += dump_head(&buf[o]);

	printf("o: %d\n", o);
	while (o < i) {
		o += dump_seg(&buf[o], i - o);
		printf("o: %d\n", o);
	}

	close(fd);
}

main(int argc, char **argv)
{
	if (argc < 2) {
		printf("lose %d\n", argc);
		exit(1);
	}
	argv++;
	while (--argc) {
		do_object(*argv++);
	}
	exit(0);
}

