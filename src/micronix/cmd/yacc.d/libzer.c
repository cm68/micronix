int yyline;
yyerror(s) char *s; {
	printf("\n%s", s );
	if( yyline ) printf( ", line %d\n", yyline );
	else printf( "\n" );
	}
