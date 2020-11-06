Port of STevie for the Philips P2000C CP/M-80 machine.

STevie was written by Tim Thompson. Ported to the P2K by Jon Bradbury.

You need Aztec C under CP/M-80 to compile STevie.

To modify STevie for your screen's escape sequences, open window.c and
provide an implementation of windgoto() (where r is the row number and
c is the column number) and windclear(). That should be all, but it
might be necessary to alter the windinit() or other functions.

To build STevie issue the command "submit buildall" at the CP/M command prompt.

After a (very) long time you should find an executable called vi.com in
the build directory.

You will find that STevie is very slow and has a few bugs, but it does work.

Cheers
JonB
