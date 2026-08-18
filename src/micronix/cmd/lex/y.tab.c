#define CHAR 257
#define CCL 258
#define NCCL 259
#define STR 260
#define DELIM 261
#define SCON 262
#define ITER 263
#define NEWE 264
#define NULLS 265
#define CAT 266
#include "ldefs.c"
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar, yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#ifndef YYSTYPE
#define YYSTYPE int
#endif
#ifndef YYVCOPY
#define YYVCOPY(x,y) x=y
#endif
YYSTYPE yylval, yyval;
int i;
int j,k;
int g;
char *p;
#define YYERRCODE 256

yylex(){
	register char *p;
	register int c, i;
	char  *t;
	int n, j, k, x;
	static int sectbegin;
	static char token[TOKENSIZE];
	static int iter;

#ifdef DEBUG
	yylval = 0;
#endif

	if(sect == DEFSECTION) {		/* definitions section */
		while(!eof) {
			if(prev == '\n'){		/* next char is at beginning of line */
				getl(p=buf);
				switch(*p){
				case '%':
					switch(c= *(p+1)){
					case '%':
						lgate();
						if(!ratfor)fprintf(fout,"#");
						fprintf(fout,"define YYNEWLINE %d\n",ctable['\n']);
						if(!ratfor)fprintf(fout,"yylex(){\nint nstr;\n");
						sectbegin = TRUE;
						i = treesize*(sizeof(*name)+sizeof(*left)+
							sizeof(*right)+sizeof(*nullstr)+sizeof(*parent))+ALITTLEEXTRA;
						c = lxalloc(i,1);
						if(c == 0)
							error("Too little core for parse tree");
						p = c;
						cfree(p,i,1);
						name = (int *)lxalloc(treesize,sizeof(*name));
						left = (int *)lxalloc(treesize,sizeof(*left));
						right = (int *)lxalloc(treesize,sizeof(*right));
						nullstr = lxalloc(treesize,sizeof(*nullstr));
						parent = (int *)lxalloc(treesize,sizeof(*parent));
						if(name == 0 || left == 0 || right == 0 || parent == 0 || nullstr == 0)
							error("Too little core for parse tree");
						return(freturn(DELIM));
					case 'p': case 'P':	/* has overridden number of positions */
						while(*p && !digit(*p))p++;
						maxpos = siconv(p);
						if(report == 2)report = 1;
						continue;
					case 'n': case 'N':	/* has overridden number of states */
						while(*p && !digit(*p))p++;
						nstates = siconv(p);
						if(report == 2)report = 1;
						continue;
					case 'e': case 'E':		/* has overridden number of tree nodes */
						while(*p && !digit(*p))p++;
						treesize = siconv(p);
						if(report == 2)report = 1;
						continue;
					case 'o': case 'O':
						while (*p && !digit(*p))p++;
						outsize = siconv(p);
						if (report ==2) report=1;
						continue;
					case 'a': case 'A':		/* has overridden number of transitions */
						while(*p && !digit(*p))p++;
						if(report == 2)report = 1;
						ntrans = siconv(p);
						continue;
					case 't': case 'T': 	/* character set specifier */
						chset = TRUE;
						lgate();
						for(i = 0; i<NCH; i++)
							ctable[i] = 0;
						while(getl(p) && scomp(p,"%T") != 0 && scomp(p,"%t") != 0){
							if((n = siconv(p)) <= 0 || n > NCH){
								warning("Character value %d out of range",n);
								continue;
								}
							while(!space(*p) && *p) p++;
							while(space(*p)) p++;
							t = p;
							while(*t){
								c = ctrans(&t);
								if(ctable[c]){
									if (printable(c))
										warning("Character '%c' used twice",c);
									else
										warning("Character %o used twice",c);
									}
								else ctable[c] = n;
								t++;
								}
							p = buf;
							}
						continue;
					case 'r': case 'R':
						warning("Ratfor is unimplemented as of yet - use olex");
						c = 'r';
					case 'c': case 'C':
						if(lgatflg)
							error("Too late for language specifier");
						ratfor = (c == 'r');
						continue;
					case '{':
						lgate();
						while(getl(p) && scomp(p,"%}") != 0)
							fprintf(fout, "%s\n",p);
						if(p[0] == '%') continue;
						error("Premature eof");
					case 's': case 'S':		/* start conditions */
						lgate();
						while(*p && index(*p," \t,") < 0) p++;
						n = TRUE;
						while(n){
							while(*p && index(*p," \t,") >= 0) p++;
							t = p;
							while(*p && index(*p," \t,") < 0)p++;
							if(!*p) n = FALSE;
							*p++ = 0;
							i = sptr*2;
							if(!ratfor)fprintf(fout,"#");
							fprintf(fout,"define %s %d\n",t,i);
							scopy(t,sp);
							sname[sptr++] = sp;
							sname[sptr] = 0;	/* required by lookup */
							if(sptr >= STARTSIZE)
								error("Too many start conditions");
							sp += slength(sp) + 1;
							if(sp >= schar+STARTCHAR)
								error("Start conditions too long");
							}
						continue;
					default:
						warning("Invalid request %s",p);
						continue;
						}	/* end of switch after seeing '%' */
				case ' ': case '\t':		/* must be code */
					lgate();
					fprintf(fout, "%s\n",p);
					continue;
				default:		/* definition */
					while(*p && !space(*p)) p++;
					if(*p == 0)
						continue;
					prev = *p;
					*p = 0;
					bptr = p+1;
					yylval = buf;
					if(digit(buf[0]))
						warning("Substitution strings may not begin with digits");
					return(freturn(STR));
					}
				}
			/* still sect 1, but prev != '\n' */
			else {
				p = bptr;
				while(*p && space(*p)) p++;
				if(*p == 0)
					warning("No translation given - null string assumed");
				scopy(p,token);
				yylval = token;
				prev = '\n';
				return(freturn(STR));
				}
			}
		/* end of section one processing */
		}
	else if(sect == RULESECTION){		/* rules and actions */
		while(!eof){
			switch(c=gch()){
			case '\0':
				return(freturn(0));
			case '\n':
				if(prev == '\n') continue;
				x = NEWE;
				break;
			case ' ':
			case '\t':
				if(sectbegin == TRUE){
					cpyact();
					while((c=gch()) && c != '\n');
					continue;
					}
				if(!funcflag)phead2();
				funcflag = TRUE;
				if(ratfor)fprintf(fout,"%d\n",30000+casecount);
				else fprintf(fout,"case %d:\n",casecount);
				if(cpyact()){
					if(ratfor)fprintf(fout,"goto 30997\n");
					else fprintf(fout,"break;\n");
					}
				while((c=gch()) && c != '\n');
				if(peek == ' ' || peek == '\t' || sectbegin == TRUE){
					warning("Executable statements should occur right after %%");
					continue;
					}
				x = NEWE;
				break;
			case '%':
				if(prev != '\n') goto character;
				if(peek == '{'){	/* included code */
					getl(buf);
					while(!eof && getl(buf) && scomp("%}",buf) != 0)
						fprintf(fout,"%s\n",buf);
					continue;
					}
				if(peek == '%'){
					c = gch();
					c = gch();
					x = DELIM;
					break;
					}
				goto character;
			case '|':
				if(peek == ' ' || peek == '\t' || peek == '\n'){
					if(ratfor)fprintf(fout,"%d\n",30000+casecount++);
					else fprintf(fout,"case %d:\n",casecount++);
					continue;
					}
				x = '|';
				break;
			case '$':
				if(peek == '\n' || peek == ' ' || peek == '\t' || peek == '|' || peek == '/'){
					x = c;
					break;
					}
				goto character;
			case '^':
				if(prev != '\n' && scon != TRUE) goto character;	/* valid only at line begin */
				x = c;
				break;
			case '?':
			case '+':
			case '.':
			case '*':
			case '(':
			case ')':
			case ',':
			case '/':
				x = c;
				break;
			case '}':
				iter = FALSE;
				x = c;
				break;
			case '{':	/* either iteration or definition */
				if(digit(c=gch())){	/* iteration */
					iter = TRUE;
				ieval:
					i = 0;
					while(digit(c)){
						token[i++] = c;
						c = gch();
						}
					token[i] = 0;
					yylval = siconv(token);
					munput('c',c);
					x = ITER;
					break;
					}
				else {		/* definition */
					i = 0;
					while(c && c!='}'){
						token[i++] = c;
						c = gch();
						}
					token[i] = 0;
					i = lookup(token,def);
					if(i < 0)
						warning("Definition %s not found",token);
					else
						munput('s',subs[i]);
					continue;
					}
			case '<':		/* start condition ? */
				if(prev != '\n')		/* not at line begin, not start */
					goto character;
				t = slptr;
				do {
					i = 0;
					c = gch();
					while(c != ',' && c && c != '>'){
						token[i++] = c;
						c = gch();
						}
					token[i] = 0;
					if(i == 0)
						goto character;
					i = lookup(token,sname);
					if(i < 0) {
						warning("Undefined start condition %s",token);
						continue;
						}
					*slptr++ = i+1;
					} while(c && c != '>');
				*slptr++ = 0;
				if(slptr > slist+STARTSIZE)		/* note not packed ! */
					error("Too many start conditions used");
				yylval = t;
				x = SCON;
				break;
			case '"':
				i = 0;
				while((c=gch()) && c != '"' && c != '\n'){
					if(c == '\\') c = usescape(c=gch());
					token[i++] = c;
					if(i > TOKENSIZE){
						warning("String too long");
						i = TOKENSIZE-1;
						break;
						}
					}
				if(c == '\n') {
					yyline--;
					warning("Non-terminated string");
					yyline++;
					}
				token[i] = 0;
				if(i == 0)x = NULLS;
				else if(i == 1){
					yylval = token[0];
					x = CHAR;
					}
				else {
					yylval = token;
					x = STR;
					}
				break;
			case '[':
				for(i=1;i<NCH;i++) symbol[i] = 0;
				x = CCL;
				if((c = gch()) == '^'){
					x = NCCL;
					c = gch();
					}
				while(c != ']' && c){
					if(c == '\\') c = usescape(c=gch());
					symbol[c] = 1;
					j = c;
					if((c=gch()) == '-' && peek != ']'){		/* range specified */
						c = gch();
						if(c == '\\') c = usescape(c=gch());
						k = c;
						if(j > k) {
							n = j;
							j = k;
							k = n;
							}
						if ('A' <= j && k <= 'Z')
							;
						else if ('a' <= j && k <= 'z')
							;
						else if ('0' <= j && k <= '9')
							;
						else
							warning("Non-portable Character Class");
						for(n=j+1;n<=k;n++)
							symbol[n] = 1;		/* implementation dependent */
						c = gch();
						}
					}
				/* try to pack ccl's */
				i = 0;
				for(j=0;j<NCH;j++)
					if(symbol[j])token[i++] = j;
				token[i] = 0;
				p = ccptr;
				if(optim){
					p = ccl;
					while(p <ccptr && scomp(token,p) != 0)p++;
					}
				if(p < ccptr)	/* found it */
					yylval = p;
				else {
					yylval = ccptr;
					scopy(token,ccptr);
					ccptr += slength(token) + 1;
					if(ccptr >= ccl+CCLSIZE)
						error("Too many large character classes");
					}
				cclinter(x==CCL);
				break;
			case '\\':
				c = usescape(c=gch());
			default:
			character:
				if(iter){	/* second part of an iteration */
					iter = FALSE;
					if('0' <= c && c <= '9')
						goto ieval;
					}
				if(alpha(peek)){
					i = 0;
					yylval = token;
					token[i++] = c;
					while(alpha(peek))
						token[i++] = gch();
					if(peek == '?' || peek == '*' || peek == '+')
						munput('c',token[--i]);
					token[i] = 0;
					if(i == 1){
						yylval = token[0];
						x = CHAR;
						}
					else x = STR;
					}
				else {
					yylval = c;
					x = CHAR;
					}
				}
			scon = FALSE;
			if(x == SCON)scon = TRUE;
			sectbegin = FALSE;
			return(freturn(x));
			}
		}
	/* section three */
	ptail();
#ifdef DEBUG
	if(debug)
		fprintf(fout,"\n/*this comes from section three - debug */\n");
#endif
	while(getl(buf) && !eof)
		fprintf(fout,"%s\n",buf);
	return(freturn(0));
	}
