#ifndef MUSH_WEBSOCK_H
#define MUSH_WEBSOCK_H

#include <stdlib.h>
#include <openssl/sha.h>

/* Length of 16 bytes, Base64 encoded (with padding). */
#define WEBSOCKET_KEY_LEN 24

/* Length of magic value. */
#define WEBSOCKET_KEY_MAGIC_LEN 36

/* Length of 20 bytes, Base64 encoded (with padding). */
#define WEBSOCKET_ACCEPT_LEN 28

/* Magic string for accept key */
#define WEBSOCKET_KEY_MAGIC "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/* WebSocket opcodes. */
enum WebSocketOp {
  WS_OP_CONTINUATION = 0x0,
  WS_OP_TEXT = 0x1,
  WS_OP_BINARY = 0x2,
  /* 0x3 - 0x7 reserved for control frames */
  WS_OP_CLOSE = 0x8,
  WS_OP_PING = 0x9,
  WS_OP_PONG = 0xA
  /* 0xB - 0xF reserved for control frames */
};

extern int validate_websocket_key(char *);
extern void complete_handshake(DESC *, const char *);

#endif /* undef MUSH_WEBSOCK_H */
