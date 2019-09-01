/*
 * the compupro disk1 is a fairly stupid i8272 controller with
 * 24 bit dma.
 * it has a boot rom onboard that is segmented into different
 * boot routines, selected by a dip switch.
 * the boot rom is enabled at reset, along with phantom, and
 * is disabled with a 0 written to relative port 3
 */

#include "sim.h"
#include <sys/ioctl.h>
#include <stdio.h>

#define DISK1_BASE  0xc0
#define F

static paddr resetchannel = DEF_CCA;    // the 24 bit channel command pointer
static paddr channel = DEF_CCA;         // the 24 bit channel command pointer
static paddr dmaaddr;                   // the 24 bit dma address
static int retrylimit = 10;
static int djdma_running = 0;
static char secbuf[2048];
static void *imdp[8];
static int need_intack;

/*
 * start the channel
 */
static void
pulse_djdma(portaddr p, byte v)
{
    int i;
    struct djcmd *cmd;
    unsigned char code;

    if (need_intack) {
        /*
         * if the last command was setintr, we are still "running" that command
         * until we get this pulse.  fill in the status and advance
         */
        physwrite(channel + 1, S_NORMAL);
        channel += 2;
        need_intack = 0;
    } else {
        /*
         * fetch from the reset channel address
         */
        channel = resetchannel;
    }
    djdma_running = 1;    
    /*
     * run channel commands until we are told to stop
     */
    cmd = 0;
    while (djdma_running) {
        if (verbose & V_DJDMA) printf("djdma: fetch channel 0x%x\n", channel);
        code = physread(channel);
        for (i = 0; i < (sizeof(djcmd) / sizeof(djcmd[0])); i++) {
            if (djcmd[i].code == code) {
                cmd = &djcmd[i];
            }
        }
        if (!cmd) {
            djdma_running = 0;
            if (verbose & V_DJDMA) printf("djdma: unknown command %d %x\n", code, code);
        } else {
            if (verbose & V_DJDMA) printf("djdma: command %d %x %s\n", code, code, cmd->name);
            i = (cmd->handler)();
            if (cmd->status) {
                physwrite(channel + cmd->status, i);
            }
            channel += cmd->increment;
        }
    }
}

/*
 * set the dma address from the channel, little-endian, no status
 */
static unsigned char
setdma()
{
    dmaaddr = physread(channel + 1) + 
        (physread(channel + 2) << 8) +
        (physread(channel + 3) << 16);
    if (verbose & V_DJDMA) printf("djdma setdma %x\n", dmaaddr);
    return 0;
}

/*
 * set the default channel address from the channel, little-endian, no status
 */
static unsigned char
setchannel()
{
    resetchannel = physread(channel + 1) + 
        (physread(channel + 2) << 8) +
        (physread(channel + 3) << 16);
    if (verbose & V_DJDMA) printf("djdma setchannel %x\n", resetchannel);
    return 0;
}

/*
 * end a channel command stream
 */
static unsigned char
djhalt()
{
    djdma_running = 0;
    return S_NORMAL;
}

/*
 * branch in channel - change the fetch address for the channel program
 */
static unsigned char
branch()
{
    channel = physread(channel + 1) + 
        (physread(channel + 2) << 8) +
        (physread(channel + 3) << 16);
    if (verbose & V_DJDMA) printf("djdma: branch %x\n", channel);
    return 0;
}

/*
 * read a sector specifiec in 3 address bytes in channel.  1 status byte
 */
static unsigned char
readsec()
{
    unsigned char trk;
    unsigned char sec;
    unsigned char drive;
    unsigned char side;
    unsigned char status;
    int bytes;

    trk = physread(channel + 1);
    sec = physread(channel + 2);
    side = sec & 0x80;
    sec &= 0x7f;
    drive = physread(channel + 3);

    if (verbose & V_BIO) {
        printf("djdma: read sector drive:%d track:%d sec:%d side:%d\n",
            drive, trk, sec, side);
    }
    if (verbose & V_DJDMA) printf("djdma: read sector drive:%d track:%d sec:%d side:%d\n",
        drive, trk, sec, side);
    /* read drive, getfdprmtrk, sec, side into dmaaddr */
    if (imdp[drive]) {
        bytes = imd_read(imdp[drive], drive, trk, side, sec, secbuf);
        if (bytes > 0) {
            // hexdump(secbuf, bytes);
            copyout(secbuf, dmaaddr, bytes);
            status = S_NORMAL;    
        } else {
            status = S_NOREAD;
        }
    } else {
        status = S_ILLDRV;
    }
    return status;
}

