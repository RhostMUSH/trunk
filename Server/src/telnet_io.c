#include "copyright.h"
#include "autoconf.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "externs.h"
#include "interface.h"
#include "telnet_io.h"

#include "libtelnet.h"

/* Telnet options table: what we support.
 * NAWS (31): we let the client send us window size (him = TELNET_DO).
 * TTYPE (24): we request terminal type (him = TELNET_DO).
 * CHARSET (42): we request charset negotiation (RFC 2066). */
#define TELNET_TELOPT_CHARSET 42
static const telnet_telopt_t telopts[] = {
    { TELNET_TELOPT_NAWS,      TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_TTYPE,     TELNET_WONT, TELNET_DO   },
    { TELNET_TELOPT_CHARSET,   TELNET_WONT, TELNET_DO   },
    { -1, 0, 0 }
};

/* Context for telnet_preprocess_input: the event handler needs to know where
 * to put clean text data. Since RhostMUSH is single-threaded, this is safe. */
static struct {
    char *buf;
    int  *len;
    int   max;
} telnet_ctx;

/* libtelnet event handler */
static void on_telnet_event(telnet_t *telnet, telnet_event_t *ev, void *user_data)
{
    DESC *d = (DESC *)user_data;
    int fd;
    int n;

    (void)telnet;

    switch (ev->type) {
    case TELNET_EV_DATA:
        /* Clean user text — buffer it for the caller's byte loop */
        if (telnet_ctx.buf && telnet_ctx.len) {
            n = ev->data.size;
            if (*telnet_ctx.len + n > telnet_ctx.max)
                n = telnet_ctx.max - *telnet_ctx.len;
            if (n > 0) {
                memcpy(telnet_ctx.buf + *telnet_ctx.len, ev->data.buffer, n);
                *telnet_ctx.len += n;
            }
        }
        break;

    case TELNET_EV_SEND:
        /* Library wants to send negotiation data — write directly to socket */
        fd = D_DESCRIPTOR(d);
        if (fd >= 0) {
            int sent = 0;
            const char *p = ev->data.buffer;
            size_t remaining = ev->data.size;
            while (remaining > 0) {
                n = write(fd, p + sent, remaining);
                if (n > 0) {
                    sent += n;
                    remaining -= n;
                } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    continue; /* retry immediately — tiny negotiation payload */
                } else {
                    break;
                }
            }
        }
        break;

    case TELNET_EV_SUBNEGOTIATION:
        if (ev->sub.telopt == TELNET_TELOPT_NAWS && ev->sub.size >= 4) {
            /* Extract 16-bit width and height (big-endian) */
            d->cold->term_width  = (unsigned short)((unsigned char)ev->sub.buffer[0] << 8 |
                                                     (unsigned char)ev->sub.buffer[1]);
            d->cold->term_height = (unsigned short)((unsigned char)ev->sub.buffer[2] << 8 |
                                                     (unsigned char)ev->sub.buffer[3]);
        } else if (ev->sub.telopt == TELNET_TELOPT_CHARSET && ev->sub.size >= 1) {
            unsigned char cmd = (unsigned char)ev->sub.buffer[0];
            if (cmd == 0 || cmd == 2) {  /* IS (0) or client accept (2) — charset confirmed */
                char name[32];
                int len = ev->sub.size - 1;
                if (len > 31) len = 31;
                memcpy(name, ev->sub.buffer + 1, len);
                name[len] = '\0';
                /* Strip leading separator (space or ; per RFC 2066) */
                { char *p = name; while (*p == ' ' || *p == ';') p++;
                  int slen = (int)strlen(p);
                  while (slen > 0 && (p[slen-1] == ' ' || p[slen-1] == '\r' ||
                         p[slen-1] == '\n' || p[slen-1] == '\t')) slen--;
                  p[slen] = '\0';
                  if (strcasecmp(p, "UTF-8") == 0 || strcasecmp(p, "UTF8") == 0)
                      d->cold->client_caps |= CLIENT_CAP_UNICODE;
                }
            }
        }
        break;

    case TELNET_EV_TTYPE:
        if (ev->ttype.cmd == TELNET_TTYPE_IS && ev->ttype.name) {
            const char *name = ev->ttype.name;
            int is_skip = 0;
            int step;

            /* Determine SEND step from flags */
            if (!(d->cold->client_caps & CLIENT_TTYPE_SEND1))
                step = 0;
            else if (!(d->cold->client_caps & CLIENT_TTYPE_SEND2))
                step = 1;
            else if (!(d->cold->client_caps & CLIENT_TTYPE_SEND3))
                step = 2;
            else
                step = 3;

            /* \x01-prefixed MTTS — capability info, not a client name */
            if (*name == '\x01') {
                name++;
                while (*name) {
                    while (*name == ' ') name++;
                    if (!*name) break;
                    if (strncasecmp(name, "mtts", 4) == 0) {
                        const char *p = name + 4;
                        while (*p == ' ') p++;
                        if (*p) {
                            unsigned long m = strtoul(p, NULL, 10);
                            if (m & 1)    d->cold->client_caps |= CLIENT_CAP_ANSI;
                            if (m & 4)    d->cold->client_caps |= CLIENT_CAP_UNICODE;
                            if (m & (8|2)) d->cold->client_caps |= CLIENT_CAP_256COLOR;
                            if (m & 256) d->cold->client_caps |= CLIENT_CAP_TRUECOLOR;
                            if (m & 2048) d->cold->client_caps |= CLIENT_CAP_SSL;
                        }
                        break;
                    }
                    if (strncasecmp(name, "utf-8", 5) == 0 ||
                        strncasecmp(name, "utf8", 4) == 0 ||
                        strncasecmp(name, "unicode", 7) == 0)
                        d->cold->client_caps |= CLIENT_CAP_UNICODE;
                    while (*name && *name != ' ') name++;
                }
                is_skip = 1;
            }

            /* Bare "MTTS <N>" — capability string */
            if (!is_skip && strncasecmp(name, "mtts", 4) == 0 && name[4] == ' ') {
                unsigned long m = strtoul(name + 5, NULL, 10);
                if (m & 1)    d->cold->client_caps |= CLIENT_CAP_ANSI;
                if (m & 4)    d->cold->client_caps |= CLIENT_CAP_UNICODE;
                if (m & (8|2)) d->cold->client_caps |= CLIENT_CAP_256COLOR;
                if (m & 256) d->cold->client_caps |= CLIENT_CAP_TRUECOLOR;
                if (m & 2048) d->cold->client_caps |= CLIENT_CAP_SSL;
                is_skip = 1;
            }

            /* "ANSI" — known generic, detection only */
            if (!is_skip && strcasecmp(name, "ansi") == 0) {
                d->cold->client_caps |= CLIENT_CAP_ANSI;
                is_skip = 1;
            }

            /* Known generic terminal types — detection only */
            if (!is_skip && (strncasecmp(name, "xterm", 5) == 0 ||
                             strncasecmp(name, "vt100", 5) == 0 ||
                             strncasecmp(name, "vt220", 5) == 0 ||
                             strncasecmp(name, "putty", 5) == 0))
                d->cold->client_caps |= CLIENT_CAP_ANSI;

            /* Duplicate detection: repeat of stored name → cycle complete */
            if (step > 0 && d->cold->client_name[0] != '\0' &&
                strcmp(name, d->cold->client_name) == 0) {
                break;
            }

            /* Store first real (non-skip) name */
            if (!is_skip && d->cold->client_name[0] == '\0') {
                strncpy(d->cold->client_name, name,
                        sizeof(d->cold->client_name) - 1);
                d->cold->client_name[
                    sizeof(d->cold->client_name) - 1] = '\0';
            }

            /* Send next follow-up SEND if steps remain */
            if (step < 3) {
                if (step == 0)
                    d->cold->client_caps |= CLIENT_TTYPE_SEND1;
                else if (step == 1)
                    d->cold->client_caps |= CLIENT_TTYPE_SEND2;
                else if (step == 2)
                    d->cold->client_caps |= CLIENT_TTYPE_SEND3;
                telnet_ttype_send((telnet_t *)d->cold->telnet);
            }

            /* No steps left and no name yet → unknown */
            if (step >= 3 && d->cold->client_name[0] == '\0') {
                strncpy(d->cold->client_name, "unknown",
                        sizeof(d->cold->client_name) - 1);
                d->cold->client_name[
                    sizeof(d->cold->client_name) - 1] = '\0';
            }
        }
        break;

    case TELNET_EV_WILL:
    case TELNET_EV_WONT:
    case TELNET_EV_DO:
    case TELNET_EV_DONT:
        /* libtelnet auto-responds per telopts table — nothing to do */
        break;

    case TELNET_EV_ERROR:
        /* Fatal protocol error — log and cleanup handled by caller */
        break;

    case TELNET_EV_WARNING:
        /* Recoverable protocol warning — log if needed */
        break;

    default:
        break;
    }
}

