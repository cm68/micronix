
typedef union 
	{
	struct shblock *yshblock;
	struct depblock *ydepblock;
	struct nameblock *ynameblock;
	} YYSTYPE;
extern YYSTYPE yylval;
# define NAME 257
# define SHELLINE 258
# define START 259
# define MACRODEF 260
# define COLON 261
# define DOUBLECOLON 262
# define GREATER 263
