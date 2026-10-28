/*
 * The network card firmware - NET.md's socket face, in the ESP32.
 *
 * The guest drives two S-100 ports: NETDATA (0x60) and NETSTAT (0x61).
 * This side gives it the byte channel, assembles the guest's frames,
 * runs them against lwIP sockets, and pushes ACK/DATA/EVENT frames back.
 * The contract is src/micronix/sys/NET.md; the simulator's card model,
 * src/hwsim/d1/nic.c, is the golden reference and this file follows it
 * command for command.  The board, pinout and timing are hw/esp32-nic.md.
 * The write is latched (74LS373) so the 6 MHz I/O cycle never races this
 * loop; the latch holds the byte and the loop reads it on the falling
 * edge of sOUT.
 *
 * Two tasks, two cores.  busTask owns one core at the highest priority
 * and never yields: it polls the buffered board's select/sOUT/sINP and
 * moves bytes in and out of the FIFOs.  netTask runs on the other core
 * with the Wi-Fi stack: it assembles frames, runs the socket state
 * machine, and pushes replies.  Nothing in the bus loop touches flash,
 * allocates, or logs.
 *
 * The card's only shared mutable state with the guest is the two FIFOs
 * and the interrupt line.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <atomic>

#include <soc/gpio_reg.h>
#include <soc/soc.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>

#include "netproto.h"

/*
 * Wi-Fi credentials are provisioned at runtime and kept in NVS - see
 * "Provisioning" in README.md.  Until the card has a network it brings
 * up its own access point; connect to that, open the page, and give it
 * an SSID and password.  Everything after that is DHCP: address, mask,
 * gateway, DNS.
 *
 * The compile-time pair below is a bench fallback, used only when NVS
 * holds nothing (secrets.h is ignored by git).
 */
#if defined(__has_include)
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#endif
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif
#ifndef NIC_AP_PASS
#define NIC_AP_PASS ""      /* empty: the setup access point is open */
#endif

/* ------------------------------------------------------------------ */
/* The board                                                           */
/* ------------------------------------------------------------------ */

/*
 * The card's pinout (hw/esp32-nic.md).  DO comes from the 74LS373
 * write latch through U20, a 74LVC245 at 3.3 V; the four control lines
 * and the latch outputs are 5V bus signals, so they are inputs.  DI
 * drives the board's data buffer directly at 3.3 V.  GPIO 34-39 are
 * input-only, which the four control lines use.  The interrupt is
 * GPIO12 through a 7406: the inverter makes it an open-collector
 * pull-down of VI5 (active low on the bus), so GPIO12 HIGH asserts the
 * line.
 *
 * SEL* is the 74LS682's /P=Q and is active low.  sOUT and sINP are
 * active high on the S-100 bus; A0 high selects NETSTAT (0x61).
 */
static const uint8_t DO_PIN[8] = {32, 33, 25, 26, 27, 14, 13, 4};
static const uint8_t DI_PIN[8] = {16, 17, 5, 18, 19, 21, 22, 23};

#define PIN_SEL   34
#define PIN_SOUT  35
#define PIN_SINP  36
#define PIN_A0    39
#define PIN_IRQ   12

static const bool SEL_ACTIVE  = false;  /* SEL* = 682 /P=Q */
static const bool SOUT_ACTIVE = true;
static const bool SINP_ACTIVE = true;
static const bool A0_STAT_HIGH = true;   /* A0 high selects NETSTAT */

#define IN1BIT(p)  (1UL << ((p) - 32))
#define IRQ_BIT    (1UL << PIN_IRQ)

static const uint32_t SEL_BIT  = IN1BIT(PIN_SEL);
static const uint32_t SOUT_BIT = IN1BIT(PIN_SOUT);
static const uint32_t SINP_BIT = IN1BIT(PIN_SINP);
static const uint32_t A0_BIT   = IN1BIT(PIN_A0);

/* DI set/clear masks per byte, so a read costs two register writes. */
static uint32_t DRAM_ATTR di_set[256];
static uint32_t DRAM_ATTR di_clr[256];

/* ------------------------------------------------------------------ */
/* The FIFOs                                                           */
/*                                                                     */
/* Both are single-producer/single-consumer rings: the network task is */
/* the only producer on rx, the bus task the only producer on tx.      */
/* Indices are free-running and masked, so no wrap test is needed.     */
/* ------------------------------------------------------------------ */

#define RXQ      8192
#define TXQ      4096
#define RXMASK   (RXQ - 1)
#define TXMASK   (TXQ - 1)

static uint8_t DRAM_ATTR rxq[RXQ];
static uint8_t DRAM_ATTR txq[TXQ];

