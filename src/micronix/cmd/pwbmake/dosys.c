#include "defs.h"

dosys(comstring,nohalt)
register char *comstring;
int nohalt;
{
register int status;

if(metas(comstring))
	status = doshell(comstring,nohalt);
else	status = doexec(comstring);

return(status);
}



metas(s)   /* Are there are any  Shell meta-characters? */
register char *s;
{
register char c;

while( (funny[c = *s++] & META) == 0 )
	;
return( c );
}

doshell(comstring,nohalt)
char *comstring;
int nohalt;
{
if((waitpid = fork()) == 0)
	{
	enbint(SIG_DFL);
	doclose();

	/*
	 * micronix sh takes -c (run the next argument as a command) and
	 * has no -e, so the v7 "-ce" spelling is -c here.  sh returns
	 * the command's own exit status, which is all make needs.
	 */
	execl(SHELLCOM, "sh", "-c", comstring, 0);
	fatal("Couldn't load Shell");
	}

return( await() );
}




await()
{
int intrupt();
int status;
register int pid;

enbint(SIG_IGN);
while( (pid = wait(&status)) != waitpid)
	if(pid == -1)
		fatal("bad wait code");
waitpid = 0;
enbint(intrupt);
return(status);
}






doclose()	/* Close open directory files before exec'ing */
{
register struct opendir *od;
for (od = firstod; od; od = od->nxtopendir)
	if (od->dirfc != NULL)
		fclose(od->dirfc);
}




/*
 * execvp: exec with the shell's own path search (".", "/bin",
 * "/usr/bin"), because micronix has no getenv("PATH") to search.  A
 * name containing a slash is tried as-is; otherwise each directory is
 * tried in turn.  execv returns on failure and the loop carries on,
 * so the first directory that holds the program wins.
 */
execvp(name, argv)
char *name;
char *argv[];
{
static char *pathv[] = { ".", "/bin", "/usr/bin", 0 };
char buf[128];
int i;

if( strchr(name, '/') )
	return( execv(name, argv) );

for(i = 0 ; pathv[i] ; ++i)
	{
	concat(pathv[i], "/", buf);
	strcat(buf, name);
	/* check first so a miss does not exec and spill a diagnostic */
	if( access(buf, 1) == 0 )
		execv(buf, argv);
	}
return(-1);
}


doexec(str)
register char *str;
{
register char *t;
char *argv[200];
register char **p;

while( *str==' ' || *str=='\t' )
	++str;
if( *str == '\0' )
	return(-1);	/* no command */

p = argv;
for(t = str ; *t ; )
	{
	*p++ = t;
	while(*t!=' ' && *t!='\t' && *t!='\0')
		++t;
	if(*t)
		for( *t++ = '\0' ; *t==' ' || *t=='\t'  ; ++t)
			;
	}

*p = NULL;

if((waitpid = fork()) == 0)
	{
	enbint(SIG_DFL);
	doclose();
	enbint(intrupt);
	execvp(str, argv);
	fatal1("Cannot load %s",str);
	}

return( await() );
}



touch(force, name)
int force;
char *name;
{
struct stat stbuff;
char junk[1];
int fd;

if( stat(name,&stbuff) < 0)
	if(force)
		goto create;
	else
		{
		fprintf(stderr, "touch: file %s does not exist.\n", name);
		return;
		}

/* micronix splits the size across st_size0/st_size1; empty is both 0 */
if((stbuff.st_size0 | stbuff.st_size1) == 0)
	goto create;

if( (fd = open(name, 2)) < 0)
	goto bad;

if( read(fd, junk, 1) < 1)
	{
	close(fd);
	goto bad;
	}
lseek(fd, 0L, 0);
if( write(fd, junk, 1) < 1 )
	{
	close(fd);
	goto bad;
	}
close(fd);
return;

bad:
	fprintf(stderr, "Cannot touch %s\n", name);
	return;

create:
	if( (fd = creat(name, 0666)) < 0)
		goto bad;
	close(fd);
}
