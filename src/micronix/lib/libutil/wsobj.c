/*
 * wsobj.c - Whitesmith's object file format common functions
 *
 * Lives in libutil, shared by the assembler (asz), the linker (ld)
 * and nm: the segment-name table and the relocation-table writer.
 */

#include <stdio.h>
#ifdef linux
#include <stdlib.h>
#endif

#include <obj.h>

/*
 * segment name strings for debugging
 */
char *wsSegNames[] = { "undef", "text", "data", "bss", "abs", "ext" };

/*
 * write a byte to file
 */
void
wsWrByte(fp, b)
FILE *fp;
unsigned char b;
{
    fputc(b, fp);
}

/*
 * write a 16-bit little-endian word
 */
void
wsWrWord(fp, val)
FILE *fp;
unsigned short val;
{
    wsWrByte(fp, val & 0xff);
    wsWrByte(fp, val >> 8);
}

/*
 * encode a relocation bump value
 * bump = distance from last relocation to this one
 */
void
wsEncBump(fp, bump)
FILE *fp;
int bump;
{
    while (bump >= REL_BUMP_LIM) {
        wsWrByte(fp, REL_BUMP_EXT + REL_BUMP_MAX);
        wsWrByte(fp, 0xff);
        bump -= REL_BUMP_LIM;
    }
    if (bump >= REL_BUMP_EXT) {
        bump -= REL_BUMP_EXT;
        wsWrByte(fp, (bump >> 8) + REL_BUMP_EXT);
        wsWrByte(fp, bump & 0xff);
    } else if (bump) {
        wsWrByte(fp, bump);
    }
}

/*
 * encode a relocation control byte
 * seg = segment type (SEG_TEXT, SEG_DATA, SEG_BSS, SEG_ABS)
 *       or -1 for symbol reference
 * symidx = symbol index (only used if seg == -1)
 * hilo = REL_WORD (0), REL_LO (1), or REL_HI (2)
 */
void
wsEncReloc(fp, seg, symidx, hilo)
FILE *fp;
int seg;
int symidx;
int hilo;
{
    int control;

    switch (seg) {
    case SEG_ABS:
        wsWrByte(fp, REL_ABS + hilo);
        break;
    case SEG_TEXT:
        wsWrByte(fp, REL_TEXT + hilo);
        break;
    case SEG_DATA:
        wsWrByte(fp, REL_DATA + hilo);
        break;
    case SEG_BSS:
        wsWrByte(fp, REL_BSS + hilo);
        break;
    default:
        /* symbol reference - encode index with hilo in low 2 bits */
        control = symidx + REL_SYM_OFS;
        if (control < REL_EXT_THR1) {
            wsWrByte(fp, ((control + REL_SYM_SHIFT) << 2) | hilo);
        } else if (control < REL_EXT_THR2) {
            wsWrByte(fp, REL_SYM_EXT | hilo);
            wsWrByte(fp, control - REL_EXT_THR1);
        } else {
            control -= REL_EXT_THR2;
            wsWrByte(fp, REL_SYM_EXT | hilo);
            wsWrByte(fp, (control >> 8) + REL_EXT_LONG);
            wsWrByte(fp, control);
        }
        break;
    }
}

/*
 * terminate a relocation table
 */
void
wsEndReloc(fp)
FILE *fp;
{
    wsWrByte(fp, 0);
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
