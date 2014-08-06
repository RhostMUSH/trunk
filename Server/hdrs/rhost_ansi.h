#ifndef _M_ANSI_H_
#define _M_ANSI_H_
/* rhost_ansi.h */

/* ANSI control codes for various neat-o terminal effects
 *
 * Some older versions of Ultrix don't appear to be able to
 * handle these escape sequences. If lowercase 'a's are being
 * stripped from @doings, and/or the output of the ANSI flag
 * is screwed up, you have the Ultrix problem.
 *
 * To fix the ANSI problem, try replacing the '\x1B' with '\033'.
 * To fix the problem with 'a's, replace all occurrences of '\a'
 * in the code with '\07'.
 *
 */


#ifndef OLD_ANSI

#define BEEP_CHAR     '\a'
#define ESC_CHAR      '\x1B'

#define ANSI_XTERM_BG	"\x1B[48;5;"
#define ANSI_XTERM_FG	"\x1B[38;5;"

#define ANSI_NORMAL   "\x1B[0m"
#define ANSI_NORMAL2	"0"
#define ANSI_PREFIX	"\x1B["
#define ANSI_POSTFIX	"m"

#define ANSI_HILITE   "\x1B[1m"
#define ANSI_INVERSE  "\x1B[7m"
#define ANSI_BLINK    "\x1B[5m"
#define ANSI_UNDERSCORE "\x1B[4m"
#define ANSI_HILITE2	"1"
#define ANSI_INVERSE2	"7"
#define ANSI_BLINK2	"5"
#define ANSI_UNDER2	"4"
/*
#define ANSI_INV_BLINK         "\x1B[7;5m"
#define ANSI_INV_HILITE        "\x1B[1;7m"
#define ANSI_BLINK_HILITE      "\x1B[1;5m"
#define ANSI_INV_BLINK_HILITE  "\x1B[1;5;7m"
*/

/* Global ANSI allow masks */
#define MASK_BLACK	0x00000001
#define MASK_RED	0x00000002
#define MASK_GREEN	0x00000004
#define MASK_YELLOW	0x00000008
#define MASK_BLUE	0x00000010
#define MASK_MAGENTA	0x00000020
#define MASK_CYAN	0x00000040
#define MASK_WHITE	0x00000080
#define MASK_BBLACK	0x00000100
#define MASK_BRED	0x00000200
#define MASK_BGREEN	0x00000400
#define MASK_BYELLOW	0x00000800
#define MASK_BBLUE	0x00001000
#define MASK_BMAGENTA	0x00002000
#define MASK_BCYAN	0x00004000
#define MASK_BWHITE	0x00008000
#define MASK_HILITE   	0x00010000
#define MASK_INVERSE  	0x00020000
#define MASK_BLINK    	0x00040000
#define MASK_UNDERSCORE 0x00080000

/* Foreground colors */

#define ANSI_BLACK	"\x1B[30m"
#define ANSI_BLACK_H	"\x1B[30;1m"
#define ANSI_RED	"\x1B[31m"
#define ANSI_RED_H	"\x1B[31;1m"
#define ANSI_GREEN	"\x1B[32m"
#define ANSI_GREEN_H	"\x1B[32;1m"
#define ANSI_YELLOW	"\x1B[33m"
#define ANSI_YELLOW_H	"\x1B[33;1m"
#define ANSI_BLUE	"\x1B[34m"
#define ANSI_BLUE_H	"\x1B[34;1m"
#define ANSI_MAGENTA	"\x1B[35m"
#define ANSI_MAGENTA_H	"\x1B[35;1m"
#define ANSI_CYAN	"\x1B[36m"
#define ANSI_CYAN_H	"\x1B[36;1m"
#define ANSI_WHITE	"\x1B[37m"
#define ANSI_WHITE_H	"\x1B[37;1m"
#define ANSI_BLACK2	"30"
#define ANSI_RED2	"31"
#define ANSI_GREEN2	"32"
#define ANSI_YELLOW2	"33"
#define ANSI_BLUE2	"34"
#define ANSI_MAGENTA2	"35"
#define ANSI_CYAN2	"36"
#define ANSI_WHITE2	"37"

