/*
 * fdj.c - reconstruction of FDJ.COM ("Morrow DJDMA Formatter revision 2")
 *
 * Reconstructed from the binary at src/hwsim/resources/cpm22/E4/902/FDJ.COM
 * (22656 bytes), a CP/M .COM compiled by Whitesmiths C V2.1 (the runtime
 * banner "V2.1: copyright (c) 1979 by Whitesmiths, Ltd." is linked in at
 * 0x51b0).  Disassembly: tools/disas -a 0x100 (8586 lines, /tmp/fdj.dis).
 *
 * Calling convention (byte-verified against the runtime at 0x5128..):
 *   c.ent  0x5128   prologue:  DE = SP at the call; args at DE+4 (arg0),
 *                   DE+6 (arg1), DE+8 (arg2) ...
 *   c.ret  0x512f   epilogue: restores SP and RETs; caller pops the args
 *   c.ents 0x5133   like c.ent but first saves the globals 0x5939/593b/593d
 *   c.ihl  0x5149   the matching return (restores those globals)
 *   c.rets 0x5174   return-a-small-value variant
 *   bdos   0x0005   CP/M call
 * Word args/returns travel in BC or HL; char in A; sign extension is the
 * ADD A,A / SBC A,A idiom.  The switch dispatch is H5103 over a table of
 * [handler, value] word pairs ending in a 0 value then the default handler.
 *
 * The DJ-DMA channel program is built in place at 0x50 (DEF_CCA), a byte at
 * a time, with a running pointer in the global at 0x5497 (0x5939/593b/593d
 * across the c.ents boundary).  A format is WRSECT (0x21) per sector - on
 * the DJ-DMA a soft-sectored write lays down the ID header and data, which
 * is the format - then RTRACK (0x29) to verify; the sector size and density
 * come from GSTAT (0x22) auto-detection, never from the program.  There is
 * no WTRACK, no SETTRACK, no SETMP.
 *
 * Reconstruction, not byte-exact transcription: identifiers are invented,
 * but control flow, argument lists, and command bytes are faithful.  The
 * option handlers H0313..H0526 are reached by JP (not CALL) from the switch,
 * so they share parse_arg's frame and still see arg0 (p) at DE+4.
 *
 * This is the CP/M Whitesmiths source, not (yet) a Micronix command: it
 * calls BDOS and talks to the board's channel directly.  It is the reference
 * the Micronix fdj (Job 2) will be written from.
 */

/* ---------------------------------------------------------------------------
 * global state (the .bss at 0x51b0..; names invented, addresses real)
 * ------------------------------------------------------------------------- */
int  value;         /* 0x53e8  just-parsed numeric value of -xNNN */
char inches;        /* 0x51b9  byte, default 8;   -i5/-i8 -> 5/8 */
char density;       /* 0x51bf  byte, default 's'; -n1->'s', -n2->'d' */
int  drive;         /* 0x53c8  -d 0..3 */
int  sectors;       /* 0x53d0  -p 1..26 */
int  holes;         /* 0x53d2  -h1 -> 0, -h10/-h11 -> 2 */
int  sides;         /* 0x53d6  -s 1..2 */
int  tracks;        /* 0x53da  -t 35/40/77 */
int  bytes;         /* 0x53dc  -b 128/256/512/1024 */

extern void prmsg();    /* H3dbc: 4-arg error print (fd, arg, fmt, 0) */
extern void usage();    /* H0bef */
extern void restart();  /* H056e: cleanup + re-prompt/exit */

/* ---------------------------------------------------------------------------
 * 0x01f0: all_digits(p)  -- non-empty run of '0'..'9'
 * ------------------------------------------------------------------------- */
int
all_digits(p)
    char *p;
{
    if (*p == 0)
        return 0;
    for (;;) {
        if (*p == 0)
            return 1;
        if (*p < '0' || *p > '9')
            return 0;
        p++;
    }
}

/* ---------------------------------------------------------------------------
 * the option table at 0x024e: [handler, char] word pairs, 0 terminator, then
 * the default handler.  Dispatched by H5103 (switch on the char in BC).
 *
 *   'p'->H04f4 sectors   'n'->H04b5 density   'h'->H045f holes
 *   't'->H041a tracks    'b'->H03c5 bytes     's'->H0371 sides
 *   'd'->H0348 drive     'i'->H0313 inches    default->H0526 unknown
 * ------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * 0x028b: parse_arg(p) -- parse one "-xNNN" argument
 * ------------------------------------------------------------------------- */
