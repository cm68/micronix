/*
 * Network database
 *
 * include/netdb.h
 *
 * The service names, from /etc/services.  getservbyname is what BSD
 * inetd calls to turn "shell" into 514/tcp; s_port is in network order,
 * as it is there.
 */
#ifndef _NETDB_H
#define _NETDB_H

struct servent {
    char *s_name;
    char **s_aliases;
    int s_port;			/* network order */
    char *s_proto;
};

extern struct servent *getservbyname();
extern struct servent *getservbyport();

#endif
