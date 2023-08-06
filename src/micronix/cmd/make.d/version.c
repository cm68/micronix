char *xxxvers "@(#)VERSION 3.1(PWB)     21 APRIL 1977\n" ;

/*
2.1 4/24/76	Base version

2.2 4/26/76     Error found by SRB in overriding pattern rules;
		corrected gram.y

2.3 4/27/76	Further correction for overriding pattern rules;
		corrected doname.c

2.4		Removed .CLEAR name, added .IGNORE.
		A .SUFFIXES rule without dependents clears the list

2.5		Stripped output

2.6		Changed doshell to accomodate new shell.

2.7		Following SRB's sugestion, added ${...} as
		alternate macro name

2.8		Defined macros AS and DTGEN in files.c.

2.9		Put in a fix to prevent removal of files
		upon interrupt in a  ::  rule.

2.10		Fixed bugs involving messages for ::
		and closing standard input

2.11		Changed time test from <= to <
		(equal times are considered in sync)

2.12		Installed -t flag (touch and update time of
		files rather than issue commands)
		Fixed bug in dosys

2.13		Fixed lex.c to allow sharps (#) in commands

2.14		Added .DEFAULT rule

2.15		Changed to <lS> I/O System (stdio.h)

2.16		Removed references to double floats and macro HAVELONGS;
		committed to use of long ints for times.
2.17		Corrected metacharacter list in dosys.c.
2.18		Miscellaneous fixes
2.19		Updated files.c to use include file stat.h
2.20		Added -q flag for Mike Lesk
2.21		Added AWK rules and  .w  suffix to  files.c
2.22		Added colon to the list of metacharacters
2.23		Macro substitutions on dependency lines.
		Redid argument and macro setting.
		Close files before exec'ing.
		Print > at beginning of command lines.
		No printing of commands beginnng with @.
2.24	Parametrized propt sequence in doname.c (4/1/77)
2.25	Added $? facility
2.26	Fixed bug in macro expansion
2.27	Repaired interrupt handling

3.1(PWB)  Use pexec(), add LEAP builtin rules - WDR 4/21/77.
*/