/* end of yylex */
#ifdef DEBUG
freturn(i)
  int i; {
	if(yydebug) {
		printf("now return ");
		if(i < NCH) allprint(i);
		else printf("%d",i);
		printf("   yylval = ");
		switch(i){
			case STR: case CCL: case NCCL:
				strpt(yylval);
				break;
			case CHAR:
				allprint(yylval);
				break;
			default:
				printf("%d",yylval);
				break;
			}
		putchar('\n');
		}
	return(i);
	}
#endif
int yyexca[] = {
-1, 1,
	0, -1,
	-2, 0,
	};
#define YYNPROD 33
extern int yychar;

int yylast = 291;
int yyact[] = {

  34,  45,  35,  23,  21,  43,  27,  28,   4,  34,
  16,  32,  41,  21,  46,  27,  28,  10,  34,  16,
  32,   8,  21,   9,  27,  28,  25,  29,  16,   6,
   7,   3,  24,  34,  27,  28,  29,  21,  11,  27,
  28,   2,   5,  16,   1,  29,  21,   0,  27,  28,
   0,   0,  16,   0,   0,  29,  21,   0,  20,   0,
  29,   0,  16,   0,   0,  21,   0,  20,   0,  29,
   0,  16,   0,   0,  31,   0,  20,   0,   0,   0,
  12,   0,   0,  12,   0,   0,   0,   0,  30,   0,
   0,  20,   0,  42,  36,  37,  38,  30,   0,   0,
   0,   0,   0,   0,   0,  39,  30,  40,   0,   0,
  20,   0,   0,   0,   0,   0,   0,   0,   0,  20,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  44,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  14,  17,  18,  15,   0,  19,  33,   0,  22,
  14,  17,  18,  15,   0,  19,  33,  26,  22,  14,
  17,  18,  15,   0,   0,  33,   0,  22,   0,   0,
   0,   0,   0,   0,  14,  17,  18,  15,   0,   0,
  33,   0,  22,  14,  17,  18,  15,   0,   0,  33,
   0,  22,  13,  14,  17,  18,  15,   7,  19,   0,
   0,  22,  14,  17,  18,  15,   0,  19,   0,   0,
  22 };
