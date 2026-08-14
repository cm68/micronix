.he "lib (1)                      3/18/82                      lib (1)"
.fo "                               -#-"
.in 5
.rm 65
.bd
PROGRAM
.in +5
.br
.bd
lib
- maintains Whitesmith's format libraries


.in -5
.bd
USAGE        
.in +5
.br
.bd
lib lfile [flags] [files]


.in -5
.bd
FUNCTION
.in +5
.br
.bd
Lib
performs all functions necessary to build and maintain object module
libraries.
It can also be used to collect arbitrary files into one bucket.
.bd
Lfile 
is the name of an existing library file or, in the case of replace
(-r), the name of the library to be created.

The flags are:

.ta 10
.in +10
.ti -10
-d	Delete the zero or more named files from the library.

.ti -10
-r	Replace the zero or more named files in an existing library or,
if no library 
.bd
lfile 
exists, create a library with the named files in the
specified order.

Named files that are not present in the library are 
appended to the end in the order specified.

.ti -10
-t	Lists files in the library in the order they occur. 
If any files are named, only those named and present are listed.

.ti -10
-x	Extract any named files that are present in the library into files
of the same name.
If no files are named, all files are extracted.

.ti -10
-v	Be verbose about it.
Files retained unmodified are each listed with a
preceding "c".
Those deleted are preceded by a "d", 
those replaced are preceded by an "r" and those appended to the end 
are preceded by an "a".
For the 
.bd
-t 
option, each file is listed with its length in bytes.

.ti -10
-6	Create a file with UNIX v. 6 format, if none exists. Effective only
with 
.bd
-r.

.ti -10
-7	Create a file with UNIX v. 7 format, if none exists.
Effective only with 
.bd
-r.
.bp
.in -10
At most, one of the flags [drtx] may be present.
If none are present, 
.bd
-t 
is assumed.
Similarly, at most one of the flags [67] should be present.
If none are present,
Whitesmith's library file format is assumed when a new library is 
created by 
.bd
-r.

The Whitesmith's library format consists of a two-byte header having the value
0177565, written less significant byte first,
followed by zero or more entries.

Each entry consists of a fourteen-byte name,
followed by a two-byte unsigned file length,
also stored less significant byte first, followed by the file contents proper.
If a name begins with a null byte,
it is taken as the end of the library file.

Note that this differs in several small ways from UNIX v. 6 format, which
has a header of 0177555, an eight-byte name, six bytes of miscellaneous
UNIX specific file attributes, and a two-byte file length.
Moreover, 
a file whose length is odd is followed by a null padding byte in the UNIX
format, but no padding is used in standard library format.

UNIX v.7 format is characterized by a header of 0177545,
a fourteen-byte
name, six bytes of UNIX specific file attributes, and a four-byte length.
Odd length files are also padded to even.


.in -5
.bd
RETURNS
.in +5
.br
.bd
Lib
returns success if no problems are encountered, else failure.
In most failures, an error message is printed to STDERR and the library file
is not modified. Output from the 
.bd
-t 
flag, and verbose remarks, are written to
STDERR.


.in -5
.bd
EXAMPLE
.in +5
.br
To build a library and check its contents:

.in +8
.bd
lib clib -r one.o two.o three.o

.bd
lib clb -tv
.in -8

.bp
.in -5
.bd
NOTES
.in +5
.br
If all files are deleted from a library, a vestigial file
remains.  Modifying UNIX v.6 or UNIX v.7 format files causes all 
attributes of all file entries to be zeroed.

It does not check for large files  (over 65534 bytes).


NOTE:
This program and documentation are products of Whitesmiths, Ltd., 
and are sold separately with the