void
parse_arg(p)
    char *p;
{
    while (*p == '-')               /* skip leading '-'s */
        p++;
    if (*p == 0 || !all_digits(p + 1)) {
        prmsg(2, p, ": Missing numeric value\n", 0);    /* H02cc */
        usage();
        restart();
        return;
    }
    value = atoi(p + 1);            /* H3a42 -> H53e8 */
    switch (*p) {                   /* H5103 over the table at 0x024e */
    case 'i': set_inches(p); break; /* H0313 */
    case 'd': set_drive(p); break;  /* H0348 */
    case 's': set_sides(p); break;  /* H0371 */
    case 'b': set_bytes(p); break;  /* H03c5 */
    case 't': set_tracks(p); break; /* H041a */
    case 'h': set_holes(p); break;  /* H045f */
    case 'n': set_density(p); break;/* H04b5 */
    case 'p': set_sectors(p); break;/* H04f4 */
    default:  set_unknown(p); break;/* H0526 */
    }
}

/* ---- the option handlers: validate value, else error(p), then store ---- */

/* 0x0313 -i */
void
set_inches(p)
    char *p;
{
    if (value != 5 && value != 8)
        error(p);
    inches = value;
}

/* 0x0348 -d */
void
set_drive(p)
    char *p;
{
    if (value < 0 || value > 3)
        error(p);
    drive = value;
}

/* 0x0371 -s */
void
set_sides(p)
    char *p;
{
    if (value != 1 && value != 2)
        error(p);
    sides = value;
}

/* 0x03c5 -b */
void
set_bytes(p)
    char *p;
{
    if (value != 128 && value != 256 && value != 512 && value != 1024)
        error(p);
    bytes = value;
}

/* 0x041a -t */
void
set_tracks(p)
    char *p;
{
    if (value != 35 && value != 40 && value != 77)
        error(p);
    tracks = value;
}

/* 0x045f -h : holes==1 -> holes=0 (soft); holes==10/11 -> holes=2 (hard) */
void
set_holes(p)
    char *p;
{
    if (value == 1)
        holes = 0;
    else if (value == 10 || value == 11)
        holes = 2;
    else
        error(p);
}

/* 0x04b5 -n : stores a character 's' (single) or 'd' (double) */
void
set_density(p)
    char *p;
{
    if (value == 1)
        density = 's';
    else if (value == 2)
        density = 'd';
    else
        error(p);
}

/* 0x04f4 -p */
void
set_sectors(p)
    char *p;
{
    if (value < 1 || value > 26)
        error(p);
    sectors = value;
}

/* 0x0526 - unknown option: usage + restart */
void
set_unknown(p)
    char *p;
{
    usage();
    restart();
}

/* ---------------------------------------------------------------------------
 * 0x0545: error(p) -- ": Value out of range", then usage + restart
 * ------------------------------------------------------------------------- */
void
error(p)
    char *p;
{
    prmsg(2, p, ": Value out of range\n", 0);   /* H052f */
    usage();
    restart();
}

/* ---------------------------------------------------------------------------
 * the menu text (the .text strings):
 *   H0668 "8 inch formats: F CP/M std single / G Morrow CP/M double /
 *          H Morrow Micronix"
 *   H06d1 "5 inch hard sectored: C single / D double / E Morrow Micronix"
 *   H072a "5 inch soft sectored: A Morrow single / B Morrow double"
 *   H063f "Do it yourself / I Other / Selection:"
 * ------------------------------------------------------------------------- */

/* TODO (remaining): H056e restart, H0594/H0605 (drive sense/select),
 * the menu driver from H077c/H08c8/H092a on, the format body H2c04..H318e
 * (command builders + SETTRACK/verify/execute), the geometry helpers
 * H4c6e/H4c89, and the library (H3a42 atoi, H3dbc prmsg, H38fa, printf...).
 */