int yypact[] = {

-248,-1000,-1000,-231,-1000,  16,-257,-1000,  16,-1000,
-1000,-1000, -27,-262,-1000,-1000,-1000,-1000,-1000,  25,
  25,  25,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
  25,  -8,  25, -32,-1000,-1000, -18,   6, -36,  -3,
 -18,-124,-1000,-1000,-111,-1000,-1000 };
int yypgo[] = {

   0,  44,  41,  31,  38,  21,  23,  17,  74 };
int yyr1[] = {

   0,   1,   2,   2,   2,   6,   6,   3,   3,   4,
   5,   5,   7,   7,   8,   8,   8,   8,   8,   8,
   8,   8,   8,   8,   8,   8,   8,   8,   8,   8,
   8,   8,   8 };
int yyr2[] = {

   0,   1,   4,   3,   1,   1,   0,   3,   0,   1,
   2,   1,   2,   2,   1,   1,   1,   1,   1,   2,
   2,   2,   3,   2,   3,   5,   3,   4,   2,   2,
   2,   3,   1 };
int yychk[] = {

-1000,  -1,  -2,  -3, 256,  -4, 260, 261,  -5,  -6,
  -7,  -4,  -8, 256, 257, 260,  46, 258, 259, 262,
  94,  40, 265, 260,  -6,  -7, 264,  42,  43,  63,
 124,  -8,  47, 263,  36, 264,  -8,  -8,  -8,  -8,
  -8,  44, 125,  41, 263, 125, 125 };
