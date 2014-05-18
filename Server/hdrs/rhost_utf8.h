#ifndef _M_UTF8_H_
#define _M_UTF8_H_
/* rhost_utf8.h */

/* UTF-8 related constants for identifying
 * valid UTF-8 byte sequences and converting
 * between UTF-8 and a unicode code point value
 */
#define UTF8_BITMASK	0xF8	// Bitmask to test for first UTF8 byte
#define UTF8_4BYTE		0xF0	// 4BYTE char should have this value after masking
#define UTF8_3BYTE		0xE0	// 3BYTE char should have this value after masking
#define UTF8_2BYTE		0xDF	// 2BYTE char should have this value after masking

#define UTF8_CBYTE_MASK	0xC0	// Bitmask to test for CBYTE
#define UTF8_CBYTE		0x80	// CBYTE should have this value after mask


#define IS_4BYTE(a)		((a & UTF8_BITMASK) == UTF8_4BYTE)
#define IS_3BYTE(a)		((a & UTF8_BITMASK) == UTF8_3BYTE)
#define IS_2BYTE(a)		((a & UTF8_BITMASK) == UTF8_2BYTE)
#define IS_CBYTE(a)		((a & UTF8_CBYTE_MASK) == UTF8_CBYTE)

#endif