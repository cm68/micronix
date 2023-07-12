.NM hex translate object file to ASCII formats
.SY
hex -[flags] <ifile>
.FU
Hex
translates executable images produced by link 
to Intel standard hex format or to Motorola S-record object file 
format.  The executable image is read from <ifile>, if specified,
otherwise from the file xeq.

The flags are:
.IP -h
Do not start record for Intel hex files.
.IP -m*
Insert the string *, instead of <ifile>, into the
Motorola SO record generated when -s is given.

.IP -r##	
Interpret the input file as "raw" binary and not as
object file.  Output is produced as described below, in either 
format, except
that the starting address of the module is specified by the long
integer ##.

.IP -s	
Produce S-record rather than the default hex format.

.IP +#	
Start output with the #th byte.  # should be between 0 and
one less than the value specified by the -#flag. -4 +3 produces bytes
3, 7, 11, 1 5, ...; -2 +0 produces all even bytes; -1 +1 outputs all
odd bytes; 0 is the default which outputs all bytes.

.IP -#	
Output every #th byte. -2 outputs every other byte, -4 every
fourth; 1 is the default which outputs all bytes.

Output is written to STDOUT.

.ul
Hex Files

A file in Intel hex format consists of the following records, in
the order listed:

1)	A "$" alone on a line to indicate the end of the (non-existent)
symbol table. If -h is specified, this line is omitted.

2)	Data records for the text segment, if any.  These represent
32 image bytes per line, possibly terminated by a shorter line.

3)	Data records for the data segment, if any, in the same format
as the text segment records.

4)	An end record specifying the start address.

Data records each begin with a ":" and consist of pairs of hexadecimal
digits, each pair representing the numerical value of a byte.  The
last pair is a checksum such that the numerical sum of all the bytes
represented on the line, modulo 256, is zero.  the bytes on a data
record line are:

a)	The number of image bytes on the line.

b)	Two bytes for the load offset of the first image byte.  The
offset is written more significant byte first so that it reads correctly
as a four-digit hexadecimal number.

c)	A zero byte "00".

d)	The image bytes in increasing order of address.

e)	The checksum byte.

An end record also begins with a ":" and is written as digit pairs
with a trailing checksum.  Its format is:

a)	A zero byte "00".

b)	Two bytes for the start address, in the same format as the
load offset in a data record.

c)	A one byte "01".

d)	The checksum byte.

.ul
S-Record Files

A file in Motorola S-record format is a series of records each
containing the following fields:

	<S field><count><addr><data bytes><checksum>

All information is represented as pairs of hexadecimal digits, each
pair representing the numerical value of a byte.

<S filed> determines the interpretation of the remainder of the 
line; valid S fields are "SO", "S1", S2", "S8", "S9". <count> indicates
the number of bytes represented in the rest of the line, so the total
number of characters in the line is <count> * 2 + 4.

<addr> indicates the byte address of the first data byte in the data
field. SO records have two zero bytes as their address field; S1 and
S2 records have <addr> fields of two and three bytes in length, 
respectively; S9 and S8 records have <addr> fields of two and three
bytes in length, respectively, and contain no data bytes. <addr> is
represented most significant byte first.

The SO record contains the name of the input file, formatted as
data bytes.  If input was from xeq, XEQ is used as the name.  S1 and
S2 records represent text or data segment bytes to be loaded.  They
normally contain 32 image bytes, output in increasing order of address;
the last record of each segment may be shorter.  The text segment
is output first, followed by the data segment.  S9 records contain
only a two-byte start address in their <addr> field; S8 records
contain a three-byte address.  The start address of an object file
is the start of the text section.

<checksum> is a single byte value such that the numerical sum of
all the bytes represented on the line (except the S field),
taken modulo 256, is 255 (0xFF).
.RT
Hex
returns success if no error messages are printed, that is, if
all records make sense and all reads and writes succeed; otherwise
it reports failure.
.EG
The file hello.c, consisting of :
	char *p {"hello world"};

when compiled produces the following Intel hex file:
% hex hello.o

$
:0E000002006856C6C6F20776F726C640094
:000000010
.SA
link (1), obj (1), cc (1), as (1), cp1 (1), cp2 (1), cpp (1)
.NO
This program and documentation are products of Whitesmiths, Ltd.,