/* Initialize libtelnet for a descriptor */
void telnet_init_desc(DESC *d)
{
    if (!d)
        return;

    /* Don't re-initialize if already set */
    if (d->cold->telnet)
        return;

    d->cold->telnet = telnet_init(telopts, on_telnet_event, 0, d);

    /* Proactively request NAWS via DO NAWS — most clients wait for server to initiate.
     * On @reboot: re-negotiate to get client's current terminal size;
     * stored term_width/height are preserved until the response arrives. */
    telnet_negotiate((telnet_t *)d->cold->telnet, TELNET_DO, TELNET_TELOPT_NAWS);

    /* Fresh connection: reset caps, query TTYPE for terminal name and CHARSET for UTF-8.
     * On @reboot: client_name, caps, term_width, and term_height are already populated
     * from DESC_COLD — keep stored data. */
    if (d->cold->client_name[0] == '\0') {
        d->cold->term_width = 0;
        d->cold->term_height = 0;
        d->cold->client_caps = 0;
        telnet_ttype_send((telnet_t *)d->cold->telnet);
        unsigned char req[] = { 1, ' ', 'U', 'T', 'F', '-', '8' };
        telnet_subnegotiation((telnet_t *)d->cold->telnet,
                              TELNET_TELOPT_CHARSET, (const char *)req, sizeof(req));
    }
}

