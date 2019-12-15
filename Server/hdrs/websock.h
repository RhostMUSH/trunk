/*
 * WebSockets (RFC 6455) support.
 * RhostMUSH version.
 */

#ifndef MUSH_WEBSOCK_H
#define MUSH_WEBSOCK_H

#define IsWebSocket(d) ((d)->flags & DS_WEBSOCKETS)

#define WEBSOCKET_CHANNEL_AUTO (0)
#define WEBSOCKET_CHANNEL_TEXT ('t')
#define WEBSOCKET_CHANNEL_JSON ('j')
#define WEBSOCKET_CHANNEL_HTML ('h')
#define WEBSOCKET_CHANNEL_PUEBLO ('p')
#define WEBSOCKET_CHANNEL_PROMPT ('>')

#define WS_GET_REQUEST_URL "/wsclient"

/* websock.c */
int process_websocket_request(DESC *d, const char *command);
int process_websocket_header(DESC *d, const char *command);
int process_websocket_frame(DESC *d, char *tbuf1, int got);
void to_websocket_frame(const char **bp, int *np, char channel);

#endif /* undef MUSH_WEBSOCK_H */
