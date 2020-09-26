/*
 * the 8250 abstraction - our serial port can talk to an xterm or console
 * there's no reason why this couldn't be any pty as well
 */

/*
 * there's one of these for every ACE.
 */
struct ace {
    char *name;         // passed in
    int index;          // 

    int outfd;          // pipe to xterm, etc
    int infd;

    byte lcr;           // line control
    byte lsr;           // line status register
    byte mcr;           // modem control character
    byte msr;           // modem status character
    byte dll;           // baud rate lo
    byte dlm;           // baud rate hi
    byte inte;          // interrupt enables
    byte loop;          // loopback count 
    byte loopc;         // loopback character
    byte inti;          // interrupt id
    int_line vector;    // assert when something happens
    byte flags;
#define ACE_TXPEND      0x01
#define ACE_LOOPC       0x02
    byte txpend;        // we have a write in flight
    portaddr base;
};

struct ace *acep;       // current selected ace
struct ace *acea;       // array of allocated ace's
static int nace;

static char *lcr_bits[] = {"WLS0","WLS1","STB","PEN","EPS","STP","SBRK","DLAB"};
static char *lsr_bits[] = {"DR","OVER","PERR","FERR","BRK","TXE","TE", 0 };
static char *mcr_bits[] = {"DTR","RTS","OUT1","OUT2","LOOP",0,0,0 };
static char *msr_bits[] = {"DCTS","DDSR","TERI","DDCD","CTS","DSR","RI","DCD"};
static char *inte_bits[] = {"RX","TXEMPTY","RXSTAT","MDMSTAT",0,0,0,0 };
static char *inti_bits[] = {"NOINT","INTI-0","INTI-1",0,0,0,0,0 };
static char *ace_bits[] = {"TXP", "LOOPC", 0, 0, 0, 0, 0, 0 };

void *
create_8250(char *name, int index, portaddr p, int_line interrupt)
{
    struct ace *ap;

    if ((index + 1) > nace) {
        acea = realloc(acea, sizeof(*ap) * (index + 1));
    }
    acea[index] = ap = malloc(sizeof(*ap));
    ap->name = strdup(name);
    ap->index = index;
    ap->vector = interrupt;
    ap->base = p;
}

static int
init_8250()
{
    struct termios tio;
    int termmask = 0;
    int i;
    int pollpid;
    char tname[10];

    atexit(exit_hook);

    for (i = 0; i < 3; i++) {
        ace[i].infd = -1;
    }

    if (config_sw & CONF_SET) {
        termmask = (config_sw >> 8) & 0x7;
    }

    mysignal(SIGIO, sigio_handler);

    for (i = 0; i < 3; i++) {
        sprintf(tname, "uart%d", i);
        ace_init(i, strdup(tname), vi_3 + i);
    }

    // if terminal 0 is not an xterm, use mytty - this requires poll child to get SIGIO
    if (!(termmask & 0x1)) {
        int pipe_down[2];
        int pipe_up[2];
        int mypid = getpid();

        termfd = open(myttyname, O_RDWR);
        if (termfd == -1) {
            perror("terminal");
        }
        tcgetattr(termfd, &original_tio);

        pipe(pipe_down);
        pipe(pipe_up);

        pollpid = fork();
        if (!pollpid) {                     // poll child
            struct timeval timeout;
            fd_set readfds;
            int fds;
            char buf[1024];

            tcgetattr(termfd, &tio);
            cfmakeraw(&tio);
            tcsetattr(termfd, TCSANOW, &tio);
            fds = pipe_down[0];
            if (fds < termfd) fds = termfd;
            fds++;

            while (1) {
                FD_ZERO(&readfds);
                FD_SET(termfd, &readfds);            
                FD_SET(pipe_down[0], &readfds);            
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                select(fds, &readfds, 0, 0, &timeout);
                ioctl(termfd, FIONREAD, &i);
                if (i) {
                    if (i > sizeof(buf)) i = sizeof(buf);
                    i = read(termfd, buf, i);
                    write(pipe_up[1], buf, i);
                    kill(mypid, SIGIO);
                }
                ioctl(pipe_down[0], FIONREAD, &i);
                if (i) {
                    if (i > sizeof(buf)) i = sizeof(buf);
                    i = read(pipe_down[0], buf, i);
                    write(termfd, buf, i);
                }
            }
        }
        ace[0].infd = pipe_up[0];
        ace[0].outfd = pipe_down[1];
    }

    // create xterms
    for (i = 0; i < 3; i++) {
        if (termmask & (1 << i)) {
            sprintf(tname, "multio-%d", i);
            open_terminal(tname, SIGIO, &ace[i].infd, &ace[i].outfd, 0, 0);
        }
    }

    terminal_fd_in = ace[0].infd;
    terminal_fd_out = ace[0].outfd;
    return 0;
}
}

