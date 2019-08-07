/*
 * the hdca is the port-mapped, non-dma hard drive controller
 * used in the M10, M20 and M26
 */

#include "sim.h"

#define	HDCA_CSR 	0x50
#define	HDCA_SEC	0x51
#define	HDCA_FUNC   0x52
#define HDCA_DATA   0x53

// 0x50 - HDCA control port
#define W0_FRENBL   0x01    // enable drive select and function register
#define W0_RUN      0x02    // controller run enable
#define W0_DSKCLK   0x04    // use drive clock
#define W0_WPROT    0x08    // enable disk writes

bitdump(byte v, char **nv)
{
    int i;
    for (i = 0; i < 8; i++) {
        if (v & (1 << i)) {
            if (nv[i]) printf(nv[i]);
            v &= ~(1 << i);
            if (v) printf("|");
        }
    }
}

char *csrw[] = { "frenbl", "run", "dskclk", "wprot", 0, 0, 0, 0 };
static void
wr_hdca_csr(portaddr p, byte v)
{
    printf("hdca csr -> 0x%x", v);
    bitdump(v, csrw);
    printf("\n");
}

// 0x51 - HDCA command register
#define W1_RAMDATA  0       // reset ram pointer to data
#define W1_READSEC  1       // read a sector
#define W1_READHDR  3       // read a sector header
#define W1_WRITESEC 5       // write a sector
#define W1_WRITEHDR 7       // write a sector header
#define W1_RAMHDR   8       // reset ram pointer to header

static void
wr_hdca_cmd(portaddr p, byte v)
{
    switch (v) {
    case 0: printf("hdca cmd: reset ram pointer to sector\n"); break;
    case 8: printf("hdca cmd: ram pointer to sector\n"); break;
    case 1: printf("hdca cmd: read sector\n"); break;
    case 3: printf("hdca cmd: read header\n"); break;
    case 5: printf("hdca cmd: write sector\n"); break;
    case 7: printf("hdca cmd: write header\n"); break;
    default:
        printf("hdca cmd: unknown command\n");
    }
    printf("hdca sec -> 0x%x\n", v);
}

// 0x52 HDCA 
static void
wr_hdca_func(portaddr p, byte v)
{
    printf("hdca func -> 0x%x\n", v);
}

static void
wr_hdca_data(portaddr p, byte v)
{
    printf("hdca data -> 0x%x\n", v);
}

// 0x50 - primary status register
#define R0_NTRK0    0x01    // 0 if track 0
#define R0_OPDONE   0x02    // op done
#define R0_COMPLT   0x04    // all seeks complete
#define R0_TIMOUT   0x08    // time out
#define R0_NFAULT   0x10    // not fault
#define R0_NREADY   0x20    // not ready
#define R0_ILEVEL   0x40    // index toggle
#define R0_HALT     0x80    // not busy

static byte
rd_hdca_csr(portaddr p)
{
    dumpcpu();
    printf("input hdca primary status\n");
    return 0xff;
}

// 0x51 - secondary status register
#define R1_REV      0xc0    // board revision
#define R1_DATACRC  0x02    // data crc error
#define R1_SDONE    0x01    // command done (irq?)

static byte
rd_hdca_stat(portaddr p)
{
    printf("input hdca secondary status\n");
    return 0xff;
}

// 0x52 - clear interrupt
static byte
rd_hdca_intclr(portaddr p)
{
    printf("input hdca intclr\n");
    return 0xff;
}

static byte
rd_hdca_data(portaddr p)
{
    printf("input hdca data\n");
    return 0xff;
}

static int
hdca_init()
{
	register_output(HDCA_CSR, &wr_hdca_csr);
	register_output(HDCA_SEC, &wr_hdca_cmd);
	register_output(HDCA_FUNC, &wr_hdca_func);
	register_output(HDCA_DATA, &wr_hdca_data);
	register_input(HDCA_CSR, &rd_hdca_csr);
	register_input(HDCA_SEC, &rd_hdca_stat);
	register_input(HDCA_FUNC, &rd_hdca_intclr);
	register_input(HDCA_DATA, &rd_hdca_data);
    return 0;
}

/*
 * this grammar makes the compiler call this function before main()
 * this means we can add drivers by just adding them to the link
 */
__attribute__((constructor))
void
register_hdca_driver()
{
    register_startup_hook(hdca_init);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