static std::atomic<uint32_t> rx_head(0);   /* producer: netTask */
static std::atomic<uint32_t> rx_tail(0);   /* consumer: busTask */
static std::atomic<uint32_t> tx_head(0);   /* producer: busTask */
static std::atomic<uint32_t> tx_tail(0);   /* consumer: netTask */

static volatile bool joined = false;       /* NETSTAT_RDY, set by netTask */

/* The interrupt handshake: both sides touch the line under this. */
static portMUX_TYPE irqMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool irq_asserted = false;

/* ------------------------------------------------------------------ */
/* The bus side - IRAM only, no flash, no logging                      */
/* ------------------------------------------------------------------ */

static inline IRAM_ATTR bool rx_empty(void)
{
    return rx_tail.load(std::memory_order_relaxed) ==
           rx_head.load(std::memory_order_acquire);
}

static inline IRAM_ATTR uint8_t rx_peek(void)
{
    uint32_t t = rx_tail.load(std::memory_order_relaxed);

    if (t == rx_head.load(std::memory_order_acquire))
        return 0;
    return rxq[t & RXMASK];
}

static inline IRAM_ATTR uint8_t rx_get(void)
{
    uint32_t t = rx_tail.load(std::memory_order_relaxed);
    uint8_t b;

    if (t == rx_head.load(std::memory_order_acquire))
        return 0;
    b = rxq[t & RXMASK];
    rx_tail.store(t + 1, std::memory_order_release);
    return b;
}

static inline IRAM_ATTR bool tx_room(void)
{
    return (uint32_t)(tx_head.load(std::memory_order_acquire) -
                      tx_tail.load(std::memory_order_relaxed)) < TXQ;
}

static inline IRAM_ATTR void tx_put(uint8_t b)
{
    uint32_t h = tx_head.load(std::memory_order_relaxed);

    if ((uint32_t)(h - tx_tail.load(std::memory_order_acquire)) >= TXQ)
        return;                         /* the guest honours TXRDY */
    txq[h & TXMASK] = b;
    tx_head.store(h + 1, std::memory_order_release);
}

static inline IRAM_ATTR uint8_t status_byte(void)
{
    uint8_t s = 0;

    if (!rx_empty())
        s |= NETSTAT_RXRDY;
    if (tx_room())
        s |= NETSTAT_TXRDY;
    if (joined)
        s |= NETSTAT_RDY;
    return s;
}

static inline IRAM_ATTR uint8_t do_byte(uint32_t in0, uint32_t in1)
{
    uint8_t v = 0;

    if (in1 & (1UL << 0))  v |= 0x01;   /* DO0 = GPIO32 */
    if (in1 & (1UL << 1))  v |= 0x02;   /* DO1 = GPIO33 */
    if (in0 & (1UL << 25)) v |= 0x04;   /* DO2 = GPIO25 */
    if (in0 & (1UL << 26)) v |= 0x08;   /* DO3 = GPIO26 */
    if (in0 & (1UL << 27)) v |= 0x10;   /* DO4 = GPIO27 */
    if (in0 & (1UL << 14)) v |= 0x20;   /* DO5 = GPIO14 */
    if (in0 & (1UL << 13)) v |= 0x40;   /* DO6 = GPIO13 */
    if (in0 & (1UL << 4))  v |= 0x80;   /* DO7 = GPIO4 */
    return v;
}

/*
 * The bus loop.  The write is latched (U23, 74LS373) with
 * LE = sOUTb AND SEL*, so the 6 MHz cycle never races this loop: the
 * latch is transparent while sOUT is high and holds the byte when it
 * falls.  The loop reads the held byte on the falling edge, through
 * U20.  It must never block or take a fault.  Read cycles drive DI
 * continuously with the byte the guest would read (rx head for NETDATA,
 * the status byte for NETSTAT); a read strobe only consumes the byte,
 * which buys the setup time the Z80 needs.
 *
 * The interrupt follows the rx FIFO's emptiness.  Assertion happens in
 * the network task under irqMux after a frame is queued; release happens
 * here under the same mux when the last byte is drained, with the empty
 * test re-done inside the mux.  That orders the two against each other:
 * whichever runs second sees the other's state.
 */