#ifdef notdef
struct regdump {
    char *regname;
    unsigned char value;
    char **bitdef;
};

struct regdump *
ace_dump(void *iap, struct regdump *rp)
{
    struct ace *ap = (struct ace *)iap;
    struct regdump *r;
    r = rp = realloc(rp, sizeof(*rp) * 9);
    r->name = "lcr"; r->value = ap->lcr; r->bitdef = lcr_bits; r++;
    r->name = "lsr"; r->value = ap->lsr; r->bitdef = lsr_bits; r++;
    r->name = "mcr"; r->value = ap->mcr; r->bitdef = mcr_bits; r++;
    r->name = "msr"; r->value = ap->msr; r->bitdef = msr_bits; r++;
    r->name = "inte"; r->value = ap->inte; r->bitdef = inte_bits; r++;
    r->name = "inti"; r->value = ap->inti; r->bitdef = inti_bits; r++;
    r->name = "dll"; r->value = ap->dll; r->bitdef = 0; r++;
    r->name = "dlm"; r->value = ap->dlm; r->bitdef = 0; r++;
    r->name = "flag"; r->value = ap->flag; r->bitdef = ace_bits; r++;
    r->name = (char *)0;
}
#endif

static struct ace *acep;
static int trace_8250;

#define TXE_TIMEOUT     5000     // how long to wait before setting LSR_TXE

// linestat                 base + 5    0x4d
#define LSR_DR      0x01    // input data ready
#define LSR_OVER    0x02    // input data overrun
#define LSR_PERR    0x04    // input parity error
#define LSR_FERR    0x08    // input framing error
#define LSR_BRK     0x10    // break
#define LSR_TXE     0x20    // transmit buffer empty
#define LSR_TE      0x40    // transmitter empty

// modem control            base + 4    0x4c
#define MCR_DTR     0x01    // data terminal ready
#define MCR_RTS     0x02    // request to send
#define MCR_LOOP    0x10    // loopback

// line control             base + 3    0x4b
#define LCR_82      0x07    // 8 data bits, 2 stop bits
#define LCR_DLAB    0x80    // accessio baud in inte, txb

// interrupt enable         base + 1    0x49
#define INTE_RDAV   0x01    // read available
#define INTE_TXE    0x02    // tx hold empty
#define INTE_RLINE  0x04    // read line status change
#define INTE_MDM    0x08    // modem status change

// interrupt identify       base + 2    0x4a
#define INTI_NOINT  0x01    // no interrupt
#define INTI_MASK   0x06    // mask of interrupt causes
#define     INTI_RSTAT  0x6     // reciever status
#define     INTI_RDAV   0x4     // rx char available
#define     INTI_TXE    0x2     // tx buffer empty
#define     INTI_MDM    0x0     // modem status change

// modem status
#define MSR_DCTS    0x01
#define MSR_DDSR    0x02
#define MSR_TERI    0x04
#define MSR_DDCD    0x08
#define MSR_CTS     0x10
#define MSR_DSR     0x20
#define MSR_RI      0x40
#define MSR_DCD     0x80

char *
printable(char v)
{
    static char sbuf[10];

    if (v >= ' ' && v < 0x7f) {
        sprintf(sbuf, "%c", v);
    } else switch(v) {
    case '\n':  return "\\n";
    case '\t':  return "\\t";
    case '\r':  return "\\r";
    case '\b':  return "\\b";
    case 0:    return "NULL";
    case 26:    return "^Z";
    default:
        sprintf(sbuf, "%02x %d", v, v);
        break; 
    }
    return sbuf;
}

/*
 * set the state of the interrupt line conditioned on enable bits
 * these are prioritized we only implement TXE and RDAV.
 */
void
uart_interrupt_check(int i)
{
    struct ace *ap = &ace[i];
    int bytes;

    if (ap->infd == -1) {
        return;
    }

    if (ioctl(ap->infd, FIONREAD, &bytes) == -1) {
        message("uart_interrupt_check ioctl error\n");
    }

    ap->inti = INTI_NOINT;
    if ((ap->inte & INTE_RDAV) && bytes) {
        ap->inti = INTI_RDAV;
    } else if (ap->inte & INTE_TXE) {
        if (!ap->txpend) {
            ap->inti = INTI_TXE;
        }
    }
#ifdef notdef
    if (trace & trace_uart) {
        printf("uart_interrupt_check %s inte: %02x inti: %02x %s txpend: %d bytes: %d\n",
            ap->name, ap->inte, ap->inti, bitdef(ap->inti, inti_bits), ap->txpend, bytes);
    }
#endif
    if (ap->inti == INTI_NOINT) {
        set_interrupt(ap->vector, int_clear);
    } else {
        set_interrupt(ap->vector, int_set);
    }
}