/* ---------------------------------------------------------------------------
 * the format table (data at load 0x53f6; reached as H4de6 + 16*menu_char,
 * H4d26 being the 16-bit multiply).  16 bytes per entry, layout:
 *   [0] inches  [2] tracks  [4] sides  [6] sectors  [8] bytes
 *   [0xA] density ('s'/'d')  [0xC] holes (0 soft / 2 hard)  [0xE] (unused here)
 *
 *   a  A) 5.25 soft single   40   1   5 x 1024  d   soft   5"
 *   b  B) 5.25 soft double   40   2   5 x 1024  d   soft   5"
 *   c  C) 5.25 hard single   40   1  10 x  512  d   hard   5"
 *   d  D) 5.25 hard double   40   2  10 x  512  d   hard   5"
 *   e  E) 5.25 hard micronix 40   2  10 x  512  d   hard   5"
 *   f  F) 8" CP/M single     77   1  26 x  128  s   soft   8"
 *   g  G) 8" CP/M double     77   2   8 x 1024  d   soft   8"
 *   h  H) 8" Micronix        77   2  15 x  512  d   soft   8"
 *
 * The table names only the tracks-1+ geometry.  Track 0 of an 8" disk is the
 * IBM-3740 boot track, 26 x 128 single density, no matter which 8" option was
 * chosen; a 5.25" disk has no such track-0 split.  That is the firmware's
 * own "8 inch track 0 is single density" rule, not something the table says.
 * ------------------------------------------------------------------------- */

/* TODO: the menu driver H077c/H085c (select_format), H08c8 (main), H09b8
 * (do_format), the per-option DIY prompts H12ce..H1516, the command builders
 * H2c04..H318e, and the geometry/format body that turns this table into the
 * SETTRACK + whole-track-write channel program.
 */

/* ---------------------------------------------------------------------------
 * flags (byte .bss)
 *   H51b8  command-line mode (args were given)
 *   H51be  a format has been chosen / done
 *   H51c1  "8 inch drive present on the far port" -> offset drives by 4
 * ------------------------------------------------------------------------- */

/* 0x092a: banner -- print the title once, unless in command-line mode */
void
banner()
{
    if (!cmdline)               /* H51b8 == 0 */
        puts("\tMorrow DJDMA Formatter revision 2\n");
}

/* ---------------------------------------------------------------------------
 * 0x09b8: select_drive() -- A/B/C/D or 0..3 or CR, return 1 (0 on ESC)
 *   dispatch table H093f:  ESC->cancel(0),  'd''c''b''a'->drive=c-'a',
 *   '3''2''1''0'->drive=c-'0',  CR/LF->drive=0,  default->finish
 * ------------------------------------------------------------------------- */
void
sel_drive_letter(c)            /* H0a3f: 'a'..'d' */
    char c;
{
    drive = c - 'a';
    finish_drive();
}

void
sel_drive_digit(c)             /* H0a2c: '0'..'3' */
    char c;
{
    drive = c - '0';
    finish_drive();
}

void
sel_drive_default()            /* H09d7: CR/LF -> drive 0 */
{
    drive = 0;
    finish_drive();
}

void
finish_drive()                 /* H09de: echo " Drive N." and apply the 8" offset */
{
    puts(" Drive ");
    putchar('0' + drive);      /* H306c */
    puts(".\n");
    if (inches == 8 && eight_inch_present)   /* (H51b9==8) xor H51c1 */
        drive += 4;
}

int
select_drive()
{
    char c;

    puts("\n\tA) drive 0\n\tB) drive 1\n\tC) drive 2\n\tD) drive 3\n\nSelection: ");
    c = getchar();
    switch (c) {
    case '\033': return 0;              /* H0a52 */
    case 'd': case 'c': case 'b': case 'a':
        sel_drive_letter(c); break;
    case '3': case '2': case '1': case '0':
        sel_drive_digit(c); break;
    case '\r': case '\n':
        sel_drive_default(); break;
    default:
        finish_drive(); break;
    }
    return 1;
}

/* 0x0a5e: mark "chosen" and return that flag */
int
mark_chosen()
{
    chosen = 1;                 /* H51be = 1 */
    return chosen;
}

/* ---------------------------------------------------------------------------
 * 0x0bef: usage() -- the -b/-d/-h/-i/-n/-p/-s/-t help text
 * ------------------------------------------------------------------------- */
void
usage()
{
    prmsg(2, "Valid argments are b, h, i, d, n, p, s, and t.\n"
             "Here are meanings and legal values:\n",
          "\t -b bytes/sector (128,256,512,1024)  -d drive (0..3)\n"
          "\t -h holes (1,10)  -i inches (5,8)  -n density (1,2)\n"
          "\t -p sectors (1-26)  -s sides (1,2)  -t tracks (35,40)\n",
          "\nExample: fdj -b512 -h10 -i5 -d0 -n2 -p10 -s2 -t40\n");
}

/* ---------------------------------------------------------------------------
 * 0x0c76: ask_bytes() -- the DIY "Sector size:" prompt
 *   table H0c11:  'a'->128 'b'->256 'c'->512 'd'->1024, ESC->cancel
 * ------------------------------------------------------------------------- */