/* Background colors */

#define ANSI_BBLACK	"\x1B[40m"
#define ANSI_BRED	"\x1B[41m"
#define ANSI_BGREEN	"\x1B[42m"
#define ANSI_BYELLOW	"\x1B[43m"
#define ANSI_BBLUE	"\x1B[44m"
#define ANSI_BMAGENTA	"\x1B[45m"
#define ANSI_BCYAN	"\x1B[46m"
#define ANSI_BWHITE	"\x1B[47m"
#define ANSI_BBLACK2	"40"
#define ANSI_BRED2	"41"
#define ANSI_BGREEN2	"42"
#define ANSI_BYELLOW2	"43"
#define ANSI_BBLUE2	"44"
#define ANSI_BMAGENTA2	"45"
#define ANSI_BCYAN2	"46"
#define ANSI_BWHITE2	"47"

#else

#define BEEP_CHAR     '\07'
#define ESC_CHAR      '\033'

#define ANSI_XTERM_BG	"\033[48;5;"
#define ANSI_XTERM_FG	"\033[38;5;"

#define ANSI_NORMAL   "\033[0m"

#define ANSI_HILITE   "\033[1m"
#define ANSI_INVERSE  "\033[7m"
#define ANSI_BLINK    "\033[5m"
#define ANSI_UNDERSCORE "\033[4m"

#define ANSI_INV_BLINK         "\033[7;5m"
#define ANSI_INV_HILITE        "\033[1;7m"
#define ANSI_BLINK_HILITE      "\033[1;5m"
#define ANSI_INV_BLINK_HILITE  "\033[1;5;7m"

/* Foreground colors */

#define ANSI_BLACK	"\033[30m"
#define ANSI_BLACK_H	"\033[30;1m"
#define ANSI_RED	"\033[31m"
#define ANSI_RED_H	"\033[31;1m"
#define ANSI_GREEN	"\033[32m"
#define ANSI_GREEN_H	"\033[32;1m"
#define ANSI_YELLOW	"\033[33m"
#define ANSI_YELLOW_H	"\033[33;1m"
#define ANSI_BLUE	"\033[34m"
#define ANSI_BLUE_H	"\033[34;1m"
#define ANSI_MAGENTA	"\033[35m"
#define ANSI_MAGENTA_H	"\033[35;1m"
#define ANSI_CYAN	"\033[36m"
#define ANSI_CYAN_H	"\033[36;1m"
#define ANSI_WHITE	"\033[37m"
#define ANSI_WHITE_H	"\033[37;1m"

/* Background colors */

#define ANSI_BBLACK	"\033[40m"
#define ANSI_BRED	"\033[41m"
#define ANSI_BGREEN	"\033[42m"
#define ANSI_BYELLOW	"\033[43m"
#define ANSI_BBLUE	"\033[44m"
#define ANSI_BMAGENTA	"\033[45m"
#define ANSI_BCYAN	"\033[46m"
#define ANSI_BWHITE	"\033[47m"

#endif

#define ANSIEX(x)	(No_Ansi_Ex(player) ? "" : (x))

#ifdef ZENTY_ANSI
#ifdef TINY_SUB
/* Begin %x subs */
#define SAFE_CHR	'x'
#define SAFE_CHRST	"%x"
#define SAFE_ANSI_NORMAL  "%xn"

#define SAFE_ANSI_HILITE   "%xh"
#define SAFE_ANSI_INVERSE  "%xi"
#define SAFE_ANSI_BLINK    "%xf"
#define SAFE_ANSI_UNDERSCORE "%xu"