static void
ace_init(int i, char *name, int_line vector)
{
    struct ace *ap = &ace[i];

    ap->msr = MSR_CTS | MSR_DSR /* | MSR_RI */ | MSR_DCD;
    ap->line = i;
    ap->name = name;
    ap->vector = vector;
    reg_intbit(vector, ap->name);
    register_interrupt(vector, vi_handler);
    ap->infd = -1;
    ap->outfd = -1;
}

/*
 * recieve buffer.  
 * the selftest uses the loopback function, so we need to handle it.
 * while loopback is set in the mcr, then we return any character written to txb.
 * if LCR_DLAB is set, access to lsb of baud rate divisor
 */
static byte 
rd_rxb(portaddr p) 
{
    byte retval;
    int bytes;

    if (acep->lcr & LCR_DLAB) {
        return acep->dll;
    }
    if (acep->mcr & MCR_LOOP) {
        if (!acep->loop) {
            if (trace & trace_uart) printf("%s: read unwritten loopback\n", acep->name);
        }
        acep->loop = 0;
        retval = acep->loopc;
    } else if (acep->infd == -1) {
        retval = 0;
    } else {
        ioctl(acep->infd, FIONREAD, &bytes);
        if (bytes) {
            if (read(acep->infd, &retval, 1) != 1) {
                if (trace & trace_uart) printf("%s: rd_rxb failed\n", acep->name);
            }
        } else {
            retval = 0;
        }
    }
    // if we set a read interrupt, maybe clear it
    uart_interrupt_check(acep->line);

    if (trace & trace_uart) {
        printf("%s: read rxb = %s%s\n", acep->name,
            acep->mcr & MCR_LOOP ? "(loopback) " : "", printable(retval));
    }
    return retval;
}

/*
 * a timeout is set to call this after TXE_TIMEOUT
 */
void
txe_set(int i)
{
    struct ace *ap = &ace[i - 1];

    if (trace & trace_uart) {
        printf("txe_set: %s %d\n", ap->name, i);
    }
    ap->lsr |= LSR_TXE;
    ap->txpend = 0;
    uart_interrupt_check(ap->line);
}

/*
 * transmitter buffer
 * if LCR_DLAB is set, access to lsb of baud rate divisor
 */
static void
wr_txb(portaddr p, byte v) 
{
    if (acep->lcr & LCR_DLAB) {
        acep->dll = v;
        return;
    }
    if (acep->mcr & MCR_LOOP) {
        acep->loopc = v;
        acep->loop = 1;
    } else if (acep->outfd != -1) {
        write(acep->outfd, &v, 1);
    }
    if (trace & trace_uart) {
        printf("%s: write txb %s%s\n", acep->name,
            acep->mcr & MCR_LOOP ? "(loopback) " : "", printable(v));
    }
    if (acep->outfd != -1) {
        acep->txpend = 1;
        timeout("uart_txe", TXE_TIMEOUT, txe_set, group);
        uart_interrupt_check(acep->line);
    }
}

/*
 * interrupt enable register
 * if LCR_DLAB is set, access to msb of baud rate divisor
 */
static byte 
rd_inte(portaddr p)
{
    if (acep->lcr & LCR_DLAB) {
        return acep->dlm;
    }
    if (trace & trace_uart) {
        printf("%s: read inte %02x %s\n", acep->name, acep->inte, bitdef(acep->inte, inte_bits));
    }
    return acep->inte;
}

static void
wr_inte(portaddr p, byte v)
{
    if (acep->lcr & LCR_DLAB) {
        acep->dlm = v;
        return;
    }
    acep->inte = v;
    if (trace & trace_uart) {
        printf("%s: write inte %02x %s\n", acep->name, acep->inte, bitdef(acep->inte, inte_bits));
    }
    uart_interrupt_check(acep->line);
}

/*
 * line control register
 * contains the divisor access bit that changes the meaning of txb,rxb,and inte
 */
static byte 
rd_lcr(portaddr p)
{
    if (trace & trace_uart) {
        printf("%s: read lcr %02x %s\n", acep->name, acep->lcr, bitdef(acep->lcr, lcr_bits));
    }
    return acep->lcr;
}