/*
 * generate an interrupt.  the next output pulse is an intack, and does
 * not start the channel.
 */
static unsigned char
setintr()
{
    need_intack = 1;
    if (verbose & V_DJDMA) printf("djdma: setintr\n");
    return 0;
}

/*
 * write a sector using data at the dma address to (trk, sec, drive)
 */
static unsigned char
writesec()
{
    unsigned char trk;
    unsigned char sec;
    unsigned char drive;
    unsigned char side;
    unsigned char status;
    int bytes;

    trk = physread(channel + 1);
    sec = physread(channel + 2);
    side = sec & 0x80;
    sec &= 0x7f;
    drive = physread(channel + 3);

    if (verbose & V_BIO) {
        printf("djdma: write sector drive:%d track:%d sec:%d side:%d\n",
            drive, trk, sec, side);
    }
    if (verbose & V_DJDMA) printf("djdma: write sector drive:%d track:%d sec:%d side:%d\n",
        drive, trk, sec, side);
    if (imdp[drive]) {
        imd_trkinfo(imdp[drive], trk, 0, &bytes);
        copyin(secbuf, dmaaddr, bytes);
        bytes = imd_write(imdp[drive], drive, trk, side, sec, secbuf);
        if (bytes > 0) {
            status = S_NORMAL;    
        } else {
            status = S_PROT;
        }
    } else {
        status = S_ILLDRV;
    }
    return status;
}

/*
 * sense drive status
 */
static unsigned char
sense()
{
    unsigned char drive;

    drive = physread(channel + 1);
    if (verbose & V_DJDMA) printf("djdma: sense drive:%d\n", drive);
    physwrite(channel + 2, 
        // SB1_HARD SB1_FIVE SB1_MTRCON SB1_NORDY SB1_NOHDLD
        SB1_DD | SB1_HDLD);
    physwrite(channel + 3, // SB2_128 SB2_256 SB2_512 
        SB2_1024);
    physwrite(channel + 4, // SB3_SERIN SB3_TRK0 SB3_WPROT SB3_DSDD8 | 
        SB3_INDEX | SB3_RDY);
    return S_NORMAL;
}

/*
 * set retry limit
 */
static unsigned char
setretry()
{
    unsigned char drive;

    retrylimit = physread(channel + 1);
    if (verbose & V_DJDMA) printf("djdma: setretry %d\n", retrylimit);
    return 0;
}

/*
 * set logical drive
 */
static unsigned char
setdrive()
{
    unsigned char drive;

    drive = physread(channel + 1);
    if (verbose & V_DJDMA) printf("djdma: set drive:%d\n", drive);
    return S_NORMAL;
}

/*
 * read track
 */
static unsigned char
readtrk()
{
    unsigned char drive;
    unsigned char side;
    unsigned char trk;
    paddr sectab;
    int secs;
    int secsize;
    int i;
    int bytes;

    trk = physread(channel + 1);
    side = physread(channel + 2);
    drive = physread(channel + 3);
    sectab = 
        physread(channel + 4) + 
        (physread(channel + 5) << 8) +
        (physread(channel + 6) << 16);
    imd_trkinfo(imdp[drive], trk, &secs, &secsize);

    if (verbose & V_DJDMA) printf("djdma: readtrk drive:%d trk:%d side:%x sectab:%x secs:%d\n", drive, trk, side, sectab, secs);
    for (i = 0; i < secs; i++) {
        if (physread(sectab + i) == 0xff) {
            continue;
        }
        bytes = imd_read(imdp[drive], drive, trk, side, i + 1, secbuf);
        if (bytes > 0) {

            copyout(secbuf, dmaaddr + secsize * i, bytes);
            if (verbose & V_DJDMA) printf("copyout to %x for %d\n", dmaaddr + secsize * i, bytes);
            physwrite(sectab + i, S_NORMAL);
        } else {
            physwrite(sectab + i, S_NOREAD);
        }
    }
    return S_NORMAL;
}

