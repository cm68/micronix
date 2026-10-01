/*
 * file [-f list] file ...
 *
 * v7 file (usr/src/cmd/file.c), ported to micronix.
 *
 * cmd/file/file.c
 *
 * The classifier is v7's and keeps its shape: the two comment
 * skippers (ccom for C, ascom for assembler), the keyword table
 * probed at the current offset (lookup), the same four tables - c,
 * fort, as, asc - and the same letter statistics in english() for
 * deciding whether unadorned text is prose or just ascii.
 *
 * WHICH CLASSIFICATION THIS FOLLOWS.  The /bin/file on the 1.6
 * standalone is not this program.  Its strings give away a different
 * classifier with a different vocabulary:
 *
 *	Software Tools archive, Whitesmiths library, Relocatable
 *	module, Executable, Empty, Character special file, Block
 *	special file, Directory, Data, Text file, CP/M text file,
 *	Text processor input, Ratfor source file, Pascal source file,
 *	A-natural source file, C source file
 *
 * and no fortran, no assembler, no roff, and no English-versus-ascii
 * distinction at all.  It is a Whitesmiths rewrite - the machine's
 * own /bin/file, 3421 bytes on the disk - and its source is not in
 * extra/v7 and is not recoverable from the binary.
 *
 * This port follows v7's classification, because v7's source is what
 * there is to port and its behavior is what can be read against the
 * original line by line.  The 1982 binary's own types - A-natural
 * source, CP/M text, Ratfor source, Pascal source, Whitesmiths
 * library, Software Tools archive, and the rest - are not guessed at
 * here: a detector nobody wrote down is a detector nobody can check.
 * Where the two vocabularies meet they are only worded differently -
 * v7's "c program text" is the 1982 binary's "C source file", and
 * its "ascii text" and "English text" are both its "Text file".
 *
 * What the port had to say differently:
 *
 *	the special files	v7 read the device out of st_rdev with
 *			major() and minor().  micronix's stat has no
 *			st_rdev; a device's number is st_addr[0], the
 *			way ls (1) reads it, so that is what the two
 *			special cases print, and the two goto-linked
 *			arms of v7's switch are two plain cases.
 *
 *	the executable	v7 switched on the sixteen bit word at the
 *			head of the file and knew the PDP-11 a.out
 *			magics 0407, 0410 and 0411.  Nothing on this
 *			machine has an a.out header: ld writes the
 *			Whitesmiths object, whose first byte is 0x99
 *			(include/obj.h), so those three cases are gone
 *			and 0x99 is tested instead.  The object magic
 *			is one byte where all three of v7's were two,
 *			so it is tested ahead of the switch that keeps
 *			the archive magics; the two archive magics are
 *			the same numbers on both machines - see
 *			cmd/ar/ar.c, which reads them the same way -
 *			and stay in the switch.  "not stripped" is v7's
 *			own test for the same thing - a symbol table
 *			still present - read at the PDP-11's word 4 and
 *			here at the word at offset 2 of struct obj.
 *
 *	the execute bits	v7 asked for S_IEXEC and its two shifts,
 *			which is 0111.  This says 0111.
 *
 *	the troff test	v7 compared the second byte against '\357',
 *			which is 0357, 239.  buf is unsigned char here,
 *			so a byte with the top bit set is a plain number
 *			and the test reads the same as v7's.
 *
 *	the declaration	v7 wrote "int l = strlen(p)" inside a
 *			block, which is C99; it is at the top of main
 *			here, as ccc wants it.
 *
 *	the ascii test	english() tests each byte with "bp[j] <
 *			128".  bp is unsigned char, so the byte is
 *			0..255 and the test is the ascii one v7 meant,
 *			not a signed comparison against -128.
 *
 * Printing is v7's down to the tab after the name, and that tab has
 * to be written \t: ccc turns a tab typed inside a string literal
 * into a blank.
 *
 * vim: tabstop=8 shiftwidth=8 noexpandtab:
 */

#include <types.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * a device is printed the way ls (1) prints one: st_addr[0] holds the
 * number, major in the high byte - see cmd/ls/ls.c.
 */
#define	major(x)	(((x) >> 8) & 0xff)
#define	minor(x)	((x) & 0xff)

/*
 * the first byte of a Whitesmiths object file - include/obj.h
 */
#define	OBJMAGIC	0x99

/*
 * english() counts letters; the table it counts into is indexed by
 * the folded byte, so it only has to cover ascii.
 */