static void
wr_lcr(portaddr p, byte v)
{
    acep->lcr = v;
    if (trace & trace_uart) {
        printf("%s: write lcr %02x %s\n", acep->name, acep->lcr, bitdef(acep->lcr, lcr_bits));
    }
}

static byte last_lsr[3];
/*
 * line status register.
 */
static byte 
rd_lsr(portaddr p)
{
    int bytes = 0;

    if (acep->mcr & MCR_LOOP) {
        bytes = acep->loop;
    } else if (acep->infd != -1) {
        ioctl(acep->infd, FIONREAD, &bytes);
    }
    acep->lsr = 0;
    acep->lsr |= ((acep->txpend == 0) ? LSR_TXE : 0) | (bytes ? LSR_DR : 0);

    if (trace & trace_uart) {
        if (last_lsr[group - 1] != acep->lsr) {
            printf("%s: read linestat %02x %s\n", acep->name, acep->lsr, bitdef(acep->lsr, lsr_bits));
            last_lsr[group - 1] = acep->lsr;
        }
    }
    return acep->lsr;
}

// the chip docs say this register is not well defined for write.
static void
wr_lsr(portaddr p, byte v)
{
    if (trace & trace_uart) printf("%s: write linestat %02x %s\n", acep->name, v, bitdef(v, lsr_bits));
}

/*
 * modem control register.
 * contains the loopback enable
 */
static byte 
rd_mcr(portaddr p)
{
    if (trace & trace_uart) {
        printf("%s: read mdmctl %02x %s\n", acep->name, acep->mcr, bitdef(acep->mcr, mcr_bits));
    }
    return acep->mcr;
}

static void
wr_mcr(portaddr p, byte v)
{
    // if a new set of loopback
    if ((v & MCR_LOOP) && !(acep->mcr & MCR_LOOP)) {
        acep->loop = 0;
    }
    acep->mcr = v;
    if (trace & trace_uart) {
        printf("%s: write mdmctl %02x %s\n", acep->name, acep->mcr, bitdef(acep->mcr, mcr_bits));
    }
}

/*
 * the interrupt identify register indicates which interrupt causes are pending
 * each of the 4 interrupts are reset differently: (in order)
 * 110 - reciever line error or break  - read the lsr
 * 100 - reciever data available - read the rxd
 * 010 - transmitter buffer empty - writing the tx or reading this cause from inti
 * 000 - CTS, DSR, RI, DCD - read msr
 */
static byte 
rd_inti(portaddr p)
{
    if (trace & trace_uart) {
        printf("%s: read inti %02x %s\n", 
            acep->name, acep->inti, bitdef(acep->inti, inti_bits));
    }
    return acep->inti;
}

static byte 
rd_msr(portaddr p)
{
    if (trace & trace_uart) {
        printf("%s: read mdmstat %02x %s\n", 
            acep->name, acep->msr, bitdef(acep->msr, msr_bits));
    }
    return acep->msr;
}

// it is unclear from the chip doc what happens when you write this register
static void 
wr_msr(portaddr p, byte v)
{
    acep->msr = v;

    if (trace & trace_uart) {
        printf("%s: write mdmstat %02x %s\n", acep->name, acep->msr, bitdef(acep->msr, msr_bits));
    }
}

int terminal_fd_in;
int terminal_fd_out;

int termfd;
char *myttyname;

static struct termios original_tio;

static void
exit_hook()
{
    tcsetattr(termfd, TCSANOW, &original_tio);
}

void
sigio_handler(int sig)
{
    int i;
 
    for (i = 0; i < 3; i++) {
        if (ace[i].infd == -1) {
            continue;
        }
        uart_interrupt_check(i);
    }
}

/*
 * select a specific 8250
 */
void
activate_8250(byte port, int device)
{
    acep = ace[device];

    register_input(port + 0, &rd_rxb);
    register_input(port + 1, &rd_inte);
    register_input(port + 2, &rd_inti);
    register_input(port + 3, &rd_lcr);
    register_input(port + 4, &rd_mcr);
    register_input(port + 5, &rd_lsr);
    register_input(port + 6, &rd_msr);

    register_output(port + 0, &wr_txb);
    register_output(port + 1, &wr_inte);
    register_output(port + 2, &undef_outreg);
    register_output(port + 3, &wr_lcr);
    register_output(port + 4, &wr_mcr);
    register_output(port + 5, &wr_lsr);
    register_output(port + 6, &wr_msr);
}

__attribute__((constructor))        // run before main()
void
register_8250_driver()
{
    myttyname = strdup(ttyname(0));
    trace_8250 = register_trace("8250");
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
