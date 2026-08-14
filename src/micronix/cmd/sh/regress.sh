#!/bin/sh
#
# regress.sh - is it still what the stock shell does?
#
# micronix/cmd/sh/regress.sh
#
# The baseline is not this file and not a list of expected answers.
# It is the stock shell - the Whitesmiths binary we have no source
# for - and the only way to ask it anything is to run it.  So every
# case here is run through BOTH shells under the simulator and the two
# answers are compared.  There is nothing to keep up to date when the
# stock shell surprises us; it simply starts disagreeing.
#
# This is a HOST script.  It drives usersim, which is a host program,
# and the two shells inside it are guest binaries.
#
#	./regress.sh			everything
#	./regress.sh pattern		only the cases matching it
#
# setsid, because a case may signal its process group and this should
# not reach the terminal that started it.
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

ROOT=${ROOT:-/vault/src/micronix/filesystem}
SIM=${SIM:-/vault/src/micronix/src/usersim/usersim}
STOCK=${STOCK:-/newer/bin/sh}		# the binary with no source
OURS=${OURS:-/bin/sh}
TMP=${TMPDIR:-/tmp}/shregress.$$

trap 'rm -f $TMP.in $TMP.a $TMP.b' 0

#
# One line in, whatever comes out - stdout and stderr together, since
# where a message goes is part of what the shell does.
#
runsh()
{
	printf '%s\nexit\n' "$2" > $TMP.in
	timeout 20 setsid $SIM -d $ROOT "$1" < $TMP.in 2>&1 |
		grep -v '^ran-dot-sh$'
}

#
# The same, but on a pty, because some of what the shell does it only
# does for a terminal - history is expanded for what is typed and not
# for what is read out of a file.  script(1) gives us the terminal.
# The whole input arrives at once, so the prompts and the answers
# interleave oddly, but they interleave the SAME oddly for both
# shells, and it is stable run to run.
#
runtty()
{
	shell=$1
	shift
	for line in "$@"; do printf '%s\n' "$line"; done
	printf 'exit\n'
}

ttysh()
{
	sh=$1
	shift
	runtty "$sh" "$@" |
		timeout 30 setsid script -q -c "$SIM -d $ROOT $sh" /dev/null 2>&1 |
		grep -v '^ran-dot-sh$'
}

pass=0
fail=0
skip=0
known=0

check()
{
	case "$1" in
	*"$SELECT"*) ;;
	*) skip=`expr $skip + 1`; return ;;
	esac

	runsh $STOCK "$1" > $TMP.a
	runsh $OURS  "$1" > $TMP.b

	if cmp -s $TMP.a $TMP.b; then
		pass=`expr $pass + 1`
		test -n "$VERBOSE" && printf 'ok      %s\n' "$1"
	else
		fail=`expr $fail + 1`
		printf 'DIFFER  %s\n' "$1"
		sed 's/^/          stock: /' $TMP.a
		sed 's/^/          ours:  /' $TMP.b
	fi
}

#
# A case we know answers differently, and why.  Listed rather than
# left out, and asserted rather than excused: if one of these ever
# starts MATCHING, the reason has gone stale and this says so.  A
# difference nobody wrote a reason for is a regression.
#
differs()
{
	case "$1" in
	*"$SELECT"*) ;;
	*) skip=`expr $skip + 1`; return ;;
	esac

	runsh $STOCK "$1" > $TMP.a
	runsh $OURS  "$1" > $TMP.b

	if cmp -s $TMP.a $TMP.b; then
		fail=`expr $fail + 1`
		printf 'NOW SAME  %s\n' "$1"
		printf '          (expected to differ: %s)\n' "$2"
	else
		known=`expr $known + 1`
		test -n "$VERBOSE" && printf 'known   %s  (%s)\n' "$1" "$2"
	fi
}

