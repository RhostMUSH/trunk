#include "autoconf.h"
#include "interface.h"
#include "externs.h"
#include "websock2.h"

// extern int FDECL(encode_base64, (const char *, int, char *, char **));
extern int FDECL(process_output, (DESC * d));

/* Base64 encoder. Temporary until encode_base64 can be sorted out */
static void
encode64(char *dst, const char *src, size_t srclen)
{
  static const char enc[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
  };

  // Encode 3-byte units. Manually unrolled for performance.
  while (3 <= srclen) {
    *dst++ = enc[src[0] >> 2 & 0x3F];
    *dst++ = enc[(src[0] << 4 & 0x30) | (src[1] >> 4 & 0xF)];
    *dst++ = enc[(src[1] << 2 & 0x3C) | (src[2] >> 6 & 0x3)];
    *dst++ = enc[src[2] & 0x3F];

    src += 3;
    srclen -= 3;
  }

  // Encode residue (0, 1, or 2 bytes).
  switch (srclen) {
  case 1:
    *dst++ = enc[src[0] >> 2 & 0x3F];
    *dst++ = enc[src[0] << 4 & 0x30];
    *dst++ = '=';
    *dst++ = '=';
    break;

  case 2:
    *dst++ = enc[src[0] >> 2 & 0x3F];
    *dst++ = enc[(src[0] << 4 & 0x30) | (src[1] >> 4 & 0xF)];
    *dst++ = enc[src[1] << 2 & 0x3C];
    *dst++ = '=';
    break;
  }
}

/* Compute websocket key for server response */
static void
compute_websocket_accept(char *dst, const char *key) {
    char combined[WEBSOCKET_KEY_LEN + WEBSOCKET_KEY_MAGIC_LEN];
    char hash[20];

    /* Generate accept key */
    memcpy(combined, key, WEBSOCKET_KEY_LEN);
    memcpy(combined + WEBSOCKET_KEY_LEN, WEBSOCKET_KEY_MAGIC, WEBSOCKET_KEY_MAGIC_LEN);

    /* Computer SHA-1 hash of combined value */
    SHA1((unsigned char *)combined, sizeof(combined), (unsigned char *)hash);
    encode64(dst, hash, sizeof(hash));
    fprintf(stderr, "[WS] Accept key: %s\n", dst);
}

/* Validate client websocket key for handshake */
int
validate_websocket_key(char *key) {
    int res = 0;

    if (key && strlen(key) == WEBSOCKET_KEY_LEN) {
        fprintf(stderr, "[WS] KEYGEN: %s [VALIDATED]\n", key);
        res = 1;
    } else {
        fprintf(stderr, "[WS] KEYGEN: %s [NOT VALID]\n", key);
    }

    return res;
}

/* Create handshake accept response */
void
complete_handshake(DESC *d, const char *key) {
    const char *const RESPONSE =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n\r\n";
    char *accept = alloc_lbuf("websock.accept");
    char *buf = alloc_lbuf("websoc.accept_buf");

    // Get the accept response key
    memset(accept, '\0', LBUF_SIZE);
    compute_websocket_accept(accept, key);

    // Set socket timeout to mush default
    d->timeout = mudconf.idle_timeout;

    // Construct payload
    memset(buf, '\0', LBUF_SIZE);
    sprintf(buf, RESPONSE, accept);
    fprintf(stderr, "[WS] Handshake Response: [SENT]\n%s", buf);
    queue_write(d, buf, strlen(buf));
    process_output(d);
    free_lbuf(accept);
    free_lbuf(buf);

    // Change API descriptor flag for websockets to maintain connection
    d->flags &= ~DS_API;
    d->flags |= DS_WEBSOCKETS;

    // Send login screen
    welcome_user(d);
}
