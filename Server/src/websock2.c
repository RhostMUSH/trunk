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
        *tb->hdr.end++ = 127;

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
    d->cold->timeout = mudconf.idle_timeout;

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
    D_FLAGS(d) &= ~DS_API;
    D_FLAGS(d) |= DS_WEBSOCKETS;

    // Send login screen
    welcome_user(d);

    /* Set initial state */
    d->cold->checksum[0] = 4;
    d->cold->ws_last_pong = time(NULL);
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
        if (D_OUTPUT_HEAD(d) == NULL) {
            D_OUTPUT_HEAD(d) = tb;
        } else {
            D_OUTPUT_TAIL(d)->hdr.nxt = tb;
        }
        D_OUTPUT_TAIL(d) = tb;
    } while (len > 0);
}

static void
websocket_send_pong(DESC *d)
{
    TBLOCK *tb = (TBLOCK *)alloc_lbuf("websocket.pong");
    tb->hdr.nxt = NULL;
    tb->hdr.start = tb->data;
    tb->hdr.end = tb->data;

    *tb->hdr.end++ = 0x80 | WS_OP_PONG;
    *tb->hdr.end++ = 0;

    tb->hdr.nchars = tb->hdr.end - tb->hdr.start;

    if (D_OUTPUT_HEAD(d) == NULL) {
        D_OUTPUT_HEAD(d) = tb;
    } else {
        D_OUTPUT_TAIL(d)->hdr.nxt = tb;
    }
    D_OUTPUT_TAIL(d) = tb;
}

void
websocket_send_close(DESC *d, unsigned short code)
{
    TBLOCK *tb = (TBLOCK *)alloc_lbuf("websocket.close");
    tb->hdr.nxt = NULL;
    tb->hdr.start = tb->data;
    tb->hdr.end = tb->data;

    *tb->hdr.end++ = 0x80 | WS_OP_CLOSE;
    *tb->hdr.end++ = 2;
    *tb->hdr.end++ = (code >> 8) & 0xFF;
    *tb->hdr.end++ = code & 0xFF;

    tb->hdr.nchars = tb->hdr.end - tb->hdr.start;

    if (D_OUTPUT_HEAD(d) == NULL) {
        D_OUTPUT_HEAD(d) = tb;
    } else {
        D_OUTPUT_TAIL(d)->hdr.nxt = tb;
    }
    D_OUTPUT_TAIL(d) = tb;
}

void
websocket_send_ping(DESC *d)
{
    TBLOCK *tb = (TBLOCK *)alloc_lbuf("websocket.ping");
    tb->hdr.nxt = NULL;
    tb->hdr.start = tb->data;
    tb->hdr.end = tb->data;

    *tb->hdr.end++ = 0x80 | WS_OP_PING;
    *tb->hdr.end++ = 0;

    tb->hdr.nchars = tb->hdr.end - tb->hdr.start;

    if (D_OUTPUT_HEAD(d) == NULL) {
        D_OUTPUT_HEAD(d) = tb;
    } else {
        D_OUTPUT_TAIL(d)->hdr.nxt = tb;
    }
    D_OUTPUT_TAIL(d) = tb;
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
  memcpy(mask, d->cold->checksum, sizeof(mask));
  state = mask[0];
  type = mask[5];
  err = mask[6];
  len = d->cold->ws_frame_len;

  /* Process buffer bytes. */
  for (cp = tbuf1, end = tbuf1 + got; cp != end; ++cp) {
    const unsigned char ch = *cp;

    switch (state++) {
    case 4:
      /* Received frame type. */
      op = ch & 0x0F;

      switch (op) {
      case WS_OP_CONTINUATION:
	if (!d->cold->ws_fragmented) {
	  err = 1;
	} else {
	  err = 0;
	  op = type & 0x0F;
	  d->cold->ws_fragmented = !(ch & 0x80);
	}
	break;

      case WS_OP_TEXT:
	if (d->cold->ws_fragmented) {
	  err = 1;
	} else {
	  err = 0;
	  d->cold->ws_fragmented = !(ch & 0x80);
	}
	break;

      case WS_OP_CLOSE:
	if (!(ch & 0x80)) {
	  err = 1;
	  op = WS_OP_CONTINUATION;
	} else {
	  err = 2;
	}
	break;

      case WS_OP_PING:
	if (!(ch & 0x80)) {
	  err = 1;
	  op = WS_OP_CONTINUATION;
	} else {
	  err = 3;
	}
	break;

      case WS_OP_PONG:
	if (!(ch & 0x80)) {
	  err = 1;
	  op = WS_OP_CONTINUATION;
	} else {
	  err = 4;
	}
	break;

      default:
	err = 1;
	d->cold->ws_fragmented = 0;
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
      if (len > LBUF_SIZE) {
	err = 1;
	d->cold->ws_fragmented = 0;
      }
      /* FALLTHROUGH */
    case 17:
    case 18:
    case 19:
      /* 32-bit mask key. Store at mask[1..4] to preserve mask[0] for state. */
      mask[state - 15] = ch;
      if (state == 19) {
	/* All 4 mask key bytes received. */
	if (len) {
	  /* Begin payload. */
	  state = 0;
	} else {
	  /* Empty payload — end of frame. */
	  state = 4;

	  switch (type & 0x0F) {
	  case WS_OP_CLOSE:
	    websocket_send_close(d, WS_CLOSE_NORMAL);
	    d->cold->ws_closing = 1;
	    break;
	  case WS_OP_PING:
	    websocket_send_pong(d);
	    break;
	  case WS_OP_PONG:
	    d->cold->ws_last_pong = time(NULL);
	    break;
	  }
	}
      }
      break;

    default:
      /* Payload data; write only for recognized data frames. */
      if (err == 0) {
	*wp++ = ch ^ mask[1 + state];
      }
      if (--len) {
	state &= 0x3;
      } else {
	state = 4;
	switch (type & 0x0F) {
	case WS_OP_CLOSE:
	  websocket_send_close(d, WS_CLOSE_NORMAL);
	  d->cold->ws_closing = 1;
	  break;
	case WS_OP_PING:
	  websocket_send_pong(d);
	  break;
	case WS_OP_PONG:
	  d->cold->ws_last_pong = time(NULL);
	  break;
	}
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
  memcpy(d->cold->checksum, mask, sizeof(mask));
  d->cold->ws_frame_len = len;

  return wp - tbuf1;
}
#endif