#define	NASC	128

int in;
int i  = 0;
unsigned char buf[512];
char *fort[] = {
	"function","subroutine","common","dimension","block","integer",
	"real","data","double",0};
char *asc[] = {
	"sys","mov","tst","clr","jmp",0};
char *c[] = {
	"int","char","float","double","struct","extern",0};
char *as[] = {
	"globl","byte","even","text","data","bss","comm",0};
int	ifile;

main(argc, argv)
char **argv;
{
	FILE *fl;
	register char *p;
	char ap[128];
	int l;

	if (argc>1 && argv[1][0]=='-' && argv[1][1]=='f') {
		if ((fl = fopen(argv[2], "r")) == NULL) {
			printf("Can't open %s\n", argv[2]);
			exit(2);
		}
		while ((p = fgets(ap, 128, fl)) != NULL) {
			l = strlen(p);
			if (l>0)
				p[l-1] = '\0';
			printf("%s:\t", p);
			type(p);
			if (ifile>=0)
				close(ifile);
		}
		exit(1);
	}
	while(argc > 1) {
		printf("%s:\t", argv[1]);
		type(argv[1]);
		argc--;
		argv++;
		if (ifile >= 0)
			close(ifile);
	}
}

type(file)
char *file;
{
	int j,nl;
	unsigned char ch;
	struct stat mbuf;

	ifile = -1;
	if(stat(file, &mbuf) < 0) {
		printf("cannot stat\n");
		return;
	}
	switch (mbuf.st_mode & S_IFMT) {

	case S_IFCHR:
		printf("character special (%d/%d)\n",
			major(mbuf.st_addr[0]), minor(mbuf.st_addr[0]));
		return;

	case S_IFDIR:
		printf("directory\n");
		return;

	case S_IFBLK:
		printf("block special (%d/%d)\n",
			major(mbuf.st_addr[0]), minor(mbuf.st_addr[0]));
		return;
	}

	ifile = open(file, 0);
	if(ifile < 0) {
		printf("cannot open\n");
		return;
	}
	in = read(ifile, buf, 512);
	if(in == 0){
		printf("empty\n");
		return;
	}
	/*
	 * The object magic is one byte and the archive magics are two,
	 * so the object test is not a case of the same switch - v7's
	 * three a.out magics were all two bytes and could share it.
	 */
	if (buf[0] == OBJMAGIC) {
		printf("executable");
		if((buf[2] | (buf[3]<<8)) != 0)
			printf(" not stripped");
		printf("\n");
		goto out;
	}
	switch (buf[0] | (buf[1]<<8)) {

	case 0177555:
		printf("old archive\n");
		goto out;

	case 0177545:
		printf("archive\n");
		goto out;
	}

	i = 0;
	if(ccom() == 0)goto notc;
	while(buf[i] == '#'){
		j = i;
		while(buf[i++] != '\n'){
			if(i - j > 255){
				printf("data\n");
				goto out;
			}
			if(i >= in)goto notc;
		}
		if(ccom() == 0)goto notc;
	}
check:
	if(lookup(c) == 1){
		while((ch = buf[i++]) != ';' && ch != '{')if(i >= in)goto notc;
		printf("c program text");
		goto outa;
	}
	nl = 0;
	while(buf[i] != '('){
		if(buf[i] == 0 || buf[i] >= 0200)
			goto notas;
		if(buf[i] == ';'){
			i++;
			goto check;
		}
		if(buf[i++] == '\n')
			if(nl++ > 6)goto notc;
		if(i >= in)goto notc;
	}
	while(buf[i] != ')'){
		if(buf[i++] == '\n')
			if(nl++ > 6)goto notc;
		if(i >= in)goto notc;
	}
	while(buf[i] != '{'){
		if(buf[i++] == '\n')
			if(nl++ > 6)goto notc;
		if(i >= in)goto notc;
	}
	printf("c program text");
	goto outa;
notc:
	i = 0;
	while(buf[i] == 'c' || buf[i] == '#'){
		while(buf[i++] != '\n')if(i >= in)goto notfort;
	}
	if(lookup(fort) == 1){
		printf("fortran program text");
		goto outa;
	}
notfort:
	i=0;
	if(ascom() == 0)goto notas;
	j = i-1;
	if(buf[i] == '.'){
		i++;
		if(lookup(as) == 1){
			printf("assembler program text");
			goto outa;
		}
		else if(buf[j] == '\n' && isalpha(buf[j+2])){
			printf("roff, nroff, or eqn input text");
			goto outa;
		}
	}
	while(lookup(asc) == 0){
		if(ascom() == 0)goto notas;
		while(buf[i] != '\n' && buf[i++] != ':')
			if(i >= in)goto notas;
		while(buf[i] == '\n' || buf[i] == ' ' || buf[i] == '\t')if(i++ >= in)goto notas;
		j = i-1;
		if(buf[i] == '.'){
			i++;
			if(lookup(as) == 1){
				printf("assembler program text");
				goto outa;
			}
			else if(buf[j] == '\n' && isalpha(buf[j+2])){
				printf("roff, nroff, or eqn input text");
				goto outa;
			}
		}
	}
	printf("assembler program text");
	goto outa;
notas:
	for(i=0; i < in; i++)if(buf[i]&0200){
		if (buf[0] == 0100 && buf[1] == 0357) {
			printf("troff output\n");
			goto out;
		}
		printf("data\n");
		goto out;
	}
	if (mbuf.st_mode&0111)
		printf("commands text");
	else
	    if (english(buf, in))
		printf("English text");
	else
	    printf("ascii text");
outa:
	while(i < in)
		if(buf[i++] > 127){
			printf(" with garbage\n");
			goto out;
		}
	printf("\n");
out:;
}

