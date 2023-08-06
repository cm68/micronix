#include "tdef.h"
static char Sccsid[] "@(#)ni.c	1.3 of 4/26/77";
char obuf[OBUFSZ];
char *obufp obuf;
int r[NN] {'%','nl','yr','hp','ct','dn','mo','dy','dw','ln','dl','st','sb','c.'};
int pto 10000;
int pfrom 1;
int print 1;
char nextf[NS] "/usr/lib/tmac.xxxxx";
int nfi 14;
#ifdef NROFF
char termtab[NS] "/usr/lib/term/37";
int tti 14;
#endif
char suftab[] "/usr/lib/suftab";
int init 1;
int fc IMP;
int eschar '\\';
int pl 11*INCH;
int po PO;
int dfact 1;
int dfactd 1;
int res 1;
int smnt 4;
int ascii ASCII;
int ptid PTID;
char ptname[] "/dev/cat";
int lg LG;
int pnlist[NPN] {-1};
int *pnp pnlist;
int npn 1;
int npnflg 1;
int oldbits -1;
int xflg 1;
int dpn -1;
int totout 1;
int ulfont 1;
int ulbit 1<<9;
int tabch TAB;
int ldrch LEADER;
int xxx;
extern caseds(), caseas(), casesp(), caseft(), caseps(), casevs(),
casenr(), caseif(), casepo(), casetl(), casetm(), casebp(), casech(),
casepn(), tbreak(), caseti(), casene(), casenf(), casece(), casefi(),
casein(), caseli(), casell(), casens(), casemk(), casert(), caseam(),
casede(), casedi(), caseda(), casewh(), casedt(), caseit(), caserm(),
casern(), casead(), casers(), casena(), casepl(), caseta(), casetr(),
caseul(), caselt(), casenx(), caseso(), caseig(), casetc(), casefc(),
caseec(), caseeo(), caselc(), caseev(), caserd(), caseab(), casefl(),
done(), casess(), casefp(), casecs(), casebd(), caselg(), casehc(),
casehy(), casenh(), casenm(), casenn(), casesv(), caseos(), casels(),
casecc(), casec2(), caseem(), caseaf(), casehw(), casemc(), casepm(),
casecu(), casepi(), caserr(), caseuf(), caseie(), caseel(), casepc(),
caseht();
struct contab {
	int rq;
	int (*f)();
}contab[NM]{
	'ds',caseds,
	'as',caseas,
	'sp',casesp,
	'ft',caseft,
	'ps',caseps,
	'vs',casevs,
	'nr',casenr,
	'if',caseif,
	'ie',caseie,
	'el',caseel,
	'po',casepo,
	'tl',casetl,
	'tm',casetm,
	'bp',casebp,
	'ch',casech,
	'pn',casepn,
	'br',tbreak,
	'ti',caseti,
	'ne',casene,
	'nf',casenf,
	'ce',casece,
	'fi',casefi,
	'in',casein,
	'li',caseli,
	'll',casell,
	'ns',casens,
	'mk',casemk,
	'rt',casert,
	'am',caseam,
	'de',casede,
	'di',casedi,
	'da',caseda,
	'wh',casewh,
	'dt',casedt,
	'it',caseit,
	'rm',caserm,
	'rr',caserr,
	'rn',casern,
	'ad',casead,
	'rs',casers,
	'na',casena,
	'pl',casepl,
	'ta',caseta,
	'tr',casetr,
	'ul',caseul,
	'cu',casecu,
	'lt',caselt,
	'nx',casenx,
	'so',caseso,
	'ig',caseig,
	'tc',casetc,
	'fc',casefc,
	'ec',caseec,
	'eo',caseeo,
	'lc',caselc,
	'ev',caseev,
	'rd',caserd,
	'ab',caseab,
	'fl',casefl,
	'ex',done,
	'ss',casess,
	'fp',casefp,
	'cs',casecs,
	'bd',casebd,
	'lg',caselg,
	'hc',casehc,
	'hy',casehy,
	'nh',casenh,
	'nm',casenm,
	'nn',casenn,
	'sv',casesv,
	'os',caseos,
	'ls',casels,
	'cc',casecc,
	'c2',casec2,
	'em',caseem,
	'af',caseaf,
	'hw',casehw,
	'mc',casemc,
	'pm',casepm,
#ifdef NROFF
	'pi',casepi,
#endif
	'uf',caseuf,
	'pc',casepc,
	'ht',caseht,
};

/*
troff environment block
*/

int block 0;
int ics ICS;
int ic 0;
int icf 0;
int chbits 0;
int nmbits 0;
int apts PS;
int apts1 PS;
int pts PS;
int pts1 PS;
int font FT;
int font1 FT;
int sps SPS;
int spacesz SS;
int lss VS;
int lss1 VS;
int ls 1;
int ls1 1;
int ll LL;
int ll1 LL;
int lt LL;
int lt1 LL;
int ad 1;
int nms 1;
int ndf 1;
int fi 1;
int cc '.';
int c2 '\'';
int ohc OHC;
int tdelim IMP;
int hyf 1;
int hyoff 0;
int un1 -1;
int tabc 0;
int dotc '.';
int adsp 0;
int adrem 0;
int lastl 0;
int nel 0;
int admod 0;
int *wordp 0;
int spflg 0;
int *linep 0;
int *wdend 0;
int *wdstart 0;
int wne 0;
int ne 0;
int nc 0;
int nb 0;
int lnmod 0;
int nwd 0;
int nn 0;
int ni 0;
int ul 0;
int cu 0;
int ce 0;
int in 0;
int in1 0;
int un 0;
int wch 0;
int pendt 0;
int *pendw 0;
int pendnf 0;
int spread 0;
int it 0;
int itmac 0;
int lnsize LNSIZE;
int *hyptr[NHYP] {0};
int tabtab[NTAB] {DTAB,DTAB*2,DTAB*3,DTAB*4,DTAB*5,DTAB*6,DTAB*7,DTAB*8,
	DTAB*9,DTAB*10,DTAB*11,DTAB*12,DTAB*13,DTAB*14,DTAB*15,0};
int line[LNSIZE]{0};
int word[WDSIZE]{0};
int blockxxx[EVS-64-NHYP-NTAB-WDSIZE-LNSIZE] {0};
/*spare 7 words*/
int oline[LNSIZE+1];
