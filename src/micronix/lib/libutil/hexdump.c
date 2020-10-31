/*
 * formatted memory dumper subroutines
 * we are exclusively interested in 16 bit offsets
 */
static char pchars[16] = 0;
static int pcol = 0;

/*
 * dump out the sanitized ascii
 */
dumpascii()
{
    int i;
    char c;

    for (i = 0; i < pcol; i++) {
        c = pchars[i];
        if ((c <= 0x20) || (c >= 0x7f))
            c = '.';
        printf("%c", c);
        if ((i % 4) == 3) { printf(" "); }
    }
    printf("\n");
}

hexdump(addr, len)
char *addr;
int len;
{
    int i;
    char c;

    pcol = 0;
	i = 0;
 
    while (len) {
        if (pcol == 0)
            printf("%04x: ", addr);
        c = addr[i++];
        pchars[pcol] = c;
        printf("%02x ", c & 0xff);
        if ((pcol % 4) == 3) { printf(" "); }
        len--;
        if (pcol++ == 15) {
            dumpascii();
            pcol = 0;
        }
    }
    if (pcol != 0) {
        for (i = pcol; i < 16; i++)
            printf("   ");
        dumpascii();
    }
}
