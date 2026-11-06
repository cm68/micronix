
/* prototypes for functions defined below */
int yyinit(void);
main( argc, argv ) int argc; char *argv[]; {
  yyinit( argc, argv );
  if( yyparse() ) return;
  yyaccpt();
  }
