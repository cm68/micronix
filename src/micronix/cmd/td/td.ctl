; names for /bin/td off the Micronix 1.6 distribution floppy, for
; src/tools/disas:
;
;	disas -f td.ctl td.dist
;
; There is no source for td.  This file names what the reconstruction in
; td.c had to know; see README beside this file.
;
; It has the 99 94 header, with the text at 0100 for 46cb bytes and the data
; at 5000, and the older of the two runtimes - the one login, init and entab
; have: c.ent2 and c.ret2 are the register saving pair and the cells are
; r2, r3 and r4.  The text begins with a jump over the startup, which is not
; the usual layout, because the program has a string in front of main.
;
; NOTE ON SYNTAX: disas reads a value as decimal unless it starts with a 0
; or ends in h, so every address here is written 0x.... .
;
start 0x0100

define c.ent 0x4770
define c.ret 0x4777
define c.ent2 0x477b
define c.ret2 0x4790
define r2 0x5c6d
define r3 0x5c6f
define r4 0x5c71

; the program's variables
define aflag 0x5000
define devdest 0x5002
define iflag 0x5004
define uflag 0x5006
define verbose 0x5008
define mounted 0x500a
define source 0x500c
define destdir 0x500e
define destarg 0x5010
define srclen 0x5212
define volume 0x5214
define now 0x5216
define since 0x521a
define deststat 0x521e
define errno 0x5c5b

; td's own functions, in address order.  The names are what each does;
; none is the author's.  Own code runs from 010b to 1a73; the file system
; check at 1a75 is the last of it and the library begins at 1bbc.
main code 0x012d
options code 0x0164		; one argument that begins with a dash
setup code 0x03b1		; the command line, and what to do before the walk
interact code 0x06ef		; the questions asked when nothing is said
dump code 0x07db		; the walk, begun
copyfile code 0x0847		; one file, and the next disk if it fills
visit code 0x0ace		; one thing found in the tree
errs code 0x0cbd		; a string to the standard error
errn code 0x0ce2		; a number to the standard error
exists code 0x0d19
dirname code 0x0d46
basename code 0x0e95
walk code 0x0ed1		; the tree, a directory at a time
sigoff code 0x1058		; ignore the signals, remembering how they stood
catch code 0x10a6		; the handler: cleanup(0)
sigon code 0x10b4		; catch the signals that were not ignored
isatty code 0x1108
ask code 0x1133			; prompt and read a line
newvolume code 0x12a6		; unmount, ask for the next disk, mount it
cleanup code 0x1447		; the end, by any way
initsig code 0x155e
wanted code 0x1587		; whether this file is dumped
mkpath code 0x16ca		; mkdir -p
dtfind code 0x188e		; /etc/dtab: the time of the last dump
dtput code 0x1977		; /etc/dtab: write it
isfs code 0x1a75		; a device holds a file system

; the library
fopen code 0x1bbc
getfile code 0x1cd8
openfile code 0x1d4a
fclose code 0x1e78
fread code 0x1f17
gets code 0x211d
puts code 0x22db
fputs code 0x2347
_flsbuf code 0x25b4
exit code 0x2e33
btoi code 0x2f1c		; the converter handed 512 for the length
shell code 0x325d		; sh -c, with the signals looked after
mntname code 0x33ca		; /tmp/<pid>-<clock>
streq code 0x3473
instr code 0x3583
itob code 0x361a
ltob code 0x36ef
perror code 0x3a7a
execl code 0x3d8d
access code 0x3da5
chmod code 0x3ddc
chown code 0x3dff
close code 0x3e22
creat code 0x3e31
exec code 0x3e55
_exit code 0x3e76
fork code 0x3e7c
getpid code 0x3e8f
getuid code 0x3e94
gtty code 0x3e99
link code 0x3eb9
mknod code 0x3edc
mount code 0x3f0a
open code 0x3f38
read code 0x3f5c
seek code 0x3f88
signal code 0x3fb3
_signal code 0x409e
stat code 0x4136
time code 0x4159
umount code 0x4178
unlink code 0x4190
wait code 0x41a8
write code 0x41c5
strlen code 0x3c8a
cpystr code 0x351e
lcmp code 0x4388
lsub code 0x471d
lmul code 0x4634
lmod code 0x4585
