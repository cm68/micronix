/*
 * produce intel hex files
 */
#undef	DEBUG

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

unsigned char inbuf[16]
#ifdef DEBUG
	= { 0x21,0x00,0x00,0x39,0x22,0x15,0x02,0x31,0x57,0x02,0xCD,0xC1,0x01,0xFE,0xFF,0xC2 }
#endif
;
unsigned char outbuf[3];

int infd;
int outfd;
unsigned char sum;

void
eol()
{
	write(outfd, "\n", 1);
}

void
outhex(unsigned char v)
{
	sprintf(outbuf, "%02X", v);
	write(outfd, outbuf, 2);
	sum += v;
}

void
record(int addr, int len)
{
	int i;

	sum = 0;

	write(outfd, ":", 1);
	outhex(len);
	outhex((addr >> 8) & 0xff);
	outhex(addr & 0xff);
	outhex(0);
	for (i = 0; i < len; i++) {
		outhex(inbuf[i]);
	}
	outhex(-sum);
	eol();
}

void
dump(char *name)
{
	char *outname;
	int len;
	int i;

	int addr = 0x100;

	infd = open(name, O_RDONLY);
	if (infd < 0) {
		perror(name);
		return;
	}
	len = strlen(name);
	if (strcasecmp(name + len - 4, ".com") == 0) {
		len -= 4;
	}
	outname = malloc(len + 5);
	strncpy(outname, name, len);
	outname[len] = 0;
	strcat(outname, ".hex");
	outfd = creat(outname, 0777);
	if (outfd < 0) {
		perror(outname);
		close(infd);
		return;
	}

	while (1) {
		len = read(infd, inbuf, 16);
		if (len == 0)
			break;
		record(addr, len);
		addr += len;
	}
	write(outfd, ":0000000000", 11);
	eol();
	close(infd);
	close(outfd);
}

int
main(int argc, char **argv)
{
#ifdef DEBUG
	outfd = 1;
	record(0x100, 16);
#else
	while (--argc) {
		dump(*++argv);	
	}
#endif
}