int
ask_bytes()
{
    char c;

    puts("\n\t A)  128\n\t B)  256\n\t C)  512\n\t D) 1024\n\nSector size: ");
    c = getchar();
    switch (c) {
    case '\033': return 0;
    case 'a': bytes = 128;  break;
    case 'b': bytes = 256;  break;
    case 'c': bytes = 512;  break;
    case 'd': bytes = 1024; break;
    }
    puts("\t");                      /* H0c3c */
    putnum(bytes);                   /* H115d */
    puts(" bytes per sector\n");
    return 1;
}

/* TODO: the remaining DIY prompts H12ce (inches), H0d6e (density),
 * H0e3c (sides), H13a2 (holes), H1516 (tracks), H15b3 (sectors),
 * H1451 (?), then H0f33/H17ce/H3352 (the actual format) and the
 * command builders H2c04..H318e.
 */

/* ---------------------------------------------------------------------------
 * The DJ-DMA channel program (the heart of fdj, 0x2c04..0x318e).
 *
 * Commands are assembled a byte at a time at 0x50 (DEF_CCA), the running
 * pointer in the global 0x5497 (0x5939/593b/593d across c.ents), then run
 * by H2f28 which pulses the attention port 0xEF.  The opcodes laid down:
 *
 *   H2c25  send_setdma():  [23, lo, hi, 00, 25]   SETDMA + HALT
 *   H2ca0  read_track():   H2c25 then [29, cyl, head, drv, sectab(3), 25]
 *                          RTRACK with a sector table, then HALT
 *   H2c19/H2c1f  (empty in this build - the read/write-sector bodies are
 *                  the DB run after 0x2c0a the disassembler kept as data)
 *   H2f28  exec():  pulse 0xEF, wait for the controller
 *
 * The write-track path (H364a, the computed opcode from 0x3c15/0x3c35) is
 * what lays a format down; it and the geometry helpers H4c6e/H4c89/H4cf3
 * (16-bit multiply / shift) remain to be transcribed.
 * ------------------------------------------------------------------------- */

/* TODO (remaining, in address order):
 *   H0f33/H17ce/H3352  the format driver (menu-selected and cmd-line paths)
 *   H12ce/H0d6e/H0e3c/H13a2/H1516/H15b3/H1451  the other DIY prompts
 *   H2f28 exec, H2fda getchar, H306c putchar, H30a1 puts, H318e/H324e/...
 *   the geometry helpers H4c6e/H4c89/H4cf3/H4d26 (16-bit mul/div/shift)
 *   the library (H3a42 atoi, H3dbc prmsg, H38fa, printf/scanf...)
 */

/* ---------------------------------------------------------------------------
 * the DIY prompts: each prints a menu, reads a char, sets one global.
 * '~' in do_format shows the summary and re-prompts.
 * ------------------------------------------------------------------------- */

/* 0x0d6e: ask_tracks() -- 35/40/77 */
int
ask_tracks()
{
    char c;

    puts("\n\tA) 35 tracks\n\tB) 40 tracks\n\tC) 77 tracks\n\nSelection: ");
    for (;;) {
        c = getchar();
        switch (c) {
        case '\033': return 0;
        case 'a': tracks = 35; puts("35 tracks selected.\n"); return 1;
        case 'b': tracks = 40; puts("40 tracks selected.\n"); return 1;
        case 'c': tracks = 77; puts("77 tracks selected.\n"); return 1;
        }
    }
}

/* 0x0e3c: ask_sides() -- 8" drives are single-sided, no prompt */
int
ask_sides()
{
    char c;

    if (inches == 8)
        return 1;
    puts("\n\tA) Single sided.\n\tB) Double sided.\n\nSelection:");
    for (;;) {
        c = getchar();
        switch (c) {
        case '\033': return 0;
        case 'a': case '1': sides = 1; puts("\tSingle sided.\n"); return 1;
        case 'b': case '2': sides = 2; puts("\tDouble sided.\n"); return 1;
        }
    }
}

/* ---------------------------------------------------------------------------
 * 0x0f33: do_format() -- the final "insert diskette, press RETURN" prompt.
 * Returns 1 to format, 0 to quit; '~' re-shows the summary and re-prompts.
 * ------------------------------------------------------------------------- */
