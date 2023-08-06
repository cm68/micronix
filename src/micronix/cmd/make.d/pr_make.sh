(date; pwd; ls -l) | pr $1
if -w gram.c rm  gram.c
pr $1 pr_make.sh pwinfo Makefile defs *.[cy]
