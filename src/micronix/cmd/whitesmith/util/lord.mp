.NM
lord - order libraries
.SY
lord -[c* d* i r* s]             
.FU
Lord reads in a list of module names, with associated interdepencies, from
STDIN, and outputs to STDOUT a topologically sorted list of module
names such that, if at all possible, no module depends on an earlier
module in the list.  Each module is introduced by a line containing
its name followed by a colon.  Subsequent lines are interpreted
as either:

.IP defs
things defined by the module,

.IP refs
things referred to by the module, or other stuff.

.in 0.5i
Refs and defs have the syntax given by one or more formats entered
as flags on the command line.  Each character of the format must match
the corresponding character at the beginning of an input line; a ?
will match any character except newline.  If all characters of the format
match, the rest of the input line is taken as a ref or def name.  Thus,
the format flag "-d0x????D" would identify as valid def any line
beginning with "0x", four arbitrary characters and a "D", so that
the input line "0x3ff0D_inbuf" would be taken as a def named "_inbuf".

The flags are:
.IP "-c*" 
prepend the string * to the output stream.  Implies -s.
Each module name is output preceded by a space; the output stream 
is terminated with a newline.  Hence, lord can be used to build a command line.
.IP -d*
use the string * as a fromat for defs.
.IP -i
ignore other stuff.  Default is to complain about any line
not recognizable as a def or ref.
.IP -r*
use the string * as a format for refs.
.IP -s
suppress output of defs and refs; output only module names
in order.
.sp
.in 0.5i
Up to ten formats may be input for defs, and up to ten for refs.

If no -d flags are given, lord uses the default def formats: 
"0x????????B", "0x????????D", 
"0x????????T", 0x????B", "0x????D", "0x????????U" and "0x????U".
These are compatible with the default output of rel (obj).

If there  are any circular dependencies among the modules, 
lord writes "not completely sorted" to STDERR and outputs a 
partially-ordered list.
In general, rearrangements are made only when necessary, so an ordered
set of modules should pass through lord unchanged.
.RT
Lord returns success if no error messages are printed, otherwise failure.
.EG
To create a library of ordered object modules under Idris:
.br
	% rel *.0 | lord -c"lib libx.a -c" | sh
.sp
To order a set of objects using Micronix nm:
.sp 
.bd
	% nm *.o > nmlist
.br
.bd
	% lord < nmlist -c"ar r libx.a" | \\
		-d"??????T" -d"??????D" \\
		-d"??????B" -r"??????U" | sh
.SA
lib (1), obj (1)
.NO
This program and documentation are products of Whitesmiths, Ltd.
