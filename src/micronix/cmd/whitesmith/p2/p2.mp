.NM cp2 generate code for 8080 C programs      
.SY
cp2 -[p -o* -x#] <file>
.FU
Cp2
is the code generating pass of the C compiler.  It accepts a
sequential file of flow graphs and parse trees from 
cp1
and writes a sequential file of A-Natural statements, suitable 
for input to the A-Natural assembler, as.

As much as possible, the compiler generates free-standing code; but
for those operations which cannot be done compactly, it generates inline
calls to a set of machine-dependent library routines.  The 8080  runtime
library is documented in Section IV of the C Reference Manual.

The flags are:
.IP -p	
Emit profiler calls on entry to each function.  No profiler
is currently provided for the 8080.

.IP "-o *"
Write the output file * and write the error messages to 
STDOUT.  Default is STDOUT for output and STDERR for error messages.

.IP -x#	
Map the three virtual sections, for Functions (04), Literals (02),
and Variables (01), to the physical sections Code (bit is one) and Data
(bit is zero).  thus, "-x4" is for separate I/D space, "-x6" is for
ROM/RAM code, and "-x7" is for compiling tables into ROM.  Default
is 6.

.in -0.5i
If <file> is present, it is used as the input file instead of the
default STDIN.  On many systems, <file> is mandatory because STDIN is
interpreted as a text file, and hence becomes corrupted.

Files output from cp1
for use with 8080 code generator should be generated with: "-b0" to
eliminate holes in structures and auto allocation, and "-n8" for full
length external names.  A boundary of "-b1" (default) is also acceptable
if compact storage is less important than compatibility with data
structures on other machines.  Note that use of the
cp1
flags "-b2", "-b3", or "-1" will produce bizarre behavior and incorrect
code in
cp2,
often with no meaningful diagnostics.

Wherever possible, labels in the emitted code each contain a comment
which gives the source line from which the code immediately following
obtains.
.EG
usually follows cpp and cp1 as follows:
.DS
.in +1i 
cpp -x -o temp1 file.c
cp1 -b0 -n8 -o temp2.80 temp1
cp2 -o file.s temp2
.DE
.SA
as (1), cp1 (1), cc (1), cpp (1)
.NO
This program and documentation are products of Whitesmiths, Ltd.,
