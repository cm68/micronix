#include <stdio.h>
#include <stdlib.h>
extern FILE *yyin, *yyout;
main(){
yyin = stdin; yyout = stdout;
yylex();
exit(0);
}
