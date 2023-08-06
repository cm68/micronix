# include "defs"

dosys(comstring,nohalt)
char *comstring;
int nohalt;
{
int status;

if(metas(comstring))
	status = doshell(comstring,nohalt);
else	status = doexec(comstring);

return(status>>8);
}



metas(s)   /* Are there are any  Shell meta-characters? */
char *s;
{
for(;;) switch(*s++)
	{
	case '|':   case '^':
	case '(':   case ')':
	case ';':   case '&':
	case '<':   case '>':
	case '*':   case '?':
	case '[':   case ']':
	case '\\':  case '\n':
	case '\'':  case '"':
	case '`':   case '$':
	case ':':
		return(1);

	case '\0':
		return(0);
	};
}

doshell(comstring,nohalt)
char *comstring;
int nohalt;
{
if(fork() == 0)
	{
	enbint(0);
	doclose();

	if( execl(SHELLCOM, "sh", "-c", comstring, 0) )
		fatal("Couldn't load Shell");
	}

return( await() );
}




await()
{
int intrupt();
int status;

disint();
wait(&status);
enbint(intrupt);
return(status);
}






doclose()	/* Close open directory files before exec'ing */
{
register struct opendir *od;
for (od = firstod; od != 0; od = od->nextp)
	if (od->dirfc != NULL)
		fclose(od->dirfc);
}





doexec(str)
char *str;
{
register char *s, *t;
char *argv[100], pool[1000];
int status;
int argc;


argc = 0;
s = pool;

for(t=str ; *t!='\0' ; )
	{
	while(*t==' '|| *t=='\t') ++t;
	if(*t == '\0') break;

	argv[argc++] = s;
	while(*t!=' '  && *t!='\t' && *t!='\0') *s++ = *t++;
	*s++ = '\0';
	}

if(argc == 0)   /* no command */
	return(-1);
argv[argc] = 0;

if(fork() == 0)
	{
	enbint(0);
	doclose();

	pexec(argv[0], argv);
	fprintf(stderr,  "*** Cannot load %s",argv[0]);
	fatal("");
	}

return( await() );
}


 char junkname[16];

touch(s)
char *s;
{
if(junkname[0] == 0)
	sprintf(junkname,  "MAKEJUNK%d", getpid() );

link(s, junkname);
unlink(junkname);
}
