/*
 * STevie - ST editor for VI enthusiasts.    ...Tim Thompson...twitch!tjt...
 */

#ifdef linux
#define	INIT
#else
#define	INIT = 0
#endif

#define NULL 0
#define NORMAL 0
#define CMDLINE 1
#define INSERT 2
#define APPEND 3
#define FORWARD 4
#define BACKWARD 5
#define WORDSEP " \t\n()[]{},;:'\"-="

/*
 * these are tuned to just fill up a 64k micronix machine
 */
#define	MAXROWS		60
#define	MAXCOLS		80
#define FILELENG 16000

extern int State;
extern int Rows;
extern int Columns;
extern char *Realscreen;
extern char *Nextscreen;
extern char *Filename;
extern char *Filemem;
extern char *Filemax;
extern char *Fileend;
extern char *Topchar;
extern char *Botchar;
extern char *Curschar;
extern char *Insstart;
extern int Cursrow, Curscol, Cursvcol;
extern int Prenum;
extern int Debug;
extern int Changed;

extern char Redobuff[], Undobuff[], Insbuff[];
extern char *Uncurschar, *Insptr;
extern int Ninsert, Undelchars;

char *malloc(), *strchr(), *strcpy();

char *nextline(), *prevline(), *coladvance(), *ssearch();
char *fwdsearch(), *bcksearch();