/*
 * write track
 */
static unsigned char
writetrk()
{
    if (verbose & V_DJDMA) printf("djdma: writetrk\n");
    return S_NORMAL;
}

/*
 * set track count
 */
static unsigned char
settrk()
{
    if (verbose & V_DJDMA) printf("djdma: set tracks\n");
    return S_NORMAL;
}

/*
 * set drive timing
 */
static unsigned char
settiming()
{
    return S_NORMAL;
}

int poll_enabled = 1;
int poll_registered;

static void
djdma_poll_func()
{
    int bytes;
    char conschar;

    if (poll_enabled) {
        // if have not read the last character, no point in checking
        if (physread(0x3f) == S_NORMAL) {
            return;
        }
        ioctl(terminal_fd, FIONREAD, &bytes);
        if (bytes) {
            if (read(terminal_fd, &conschar, 1) != 1) {
                printf("djdma_poll_func: read problem\n");
                return;
            }
            physwrite(SERDATA, conschar);
            physwrite(SERFLAG, S_NORMAL);
        }
    }
}

/*
 * serial in
 */
static unsigned char
serin()
{
    switch (physread(channel + 1)) {
    case 0:
        poll_enabled = 0;
        break;
    case 1:
        poll_enabled = 1;
        break;
    }
    return S_NORMAL;
}

/*
 * serial out
 */
static unsigned char
serout()
{
    byte outch;

    if (!poll_registered) {
        register_poll_hook(&djdma_poll_func);
        poll_registered = 1;
    }
    outch = physread(channel + 1);
    write(terminal_fd, &outch, 1);
    return S_NORMAL;
}

/*
 * set djdma memory
 * this is used by the bios to set drive speed, settling time, track count
 * this is a shitty API
 */
static unsigned char
write_djmem()
{
    return S_NORMAL;
}

/*
 * read djdma memory
 */
static unsigned char
read_djmem()
{
    return S_NORMAL;
}

/*
 * execute controller code
 */
static unsigned char
djexec()
{
    return S_NORMAL;
}

byte lowmem_save[38];
byte bootstrap[] = {
    0x21, 0x4a, 0x00,   //      ld hl, 004a
    0x36, 0x00,         //      ld (hl), 0
    0x7e,               // loop:ld a, (hl)
    0xb7,               //      or a,a
    0xca, 0x3d, 0x00,   //      jp z, loop
    0xfe, 0x40,         //      cp a,0x40
    0xc2, 0x3d, 0x00,   //      jp nz, loop
    0xc3, 0x80, 0x00,   //      jp 80
    0xff                //      db ff
};

extern char **drivenames;
/*
 * hook up the registers and do reset processing
 * this runs the load sector 0 to physical memory 0
 */
static int
djdma_init()
{
    int i;
    char **s;

	register_output(DJDMA_PORT, &pulse_djdma);

    if (drivenames) {
        for (i = 0; drivenames[i]; i++) {
            imdp[i] = load_imd(drivenames[i]);
            if (!imdp[i]) {
                printf("djdma_init: could not open %s\n",
                    drivenames[i]);
                return 1;
            }
        }
    }

    for (i = 0; i < 38; i++) {
        lowmem_save[i] = physread(i);
        physwrite(i, 0);
    }
    for (i = 0; i < sizeof(bootstrap); i++) {
        physwrite(i + 0x38, bootstrap[i]);
    }
    imd_read(imdp[0], 0, 0, 0, 1, secbuf);
    copyout(secbuf, 0x80, 0x80);
    physwrite(0x4a, 0x40);
    return 0;
}

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_djdma_driver()
{
    register_startup_hook(djdma_init);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