lookup(tab)
char *tab[];
{
	char r;
	int k,j,l;

	while(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n')i++;
	for(j=0; tab[j] != 0; j++){
		l=0;
		for(k=i; ((r=tab[j][l++]) == buf[k] && r != '\0');k++);
		if(r == '\0')
			if(buf[k] == ' ' || buf[k] == '\n' || buf[k] == '\t'
			    || buf[k] == '{' || buf[k] == '/'){
				i=k;
				return(1);
			}
	}
	return(0);
}

ccom()
{
	unsigned char cc;

	while((cc = buf[i]) == ' ' || cc == '\t' || cc == '\n')if(i++ >= in)return(0);
	if(buf[i] == '/' && buf[i+1] == '*'){
		i += 2;
		while(buf[i] != '*' || buf[i+1] != '/'){
			if(buf[i] == '\\')i += 2;
			else i++;
			if(i >= in)return(0);
		}
		if((i += 2) >= in)return(0);
	}
	if(buf[i] == '\n')if(ccom() == 0)return(0);
	return(1);
}

ascom()
{
	while(buf[i] == '/'){
		i++;
		while(buf[i++] != '\n')if(i >= in)return(0);
		while(buf[i] == '\n')if(i++ >= in)return(0);
	}
	return(1);
}

/*
 *	english - is this ascii, or is it English?
 *
 *	v7's letter statistics, unchanged.  The table is indexed by the
 *	folded byte, bp[j]|040, which turns a capital into the lower
 *	case letter below it; every byte is known to be ascii by the
 *	time this is called, so the index is in range.
 */
english (bp, n)
unsigned char *bp;
int n;
{
	int ct[NASC], j, vow, freq, rare;
	int badpun = 0, punct = 0;

	if (n<50) return(0); /* no point in statistics on squibs */
	for(j=0; j<NASC; j++)
		ct[j]=0;
	for(j=0; j<n; j++)
	{
		/*
		 * bp is unsigned char, so a byte at or above 128 is a
		 * plain number and "bp[j] < NASC" is the ascii test it
		 * looks like, not a signed comparison against -128.
		 */
		if (bp[j] < NASC)
			ct[bp[j]|040]++;
		switch (bp[j])
		{
		case '.':
		case ',':
		case ')':
		case '%':
		case ';':
		case ':':
		case '?':
			punct++;
			if ( j < n-1 &&
			    bp[j+1] != ' ' &&
			    bp[j+1] != '\n')
				badpun++;
		}
	}
	if (badpun*5 > punct)
		return(0);
	vow = ct['a'] + ct['e'] + ct['i'] + ct['o'] + ct['u'];
	freq = ct['e'] + ct['t'] + ct['a'] + ct['i'] + ct['o'] + ct['n'];
	rare = ct['v'] + ct['j'] + ct['k'] + ct['q'] + ct['x'] + ct['z'];
	if (2*ct[';'] > ct['e']) return(0);
	if ( (ct['>']+ct['<']+ct['/'])>ct['e']) return(0); /* shell file test */
	return (vow*5 >= n-ct[' '] && freq >= 10*rare);
}
