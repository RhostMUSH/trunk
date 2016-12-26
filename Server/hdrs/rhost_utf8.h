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

// Fancy quote to ascii quote conversion
#define ASCII_DOUBLE_QUOTE      0x0022
#define DOUBLE_QUOTE_LEFT       0x201C
#define DOUBLE_QUOTE_RIGHT      0x201D
#define DOUBLE_QUOTE_REVERSED   0x201F

// Full width colon to standard colon conversion
#define ASCII_COLON     0x003A
#define FULLWIDTH_COLON 0xFF1A 

// Standard space
#define ASCII_SPACE     0x0020

#endif