//#define SAFE_ANSI_INV_BLINK         "%x"
//#define SAFE_ANSI_INV_HILITE        "%x"
//#define SAFE_ANSI_BLINK_HILITE      "%x"
//#define SAFE_ANSI_INV_BLINK_HILITE  "%x"

/* Foreground colors */

#define SAFE_ANSI_BLACK	"%xx"
#define SAFE_ANSI_RED	"%xr"
#define SAFE_ANSI_GREEN	"%xg"
#define SAFE_ANSI_YELLOW	"%xy"
#define SAFE_ANSI_BLUE	"%xb"
#define SAFE_ANSI_MAGENTA	"%xm"
#define SAFE_ANSI_CYAN	"%xc"
#define SAFE_ANSI_WHITE	"%xw"

/* Background colors */

#define SAFE_ANSI_BBLACK	"%xX"
#define SAFE_ANSI_BRED	"%xR"
#define SAFE_ANSI_BGREEN	"%xG"
#define SAFE_ANSI_BYELLOW	"%xY"
#define SAFE_ANSI_BBLUE	"%xB"
#define SAFE_ANSI_BMAGENTA	"%xM"
#define SAFE_ANSI_BCYAN	"%xC"
#define SAFE_ANSI_BWHITE	"%xW"
#else
/* Begin %c subs */
#define SAFE_CHR	'c'
#define SAFE_CHRST	"%c"
#define SAFE_ANSI_NORMAL  "%cn"

#define SAFE_ANSI_HILITE   "%ch"
#define SAFE_ANSI_INVERSE  "%ci"
#define SAFE_ANSI_BLINK    "%cf"
#define SAFE_ANSI_UNDERSCORE "%cu"


//#define SAFE_ANSI_INV_BLINK         "%c"
//#define SAFE_ANSI_INV_HILITE        "%c"
//#define SAFE_ANSI_BLINK_HILITE      "%c"
//#define SAFE_ANSI_INV_BLINK_HILITE  "%c"

/* Foreground colors */

#define SAFE_ANSI_BLACK	"%cx"
#define SAFE_ANSI_RED	"%cr"
#define SAFE_ANSI_GREEN	"%cg"
#define SAFE_ANSI_YELLOW	"%cy"
#define SAFE_ANSI_BLUE	"%cb"
#define SAFE_ANSI_MAGENTA	"%cm"
#define SAFE_ANSI_CYAN	"%cc"
#define SAFE_ANSI_WHITE	"%cw"

/* Background colors */

#define SAFE_ANSI_BBLACK	"%cX"
#define SAFE_ANSI_BRED	"%cR"
#define SAFE_ANSI_BGREEN	"%cG"
#define SAFE_ANSI_BYELLOW	"%cY"
#define SAFE_ANSI_BBLUE	"%cB"
#define SAFE_ANSI_BMAGENTA	"%cM"
#define SAFE_ANSI_BCYAN	"%cC"
#define SAFE_ANSI_BWHITE	"%cW"

/* End of %c subs */
#endif

#ifdef INCLUDE_ASCII_TABLE
// Is the %c/%x code ansi?
static char isAnsi[256] =
{
/*  0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F   */
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // 0
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // 1
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // 2
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // 3
    0, 0, 1, 1, 0, 0, 0, 1,  0, 0, 0, 0, 0, 1, 0, 0, // 4
    0, 0, 1, 0, 0, 0, 0, 1,  1, 1, 0, 0, 0, 0, 0, 0, // 5
    0, 0, 1, 1, 0, 0, 1, 1,  1, 1, 0, 0, 0, 1, 1, 0, // 6
    0, 0, 1, 0, 0, 1, 0, 1,  1, 1, 0, 0, 0, 0, 0, 0, // 7
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // 8
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // 9
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // A
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // B
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // C
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // D
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, // E
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0  // F
};
#endif

#else
#ifdef TINY_SUB
#define SAFE_CHR	'x'
#else
#define SAFE_CHR	'c'
#endif
#endif
#endif
