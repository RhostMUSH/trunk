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
 * Add more options here as needed (TTYPE, MCCP2, etc.). */
static const telnet_telopt_t telopts[] = {
    { TELNET_TELOPT_NAWS,      TELNET_WONT, TELNET_DO   },
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
    d->cold->term_width = 0;
    d->cold->term_height = 0;

    /* Proactively request NAWS via DO NAWS — most clients wait for server to initiate */
    telnet_negotiate((telnet_t *)d->cold->telnet, TELNET_DO, TELNET_TELOPT_NAWS);
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