int
do_format()
{
    char c;

    puts("\n");
    if (inches == 5)
        puts("Insert a write enabled diskette in 5 1/4 inch drive ");
    else
        puts("Insert a write enabled diskette in 8 inch drive ");
    putchar('0' + (drive & 3));         /* H0f5e */
    puts("Press <RETURN> to format, anything else to quit ");
    c = getchar();
    if (c == '~') {                     /* show the config and ask again */
        show_config();                  /* H1072 */
        do_format();
        return;
    }
    puts("\n");
    return (c == '\r' || c == '\n');
}

/* ---------------------------------------------------------------------------
 * 0x1072: show_config() -- the summary printed on '~'
 *   <inches> inch  <tracks> tracks  <sides>  <bytes> bytes per sector
 *   <single|double> density  <hard|soft> sectored  ... origin
 * ------------------------------------------------------------------------- */
void
show_config()
{
    putnum(inches);  puts(" inch\n");
    putnum(tracks);  puts(" tracks\n");
    if (inches == 5 && sides == 1)
        puts("single sided\n");
    else
        puts("double sided\n");
    putnum(bytes);   puts(" bytes per sector\n");
    if (density == 's')
        puts("single density\n");
    else
        puts("double density\n");
    if (holes == 2)
        puts("hard sectored\n");
    else
        puts("soft sectored\n");
    /* ... sectors per track, origin (H0fd8/H0fec) follow */
}

/* TODO: H12ce (ask_inches), H13a2 (ask_holes), H1516/H15b3 (sectors?),
 * H1451, H17ce/H3352 (drive ready + the real format), and the sector/geometry
 * write path that turns this config into SETTRACK + whole-track-write.
 */

/* 0x12ce: ask_inches() -- DIY "5 or 8 inch" */
int
ask_inches()
{
    char c;

    puts("\n\t A) 5\n\t B) 8\n\nSelection: ");
    for (;;) {
        c = getchar();
        switch (c) {
        case '\033': return 0;
        case 'a': case '\r': case '\n': case '5':
            inches = 5; puts("\t5 inch format selected\n"); return 1;
        case 'b': case '8':
            inches = 8; puts("\t8 inch format selected\n"); return 1;
        }
    }
}

/* 0x115d/0x1146/0x1311: putnum(n) -- decimal number, via H1311 (itoa) + H30a1 */
void
putnum(n)
    int n;
{
    puts(itoa(n));              /* H1311 -> buffer 0x547f, H30a1 */
}

/* ---------------------------------------------------------------------------
 * 0x116f: format_all() -- the format + verify loop.
 *   for track in 0..tracks-1, for side in 0..sides-1:
 *       H1b2d formats one track/side, H2dd2 reads it back; any failure
 *       returns 0 ("Probable bad diskette" / "Sector verification failure").
 * ------------------------------------------------------------------------- */
int
format_all()
{
    int track;
    int side;

    for (track = 0; track < tracks; track++)
        for (side = 0; side < sides; side++) {
            format_track();             /* H1b2d (the SETTRACK+write path) */
            if (!verify_track(track, side))  /* H2dd2 */
                return 0;
        }
    return 1;
}

/* TODO (the deepest layer, which turns the geometry into channel commands):
 *   H1b2d  format one track  (SETTRACK 0x2d + the computed write opcode)
 *   H2dd2  verify one track  (RTRACK 0x29 + compare)
 *   H17ce  "Drive not ready" probe;  H3352  the command-line format entry
 *   H2f28  pulse 0xEF / wait;  H2fda getchar;  H30a1 puts;  H306c putchar
 *   H3b88/H3dbc/H3a42/H1311  itoa/prmsg/atoi/itoa helpers
 *   H4c6e/H4c89/H4cf3/H4d26  16-bit mul/div/shift
 */

/* ---------------------------------------------------------------------------
 * 0x1b53: the DJ-DMA status->message table ([handler, code] pairs; 0 term,
 * then the default).  Codes match the firmware/djdma.c S_* values:
 *
 *   0x80 improper command  0x81 improper drive  0x82 drive not ready
 *   0x83 improper track    0x84 unreadable      0x85 no sync byte
 *   0x86 header CRC        0x87 seek error      0x88..0x8d header compare
 *   0x8e data CRC         0x8f improper sector  0x90 write protected
 *   0x91 lost data        0x92 lost command    0x93 timeout (ID mark)
 *   0x94 heads failed to home      else "Unknown error, returned code was %"
 *
 * 0x1da0: report_status(code) -- 0x40 (S_NORMAL) is not an error; otherwise
 * it decrements a retry budget (0x54f8) and prints the matching message.
 * ------------------------------------------------------------------------- */