int yydef[] = {

   8,  -2,   1,   0,   4,   6,   0,   9,   6,   3,
  11,   5,   0,   0,  14,  15,  16,  17,  18,   0,
   0,   0,  32,   7,   2,  10,  12,  19,  20,  21,
   0,  23,   0,   0,  30,  13,  28,  29,   0,  22,
  24,   0,  26,  31,   0,  27,  25 };
#define YYFLAG -1000
#define YYERROR goto yyerrlab
#define YYACCEPT return(0)
#define YYABORT return(1)

/*	parser for yacc output	*/

int yydebug = 0; /* 1 for debugging */
YYSTYPE yyv[YYMAXDEPTH]; /* where the values are stored */
int yychar = -1; /* current input token number */
int yynerrs = 0;  /* number of errors */
int yyerrflag = 0;  /* error recovery flag */

yyparse() {

	int yys[YYMAXDEPTH];
	int yyj, yym;
	register YYSTYPE *yypvt;
	register yystate, *yyps, yyn;
	register YYSTYPE *yypv;
	register *yyxi;
	/*
	 * ccc's c1 miscompiles `++yyps > &yys[YYMAXDEPTH]` for frame
	 * operands (CODEGENGAPS entry 22): the bound address is dropped
	 * and the compare runs against the size constant.  Pre-compute
	 * the bound and split the increment.
	 */
	int *yyslim = &yys[YYMAXDEPTH];

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyps= &yys[-1];
	yypv= &yyv[-1];

 yystack:    /* put a state and value onto the stack */

	if( yydebug  ) printf( "state %d, value %d, char %d\n",yystate,yyval,yychar );
		++yyps;
		if( yyps> yyslim ) yyerror( "yacc stack overflow" );
		*yyps = yystate;
		++yypv;
		YYVCOPY(*yypv,yyval);

 yynewstate:

	yyn = yypact[yystate];

	if( yyn<= YYFLAG ) goto yydefault; /* simple state */

	if( yychar<0 ) if( (yychar=yylex())<0 ) yychar=0;
	if( (yyn += yychar)<0 || yyn >= yylast ) goto yydefault;

	if( yychk[ yyn=yyact[ yyn ] ] == yychar ){ /* valid shift */
		yychar = -1;
		YYVCOPY(yyval,yylval);
		yystate = yyn;
		if( yyerrflag > 0 ) --yyerrflag;
		goto yystack;
		}

 yydefault:
	/* default state action */

	if( (yyn=yydef[yystate]) == -2 ) {
		if( yychar<0 ) if( (yychar=yylex())<0 ) yychar = 0;
		/* look through exception table */

		yyxi = yyexca;
		for(;;){
			if( (*yyxi == (-1)) && (yyxi[1] == yystate) ) break;
			yyxi += 2;
			}

		while( *(yyxi+=2) >= 0 ){
			if( *yyxi == yychar ) break;
			}
		if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */
		}

	if( yyn == 0 ){ /* error */
		/* error ... attempt to resume parsing */

		switch( yyerrflag ){

		case 0:   /* brand new error */

			yyerror( "syntax error" );
		yyerrlab:
			++yynerrs;

		case 1:
		case 2: /* incompletely recovered error ... try again */

			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */

			while ( yyps >= yys ) {
			   yyn = yypact[*yyps] + YYERRCODE;
			   if( yyn>= 0 && yyn < yylast && yychk[yyact[yyn]] == YYERRCODE ){
			      yystate = yyact[yyn];  /* simulate a shift of "error" */
			      goto yystack;
			      }
			   yyn = yypact[*yyps];

			   /* the current yyps has no shift onn "error", pop stack */

			   if( yydebug ) printf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1] );
			   --yyps;
			   --yypv;
			   }

			/* there is no state on the stack with an error shift ... abort */

	yyabort:
			return(1);


		case 3:  /* no shift yet; clobber input char */

			if( yydebug ) printf( "error recovery discards char %d\n", yychar );

			if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
			yychar = -1;
			goto yynewstate;   /* try again in the same state */

			}

		}

	/* reduction by production yyn */

		if( yydebug ) printf("reduce %d\n",yyn);
		yyps -= yyr2[yyn];
		yypvt = yypv;
		yypv -= yyr2[yyn];
		YYVCOPY(yyval,yypv[1]);
		yym=yyn;
			/* consult goto table to find next state */
		yyn = yyr1[yyn];
		yyj = yypgo[yyn] + *yyps + 1;
		yystate = yyact[yyj];
		if( yyj>=yylast || (yychk[yystate] + yyn) ) yystate = yyact[yypgo[yyn]];
		switch(yym){
			
case 1:{	
#ifdef DEBUG
		if(debug) sect2dump();
#endif
	} break;
case 3:{
		if(!funcflag)phead2();
		funcflag = TRUE;
	} break;
case 4:{
#ifdef DEBUG
		if(debug) {
			sect1dump();
			sect2dump();
			}
#endif
		} break;
case 7:{	scopy(yypvt[-1],dp);
		def[dptr] = dp;
		dp += slength(yypvt[-1]) + 1;
		scopy(yypvt[-0],dp);
		subs[dptr++] = dp;
		if(dptr >= DEFSIZE)
			error("Too many definitions");
		dp += slength(yypvt[-0]) + 1;
		if(dp >= dchar+DEFCHAR)
			error("Definitions too long");
		subs[dptr]=def[dptr]=0;	/* for lookup - require ending null */
	} break;
case 9:{
#ifdef DEBUG
		if(sect == DEFSECTION && debug) sect1dump();
#endif
		sect++;
		} break;
case 10:{	yyval = mn2(RNEWE,yypvt[-1],yypvt[-0]);
		} break;
case 11:{	yyval = yypvt[-0];} break;
case 12:{
		if(divflg == TRUE)
			i = mn1(S1FINAL,casecount);
		else i = mn1(FINAL,casecount);
		yyval = mn2(RCAT,yypvt[-1],i);
		divflg = FALSE;
		casecount++;
		} break;
case 13:{
#ifdef DEBUG
		if(debug) sect2dump();
#endif
		} break;
case 14:{	yyval = mn0(yypvt[-0]); } break;
case 15:{
		p = yypvt[-0];
		i = mn0(*p++);
		while(*p)
			i = mn2(RSTR,i,*p++);
		yyval = i;
		} break;
case 16:{	symbol['\n'] = 0;
		if(psave == FALSE){
			p = ccptr;
			psave = ccptr;
			for(i=1;i<'\n';i++){
				symbol[i] = 1;
				*ccptr++ = i;
				}
			for(i='\n'+1;i<NCH;i++){
				symbol[i] = 1;
				*ccptr++ = i;
				}
			*ccptr++ = 0;
			if(ccptr > ccl+CCLSIZE)
				error("Too many large character classes");
			}
		else
			p = psave;
		yyval = mn1(RCCL,p);
		cclinter(1);
		} break;
case 17:{	yyval = mn1(RCCL,yypvt[-0]); } break;
case 18:{	yyval = mn1(RNCCL,yypvt[-0]); } break;
case 19:{	yyval = mn1(STAR,yypvt[-1]); } break;
case 20:{	yyval = mn1(PLUS,yypvt[-1]); } break;
case 21:{	yyval = mn1(QUEST,yypvt[-1]); } break;
case 22:{	yyval = mn2(BAR,yypvt[-2],yypvt[-0]); } break;
case 23:{	yyval = mn2(RCAT,yypvt[-1],yypvt[-0]); } break;
case 24:{	if(!divflg){
			j = mn1(S2FINAL,-casecount);
			i = mn2(RCAT,yypvt[-2],j);
			yyval = mn2(DIV,i,yypvt[-0]);
			}
		else {
			yyval = mn2(RCAT,yypvt[-2],yypvt[-0]);
			warning("Extra slash removed");
			}
		divflg = TRUE;
		} break;
case 25:{	if(yypvt[-3] > yypvt[-1]){
			i = yypvt[-3];
			yypvt[-3] = yypvt[-1];
			yypvt[-1] = i;
			}
		if(yypvt[-1] <= 0)
			warning("Iteration range must be positive");
		else {
			j = yypvt[-4];
			for(k = 2; k<=yypvt[-3];k++)
				j = mn2(RCAT,j,dupl(yypvt[-4]));
			for(i = yypvt[-3]+1; i<=yypvt[-1]; i++){
				g = dupl(yypvt[-4]);
				for(k=2;k<=i;k++)
					g = mn2(RCAT,g,dupl(yypvt[-4]));
				j = mn2(BAR,j,g);
				}
			yyval = j;
			}
	} break;
case 26:{
		if(yypvt[-1] < 0)warning("Can't have negative iteration");
		else if(yypvt[-1] == 0) yyval = mn0(RNULLS);
		else {
			j = yypvt[-2];
			for(k=2;k<=yypvt[-1];k++)
				j = mn2(RCAT,j,dupl(yypvt[-2]));
			yyval = j;
			}
		} break;
case 27:{
				/* from n to infinity */
		if(yypvt[-2] < 0)warning("Can't have negative iteration");
		else if(yypvt[-2] == 0) yyval = mn1(STAR,yypvt[-3]);
		else if(yypvt[-2] == 1)yyval = mn1(PLUS,yypvt[-3]);
		else {		/* >= 2 iterations minimum */
			j = yypvt[-3];
			for(k=2;k<yypvt[-2];k++)
				j = mn2(RCAT,j,dupl(yypvt[-3]));
			k = mn1(PLUS,dupl(yypvt[-3]));
			yyval = mn2(RCAT,j,k);
			}
		} break;
case 28:{	yyval = mn2(RSCON,yypvt[-0],yypvt[-1]); } break;
case 29:{	yyval = mn1(CARAT,yypvt[-0]); } break;
case 30:{	i = mn0('\n');
		if(!divflg){
			j = mn1(S2FINAL,-casecount);
			k = mn2(RCAT,yypvt[-1],j);
			yyval = mn2(DIV,k,i);
			}
		else yyval = mn2(RCAT,yypvt[-1],i);
		divflg = TRUE;
		} break;
case 31:{	yyval = yypvt[-1]; } break;
case 32:{	yyval = mn0(RNULLS); } break;
		}
		goto yystack;  /* stack new state and value */

	}
