/*
 * BSD sockets over the network card
 *
 * include/sys/socket.h
 *
 * The card's clone node (/dev/net0, sys/NET.md) is a TCP socket, and
 * libsocket is the body behind these calls: socket() opens the clone,
 * bind, listen and connect are the driver's ioctls, accept() takes a
 * parked connection on a duplicate of the listener so the listener
 * keeps its own descriptor, and read and write move the bytes.
 * select(2) is the existing descriptor-mask call; there is no fd_set
 * layer here.
 *
 * The socket calls are IPv4 and TCP only, which is what the card's
 * clone node is.  The card has no half-close, no options and no name
 * store: setsockopt() succeeds and does nothing, shutdown() does
 * nothing, and getpeername()/getsockname() answer from what bind(),
 * connect() and accept() remembered.
 */
#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

#include <types.h>

#define AF_INET		2
#define PF_INET		AF_INET
#define SOCK_STREAM	1
#define SOCK_DGRAM	2

#define SOL_SOCKET	1
#define SO_REUSEADDR	1
#define SO_LINGER	2

#define INADDR_ANY	((UINT32) 0)
#define INADDR_NONE	((UINT32) -1)

struct in_addr {
    UINT32 s_addr;		/* network order */
};

struct sockaddr {
    UINT16 sa_family;
    char sa_data[14];
};

struct sockaddr_in {
    UINT16 sin_family;
    UINT16 sin_port;		/* network order */
    struct in_addr sin_addr;
    char sin_zero[8];
};

/*
 * The card and the wire are big-endian; the machine is not.  These are
 * functions rather than macros because the long ones are easier to
 * read and the calls are never in a loop that matters.
 */
extern UINT16 htons();
extern UINT16 ntohs();
extern UINT32 htonl();
extern UINT32 ntohl();

extern int socket();
extern int bind();
extern int listen();
extern int accept();
extern int connect();
extern int getpeername();
extern int getsockname();
extern int setsockopt();
extern int shutdown();

extern UINT32 inet_addr();
/*
 * BSD passes the address by value; this compiler will not pass a
 * structure as a call argument, so this takes a pointer.
 */
extern char *inet_ntoa();

/*
 * The card's network association (N_JOIN), not a socket call: one call
 * puts the card on its network for every socket the process holds.
 * A daemon does it once, on one of its sockets, before binding.
 */
extern int net_join();

#define send(s, buf, len, flags)	write((s), (buf), (len))
#define recv(s, buf, len, flags)	read((s), (buf), (len))

#endif
