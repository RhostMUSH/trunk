#ifdef ENABLE_WEBSOCKETS
#include "autoconf.h"
#include "interface.h"
#include "externs.h"
#include "websock2.h"

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

/* Compute WebSocket key for server response */
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
}

static void
compute_payload_len(TBLOCK *tb, int len)
{
    if (len < 126) {
        *tb->hdr.end++ = len;
    } else if (len < 65536) {
        *tb->hdr.end++ = 126;
        *tb->hdr.end++ = (len >> 8) & 0xFF;
        *tb->hdr.end++ = len & 0xFF;
    } else { // payload > 65536
        int pos;
        *tb->hdr.end++ = 126;

        for (pos = 56; pos >= 0; pos -= 8) {
            *tb->hdr.end++ = (len >> pos) & 0xFF;
        }
    }
}

/* rewrite text as a WebSocket frame */
static void
write_message(TBLOCK *tb, const char *msg, int len, enum WebSocketOp op, int fin)
{
    char status = (fin) ? 0x80 : 0x00;
    *tb->hdr.end++ = status | op;
    compute_payload_len(tb, len);
    memcpy(tb->hdr.end, msg, len);
    tb->hdr.end += len;
    tb->hdr.nchars = tb->hdr.end - tb->hdr.start;
}

/* Validate client WebSocket key for handshake */
int
validate_websocket_key(char *key) {
    return (key && strlen(key) == WEBSOCKET_KEY_LEN);
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
    STARTLOG(LOG_ALWAYS, "NET", "WS");
    log_text("Handshake Response: [ACCEPT]");
    ENDLOG;
    queue_write(d, buf, strlen(buf));
    process_output(d);
    free_lbuf(accept);
    free_lbuf(buf);

    // Change API descriptor flag for websockets to maintain connection
    d->flags &= ~DS_API;
    d->flags |= DS_WEBSOCKETS;

    // Send login screen
    welcome_user(d);

    /* Set initial state */
    d->checksum[0] = 4;
}

/* Abort an invalid handshake request */
void
abort_handshake(DESC *d)
{
    char *RESPONSE =
        "HTTP/1.1 426 Upgrade Required\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";

    STARTLOG(LOG_ALWAYS, "NET", "WS");
    log_text("Handshake Response: [ABORT]");
    ENDLOG;
    queue_write(d, RESPONSE, strlen(RESPONSE));
}

void
websocket_write(DESC *d, const char *output, int len)
{
    int left, n, fin = 1;
    enum WebSocketOp op;

    do {
        /* Create new buffer for the WebSocket frame */
        TBLOCK *tb = (TBLOCK *)alloc_lbuf("queue_write.websocket");
        tb->hdr.nxt = NULL;
        tb->hdr.start = tb->data;
        tb->hdr.end = tb->data;

        /* Determine if this is a text or continue operation */
        op = (fin) ? WS_OP_TEXT : WS_OP_CONTINUATION;

        /* break up message as needed */
        left = LBUF_SIZE - sizeof(TBLKHDR) - WS_HDR_SIZE;
        if (len <= left) {
            n = len;
            fin = 1;
            len = 0;
        } else {
            n = left;
            fin = 0;
            len -= left;
        }

        /* Add output to the new buffer */
        write_message(tb, output, n, op, fin);

        /* If there is already buffered output add this to chain */
        if (d->output_head == NULL) {
            d->output_head = tb;
        } else {
            d->output_tail->hdr.nxt = tb;
        }
        d->output_tail = tb;
    } while (len > 0);
}

int
process_websocket_frame(DESC *d, char *tbuf1, int got)
{
  char mask[1 + 4 + 1 + 1];
  unsigned char state, type, err;
/*  uint64_t len; */
  long len;
  char *wp;
  const char *cp, *end;
  enum WebSocketOp op;

  wp = tbuf1;

  /* Restore state. */
  memcpy(mask, d->checksum, sizeof(mask));
  state = mask[0];
  type = mask[5];
  err = mask[6];
  len = d->ws_frame_len;

  /* Process buffer bytes. */
  for (cp = tbuf1, end = tbuf1 + got; cp != end; ++cp) {
    const unsigned char ch = *cp;

    switch (state++) {
    case 4:
      /* Received frame type. */
      op = ch & 0x0F;

      switch (op) {
      case WS_OP_CONTINUATION:
        /* Continue the previous opcode. */
        /* TODO: Error handling (only data frames can be continued). */
        err = 0;
        op = type & 0x0F;
        break;

      case WS_OP_TEXT:
        /* First frame of a new message. */
        err = 0;
        break;

      default:
        /* Ignore unrecognized opcode. */
        err = 1;
        break;
      }

      type = (ch & 0xF0) | op;
      break;

    case 5:
      /* Mask bit (required to be 1) and payload length (7 bits). */
      /* TODO: Error handling (check for the mask bit). */
      switch (ch & 0x7F) {
      case 126:
        /* 16-bit payload length. */
        break;

      case 127:
        /* 64-bit payload length. */
        state = 8;
        break;

      default:
        /* 7-bit payload length. */
        len = ch & 0x7F;
        state = 16;
        break;
      }
      break;

    case 6:
      /* 16-bit payload length. */
      len = ch & 0xFF;
      break;

    case 7:
      len = (len << 8) | (ch & 0xFF);
      state = 16;
      break;

    case 8:
      /* 64-bit payload length. */
      /* TODO: Error handling (first bit must be 0). */
      len = ch & 0x7F;
      break;

    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      len = (len << 8) | (ch & 0xFF);
      break;

    case 16:
    case 17:
    case 18:
      /* 32-bit mask key. */
      mask[state - 16] = ch;
      break;

    case 19:
      mask[4] = ch;

      if (len) {
        /* Begin payload. */
        state = 0;
     } else {
        /* Empty payload. */
        state = 4;

        /* TODO: Handle end of frame. */
      }
      break;

    default:
      /* Payload data; handle according to opcode. */
      if (!err) {
        *wp++ = ch ^ mask[state];
      } else {
        STARTLOG(LOG_ALWAYS, "NET", "WS");
        log_text("Unknown operation received.\n");
        ENDLOG;
        break;
      }

      if (--len) {
        /* More payload bytes. */
        state &= 0x3;
      } else {
        /* Last payload byte. */
        state = 4;

        /* TODO: Handle end of frame. */
      }
      break;
    }
  }

  /* Add a \n to the end to terminate the command */
  *wp++ = '\n';

  /* Preserve state. */
  mask[0] = state;
  mask[5] = type;
  mask[6] = err;
  memcpy(d->checksum, mask, sizeof(mask));
  d->ws_frame_len = len;

  return wp - tbuf1;
}
#endif