/* Free libtelnet state for a descriptor */
void telnet_free_desc(DESC *d)
{
    if (!d || !d->cold || !d->cold->telnet)
        return;

    telnet_free((telnet_t *)d->cold->telnet);
    d->cold->telnet = NULL;
}

/* Preprocess raw input through libtelnet.
 * Strips telnet negotiation, handles NAWS, returns clean text in buf.
 * Returns 1 if clean data remains, 0 if only telnet control bytes. */
int telnet_preprocess_input(DESC *d, char *buf, int *got)
{
    char clean[LBUF_SIZE];
    int clean_len = 0;

    if (!d || !d->cold->telnet || !buf || !got || *got <= 0)
        return 0;

    /* Set up context for the event handler */
    telnet_ctx.buf = clean;
    telnet_ctx.len = &clean_len;
    telnet_ctx.max = LBUF_SIZE;

    /* Feed raw bytes to libtelnet parser.
     * Event handler will populate clean[] with stripped text and handle SEND/NAWS. */
    telnet_recv((telnet_t *)d->cold->telnet, buf, *got);

    /* Clear context */
    telnet_ctx.buf = NULL;
    telnet_ctx.len = NULL;

    if (clean_len > 0) {
        memcpy(buf, clean, clean_len);
        *got = clean_len;
        return 1;
    }

    *got = 0;
    return 0;
}