#
# One interactive case: several lines typed at a terminal.
#
checktty()
{
	case "$*" in
	*"$SELECT"*) ;;
	*) skip=`expr $skip + 1`; return ;;
	esac

	ttysh $STOCK "$@" > $TMP.a
	ttysh $OURS  "$@" > $TMP.b

	if cmp -s $TMP.a $TMP.b; then
		pass=`expr $pass + 1`
		test -n "$VERBOSE" && printf 'ok tty  %s\n' "$*"
	else
		fail=`expr $fail + 1`
		printf 'DIFFER  (tty) %s\n' "$*"
		sed 's/^/          stock: /' $TMP.a
		sed 's/^/          ours:  /' $TMP.b
	fi
}

SELECT="$1"

# statements
check 'echo a ; echo b'
check 'echo one;echo two'
check 'echo a ;; echo b'
check '; echo lead'
check 'echo trail ;'
check 'echo x | grep x ; echo y'

# words, quoting, escapes
check 'echo "double ok"'
check "echo 'single ok'"
check 'echo "a"b'
check 'echo x"y z"w'
check 'echo a\b'
check 'echo "unterminated'

# substitution
check 'echo `echo one two`'
check 'echo A`echo hi`B'
check 'echo pre`echo mid`post'
check 'echo "a `echo b` c"'
check 'echo `ls -l /etc | grep passwd`'
check 'echo A`echo`B'
check 'echo `echo hi'

# groups
check '(echo a ; echo b)'
check '(echo a) | grep a'
check '(cd /etc ; pwd) ; pwd'
check '((echo nested))'
check 'echo x ; (echo y) ; echo z'
check '(exit) ; echo alive'
check 'echo (echo a)'
check '(echo a) b'
check 'echo ('
check '(echo a'
check 'echo a)'

# patterns
check 'echo /etc/pass*'
check 'echo /etc/*ss*'
check 'echo /etc/pass?d'
check 'echo /etc/[bg]*'
check 'echo /etc/[a-i]*'
check 'echo /etc/[!bg]*'
check 'echo /e*/passwd'
check 'echo /etc/*'
check 'echo /*'
check 'echo /etc/s????n'
check 'echo /etc/pass* /etc/mt*'
check 'echo /nosuch/* /etc/pass*'
check 'echo /nosuch/* /alsonone/*'
check 'echo *.nosuchthing ; echo after'
check 'echo "/etc/*"'
check 'echo "/etc/"*'
check 'echo /etc/"pass"*'
check 'echo \*'
check 'echo /etc/pas\s*'
check 'echo plain'
check 'echo /etc/passwd'

# redirection and the rest
check 'echo x >'
check 'nosuchcmd'
check 'ls /etc | grep passwd'

#
# History, which happens only at a terminal.  Fed a file the shell
# runs "!e" as a command name, and the first case here says so.
#
check 'echo one ; !e'
checktty 'echo one' 'echo two' '!e'
checktty 'echo one' '!1'
checktty 'echo one' '!t'
checktty 'echo one' '!!'
checktty 'echo one' '!$'
checktty 'echo one' 'echo !e'
checktty 'echo one' 'echo a!$b'
checktty 'echo one' 'echo x!!'
checktty 'echo one' 'echo a!!b'
checktty 'echo one' '!:'
checktty 'echo one' '! e'
checktty 'echo one' '!e-tail'
checktty 'echo one' 'echo !e' '!e'
checktty 'echo plain'

#
# The differences we mean.
#
differs 'type dir' \
	'dir is a builtin there and a seeded alias here, deliberately'
differs 'echo x |' \
	'a pipeline with an empty last stage: the stock shell runs it
	 and says nothing, and we call it a syntax error.  Not a
	 continuation - "echo x |" then "echo SECOND" prints SECOND
	 there, so the next line is an ordinary one'

printf '\n%d the same as the stock shell' $pass
test $known -gt 0 && printf ', %d differ on purpose' $known
test $fail -gt 0 && printf ', %d WRONG' $fail
test $skip -gt 0 && printf ', %d not run' $skip
printf '\n'
test $fail -eq 0