static void IRAM_ATTR busTask(void *arg)
{
    bool prev_rd = false;
    bool prev_wr = false;
    bool wr_stat = false;       /* A0 sampled at the write's leading edge */
    uint8_t driven = 0xff;

    (void)arg;
    for (;;) {
        uint32_t in0 = REG_READ(GPIO_IN_REG);
        uint32_t in1 = REG_READ(GPIO_IN1_REG);
        bool sel = ((in1 & SEL_BIT) != 0) == SEL_ACTIVE;
        bool a0  = ((in1 & A0_BIT) != 0) == A0_STAT_HIGH;
        bool rd  = sel && (((in1 & SINP_BIT) != 0) == SINP_ACTIVE);
        bool wr  = sel && (((in1 & SOUT_BIT) != 0) == SOUT_ACTIVE) && !rd;

        if (rd && !a0 && !prev_rd) {
            rx_get();
            if (rx_empty()) {
                portENTER_CRITICAL(&irqMux);
                if (rx_empty()) {
                    REG_WRITE(GPIO_OUT_W1TC_REG, IRQ_BIT);
                    irq_asserted = false;
                }
                portEXIT_CRITICAL(&irqMux);
            }
        }
        if (wr && !prev_wr)
            wr_stat = a0;           /* the address is valid for the cycle */
        if (prev_wr && !wr && !wr_stat)
            tx_put(do_byte(in0, in1));  /* U23 held it; read on the fall */
        prev_rd = rd;
        prev_wr = wr;

        {
            uint8_t v = a0 ? status_byte() : rx_peek();
            if (v != driven) {
                REG_WRITE(GPIO_OUT_W1TS_REG, di_set[v]);
                REG_WRITE(GPIO_OUT_W1TC_REG, di_clr[v]);
                driven = v;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Replies, from the network side                                      */
/* ------------------------------------------------------------------ */

static bool rx_room(int len)
{
    uint32_t h = rx_head.load(std::memory_order_relaxed);
    uint32_t t = rx_tail.load(std::memory_order_acquire);

    return (uint32_t)(RXQ - (h - t)) >= (uint32_t)(4 + len);
}

static void push_frame(uint8_t type, uint8_t sock, const uint8_t *p, int len)
{
    uint32_t h = rx_head.load(std::memory_order_relaxed);
    uint32_t t = rx_tail.load(std::memory_order_acquire);

    if ((uint32_t)(RXQ - (h - t)) < (uint32_t)(4 + len))
        return;                         /* whole frame or nothing */
    rxq[h++ & RXMASK] = type;
    rxq[h++ & RXMASK] = sock;
    rxq[h++ & RXMASK] = len & 0xff;
    rxq[h++ & RXMASK] = (len >> 8) & 0xff;
    for (int i = 0; i < len; i++)
        rxq[h++ & RXMASK] = p[i];
    rx_head.store(h, std::memory_order_release);

    portENTER_CRITICAL(&irqMux);
    if (!irq_asserted) {
        REG_WRITE(GPIO_OUT_W1TS_REG, IRQ_BIT);
        irq_asserted = true;
    }
    portEXIT_CRITICAL(&irqMux);
}

static void push_ack(uint8_t sock, uint8_t status)
{
    push_frame(P_ACK, sock, &status, 1);
}

static void push_event(uint8_t sock, uint8_t code)
{
    push_frame(P_EVENT, sock, &code, 1);
}

/*
 * A connected event carries the peer's ip[4] in network order after the
 * code (NET.md section 5).  rshd's .rhosts check needs it; the guest has
 * no other way to learn who connected.  Closed and reset stay one byte.
 */
static void push_event_ip(uint8_t sock, uint8_t code, IPAddress ip)
{
    uint8_t p[5];

    p[0] = code;
    p[1] = ip[0];
    p[2] = ip[1];
    p[3] = ip[2];
    p[4] = ip[3];
    push_frame(P_EVENT, sock, p, 5);
}

/* ------------------------------------------------------------------ */
/* The socket state machine                                            */
/* ------------------------------------------------------------------ */

struct Sock {
    uint8_t  state = S_CLOSED;
    uint8_t  mode = 0;
    uint8_t  txbuf[NIC_MAXPAY];
    int      txlen = 0;
    uint16_t bindPort = 0;
    IPAddress peer;
    uint16_t peerPort = 0;
    WiFiUDP   udp;
    WiFiClient client;
    WiFiServer *server = nullptr;
};

static Sock socks[NIC_NSOCK];

/* the frame being assembled from the guest's writes */
static uint8_t frame[4 + NIC_MAXPAY];
static int  f_n;
static bool f_payload;
static int  f_need;

static volatile bool joining = false;
static uint32_t joinStart = 0;

/* runtime network configuration, provisioned into NVS */
static char cfgSsid[33];
static char cfgPass[65];
static bool haveCreds = false;
static bool staStarted = false;
static uint32_t staStart = 0;
static bool provisioning = false;
static bool mdnsUp = false;
static uint32_t restartAt = 0;
static char apName[24];
static Preferences prefs;
static WebServer web(80);
static DNSServer dns;

static void startSTA(void);
static void startProvisioning(void);
static void stopProvisioning(void);

static void close_sock(Sock &s)
{
    s.udp.stop();
    s.client.stop();
    if (s.server) {
        s.server->end();
        delete s.server;
        s.server = nullptr;
    }
    s.state = S_CLOSED;
    s.txlen = 0;
    s.bindPort = 0;
    s.peerPort = 0;
}

static void do_open(uint8_t i, uint8_t mode)
{
    int ok;

    if (i >= NIC_NSOCK) {
        push_ack(i, E_BAD_STATE);
        return;
    }
    Sock &s = socks[i];
    if (s.state != S_CLOSED) {
        push_ack(i, E_BAD_STATE);
        return;
    }
    s.mode = mode ? 1 : 0;
    s.txlen = 0;
    s.bindPort = 0;
    s.peerPort = 0;
    if (s.mode) {
        s.state = S_TCP_IDLE;
        ok = 1;
    } else {
        ok = s.udp.begin(0);            /* ephemeral; BIND may move it */
        s.state = ok ? S_UDP : S_CLOSED;
    }
    push_ack(i, ok ? 0 : E_NO_MEMORY);
}

static void do_close(uint8_t i)
{
    if (i >= NIC_NSOCK)
        return;                         /* nic.c: no ack for a bad slot */
    close_sock(socks[i]);
    push_ack(i, 0);
}

static void do_bind(uint8_t i, const uint8_t *p)
{
    uint16_t port;

    if (i >= NIC_NSOCK) {
        push_ack(i, E_NO_SOCKET);
        return;
    }
    Sock &s = socks[i];
    if (s.state == S_CLOSED) {
        push_ack(i, E_NO_SOCKET);
        return;
    }
    port = (p[0] << 8) | p[1];
    s.bindPort = port;
    if (s.mode == 0) {
        s.udp.stop();
        if (!s.udp.begin(port)) {
            push_ack(i, E_BAD_STATE);
            return;
        }
    }
    push_ack(i, 0);
}

static void do_connect(uint8_t i, const uint8_t *p)
{
    IPAddress ip(p[0], p[1], p[2], p[3]);
    uint16_t port = (p[4] << 8) | p[5];
    Sock &s = socks[i];

    if (i >= NIC_NSOCK) {
        push_ack(i, E_NO_SOCKET);
        return;
    }
    if (s.state == S_CLOSED) {
        push_ack(i, E_NO_SOCKET);
        return;
    }
    if (s.mode == 0) {
        s.peer = ip;
        s.peerPort = port;
        push_ack(i, 0);
        return;
    }

    /*
     * tcp: accepted now, established later.  The ACK goes into the FIFO
     * before the (blocking) association attempt, so the guest is not
     * held; the EVENT reports the result.  Arduino's connect has no
     * non-blocking form, so other sockets wait out the timeout.
     */
    push_ack(i, 0);
    s.state = S_TCP_CONNECT;
    s.client.stop();
    if (s.client.connect(ip, port, 5000)) {
        s.state = S_TCP_OPEN;
        push_event_ip(i, EV_CONNECTED, s.client.remoteIP());
    } else {
        s.client.stop();
        s.state = S_CLOSED;
        push_event(i, EV_RESET);
    }
}

static void do_listen(uint8_t i)
{
    Sock &s = socks[i];

    if (i >= NIC_NSOCK) {
        push_ack(i, E_NO_SOCKET);
        return;
    }
    if (s.state == S_CLOSED) {
        push_ack(i, E_NO_SOCKET);
        return;
    }
    if (s.bindPort == 0) {
        push_ack(i, E_BAD_STATE);
        return;
    }
    if (s.server) {
        s.server->end();
        delete s.server;
    }
    s.server = new WiFiServer(s.bindPort);
    if (!s.server) {
        push_ack(i, E_NO_MEMORY);
        return;
    }
    s.server->begin();
    s.state = S_TCP_LISTEN;
    push_ack(i, 0);
}

static void do_send(uint8_t i, const uint8_t *p, int len)
{
    Sock &s = socks[i];

    if (i >= NIC_NSOCK) {
        push_ack(i, E_NO_SOCKET);
        return;
    }
    if (s.state == S_CLOSED) {
        push_ack(i, E_NO_SOCKET);
        return;
    }
    if (s.state != S_TCP_OPEN && s.state != S_UDP) {
        push_ack(i, E_BAD_STATE);
        return;
    }
    if (s.txlen > 0) {
        push_ack(i, E_NO_MEMORY);       /* a backlog is still draining */
        return;
    }
    if (s.mode == 0) {
        if (s.peerPort == 0) {
            push_ack(i, E_BAD_STATE);   /* no CONNECT: no peer */
            return;
        }
        if (!s.udp.beginPacket(s.peer, s.peerPort) ||
            s.udp.write(p, len) != (size_t)len ||
            !s.udp.endPacket()) {
            push_ack(i, E_NO_MEMORY);
            return;
        }
        push_ack(i, 0);
        return;
    }
    {
        int r = s.client.write(p, len);

        if (r == len) {
            push_ack(i, 0);
            return;
        }
        if (r < 0) {
            push_ack(i, E_CLOSED);
            return;
        }
        /* partial: accepted for transmission; poll drains the rest */
        if (len - r > NIC_MAXPAY)
            r = len - NIC_MAXPAY;
        memcpy(s.txbuf, p + r, len - r);
        s.txlen = len - r;
        push_ack(i, 0);
    }
}

static void do_status(uint8_t i)
{
    uint8_t p[5];

    if (i >= NIC_NSOCK)
        return;
    Sock &s = socks[i];
    p[0] = s.state;
    p[1] = 0;                           /* rx is pushed, not buffered */
    p[2] = 0;
    p[3] = s.txlen & 0xff;
    p[4] = (s.txlen >> 8) & 0xff;
    push_frame(P_ACK, i, p, 5);
}

static void do_join(void)
{
    if (joined) {                       /* idempotent: name the address */
        IPAddress ip = WiFi.localIP();
        uint8_t b[4] = { ip[0], ip[1], ip[2], ip[3] };
        push_frame(P_ACK, 0xff, b, 4);
        return;
    }
    if (!haveCreds) {
        startProvisioning();            /* no network yet: setup AP */
        push_ack(0xff, E_TIMEOUT);
        return;
    }
    if (joining)
        return;                         /* the answer comes when it ends */
    joining = true;
    joinStart = millis();
    if (!staStarted)
        startSTA();
}

/*
 * netconf over the guest interface: the third way to give the card a
 * network.  GETCONF answers with the pair; SETCONF stores it in NVS and
 * applies it at once, dropping the old association and reconnecting.
 */
static void do_getconf(void)
{
    uint8_t p[2 + NIC_SSIDMAX + NIC_PASSMAX];
    int n = 0;
    int sl = strlen(cfgSsid);
    int pl = strlen(cfgPass);

    p[n++] = sl;
    memcpy(p + n, cfgSsid, sl);
    n += sl;
    p[n++] = pl;
    memcpy(p + n, cfgPass, pl);
    n += pl;
    push_frame(P_ACK, 0xff, p, n);
}

static void do_setconf(const uint8_t *p, int len)
{
    int sl, pl;

    if (len < 2) {
        push_ack(0xff, E_BAD_STATE);
        return;
    }
    sl = p[0];
    if (sl < 1 || sl > NIC_SSIDMAX || len < 1 + sl + 1) {
        push_ack(0xff, E_BAD_STATE);
        return;
    }
    pl = p[1 + sl];
    if (pl > NIC_PASSMAX || len < 1 + sl + 1 + pl) {
        push_ack(0xff, E_BAD_STATE);
        return;
    }
    memcpy(cfgSsid, p + 1, sl);
    cfgSsid[sl] = '\0';
    memcpy(cfgPass, p + 1 + sl + 1, pl);
    cfgPass[pl] = '\0';
    haveCreds = true;
    prefs.putString("ssid", cfgSsid);
    prefs.putString("pass", cfgPass);

    /* apply now: the old association is not the one asked for */
    joined = false;
    joining = false;
    WiFi.disconnect(true);
    staStarted = false;
    startSTA();
    push_ack(0xff, 0);
}

static void dispatch(void)
{
    uint8_t type = frame[0];
    uint8_t sock = frame[1];
    uint8_t *p = frame + 4;
    int len = f_need - 4;

    switch (type) {
    case C_OPEN:    do_open(sock, len ? p[0] : 0); break;
    case C_CLOSE:   do_close(sock); break;
    case C_BIND:    if (len >= 2) do_bind(sock, p); else push_ack(sock, E_BAD_STATE); break;
    case C_CONNECT: if (len >= 6) do_connect(sock, p); else push_ack(sock, E_BAD_STATE); break;
    case C_LISTEN:  do_listen(sock); break;
    case C_SEND:    do_send(sock, p, len); break;
    case C_STATUS:  do_status(sock); break;
    case C_JOIN:    do_join(); break;
    case C_GETCONF: do_getconf(); break;
    case C_SETCONF: do_setconf(p, len); break;
    default:        push_ack(sock, E_BAD_STATE); break;
    }
}

/* feed the assembled frame from the guest's bytes */
static void drain_tx(void)
{
    for (;;) {
        uint32_t t = tx_tail.load(std::memory_order_relaxed);
        uint8_t b;

        if (t == tx_head.load(std::memory_order_acquire))
            return;
        b = txq[t & TXMASK];
        tx_tail.store(t + 1, std::memory_order_release);

        frame[f_n++] = b;
        if (!f_payload && f_n == 4) {
            int len = frame[2] | (frame[3] << 8);

            if (len > NIC_MAXPAY) {     /* never let the guest overrun us */
                f_n = 0;
                f_payload = false;
                push_ack(frame[1], E_BAD_STATE);
                continue;
            }
            f_need = 4 + len;
            f_payload = true;
        }
        if (f_payload && f_n >= f_need) {
            dispatch();
            f_n = 0;
            f_payload = false;
        }
    }
}

/*
 * The link, in the network task.  A JOIN gets its ACK here - the IP when
 * the station has one, E_TIMEOUT if the association has not happened in
 * 30 seconds.  RDY follows the address: it drops if the link is lost.
 * The card starts its setup AP when it has no credentials, and again if
 * a provisioned network does not answer in 20 seconds, so a wrong
 * password is recoverable without erasing anything.
 */
static void poll_join(void)
{
    if (WiFi.status() == WL_CONNECTED) {
        IPAddress ip = WiFi.localIP();

        if (ip[0] | ip[1] | ip[2] | ip[3]) {
            if (!joined) {
                uint8_t b[4] = { ip[0], ip[1], ip[2], ip[3] };

                joined = true;
                if (joining) {
                    joining = false;
                    push_frame(P_ACK, 0xff, b, 4);
                    Serial.printf("nic: joined, ip %u.%u.%u.%u\n",
                                  b[0], b[1], b[2], b[3]);
                }
                if (!mdnsUp) {
                    MDNS.begin(apName);
                    mdnsUp = true;
                }
                if (provisioning)
                    stopProvisioning();
            }
        }
        return;
    }

    if (joined)
        joined = false;                 /* link lost: RDY drops */

    if (joining && (millis() - joinStart >= 30000 ||
                    WiFi.status() == WL_CONNECT_FAILED ||
                    WiFi.status() == WL_NO_SSID_AVAIL)) {
        joining = false;
        push_ack(0xff, E_TIMEOUT);
        Serial.println("nic: join failed; send any character to edit wifi settings");
    }

    if (haveCreds && !staStarted)
        startSTA();
    if (haveCreds && staStarted && !provisioning &&
        millis() - staStart >= 20000) {
        Serial.println("nic: no link; setup AP up");
        startProvisioning();
    }
}

/* drain a socket's backlog and push arrived data */
static void poll_socket(uint8_t i)
{
    Sock &s = socks[i];

    if (s.state == S_CLOSED)
        return;

    if (s.state == S_TCP_LISTEN) {
        if (s.server) {
            WiFiClient c = s.server->available();

            if (c) {
                s.client = c;
                s.client.setNoDelay(true);
                s.server->end();
                delete s.server;
                s.server = nullptr;
                s.state = S_TCP_OPEN;
                push_event_ip(i, EV_CONNECTED, c.remoteIP());
            }
        }
        return;
    }

    if (s.state == S_TCP_OPEN) {
        if (s.txlen > 0) {
            int r = s.client.write(s.txbuf, s.txlen);

            if (r > 0) {
                if (r >= s.txlen)
                    s.txlen = 0;
                else {
                    memmove(s.txbuf, s.txbuf + r, s.txlen - r);
                    s.txlen -= r;
                }
            }
        }
        {
            int n = s.client.available();

            if (n > 0 && rx_room(NIC_MAXPAY)) {
                uint8_t buf[NIC_MAXPAY];
                int r = s.client.read(buf, n < NIC_MAXPAY ? n : NIC_MAXPAY);

                if (r > 0)
                    push_frame(P_DATA, i, buf, r);
            }
        }
        if (!s.client.connected()) {
            close_sock(s);
            push_event(i, EV_CLOSED);
        }
        return;
    }

    if (s.state == S_UDP) {
        int n = s.udp.parsePacket();

        if (n > 0) {
            if (rx_room(NIC_MAXPAY)) {
                uint8_t buf[NIC_MAXPAY];
                int r = s.udp.read(buf, n < NIC_MAXPAY ? n : NIC_MAXPAY);

                if (r > 0)
                    push_frame(P_DATA, i, buf, r);
            } else {
                uint8_t junk[64];       /* full card: drop the datagram */

                while (s.udp.available())
                    s.udp.read(junk, sizeof junk);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Provisioning                                                        */
/*                                                                     */
/* No credentials: an open access point named micronix-nic-xxxx, with  */
/* the configuration page on 192.168.4.1.  The page writes the SSID     */
/* and password to NVS and restarts; the station then comes up under   */
/* DHCP.  The same page can forget them.  A USB-serial console takes   */
/* "provision", "reset" and "status" as an escape hatch when the AP is */
/* not up.  NIC_AP_PASS puts a WPA2 key on the setup AP.               */
/* ------------------------------------------------------------------ */

static void loadCreds(void)
{
    String s, p;

    prefs.begin("nic", false);
    s = prefs.getString("ssid", "");
    p = prefs.getString("pass", "");
    if (!s.length() && WIFI_SSID[0]) {  /* bench fallback */
        s = WIFI_SSID;
        p = WIFI_PASS;
    }
    strncpy(cfgSsid, s.c_str(), 32);
    cfgSsid[32] = '\0';
    strncpy(cfgPass, p.c_str(), 64);
    cfgPass[64] = '\0';
    haveCreds = cfgSsid[0] != '\0';
}

static void startSTA(void)
{
    WiFi.setHostname(apName);           /* what DHCP and mDNS register */
    WiFi.mode(provisioning ? WIFI_AP_STA : WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(cfgSsid, cfgPass);
    staStarted = true;
    staStart = millis();
    Serial.printf("nic: connecting to '%s'\n", cfgSsid);
}

static String page(void)
{
    String h;

    h = "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>";
    h += "<title>micronix network card</title>";
    h += "<h1>micronix network card</h1><p>setup access point <b>";
    h += apName;
    h += "</b></p><p>station: ";
    h += (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString()
                                         : String("not connected");
    h += "</p><form method=post action=/save>";
    h += "SSID <input name=ssid maxlength=32><br>";
    h += "Password <input name=pass type=password maxlength=63><br>";
    h += "<button>Save and restart</button></form>";
    h += "<form method=post action=/reset>";
    h += "<button>Forget credentials</button></form>";
    return h;
}

static void handleRoot(void)
{
    web.send(200, "text/html", page());
}

static void handleSave(void)
{
    String s = web.arg("ssid");
    String p = web.arg("pass");

    s.trim();
    if (s.length() == 0 || s.length() > 32 || p.length() > 63) {
        web.send(400, "text/plain", "bad ssid or password\n");
        return;
    }
    prefs.putString("ssid", s);
    prefs.putString("pass", p);
    web.send(200, "text/html", "<p>saved; restarting.</p>");
    Serial.printf("nic: credentials for '%s' saved\n", s.c_str());
    restartAt = millis() + 800;
}

static void handleReset(void)
{
    prefs.remove("ssid");
    prefs.remove("pass");
    web.send(200, "text/html", "<p>credentials cleared; restarting.</p>");
    restartAt = millis() + 800;
}

static void handleNotFound(void)
{
    /* the usual captive-portal redirect */
    web.sendHeader("Location", "http://192.168.4.1/");
    web.send(302, "text/plain", "");
}

static void startProvisioning(void)
{
    if (provisioning)
        return;
    WiFi.mode(haveCreds ? WIFI_AP_STA : WIFI_AP);
    if (!WiFi.softAP(apName, NIC_AP_PASS[0] ? NIC_AP_PASS : nullptr)) {
        Serial.println("nic: setup AP failed");
        return;
    }
    dns.start(53, "*", WiFi.softAPIP());
    web.on("/", HTTP_GET, handleRoot);
    web.on("/save", HTTP_POST, handleSave);
    web.on("/reset", HTTP_POST, handleReset);
    web.onNotFound(handleNotFound);
    web.begin();
    if (!mdnsUp) {
        MDNS.begin(apName);
        mdnsUp = true;
    }
    provisioning = true;
    Serial.printf("nic: setup AP '%s' at %s\n", apName,
                  WiFi.softAPIP().toString().c_str());
}

static void stopProvisioning(void)
{
    if (!provisioning)
        return;
    web.stop();
    dns.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    provisioning = false;
    Serial.println("nic: setup AP down, station up");
}

/*
 * The USB-serial editor, after the WiFiDongle's: any character on the
 * console opens it.  It scans and lists the networks, then prompts for
 * the ssid and password with the current values pre-filled and editable
 * (backspace), writes the pair to NVS, and restarts.  This is the way
 * in when the setup access point is not convenient.
 */
static void editstr(const char *prompt, char *buf, int max)
{
    int cursor = strlen(buf);

    Serial.printf("%s: %s", prompt, buf);
    for (;;) {
        int c;

        if (!Serial.available())
            continue;
        c = Serial.read();
        if (c == '\r' || c == '\n') {
            Serial.print("\r\n");
            return;
        }
        if (c == 0x7f || c == 0x08) {   /* backspace/delete */
            if (cursor > 0) {
                Serial.print("\b \b");
                buf[--cursor] = '\0';
            }
            continue;
        }
        if (c < ' ' || c > 0x7e || cursor >= max)
            continue;
        buf[cursor++] = (char)c;
        buf[cursor] = '\0';
        Serial.print((char)c);
    }
}

static int editeeprom(void)
{
    char ssid[33], pass[65];
    int n;

    strncpy(ssid, cfgSsid, sizeof ssid - 1);
    ssid[sizeof ssid - 1] = '\0';
    strncpy(pass, cfgPass, sizeof pass - 1);
    pass[sizeof pass - 1] = '\0';

    Serial.println("Scanning for WiFi networks...");
    n = WiFi.scanNetworks();
    if (n <= 0)
        Serial.println("  no networks found");
    else {
        Serial.printf("  %d network(s) found:\n", n);
        for (int i = 0; i < n; i++)
            Serial.printf("  %2d: %-32s %4d dBm %s\n", i + 1,
                          WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                          WiFi.encryptionType(i) == WIFI_AUTH_OPEN
                              ? "open" : "encrypted");
    }
    WiFi.scanDelete();
    Serial.println();

    editstr("ssid", ssid, sizeof ssid - 1);
    editstr("password", pass, sizeof pass - 1);

    if (!strcmp(ssid, cfgSsid) && !strcmp(pass, cfgPass)) {
        Serial.println("no difference");
        return 0;
    }
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    Serial.printf("nic: saved '%s' to nvs\n", ssid);
    return 1;
}

static void poll_serial(void)
{
    if (!Serial.available())
        return;
    (void)Serial.read();
    Serial.println("Edit wifi configuration");
    if (editeeprom())
        ESP.restart();
}

static void poll_net(void)
{
    if (provisioning) {
        dns.processNextRequest();
        web.handleClient();
    }
    if (restartAt && millis() >= restartAt)
        ESP.restart();                  /* after the reply is on the wire */
}

static void netTask(void *arg)
{
    (void)arg;
    for (;;) {
        drain_tx();
        poll_net();
        poll_join();
        for (uint8_t i = 0; i < NIC_NSOCK; i++)
            poll_socket(i);
        poll_serial();
        vTaskDelay(pdMS_TO_TICKS(10));  /* 100 Hz, as the sim polls */
    }
}

/* ------------------------------------------------------------------ */
/* Startup                                                             */
/* ------------------------------------------------------------------ */

void setup(void)
{
    Serial.begin(115200);
    loadCreds();
    {
        uint8_t mac[6];

        WiFi.macAddress(mac);
        snprintf(apName, sizeof apName, "micronix-nic-%02x%02x",
                 mac[4], mac[5]);
    }

    /* the masks first: the bus task may start before this returns */
    for (int v = 0; v < 256; v++) {
        uint32_t set = 0, clr = 0;

        for (int b = 0; b < 8; b++) {
            if (v & (1 << b))
                set |= 1UL << DI_PIN[b];
            else
                clr |= 1UL << DI_PIN[b];
        }
        di_set[v] = set;
        di_clr[v] = clr;
    }

    for (int b = 0; b < 8; b++) {
        pinMode(DO_PIN[b], INPUT);
        pinMode(DI_PIN[b], OUTPUT);
    }
    pinMode(PIN_SEL, INPUT);
    pinMode(PIN_SOUT, INPUT);
    pinMode(PIN_SINP, INPUT);
    pinMode(PIN_A0, INPUT);
    pinMode(PIN_IRQ, OUTPUT);
    REG_WRITE(GPIO_OUT_W1TC_REG, IRQ_BIT);  /* VI5 released */

    for (uint8_t i = 0; i < NIC_NSOCK; i++)
        close_sock(socks[i]);
    f_n = 0;
    f_payload = false;

    /*
     * The bus task never yields, so core 1's idle task would starve and
     * the task watchdog would reset the card.  That is the point of the
     * poll loop: turn the watchdog off for that core.
     */
    disableCore1WDT();

    if (haveCreds)
        startSTA();
    else
        startProvisioning();

    xTaskCreatePinnedToCore(busTask, "nicbus", 2048, nullptr,
                            configMAX_PRIORITIES - 1, nullptr, 1);
    xTaskCreatePinnedToCore(netTask, "nicnet", 8192, nullptr,
                            5, nullptr, 0);
}

void loop(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));    /* the tasks do the work */
}
