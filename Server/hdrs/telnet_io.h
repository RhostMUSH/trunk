#ifndef TELNET_IO_H
#define TELNET_IO_H

/* Forward declaration — interface.h included by .c files in proper order */
struct descriptor_data;

/* Initialize libtelnet for a descriptor. Call after alloc_desc in initializesock. */
void telnet_init_desc(DESC *d);

/* Free libtelnet state for a descriptor. Call in shutdownsock before free_desc. */
void telnet_free_desc(DESC *d);

/* Preprocess raw input through libtelnet: strips telnet negotiation, handles NAWS.
 * Reads from buf[0..*got), writes clean text back into buf[0..*new_got).
 * Returns 1 if data remains to process, 0 if only telnet control (no text). */
int telnet_preprocess_input(DESC *d, char *buf, int *got);

#endif /* TELNET_IO_H */