void
report_status(code)
    int code;
{
    code &= 0xff;               /* 0x1da0 */
    if (code == 0x40)           /* S_NORMAL */
        return;
    if (--retry < 0) {          /* 0x54f8 decrement; on underflow H335b */
        /* ... timeout path ... */
    }
    /* print the message for `code` via the 0x1b53 table */
}

/* 0x1b2d: format_track(track) -- per-track progress; the real work is H324e */
void
format_track(track)
    int track;
{
    if (cmdline)                /* H51b8: no progress dots on the cmd line */
        return;
    if (track & 7)              /* a dot every 8th track */
        return;
    puts(".");
    H324e();                    /* the SETTRACK + whole-track-write */
}

/* TODO (deepest, needs a second disassembly pass - some bodies sit in the
 * runs the disassembler kept as DB): H324e (write one track), H2dd2 (verify),
 * H17ce (drive-ready probe), H3352/H335b (cmd-line format + timeout), and
 * the geometry helpers H4c6e/H4c89/H4cf3/H4d26 + itoa/atoi/prmsg.
 */

/* ===========================================================================
 * CORRECTION / the actual format mechanism
 *
 * fdj formats a soft-sectored disk by writing each sector individually with
 * WRSECT (0x21) - on the DJ-DMA, a soft-sectored write lays down the ID
 * header and the data together, which is the format - and then verifies by
 * reading the track back with RTRACK (0x29).  The command set fdj actually
 * uses is: SETDMA 0x23, RDSECT 0x20, WRSECT 0x21, GSTAT 0x22, RTRACK 0x29,
 * SETDRIVE 0x2e, SETRETRY 0x28, HALT 0x25.  There is no WTRACK (0x2a), no
 * SETTRACK (0x2d), no SETMP (0x31).  (Earlier I misread itoa's '-' (0x2d)
 * and hex-digit adjust (ADD A,27H) as SETTRACK and a computed opcode; the
 * disassembly and a byte-level re-decode both say otherwise.)
 *
 * The sector size and count the firmware needs for WRSECT come from its
 * per-drive tables, not from the program - which is why the DJ/DMA can
 * "format a diskette it does not understand" (sys/dj.h).
 * ========================================================================= */

/* 0x2c25: send_setdma(dma) - SETDMA + HALT, then run it */
void
send_setdma(dma)
    int dma;
{
    byte *p = channel;          /* 0x50 */
    *p++ = 0x23;                /* SETDMA */
    *p++ = dma;                 /* low */
    *p++ = dma >> 8;            /* high */
    *p++ = 0;
    *p++ = 0x25;                /* HALT */
    exec();                     /* H2f28 */
}

/* 0x2ca0: read_track(track, side, drive, sectab) - SETDMA then RTRACK */
void
read_track(track, side, drive, sectab)
{
    send_setdma(sectab);        /* H2c25 with the table address */
    p = channel;
    *p++ = 0x29;                /* RTRACK */
    *p++ = track;               /* DE+4 */
    *p++ = side;                /* DE+6 */
    *p++ = drive;               /* DE+8 */
    *p++ = sectab & 0xff;       /* table lo/hi/hi */
    *p++ = sectab >> 8;
    *p++ = 0;
    *p++ = 0x25;                /* HALT */
    exec();
}

/* 0x329e: write_sector(drive, track, side_sector, dma) - SETDMA then WRSECT */
int
write_sector(drive, track, side_sector, dma)
{
    send_setdma(dma);           /* H2c25(DE+10) */
    p = channel;                /* H54bd = 0x50 */
    *p++ = 0x21;                /* WRSECT */
    *p++ = track;               /* DE+6 */
    *p++ = side_sector;         /* DE+8 */
    *p++ = drive;               /* DE+4 */
    stat = p; *p++ = 0;         /* the status byte, remembered */
    *p++ = 0x25;                /* HALT */
    exec();                     /* H2f28 */
    report_status(*stat);       /* H1da0 */
    return (*stat == 0x40);     /* S_NORMAL */
}

/* 0x3352: the command-line format entry - just format_all() (H116f) */

/* TODO: H2dd2 (verify: RTRACK + compare), H17ce (drive-ready probe),
 * H2f28 exec, H2fda getchar, H30a1 puts, H306c putchar, and the library
 * (itoa H3b88, atoi H3a42, prmsg H3dbc) + 16-bit helpers H4c6e..H4d26.
 */

