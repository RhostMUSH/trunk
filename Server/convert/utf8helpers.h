#ifndef _M_UTF8_H_
#define _M_UTF8_H_
/* rhost_utf8.h */

/* UTF-8 related constants for identifying
 * valid UTF-8 byte sequences and converting
 * between UTF-8 and a unicode code point value
 */
#define BYTE4_MASK      0xF8    // Byte mask for testing for 4byte character
#define BYTE3_MASK      0xF0    // Byte mask for testing for 3byte character
#define BYTE2_MASK      0xE0    // Byte mask for testing for 2byte character

#define UTF8_4BYTE		0xF0	// 4BYTE char should have this value after masking
#define UTF8_3BYTE		0xE0	// 3BYTE char should have this value after masking
#define UTF8_2BYTE		0xC0	// 2BYTE char should have this value after masking

#define UTF8_CBYTE_MASK	0xC0	// Bitmask to test for CBYTE
#define UTF8_CBYTE		0x80	// CBYTE should have this value after mask


#define IS_4BYTE(a)		((a & BYTE4_MASK) == UTF8_4BYTE)
#define IS_3BYTE(a)		((a & BYTE3_MASK) == UTF8_3BYTE)
#define IS_2BYTE(a)		((a & BYTE2_MASK) == UTF8_2BYTE)
#define IS_CBYTE(a)		((a & UTF8_CBYTE_MASK) == UTF8_CBYTE)

// TinyMUX Ansi UTF8 code ranges
#define ANSI_MODS_LOW   0xF500
#define ANSI_MODS_HIGH  0xF507
#define ANSI_BFG_LOW    0xF600
#define ANSI_BFG_HIGH   0xF60F
#define ANSI_XFG_LOW    0xF610
#define ANSI_XFG_HIGH   0xF6FF
#define ANSI_BBG_LOW    0xF700
#define ANSI_BBG_HIGH   0xF707
#define ANSI_XBG_LOW    0xF708
#define ANSI_XBG_HIGH   0xF7FF
#define ANSI_ADJ_LOW    0xF0000
#define ANSI_ADJ_HIGH   0xF05FF

// Color Group/Type IDs
#define ANSI_MOD      1
#define ANSI_BASE_FG  2
#define ANSI_XTERM_FG 3
#define ANSI_BASE_BG  4
#define ANSI_XTERM_BG 5
#define ANSI_ADJ      6

// Maximum buffer size
#define BUFFER_SIZE 65535

// EXTANSI attr num
#define EXTANSI_ATTR 220

// EXTANSI toggle
#define EXTANSI_TOGGLE 0x00000001

extern int strip_utf8(char *, char*);
extern void convert_markup(char *, char *, char, int);
#endif
