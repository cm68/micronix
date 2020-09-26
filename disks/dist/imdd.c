int fd = 0;
unsigned char c;
int secs;
char buf[65536];

unsigned char pchars[16];
int pcol;

dp()
{
        int i;
        char c;

        for (i = 0; i < pcol; i++) {
                c = pchars[i];
                if ((c <= 0x20) || (c >= 0x7f)) c = '.';
                printf("%c", c);
        }
        printf("\n");
}

hexdump(char *addr, unsigned short len)
{
        int i;
        pcol = 0;
	int k;
	
	k = 0;
        while (len) {
                if (pcol == 0) printf("%04x: ", k);
                printf("%02x ", pchars[pcol] = addr[k++]);
                len--;
                if (pcol++ == 15) {
                        dp();
                        pcol = 0;
                }
        }
        if (pcol != 0) {
                for (i = pcol; i < 16; i++) printf("   ");
                dp();
        }
}

void
get_byte()
{
	int i;

	i = read(fd, &c, 1);
	if (i <= 0) exit(1);
}

void
out_byte(char *label)
{
	get_byte();
	printf("%s: %d %x\n", label, c, c);
}


void
secmap(char *label)
{
	int i;

	printf("%s: ", label);
	for (i = 0; i < secs; i++) {
		get_byte();
		printf("%d ", c);
	}
	printf("\n");
}

int
do_track()
{
	int secsize;
	int i;
	int s;
	int head;

	out_byte("mode");
	out_byte("cyl");
	out_byte("head"); head = c;
	out_byte("nsec"); secs = c;
	out_byte("size"); secsize = 0x80 << c;
	printf("secsize: %d\n", secsize);
	secmap("secmap");
	if (head & 0x80) {
		secmap("cylmap");
	}
	if (head & 0x40) {
		secmap("headmap");
	}
	for (s = 0 ; s < secs; s++) {
		printf("sector %d data:\n", s);
		get_byte();
		if (c) {
			if (c == 2) {
				get_byte();
				for (i = 0; i < secsize; i++) {
					buf[i] = c;
				}
			} else {
				i = read(fd, &buf, secsize);
				if (i != secsize) {
					printf("lose\n");
					exit(2);
				}
			}
			hexdump(buf, secsize);
		} else {
			printf("absent\n");
		}
	}
	return 1;
}

main(int argc, char **argv)
{
	int i;

	/* output header */
	while (1) {
		get_byte();
		if (c == 0x1a) break;
		write(1, &c, 1);
	}
	if (argc < 2) {
	/* output tracks */
	while (do_track()) 
		;
	}
	exit (0);
}

