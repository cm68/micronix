float paper[256];
struct	passwd {
	char	*pw_name;
	char	*pw_passwd;
	int	pw_uid;
	int	pw_gid;
	char	*pw_gecos;
	char	*pw_dir;
	char	*pw_shell;
};
int ibuf1[259];
int ibuf2[259];
int *ip1 ibuf1;
int *ip2 ibuf2;
int pu[2];
char acctname[] "/usr/actg/data/troffactg";
int xxx;

main(argc,argv)
int argc;
char **argv;
{

	register int i;
	int lastu;
	register struct passwd *p;
	char date[40];
	float total;

	if(fopen("/etc/passwd",ip2) <0){
		printf("Cannot open /etc/passwd\n");
		exit();
	}
	if(argc == 2)acctname[9] = (++argv)[0][0];
	if(fopen(acctname,ip1) < 0){
		printf("Cannot open: %s\n",acctname);
		exit();
	}
	i=0;
	while(1){
		date[i] = getc(ip1);
		if(date[i++] == '\n')break;
	}
	date[i]=0;

	while(getn(pu,3) == 3){
		paper[pu[1]] =+ pu[0]/1728.;
		total =+ pu[0]/1728.;
		lastu = pu[1];
	}

	printf("%s",date);
	printf("UID	Feet of paper\n");
	i=0;
	while(i<256){
		if(paper[i++] != 0.0){
			while(p=getpwent()){
				if(p->pw_uid == (i-1)){
					if(lastu == p->pw_uid)
					  printf("_\b");
					printf("%s\t%8.1f\n",
					p->pw_name,paper[i-1]);
					break;
				}
			}
			if(p == 0){
				printf("%d\t%8.1f\n",
				i-1,paper[i-1]);
				seek(*ip2,0,0);
				ibuf2[1] = ibuf2[2] = 0;
			}
		}
	}
	printf("Total\t%8.1f\n",total);
}
getpwent()
{
	register char *p;
	register c;
	static struct passwd passwd;
	static char line[100];

	p = line;
	while((c=getc(ip2)) != '\n') {
		if(c <= 0)
			return(0);
		if(p < line+98)
			*p++ = c;
	}
	*p = 0;
	p = line;
	passwd.pw_name = p;
	p = pwskip(p);
	passwd.pw_passwd = p;
	p = pwskip(p);
	passwd.pw_uid = atoi(p);
	p = pwskip(p);
	passwd.pw_gid = atoi(p);
	p = pwskip(p);
	passwd.pw_gecos = p;
	p = pwskip(p);
	passwd.pw_dir = p;
	p = pwskip(p);
	passwd.pw_shell = p;
	return(&passwd);
}

pwskip(ap)
char *ap;
{
	register char *p;

	p = ap;
	while(*p != ':') {
		if(*p == 0)
			return(p);
		p++;
	}
	*p++ = 0;
	return(p);
}
getn(p,n)
char *p;
int n;
{
	register i;

	for(i=0; i<n; i++){
		if((p[i]=getc(ip1)) == -1)break;
	}
	return(i);
}
