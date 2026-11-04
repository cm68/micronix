/*
 * getservbyname, getservbyport - the /etc/services lookup
 *
 * lib/libsocket/services.c
 *
 * A line is
 *
 *	<name> <port>/<proto> [alias...]
 *
 * with '#' starting a comment.  The answer is one static servent,
 * overwritten by each call, and s_port is in network order as it is in
 * BSD: the caller hands it straight to bind or connect.
 */
#include <types.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define NAMELEN	32
#define PROTOLEN 8
#define ALIASLEN 96
#define MAXALIAS 8

static struct servent sv;
static char namebuf[NAMELEN];
static char protobuf[PROTOLEN];
static char aliasbuf[ALIASLEN];
static char *aliasvec[MAXALIAS];

/*
 * Split a line into tokens in place.  Returns the count, up to max.
 */
static int
tokens(line, tok, max)
    char *line;
    char **tok;
    int max;
{
    char *p;
    int n;

    n = 0;
    p = line;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n')
            p++;
        if (*p == 0)
            break;
        if (n >= max)
            break;
        tok[n++] = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n')
            p++;
        if (*p)
            *p++ = 0;
    }
    return (n);
}

/*
 * One line, one service.  Fills sv and says whether the name matches
 * (and the protocol, when wantproto is not empty).
 */
static int
match(line, wantname, wantproto, wantport, byport)
    char *line;
    char *wantname;
    char *wantproto;
    int wantport;
    int byport;
{
    char *tok[2 + MAXALIAS];
    char *port, *proto;
    int n, i, aliases;
    long v;

    n = tokens(line, tok, 2 + MAXALIAS);
    if (n < 2)
        return (0);

    port = tok[1];
    proto = strchr(port, '/');
    if (proto == 0)
        return (0);
    *proto++ = 0;
    v = atoi(port);
    if (v <= 0 || v > 65535L)
        return (0);

    if (wantproto && wantproto[0] && strcmp(proto, wantproto) != 0)
        return (0);

    if (byport) {
        if (htons((UINT16) v) != wantport)
            return (0);
    } else {
        i = 0;
        if (strcmp(tok[0], wantname) != 0) {
            for (i = 0; i < n - 2; i++)
                if (strcmp(tok[2 + i], wantname) == 0)
                    break;
            if (i >= n - 2)
                return (0);
        }
    }

    strncpy(namebuf, tok[0], NAMELEN - 1);
    namebuf[NAMELEN - 1] = 0;
    strncpy(protobuf, proto, PROTOLEN - 1);
    protobuf[PROTOLEN - 1] = 0;

    /*
     * The aliases are copied back to back in aliasbuf and the vector
     * points at them; an entry with none still has a null first slot.
     */
    {
        char *p;
        int left;

        p = aliasbuf;
        left = ALIASLEN;
        aliases = 0;
        for (i = 2; i < n && aliases < MAXALIAS - 1; i++) {
            int len;

            len = strlen(tok[i]) + 1;
            if (len > left)
                break;
            strcpy(p, tok[i]);
            aliasvec[aliases++] = p;
            p += len;
            left -= len;
        }
    }
    aliasvec[aliases] = 0;

    sv.s_name = namebuf;
    sv.s_aliases = aliasvec;
    sv.s_port = htons((UINT16) v);
    sv.s_proto = protobuf;
    return (1);
}

struct servent *
getservbyname(name, proto)
    char *name, *proto;
{
    FILE *f;
    char line[128];
    char *p;

    if (name == 0)
        return (0);
    f = fopen("/etc/services", "r");
    if (f == 0)
        return (0);
    while (fgets(line, sizeof line, f) != 0) {
        p = strchr(line, '#');
        if (p)
            *p = 0;
        if (match(line, name, proto, 0, 0)) {
            fclose(f);
            return (&sv);
        }
    }
    fclose(f);
    return (0);
}

struct servent *
getservbyport(port, proto)
    int port;
    char *proto;
{
    FILE *f;
    char line[128];
    char *p;

    f = fopen("/etc/services", "r");
    if (f == 0)
        return (0);
    while (fgets(line, sizeof line, f) != 0) {
        p = strchr(line, '#');
        if (p)
            *p = 0;
        if (match(line, "", proto, port, 1)) {
            fclose(f);
            return (&sv);
        }
    }
    fclose(f);
    return (0);
}

/*
 * vim: tabstop=4 shiftwidth=4 expandtab:
 */