/* ---------------------------------------------------------------------------
 * 0x17ce: probe_drive() -- sense the drive, validate the diskette, and
 * reconcile the requested geometry with what the media actually is.
 *
 * This is the answer to "how does the sector size / density reach the
 * firmware": it does not.  fdj asks the firmware with GSTAT (H29f1 = 0x22)
 * and the firmware auto-detects the media's density, sector size, and
 * hard/soft sectoredness from the diskette itself; fdj then checks that
 * against what the user asked for.  That is why the DJ/DMA can format a
 * diskette it "does not understand" (sys/dj.h) - the media, not the program,
 * supplies the geometry.
 * ------------------------------------------------------------------------- */
int
probe_drive()
{
    int total = sectors * bytes;        /* H4d26 = 16-bit mul -> H53c2 */
    int status = sense_drive(drive);    /* H29f1 = GSTAT -> H548d */

    if (status == 0x82) {               /* S_UNREADY */
        report_status(0x82);
        puts(" Drive not ready\n");
        chosen = 1;
        return 0;
    }
    if (status & 0x40) {                /* write protected (SB3_WPROT) */
        puts("Read only diskette\n");
        chosen = 1;
        return 0;
    }
    /* hard vs soft sectored (SB1_HARD bit 0x02) must match -h */
    if ((status & 0x02 ? 1 : 0) != (holes == 2)) {
        puts("Wrong type of diskette\n");
        puts(holes == 2 ? "Hard sectored diskette required (10 hole)\n"
                        : "Soft sectored diskette required (1 hole)\n");
        return 0;
    }
    /* reconcile density / sides against the media and pick a format entry */
    if (inches == 5 && holes == 0 && density == 'd')
        entry = H217a;                  /* 5.25 soft double */
    else if (inches == 5 && density == 's')
        entry = H23f7;                  /* 5.25 soft single */
    else
        entry = H1efd;                  /* default */
    if (inches == 8 && (status & 0x04)) {   /* double sided 8" */
        sides = 2;
        puts("\tDouble sided.\n");
    } else if (inches == 8) {
        sides = 1;
        puts("\tSingle sided.\n");
    }
    H15ef(entry);                       /* use the detected entry */
    if (inches == 8)
        ...;                            /* (tail continues) */
    return 1;
}

/* TODO: H29f1 sense_drive (GSTAT), H15ef (apply a format entry), H1b10
 * (print status), H2dd2 (verify), H2f28 exec, H2fda/H30a1/H306c I/O, and
 * the library itoa/atoi/prmsg + 16-bit helpers H4c6e..H4d26.
 */

/* ---------------------------------------------------------------------------
 * the I/O + channel primitives
 * ------------------------------------------------------------------------- */

/* 0x29f1: sense_drive(drive) -> &channel[0]  -- GSTAT, then read the 3 status
 * bytes the firmware writes at channel+2 (dcb), +3 (slc), +4 (dsb).  This is
 * where the media's density/sector-size/sectoredness come from. */
char *
sense_drive(drive)
    int drive;
{
    byte *p = channel;
    *p++ = 0x22;        /* GSTAT */
    *p++ = drive;
    *p++ = 0;           /* dcb */
    *p++ = 0;           /* slc (sector length code) */
    *p++ = 0;           /* dsb */
    *p++ = 0x25;        /* HALT */
    exec(p);            /* H2f28 */
    return channel;     /* BC = 0x50 */
}

/* 0x2f28: exec(status) -- pulse the port (H2f15), then poll *status until the
 * controller answers; return *status == 0x40 (S_NORMAL). */
int
exec(status)
    byte *status;
{
    *status = 0;
    pulse();                    /* H2f15: OUT to 0xEF */
    while (*status == 0)        /* poll, with a 0xffff timeout */
        ;
    return *status == 0x40;
}

/* 0x2fda: getchar() -- BDOS 6 (direct console I/O) with 0xff = input */
char
getchar()
{
    char c = bdos(6, -1);       /* H010b */
    if (c == 0x03) {            /* ^C */
        H0605();
        exit();                 /* H38fa */
    }
    return c;
}

/* 0x30a1 puts / 0x306c putchar / 0x115d putnum: the string/char/number print
 * helpers; putnum is H1311 (itoa) + puts.  H1b10 prints a status message via
 * the 0x1b53 table.  H15ef applies a format-table entry to the globals. */

/* TODO: H2f15 pulse, H30a1 puts, H306c putchar, H1b10 print_status, H15ef
 * apply_entry, H2dd2 verify, and the library itoa H3b88 / atoi H3a42 /
 * prmsg H3dbc + 16-bit mul/div/shift H4c6e..H4d26.
 */

/* 0x2f15: pulse() -- OUT 0xEF, 0 (the DJ-DMA attention port), via H4b92 */
void
pulse()
{
    outport(0xEF, 0);           /* H4b92 */
}

/* 0x306c: putchar(c) -- BDOS 2, with CR before LF */
void
putchar(c)
    char c;
{
    if (c == '\n')
        bdos(2, '\r');          /* H010b */
    bdos(2, c);
}

/* 0x30a1: puts(s) -- strlen then print the run */
void
puts(s)
    char *s;
{
    print_run(s, 1, strlen(s)); /* H3254(s, 1, H3dfe(s)) */
}

/* 0x30c6: write_boot_track() -- for the 5.25" 1024-byte Micronix format,
 * build the boot sectors (0xe5 fill + boot code H2771/H28f1) and WRSECT
 * sectors 1 and 2 of track 0.  This is the last piece of the format path:
 * everything a format writes goes through write_sector (WRSECT). */
void
write_boot_track()
{
    char buf[1024];

    if (inches != 5 || bytes != 1024)
        return;
    memset(buf, 0xE5, 1024);            /* H39e3(buf, 0x400, 0xe5) */
    memcpy(buf, sides == 1 ? H2771 : H28f1, 0x100);   /* H3ac3 */
    write_sector(drive, 0, 1, buf);     /* H329e(drive, 0, 1, buf) */
    memset(buf, 0xE5, 1024);
    memcpy(buf, H2871, 0x80);
    write_sector(drive, 0, 2, buf);     /* H329e(drive, 0, 2, buf) */
}

/* TODO (library only, no more fdj logic): H3dfe strlen, H39e3 memset,
 * H3ac3 memcpy, H4b92 outport, H010b bdos, H3254 print_run, H1b10 print
 * status, H15ef apply entry, H2dd2 verify, and itoa H3b88 / atoi H3a42 /
 * prmsg H3dbc / 16-bit mul-div-shift H4c6e..H4d26.
 */

/* ---------------------------------------------------------------------------
 * 0x2dd2: verify_track(track, side) -- RTRACK the whole track into a status
 * table, then require every sector's status to be S_NORMAL (0x40).  Track 0
 * of an 8" disk is 26 sectors (single density); elsewhere the table's sector
 * count applies.  The read is retried up to 10 times.
 * ------------------------------------------------------------------------- */
int
verify_track(track, side)
    int track, side;
{
    int nsec = (inches == 8 && track == 0 && side == 0) ? 26 : sectors;
    byte sectab[26];
    int i;

    memset(sectab, 0, 26);                      /* H39e3(H549b, 26, 0) */
    for (i = 0; i < 10; i++)
        if (read_track(track, side, drive, sectab))   /* H2ca0 = RTRACK */
            break;
    if (i >= 10)
        return 0;                               /* the read never succeeded */
    for (i = 0; i < nsec; i++)
        if (sectab[i] != 0x40) {                /* not S_NORMAL */
            puts("Sector verification failure\n");  /* H2dc8 */
            report_status(sectab[i]);           /* H1da0 */
            return 0;
        }
    return 1;
}

/* ---------------------------------------------------------------------------
 * 0x15ef: record_config(entry) -- copy the current drive/geometry state into
 * the caller's 16-byte config record.  probe_drive calls it to build the
 * disk's parameter block from what GSTAT detected.  Field order (best-effort
 * read of the stores): [0]=drive [1]=tracks [3..4]=0 [5]=origin [12]=sides-1.
 * ------------------------------------------------------------------------- */
void
record_config(entry)
    char *entry;
{
    entry[0]  = drive;
    entry[1]  = tracks;
    entry[12] = sides - 1;
    entry[3]  = entry[4] = 0;
    entry[5]  = origin;         /* H53d4 */
    /* tail: per-inch / per-density adjustments (H164e..) */
}

/* ---------------------------------------------------------------------------
 * End of the fdj-specific logic.  Everything below this line in the binary is
 * the Whitesmiths C runtime (libws.a): itoa (0x3b88), atoi (0x3a42), prmsg
 * (0x3dbc), strlen (0x3dfe), memset (0x39e3), memcpy (0x3ac3), outport
 * (0x4b92), print_run (0x3254), and the 16-bit mul/div/shift helpers
 * (0x4c6e/0x4c89/0x4cf3/0x4d26) - all standard libc, nothing fdj-specific.
 * ------------------------------------------------------------------------- */
