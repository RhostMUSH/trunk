
/* NEW FUNCTION PARAMETER PROTOCOL 11/14/1995 (implemented by Thorin)
 *
 * an lbuf named 'buff' is passed to each function (that has always been)
 * but there is now a 'bufcx' which is of type char** and should be used
 * to keep 'buff' bounded. You are no longer able to assume that you
 * have control over the whole lbuf, basically you pick up where the
 * last function left off, and you start inserting characters at *bufcx.
 * YOU SHOULD ONLY WRITE TO 'buff' WITH THE STANDARD LBUF MANIPULATION
 * FUNCTIONS SUCH AS safe_chr, safe_str, fval, ival, and dbval.
 *
 * A terminating null character should NOT be placed in 'buff'. This
 * will be done by the exec() function.
 *
 * Never modify characters to the left of *bufcx and don't assume that
 * *bufcx is positioned at the start of 'buff' when you enter the function.
 * You have the space from *bufcx to the end of 'buff' to write to.
 *
 * - Thorin
 */

#ifdef SOLARIS
/* Solaris has borked declarations in their headers */
char *index(const char *, int);
char *rindex(const char *, int);
#endif

#include "copyright.h"
#include "autoconf.h"

#include <time.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include <dirent.h>
#ifdef HAS_OPENSSL
#include <openssl/sha.h>
#include <openssl/evp.h>
#else
#include "shs.h"
#endif

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "flags.h"
#include "externs.h"
#include "interface.h"
#include "match.h"
#include "command.h"
#include "functions.h"
#include "misc.h"
#include "alloc.h"
#include "patchlevel.h"
#include "mail.h"

#define PAGE_VAL ((LBUF_SIZE - (LBUF_SIZE/20)) / SBUF_SIZE)
#define INCLUDE_ASCII_TABLE
#include "rhost_ansi.h"
#ifdef REALITY_LEVELS
#include "levels.h"
#endif /* REALITY_LEVELS */
#include "timez.h"
#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

typedef struct PENN_COLORMAP {
  char *name;
  int i_xterm;
} PENNANSI;

typedef struct MUX_COLORMAP {
  int i_dec;
  char *s_hex;
} MUXANSI;


char *ansi_translate_16[257]={
   "x", "r", "g", "y", "b", "m", "c", "w",
   "xh", "rh", "gh", "yh", "bh", "mh", "ch", "wh",
   "x", "b", "b", "b", "bh", "bh", "g", "c",
   "b", "bh", "bh", "bh", "gh", "g", "c", "bh", 
   "bh", "bh", "gh", "gh", "ch", "ch", "bh", "bh",
   "gh", "gh", "gh", "ch", "ch", "ch", "gh", "gh",
   "gh", "ch", "ch", "ch", "r", "m", "m", "mh", 
   "bh", "bh", "g", "g", "c", "bh", "bh", "bh",
   "g", "g", "g", "c", "c", "bh", "gh", "g",
   "g", "c", "ch", "ch", "gh", "gh", "gh", "ch", 
   "ch", "ch", "gh", "gh", "gh", "gh", "ch", "ch",
   "r", "r", "m", "m", "mh", "mh", "r", "r",
   "m", "m", "mh", "mh", "y", "y", "g", "c", 
   "bh", "bh", "g", "g", "g", "c", "ch", "ch", 
   "gh", "gh", "gh", "gh", "ch", "ch", "gh", "gh",
   "gh", "gh", "wh", "wh", "r", "m", "m", "mh",
   "mh", "mh", "rh", "rh", "rh", "mh", "mh", "mh",
   "y", "y", "mh", "mh", "mh", "mh", "y", "y",
   "y", "mh", "mh", "mh", "yh", "yh", "gh", "wh",
   "wh", "wh", "yh", "yh", "yh", "yh", "wh", "wh",
   "r", "rh", "mh", "mh", "mh", "mh", "rh", "rh",
   "rh", "mh", "mh", "mh",  "y", "y", "y", "mh",
   "mh", "mh", "y", "y", "yh", "mh", "mh", "mh",
   "y", "y", "yh", "wh", "wh", "wh", "yw", "yw",
   "yh", "wh", "wh", "wh", "rh", "rh", "rh", "mh",
   "mh", "mh", "rh", "rh", "rh", "mh", "mh", "mh",
   "rh", "rh", "mh", "mh", "mh", "mh", "yh", "yh",
   "wh", "wh", "wh", "wh", "yh", "yh", "yh", "wh", 
   "wh", "wh", "yh", "yh", "yh", "yh", "wh", "wh",
   "x", "x", "xh", "xh", "xh", "xh", "xh", "xh",
   "xh", "w", "w", "w", "w", "w", "w", "w",
   "wh", "wh", "wh", "wh", "wh", "wh", "wh", "wh", (char *)NULL,
};

#ifdef ZENTY_ANSI
static struct MUX_COLORMAP mux_namecolors[] = {
    {0, "000000"},
    {1, "800000"},
    {2, "008000"},
    {3, "808000"},
    {4, "000080"},
    {5, "800080"},
    {6, "008080"},
    {7, "c0c0c0"},
    {8, "808080"},
    {9, "ff0000"},
    {10, "00ff00"},
    {11, "ffff00"},
    {12, "0000ff"},
    {13, "ff00ff"},
    {14, "00ffff"},
    {15, "ffffff"},
    {16, "000000"},
    {17, "00005f"},
    {18, "000087"},
    {19, "0000af"},
    {20, "0000d7"},
    {21, "0000ff"},
    {22, "005f00"},
    {23, "005f5f"},
    {24, "005f87"},
    {25, "005faf"},
    {26, "005fd7"},
    {27, "005fff"},
    {28, "008700"},
    {29, "00875f"},
    {30, "008787"},
    {31, "0087af"},
    {32, "0087d7"},
    {33, "0087ff"},
    {34, "00af00"},
    {35, "00af5f"},
    {36, "00af87"},
    {37, "00afaf"},
    {38, "00afd7"},
    {39, "00afff"},
    {40, "00d700"},
    {41, "00d75f"},
    {42, "00d787"},
    {43, "00d7af"},
    {44, "00d7d7"},
    {45, "00d7ff"},
    {46, "00ff00"},
    {47, "00ff5f"},
    {48, "00ff87"},
    {49, "00ffaf"},
    {50, "00ffd7"},
    {51, "00ffff"},
    {52, "5f0000"},
    {53, "5f005f"},
    {54, "5f0087"},
    {55, "5f00af"},
    {56, "5f00d7"},
    {57, "5f00ff"},
    {58, "5f5f00"},
    {59, "5f5f5f"},
    {60, "5f5f87"},
    {61, "5f5faf"},
    {62, "5f5fd7"},
    {63, "5f5fff"},
    {64, "5f8700"},
    {65, "5f875f"},
    {66, "5f8787"},
    {67, "5f87af"},
    {68, "5f87d7"},
    {69, "5f87ff"},
    {70, "5faf00"},
    {71, "5faf5f"},
    {72, "5faf87"},
    {73, "5fafaf"},
    {74, "5fafd7"},
    {75, "5fafff"},
    {76, "5fd700"},
    {77, "5fd75f"},
    {78, "5fd787"},
    {79, "5fd7af"},
    {80, "5fd7d7"},
    {81, "5fd7ff"},
    {82, "5fff00"},
    {83, "5fff5f"},
    {84, "5fff87"},
    {85, "5fffaf"},
    {86, "5fffd7"},
    {87, "5fffff"},
    {88, "870000"},
    {89, "87005f"},
    {90, "870087"},
    {91, "8700af"},
    {92, "8700d7"},
    {93, "8700ff"},
    {94, "875f00"},
    {95, "875f5f"},
    {96, "875f87"},
    {97, "875faf"},
    {98, "875fd7"},
    {99, "875fff"},
    {100, "878700"},
    {101, "87875f"},
    {102, "878787"},
    {103, "8787af"},
    {104, "8787d7"},
    {105, "8787ff"},
    {106, "87af00"},
    {107, "87af5f"},
    {108, "87af87"},
    {109, "87afaf"},
    {110, "87afd7"},
    {111, "87afff"},
    {112, "87d700"},
    {113, "87d75f"},
    {114, "87d787"},
    {115, "87d7af"},
    {116, "87d7d7"},
    {117, "87d7ff"},
    {118, "87ff00"},
    {119, "87ff5f"},
    {120, "87ff87"},
    {121, "87ffaf"},
    {122, "87ffd7"},
    {123, "87ffff"},
    {124, "af0000"},
    {125, "af005f"},
    {126, "af0087"},
    {127, "af00af"},
    {128, "af00d7"},
    {129, "af00ff"},
    {130, "af5f00"},
    {131, "af5f5f"},
    {132, "af5f87"},
    {133, "af5faf"},
    {134, "af5fd7"},
    {135, "af5fff"},
    {136, "af8700"},
    {137, "af875f"},
    {138, "af8787"},
    {139, "af87af"},
    {140, "af87d7"},
    {141, "af87ff"},
    {142, "afaf00"},
    {143, "afaf5f"},
    {144, "afaf87"},
    {145, "afafaf"},
    {146, "afafd7"},
    {147, "afafff"},
    {148, "afd700"},
    {149, "afd75f"},
    {150, "afd787"},
    {151, "afd7af"},
    {152, "afd7d7"},
    {153, "afd7ff"},
    {154, "afff00"},
    {155, "afff5f"},
    {156, "afff87"},
    {157, "afffaf"},
    {158, "afffd7"},
    {159, "afffff"},
    {160, "d70000"},
    {161, "d7005f"},
    {162, "d70087"},
    {163, "d700af"},
    {164, "d700d7"},
    {165, "d700ff"},
    {166, "d75f00"},
    {167, "d75f5f"},
    {168, "d75f87"},
    {169, "d75faf"},
    {170, "d75fd7"},
    {171, "d75fff"},
    {172, "d78700"},
    {173, "d7875f"},
    {174, "d78787"},
    {175, "d787af"},
    {176, "d787d7"},
    {177, "d787ff"},
    {178, "d7af00"},
    {179, "d7af5f"},
    {180, "d7af87"},
    {181, "d7afaf"},
    {182, "d7afd7"},
    {183, "d7afff"},
    {184, "d7d700"},
    {185, "d7d75f"},
    {186, "d7d787"},
    {187, "d7d7af"},
    {188, "d7d7d7"},
    {189, "d7d7ff"},
    {190, "d7ff00"},
    {191, "d7ff5f"},
    {192, "d7ff87"},
    {193, "d7ffaf"},
    {194, "d7ffd7"},
    {195, "d7ffff"},
    {196, "ff0000"},
    {197, "ff005f"},
    {198, "ff0087"},
    {199, "ff00af"},
    {200, "ff00d7"},
    {201, "ff00ff"},
    {202, "ff5f00"},
    {203, "ff5f5f"},
    {204, "ff5f87"},
    {205, "ff5faf"},
    {206, "ff5fd7"},
    {207, "ff5fff"},
    {208, "ff8700"},
    {209, "ff875f"},
    {210, "ff8787"},
    {211, "ff87af"},
    {212, "ff87d7"},
    {213, "ff87ff"},
    {214, "ffaf00"},
    {215, "ffaf5f"},
    {216, "ffaf87"},
    {217, "ffafaf"},
    {218, "ffafd7"},
    {219, "ffafff"},
    {220, "ffd700"},
    {221, "ffd75f"},
    {222, "ffd787"},
    {223, "ffd7af"},
    {224, "ffd7d7"},
    {225, "ffd7ff"},
    {226, "ffff00"},
    {227, "ffff5f"},
    {228, "ffff87"},
    {229, "ffffaf"},
    {230, "ffffd7"},
    {231, "ffffff"},
    {232, "080808"},
    {233, "121212"},
    {234, "1c1c1c"},
    {235, "262626"},
    {236, "303030"},
    {237, "3a3a3a"},
    {238, "444444"},
    {239, "4e4e4e"},
    {240, "585858"},
    {241, "626262"},
    {242, "6c6c6c"},
    {243, "767676"},
    {244, "808080"},
    {245, "8a8a8a"},
    {246, "949494"},
    {247, "9e9e9e"},
    {248, "a8a8a8"},
    {249, "b2b2b2"},
    {250, "bcbcbc"},
    {251, "c6c6c6"},
    {252, "d0d0d0"},
    {253, "dadada"},
    {254, "e4e4e4"},
    {255, "eeeeee"},
    {0, NULL},
};
#endif

static struct PENN_COLORMAP penn_namecolors[]= {
   {"aliceblue", 15 },
   {"antiquewhite", 224 },
   {"antiquewhite1", 230 },
   {"antiquewhite2", 224 },
   {"antiquewhite3", 181 },
   {"antiquewhite4", 8 },
   {"aquamarine", 122 },
   {"aquamarine1",  122},
   {"aquamarine2", 122 },
   {"aquamarine3",  79},
   {"aquamarine4", 66 },
   {"azure", 15 },
   {"azure1", 15 },
   {"azure2", 255 },
   {"azure3", 251 },
   {"azure4", 102 },
   {"beige", 230 },
   {"bisque", 224 },
   {"bisque1", 224 },
   {"bisque2", 223 },
   {"bisque3", 181 },
   {"bisque4", 101 },
   {"black", 0 },
   {"blanchedalmond", 224 },
   {"blue", 12 },
   {"blue1", 12 },
   {"blue2", 12 },
   {"blue3", 20 },
   {"blue4", 18 },
   {"blueviolet", 92 },
   {"brown", 124 },
   {"brown1", 203 },
   {"brown2", 203 },
   {"brown3", 167 },
   {"brown4", 88 },
   {"burlywood", 180 },
   {"burlywood1", 222 },
   {"burlywood2", 222 },
   {"burlywood3", 180 },
   {"burlywood4", 95 },
   {"cadetblue", 73 },
   {"cadetblue1", 123 },
   {"cadetblue2", 117 },
   {"cadetblue3", 116 },
   {"cadetblue4", 66 },
   {"chartreuse", 118 },
   {"chartreuse1", 118 },
   {"chartreuse2", 118 },
   {"chartreuse3", 76 },
   {"chartreuse4", 64 },
   {"chocolate", 166 },
   {"chocolate1", 208 },
   {"chocolate2", 208 },
   {"chocolate3", 166 },
   {"chocolate4", 94 },
   {"coral", 209 },
   {"coral1", 203 },
   {"coral2", 203 },
   {"coral3", 167 },
   {"coral4", 94 },
   {"cornflowerblue", 69 },
   {"cornsilk", 230 },
   {"cornsilk1", 230 },
   {"cornsilk2", 254 },
   {"cornsilk3", 187 },
   {"cornsilk4", 102 },
   {"cyan", 14 },
   {"cyan1", 14 },
   {"cyan2", 14 },
   {"cyan3", 44 },
   {"cyan4", 30 },
   {"darkblue", 18 },
   {"darkcyan", 30 },
   {"darkgoldenrod", 136 },
   {"darkgoldenrod1", 214 },
   {"darkgoldenrod2", 214 },
   {"darkgoldenrod3", 172 },
   {"darkgoldenrod4", 94 },
   {"darkgray", 248 },
   {"darkgreen", 22 },
   {"darkgrey", 248 },
   {"darkkhaki", 143 },
   {"darkmagenta", 90 },
   {"darkolivegreen", 239 },
   {"darkolivegreen1", 191 },
   {"darkolivegreen2", 155 },
   {"darkolivegreen3", 149 },
   {"darkolivegreen4", 65 },
   {"darkorange", 208 },
   {"darkorange1", 208 },
   {"darkorange2", 208 },
   {"darkorange3", 166 },
   {"darkorange4", 94 },
   {"darkorchid", 98 },
   {"darkorchid1", 135 },
   {"darkorchid2", 135 },
   {"darkorchid3", 98 },
   {"darkorchid4", 54 },
   {"darkred", 88 },
   {"darksalmon", 174 },
   {"darkseagreen", 108 },
   {"darkseagreen1", 157 },
   {"darkseagreen2", 157 },
   {"darkseagreen3", 114 },
   {"darkseagreen4", 65 },
   {"darkslateblue", 60 },
   {"darkslategray", 238 },
   {"darkslategray1", 123 },
   {"darkslategray2", 123 },
   {"darkslategray3", 116 },
   {"darkslategray4", 66 },
   {"darkslategrey", 238 },
   {"darkturquoise", 44 },
   {"darkviolet", 92 },
   {"debianred", 161 },
   {"deeppink", 198 },
   {"deeppink1", 198 },
   {"deeppink2", 198 },
   {"deeppink3", 162 },
   {"deeppink4", 89 },
   {"deepskyblue", 39 },
   {"deepskyblue1", 39 },
   {"deepskyblue2", 39 },
   {"deepskyblue3", 32 },
   {"deepskyblue4", 24 },
   {"dimgrey", 242 },
   {"dodgerblue", 33 },
   {"dodgerblue1", 33 },
   {"dodgerblue2", 33 },
   {"dodgerblue3", 32 },
   {"dodgerblue4", 24 },
   {"firebrick", 124 },
   {"firebrick1", 203 },
   {"firebrick2", 9 },
   {"firebrick3", 160 },
   {"firebrick4", 88 },
   {"floralwhite", 15 },
   {"forestgreen", 28 },
   {"gainsboro", 253 },
   {"ghostwhite", 15 },
   {"gold", 220 },
   {"gold1", 220 },
   {"gold2", 220 },
   {"gold3", 178 },
   {"gold4", 3 },
   {"goldenrod", 178 },
   {"goldenrod1", 214 },
   {"goldenrod2", 214 },
   {"goldenrod3", 172 },
   {"goldenrod4", 94 },
   {"gray", 7 },
   {"gray0", 0 },
   {"gray1", 0 },
   {"gray2", 232 },
   {"gray3", 232 },
   {"gray4", 232 },
   {"gray5", 232 },
   {"gray6", 233 },
   {"gray7", 233 },
   {"gray8", 233 },
   {"gray9", 233 },
   {"gray10", 234 },
   {"gray11", 234 },
   {"gray12", 234 },
   {"gray13", 234 },
   {"gray14", 235 },
   {"gray15", 235 },
   {"gray16", 235 },
   {"gray17", 235 },
   {"gray18", 236 },
   {"gray19", 236 },
   {"gray20", 236 },
   {"gray21", 237 },
   {"gray22", 237 },
   {"gray23", 237 },
   {"gray24", 237 },
   {"gray25", 238 },
   {"gray26", 238 },
   {"gray27", 238 },
   {"gray28", 238 },
   {"gray29", 239 },
   {"gray30", 239 },
   {"gray31", 239 },
   {"gray32", 239 },
   {"gray33", 240 },
   {"gray34", 240 },
   {"gray35", 240 },
   {"gray36", 59 },
   {"gray37", 59 },
   {"gray38", 241 },
   {"gray39", 241 },
   {"gray40", 241 },
   {"gray41", 242 },
   {"gray42", 242 },
   {"gray43", 242 },
   {"gray44", 242 },
   {"gray45", 243 },
   {"gray46", 243 },
   {"gray47", 243 },
   {"gray48", 243 },
   {"gray49", 8 },
   {"gray50", 8 },
   {"gray51", 8 },
   {"gray52", 102 },
   {"gray53", 102 },
   {"gray54", 245 },
   {"gray55", 245 },
   {"gray56", 245 },
   {"gray57", 246 },
   {"gray58", 246 },
   {"gray59", 246 },
   {"gray60", 246 },
   {"gray61", 247 },
   {"gray62", 247 },
   {"gray63", 247 },
   {"gray64", 247 },
   {"gray65", 248 },
   {"gray66", 248 },
   {"gray67", 248 },
   {"gray68", 145 },
   {"gray69", 145 },
   {"gray70", 249 },
   {"gray71", 249 },
   {"gray72", 250 },
   {"gray73", 250 },
   {"gray74", 250 },
   {"gray75", 7 },
   {"gray76", 7 },
   {"gray77", 251 },
   {"gray78", 251 },
   {"gray79", 251 },
   {"gray80", 252 },
   {"gray81", 252 },
   {"gray82", 252 },
   {"gray83", 188 },
   {"gray84", 188 },
   {"gray85", 253 },
   {"gray86", 253 },
   {"gray87", 253 },
   {"gray88", 254 },
   {"gray89", 254 },
   {"gray90", 254 },
   {"gray91", 254 },
   {"gray92", 255 },
   {"gray93", 255 },
   {"gray94", 255 },
   {"gray95", 255 },
   {"gray96", 255 },
   {"gray97", 15 },
   {"gray98", 15 },
   {"gray99", 15 },
   {"gray100", 15 },
   {"green", 10 },
   {"green1", 10 },
   {"green2", 10 },
   {"green3", 40 },
   {"green4", 28 },
   {"greenyellow", 154 },
   {"grey", 7 },
   {"grey0", 0 },
   {"grey1", 0 },
   {"grey2", 232 },
   {"grey3", 232 },
   {"grey4", 232 },
   {"grey5", 232 },
   {"grey6", 233 },
   {"grey7", 233 },
   {"grey8", 233 },
   {"grey9", 233 },
   {"grey10", 234 },
   {"grey11", 234 },
   {"grey12", 234 },
   {"grey13", 234 },
   {"grey14", 235 },
   {"grey15", 235 },
   {"grey16", 235 },
   {"grey17", 235 },
   {"grey18", 236 },
   {"grey19", 236 },
   {"grey20", 236 },
   {"grey21", 237 },
   {"grey22", 237 },
   {"grey23", 237 },
   {"grey24", 237 },
   {"grey25", 238 },
   {"grey26", 238 },
   {"grey27", 238 },
   {"grey28", 238 },
   {"grey29", 239 },
   {"grey30", 239 },
   {"grey31", 239 },
   {"grey32", 239 },
   {"grey33", 240 },
   {"grey34", 240 },
   {"grey35", 240 },
   {"grey36", 59 },
   {"grey37", 59 },
   {"grey38", 241 },
   {"grey39", 241 },
   {"grey40", 241 },
   {"grey41", 242 },
   {"grey42", 242 },
   {"grey43", 242 },
   {"grey44", 242 },
   {"grey45", 243 },
   {"grey46", 243 },
   {"grey47", 243 },
   {"grey48", 243 },
   {"grey49", 8 },
   {"grey50", 8 },
   {"grey51", 8 },
   {"grey52", 102 },
   {"grey53", 102 },
   {"grey54", 245 },
   {"grey55", 245 },
   {"grey56", 245 },
   {"grey57", 246 },
   {"grey58", 246 },
   {"grey59", 246 },
   {"grey60", 246 },
   {"grey61", 247 },
   {"grey62", 247 },
   {"grey63", 247 },
   {"grey64", 247 },
   {"grey65", 248 },
   {"grey66", 248 },
   {"grey67", 248 },
   {"grey68", 145 },
   {"grey69", 145 },
   {"grey70", 249 },
   {"grey71", 249 },
   {"grey72", 250 },
   {"grey73", 250 },
   {"grey74", 250 },
   {"grey75", 7 },
   {"grey76", 7 },
   {"grey77", 251 },
   {"grey78", 251 },
   {"grey79", 251 },
   {"grey80", 252 },
   {"grey81", 252 },
   {"grey82", 252 },
   {"grey83", 188 },
   {"grey84", 188 },
   {"grey85", 253 },
   {"grey86", 253 },
   {"grey87", 253 },
   {"grey88", 254 },
   {"grey89", 254 },
   {"grey90", 254 },
   {"grey91", 254 },
   {"grey92", 255 },
   {"grey93", 255 },
   {"grey94", 255 },
   {"grey95", 255 },
   {"grey96", 255 },
   {"grey97", 15 },
   {"grey98", 15 },
   {"grey99", 15 },
   {"grey100",  231},
   {"honeydew", 255 },
   {"honeydew1", 255 },
   {"honeydew2",  194},
   {"honeydew2", 254 },
   {"honeydew3", 251 },
   {"honeydew4", 102 },
   {"hotpink", 205 },
   {"hotpink1", 205 },
   {"hotpink2", 205 },
   {"hotpink3", 168 },
   {"hotpink4", 95 },
   {"indianred", 167 },
   {"indianred1", 203 },
   {"indianred2", 203 },
   {"indianred3", 167 },
   {"indianred4", 95 },
   {"indigo", 54 },
   {"ivory", 15 },
   {"ivory1", 15 },
   {"ivory2", 255 },
   {"ivory3", 251 },
   {"ivory4", 102 },
   {"khaki", 222 },
   {"khaki1", 228 },
   {"khaki2", 222 },
   {"khaki3", 185 },
   {"khaki4", 101 },
   {"lavender", 255 },
   {"lavenderblush", 15 },
   {"lavenderblush1", 15 },
   {"lavenderblush2", 254 },
   {"lavenderblush3", 251 },
   {"lavenderblush4", 102 },
   {"lawngreen", 118 },
   {"lemonchiffon", 230 },
   {"lemonchiffon1", 230 },
   {"lemonchiffon2", 223 },
   {"lemonchiffon3", 187 },
   {"lemonchiffon4", 101 },
   {"lightblue", 152 },
   {"lightblue1", 159 },
   {"lightblue2", 153 },
   {"lightblue3", 110 },
   {"lightblue4", 66 },
   {"lightcoral", 210 },
   {"lightcyan", 195 },
   {"lightcyan1", 195 },
   {"lightcyan2", 254 },
   {"lightcyan3", 152 },
   {"lightcyan4", 102 },
   {"lightgoldenrod", 222 },
   {"lightgoldenrod1", 228 },
   {"lightgoldenrod2", 222 },
   {"lightgoldenrod3", 179},
   {"lightgoldenrod4", 101 },
   {"lightgoldenrodyellow", 205 },
   {"lightgray", 252 },
   {"lightgreen", 120},
   {"lightgrey", 252 },
   {"lightpink", 217 },
   {"lightpink1", 217 },
   {"lightpink2", 217 },
   {"lightpink3", 174 },
   {"lightpink4", 95 },
   {"lightsalmon", 216 },
   {"lightsalmon1", 216 },
   {"lightsalmon2", 209 },
   {"lightsalmon3", 173 },
   {"lightsalmon4", 95 },
   {"lightseagreen", 37 },
   {"lightskyblue", 117 },
   {"lightskyblue1", 153 },
   {"lightskyblue2", 153 },
   {"lightskyblue3", 110 },
   {"lightskyblue4", 66 },
   {"lightslateblue", 99 },
   {"lightslategrey", 102 },
   {"lightsteelblue", 152 },
   {"lightsteelblue1", 189 },
   {"lightsteelblue2", 153 },
   {"lightsteelblue3", 146 },
   {"lightsteelblue4", 66 },
   {"lightyellow", 230 },
   {"lightyellow1", 230 },
   {"lightyellow2", 254 },
   {"lightyellow3", 187 },
   {"lightyellow4", 102 },
   {"limegreen", 77 },
   {"linen", 255 },
   {"magenta", 13 },
   {"magenta1", 13 },
   {"magenta2", 13 },
   {"magenta3", 164 },
   {"magenta4", 90 },
   {"maroon", 131 },
   {"maroon1", 205 },
   {"maroon2", 205 },
   {"maroon3", 162 },
   {"maroon4", 89 },
   {"mediumaquamarine", 79 },
   {"mediumblue", 20 },
   {"mediumorchid", 134 },
   {"mediumorchid1", 171 },
   {"mediumorchid2", 171 },
   {"mediumorchid3", 134 },
   {"mediumorchid4", 96 },
   {"mediumpurple", 98 },
   {"mediumpurple1", 141 },
   {"mediumpurple2", 141 },
   {"mediumpurple3", 98 },
   {"mediumpurple4", 60 },
   {"mediumseagreen", 71 },
   {"mediumslateblue", 99 },
   {"mediumspringgreen", 48 },
   {"mediumturquoise", 80 },
   {"mediumvioletred", 162 },
   {"midnightblue", 4 },
   {"mintcream", 15 },
   {"mistyrose", 224 },
   {"mistyrose1", 224 },
   {"mistyrose2", 224 },
   {"mistyrose3", 181 },
   {"mistyrose4", 8 },
   {"moccasin", 223 },
   {"navajowhite", 223 },
   {"navajowhite1", 223 },
   {"navajowhite2", 223 },
   {"navajowhite3", 180 },
   {"navajowhite4", 101 },
   {"navy", 4 },
   {"navyblue", 4 },
   {"oldlace", 230 },
   {"olivedrab", 64 },
   {"olivedrab1", 155 },
   {"olivedrab2", 155 },
   {"olivedrab3", 113 },
   {"olivedrab4", 64 },
   {"orange", 214 },
   {"orange1", 214 },
   {"orange2", 208 },
   {"orange3", 172 },
   {"orange4", 94 },
   {"orangered", 202 },
   {"orangered1", 202 },
   {"orangered2", 202 },
   {"orangered3", 166 },
   {"orangered4", 88 },
   {"orchid", 170 },
   {"orchid1", 213 },
   {"orchid2", 212 },
   {"orchid3", 170 },
   {"orchid4", 96 },
   {"palegoldenrod", 223 },
   {"palegreen", 120 },
   {"palegreen1", 120 },
   {"palegreen2", 120 },
   {"palegreen3", 114 },
   {"palegreen4", 65 },
   {"paleturquoise", 159 },
   {"paleturquoise1", 159 },
   {"paleturquoise2", 159 },
   {"paleturquoise3", 116 },
   {"paleturquoise4", 66 },
   {"palevioletred", 168 },
   {"palevioletred1", 211 },
   {"palevioletred2", 211 },
   {"palevioletred3", 168 },
   {"palevioletred4", 95 },
   {"papayawhip", 230 },
   {"peachpuff", 223 },
   {"peachpuff1", 223 },
   {"peachpuff2", 223 },
   {"peachpuff3", 180 },
   {"peachpuff4", 101 },
   {"peru", 173},
   {"pink", 218 },
   {"pink1", 218 },
   {"pink2", 217 },
   {"pink3", 175 },
   {"pink4", 95 },
   {"plum", 182 },
   {"plum1", 219},
   {"plum2", 183},
   {"plum3", 176},
   {"plum4", 96 },
   {"powderblue", 152 },
   {"purple", 129 },
   {"purple1", 99 },
   {"purple2", 93 },
   {"purple3", 92 },
   {"purple4", 54 },
   {"red", 9 },
   {"red1", 9 },
   {"red2", 9 },
   {"red3", 160 },
   {"red4", 88 },
   {"rosybrown", 138 },
   {"rosybrown1", 217 },
   {"rosybrown2", 217 },
   {"rosybrown3", 174 },
   {"rosybrown4", 95 },
   {"royalblue", 62 },
   {"royalblue1", 69 },
   {"royalblue2", 63 },
   {"royalblue3", 62 },
   {"royalblue4", 24 },
   {"saddlebrown", 94 },
   {"salmon", 209 },
   {"salmon1", 209 },
   {"salmon2", 209 },
   {"salmon3", 167 },
   {"salmon4", 95 },
   {"sandybrown", 215 },
   {"seagreen", 29 },
   {"seagreen1", 85 },
   {"seagreen2", 84 },
   {"seagreen3", 78 },
   {"seagreen4", 29 },
   {"seashell", 255 },
   {"seashell1", 255 },
   {"seashell2", 254 },
   {"seashell3", 251 },
   {"seashell4", 102 },
   {"sienna", 130 },
   {"sienna1", 209 },
   {"sienna1", 209 },
   {"sienna2", 209 },
   {"sienna3", 167 },
   {"sienna4", 94 },
   {"skyblue", 116 },
   {"skyblue1", 117 },
   {"skyblue2", 111 },
   {"skyblue3", 74 },
   {"skyblue4", 60 },
   {"slateblue", 62 },
   {"slateblue1", 99 },
   {"slateblue2", 99 },
   {"slateblue3", 62 },
   {"slateblue4", 60 },
   {"slategray", 66 },
   {"slategray1", 189 },
   {"slategray2", 153 },
   {"slategray3", 146 },
   {"slategray4", 66 },
   {"slategrey", 66 },
   {"snow", 15 },
   {"snow1", 15 },
   {"snow2", 255 },
   {"snow3", 251 },
   {"snow4", 245 },
   {"springgreen", 48 },
   {"springgreen1", 48 },
   {"springgreen2", 48 },
   {"springgreen3", 41 },
   {"springgreen4", 29 },
   {"steelblue", 67 },
   {"steelblue1", 75 },
   {"steelblue2", 75 },
   {"steelblue3", 68 },
   {"steelblue4", 60 },
   {"tan", 180 },
   {"tan1", 215 },
   {"tan2", 209 },
   {"tan3", 173 },
   {"tan4", 94 },
   {"thistle", 182 },
   {"thistle1", 225 },
   {"thistle2", 254 },
   {"thistle3", 182 },
   {"thistle4", 102 },
   {"tomato", 203 },
   {"tomato1", 203 },
   {"tomato2", 203 },
   {"tomato3", 167 },
   {"tomato4", 94 },
   {"turquoise", 80 },
   {"turquoise1", 14 },
   {"turquoise2", 45 },
   {"turquoise3", 44 },
   {"turquoise4", 30 },
   {"violet", 213 },
   {"violetred", 162 },
   {"violetred1", 204 },
   {"violetred2", 204 },
   {"violetred3", 168 },
   {"violetred4", 89 },
   {"wheat", 223 },
   {"wheat1", 223 },
   {"wheat2", 223 },
   {"wheat3", 180 },
   {"wheat4", 101 },
   {"white", 15 },
   {"whitesmoke", 255 },
   {"xterm0", 0},
   {"xterm1", 1},
   {"xterm2", 2},
   {"xterm3", 3},
   {"xterm4", 4},
   {"xterm5", 5},
   {"xterm6", 6},
   {"xterm7", 7},
   {"xterm8", 8},
   {"xterm9", 9},
   {"xterm10", 10},
   {"xterm11", 11},
   {"xterm12", 12},
   {"xterm13", 13},
   {"xterm14", 14},
   {"xterm15", 15},
   {"xterm16", 16},
   {"xterm17", 17},
   {"xterm18", 18},
   {"xterm19", 19},
   {"xterm20", 20},
   {"xterm21", 21},
   {"xterm22", 22},
   {"xterm23", 23},
   {"xterm24", 24},
   {"xterm25", 25},
   {"xterm26", 26},
   {"xterm27", 27},
   {"xterm28", 28},
   {"xterm29", 29},
   {"xterm30", 30},
   {"xterm31", 31},
   {"xterm32", 32},
   {"xterm33", 33},
   {"xterm34", 34},
   {"xterm35", 35},
   {"xterm36", 36},
   {"xterm37", 37},
   {"xterm38", 38},
   {"xterm39", 39},
   {"xterm40", 40},
   {"xterm41", 41},
   {"xterm42", 42},
   {"xterm43", 43},
   {"xterm44", 44},
   {"xterm45", 45},
   {"xterm46", 46},
   {"xterm47", 47},
   {"xterm48", 48},
   {"xterm49", 49},
   {"xterm50", 50},
   {"xterm51", 51},
   {"xterm52", 52},
   {"xterm53", 53},
   {"xterm54", 54},
   {"xterm55", 55},
   {"xterm56", 56},
   {"xterm57", 57},
   {"xterm58", 58},
   {"xterm59", 59},
   {"xterm60", 60},
   {"xterm61", 61},
   {"xterm62", 62},
   {"xterm63", 63},
   {"xterm64", 64},
   {"xterm65", 65},
   {"xterm66", 66},
   {"xterm67", 67},
   {"xterm68", 68},
   {"xterm69", 69},
   {"xterm70", 70},
   {"xterm71", 71},
   {"xterm72", 72},
   {"xterm73", 73},
   {"xterm74", 74},
   {"xterm75", 75},
   {"xterm76", 76},
   {"xterm77", 77},
   {"xterm78", 78},
   {"xterm79", 79},
   {"xterm80", 80},
   {"xterm81", 81},
   {"xterm82", 82},
   {"xterm83", 83},
   {"xterm84", 84},
   {"xterm85", 85},
   {"xterm86", 86},
   {"xterm87", 87},
   {"xterm88", 88},
   {"xterm89", 89},
   {"xterm90", 90},
   {"xterm91", 91},
   {"xterm92", 92},
   {"xterm93", 93},
   {"xterm94", 94},
   {"xterm95", 95},
   {"xterm96", 96},
   {"xterm97", 97},
   {"xterm98", 98},
   {"xterm99", 99},
   {"xterm100", 100},
   {"xterm101", 101},
   {"xterm102", 102},
   {"xterm103", 103},
   {"xterm104", 104},
   {"xterm105", 105},
   {"xterm106", 106},
   {"xterm107", 107},
   {"xterm108", 108},
   {"xterm109", 109},
   {"xterm110", 110},
   {"xterm111", 111},
   {"xterm112", 112},
   {"xterm113", 113},
   {"xterm114", 114},
   {"xterm115", 115},
   {"xterm116", 116},
   {"xterm117", 117},
   {"xterm118", 118},
   {"xterm119", 119},
   {"xterm120", 120},
   {"xterm121", 121},
   {"xterm122", 122},
   {"xterm123", 123},
   {"xterm124", 124},
   {"xterm125", 125},
   {"xterm126", 126},
   {"xterm127", 127},
   {"xterm128", 128},
   {"xterm129", 129},
   {"xterm130", 130},
   {"xterm131", 131},
   {"xterm132", 132},
   {"xterm133", 133},
   {"xterm134", 134},
   {"xterm135", 135},
   {"xterm136", 136},
   {"xterm137", 137},
   {"xterm138", 138},
   {"xterm139", 139},
   {"xterm140", 140},
   {"xterm141", 141},
   {"xterm142", 142},
   {"xterm143", 143},
   {"xterm144", 144},
   {"xterm145", 145},
   {"xterm146", 146},
   {"xterm147", 147},
   {"xterm148", 148},
   {"xterm149", 149},
   {"xterm150", 150},
   {"xterm151", 151},
   {"xterm152", 152},
   {"xterm153", 153},
   {"xterm154", 154},
   {"xterm155", 155},
   {"xterm156", 156},
   {"xterm157", 157},
   {"xterm158", 158},
   {"xterm159", 159},
   {"xterm160", 160},
   {"xterm161", 161},
   {"xterm162", 162},
   {"xterm163", 163},
   {"xterm164", 164},
   {"xterm165", 165},
   {"xterm166", 166},
   {"xterm167", 167},
   {"xterm168", 168},
   {"xterm169", 169},
   {"xterm170", 170},
   {"xterm171", 171},
   {"xterm172", 172},
   {"xterm173", 173},
   {"xterm174", 174},
   {"xterm175", 175},
   {"xterm176", 176},
   {"xterm177", 177},
   {"xterm178", 178},
   {"xterm179", 179},
   {"xterm180", 180},
   {"xterm181", 181},
   {"xterm182", 182},
   {"xterm183", 183},
   {"xterm184", 184},
   {"xterm185", 185},
   {"xterm186", 186},
   {"xterm187", 187},
   {"xterm188", 188},
   {"xterm189", 189},
   {"xterm190", 190},
   {"xterm191", 191},
   {"xterm192", 192},
   {"xterm193", 193},
   {"xterm194", 194},
   {"xterm195", 195},
   {"xterm196", 196},
   {"xterm197", 197},
   {"xterm198", 198},
   {"xterm199", 199},
   {"xterm200", 200},
   {"xterm201", 201},
   {"xterm202", 202},
   {"xterm203", 203},
   {"xterm204", 204},
   {"xterm205", 205},
   {"xterm206", 206},
   {"xterm207", 207},
   {"xterm208", 208},
   {"xterm209", 209},
   {"xterm210", 210},
   {"xterm211", 211},
   {"xterm212", 212},
   {"xterm213", 213},
   {"xterm214", 214},
   {"xterm215", 215},
   {"xterm216", 216},
   {"xterm217", 217},
   {"xterm218", 218},
   {"xterm219", 219},
   {"xterm220", 220},
   {"xterm221", 221},
   {"xterm222", 222},
   {"xterm223", 223},
   {"xterm224", 224},
   {"xterm225", 225},
   {"xterm226", 226},
   {"xterm227", 227},
   {"xterm228", 228},
   {"xterm229", 229},
   {"xterm230", 230},
   {"xterm231", 231},
   {"xterm232", 232},
   {"xterm233", 233},
   {"xterm234", 234},
   {"xterm235", 235},
   {"xterm236", 236},
   {"xterm237", 237},
   {"xterm238", 238},
   {"xterm239", 239},
   {"xterm240", 240},
   {"xterm241", 241},
   {"xterm242", 242},
   {"xterm243", 243},
   {"xterm244", 244},
   {"xterm245", 245},
   {"xterm246", 246},
   {"xterm247", 247},
   {"xterm248", 248},
   {"xterm249", 249},
   {"xterm250", 250},
   {"xterm251", 251},
   {"xterm252", 252},
   {"xterm253", 253},
   {"xterm254", 254},
   {"xterm255", 255},
   {"yellow", 11 },
   {"yellow1", 11 },
   {"yellow2", 11 },
   {"yellow3", 184 },
   {"yellow4", 100 },
   {"yellowgreen", 113 },
   {NULL, 0},
};

UFUN *ufun_head, *ulfun_head;
extern NAMETAB lock_sw[];
extern NAMETAB access_nametab[];
extern NAMETAB access_nametab2[];

extern char* news_function_group(dbref, dbref, int);
extern void FDECL (quota_dis, (dbref, dbref, int *, int, char *, char **));
extern POWENT * FDECL(find_power, (dbref, char *));
extern POWENT * FDECL(find_depower, (dbref, char *));
extern void FDECL(cf_log_notfound, (dbref player, char *cmd,
            const char *thingname, char *thing));
extern int FDECL(parse_dynhelp, (dbref, dbref, int, char *, char *, char *, char *, int));
extern int FDECL(pstricmp, (char *, char *, int));
#ifdef REALITY_LEVELS
extern RLEVEL FDECL(find_rlevel, (char *));
#endif
extern void local_function(char *, char *, char**, dbref, dbref, char **, int, char**, int);
extern double mktime64(struct tm *);
extern struct tm *gmtime64_r(const double *, struct tm *);
extern struct tm *localtime64_r(const double *, struct tm *);
extern int internal_logstatus(void);
extern char * parse_ansi_name(dbref, char *);
extern int count_extended(char *);
extern int decode_base64(const char *, int, char *, char **, int);
extern int encode_base64(const char *, int, char *, char **);
extern void mail_quota(dbref, char *, int, int *, int *, int *, int *, int *, int *);
extern CMDENT * lookup_command(char *);
extern void mail_read_func(dbref, char *, dbref, char *, int, char *, char **);

/* pom.c nefinitions */
#define PI_MUSH        3.14159265358
#define EPOCH_MUSH     85
#define EPSILONg  279.611371    /* solar ecliptic long at EPOCH */
#define RHOg      282.680403    /* solar ecliptic long of perigee at EPOCH */
#define ECCEN_MUSH     0.01671542    /* solar orbit eccentricity */
#define lzero     18.251907     /* lunar mean long at EPOCH */
#define Pzero     192.917585    /* lunar mean long of perigee at EPOCH */
#define Nzero     55.204723     /* lunar mean long of node at EPOCH */
#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)
#define LEAPYEARS(y1, y2) ((y2-1)/4 - (y1-1)/4) - ((y2-1)/100 - (y1-1)/100) + ((y2-1)/400 - (y1-1)/400)

#ifndef HAS_OPENSSL
static void
safe_sha0(const char *text, size_t len, char *buff, char **bufcx)
{
#ifdef HAS_OPENSSL
  const char *digits = "0123456789abcdef";
  unsigned char hash[SHA_DIGEST_LENGTH];
  int n;

  SHA(text, len, hash);

  for (n = 0; n < SHA_DIGEST_LENGTH; n++) {
     safe_chr(digits[hash[n] >> 4], buff, bufcx);
     safe_chr(digits[hash[n] & 0x0F], buff, bufcx);
  }
#else
  SHS_INFO shsInfo;
  char *tpr_buff, *tprp_buff;
  int reverse_shs = 0;


  if ( mudconf.shs_reverse ) {
     reverse_shs = 1;
  }
  shsInfo.reverse_wanted = (BYTE) reverse_shs;
  shsInit(&shsInfo);
  shsUpdate(&shsInfo, (BYTE *) text, len);
  shsFinal(&shsInfo);
  tprp_buff = tpr_buff = alloc_lbuf("safe_sha0");
  safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%0lx%0lx%0lx%0lx%0lx", shsInfo.digest[0],
	      shsInfo.digest[1], shsInfo.digest[2], shsInfo.digest[3],
	      shsInfo.digest[4]), buff, bufcx);
  free_lbuf(tpr_buff);
#endif
}
#endif

void safer_unufun(int tval)
{
   if ( (tval != -1) && (tval != -2) ) {
       mudstate.evalnum--;
   }
}

int
safer_ufun(dbref player, dbref thing, dbref target, int aflags1, int aflags2)
{
    int tlev, tlev2;
    dbref owner;

    if ( !mudconf.safer_ufun) {
       return -1;
    }
    if ( !Good_chk(target) || !Good_chk(thing) || !Good_chk(player) ) {
       return -1;
    }
    if ( !Inherits(thing) ) {
       return -1;
    }
   
    if ( (aflags1 & AF_UNSAFE) || (aflags2 & AF_UNSAFE) ) {
       return -1;
    }
   
    owner = Owner(thing);

    if ( God(owner) ) {
       tlev = 6;
    } else if ( Immortal(owner) ) {
       tlev = 5;
    } else if ( Wizard(owner) ) {
       tlev = 4;
    } else if ( Admin(owner) ) {
       tlev = 3;
    } else if ( Builder(owner) ) {
       tlev = 2;
    } else if ( Guildmaster(owner) ) {
       tlev = 1;
    } else {
       tlev = 0;
    }

    tlev2 = -1;
    if ( God(player) || Immortal(player) ) {
       tlev2 = -1;
    } else if (Wizard(player)) {
        if (tlev > 4)
            tlev2 = 4;
    } else if (Admin(player)) {
        if (tlev > 3)
           tlev2 = 3;
    } else if (Builder(player)) {
        if (tlev > 2)
           tlev2 = 2;
    } else if (Guildmaster(player)) {
        if (tlev > 1)
           tlev2 = 1;
    } else {
        if ( tlev > 0 )
           tlev2 = 0;
    }
    if (mudstate.evalnum < MAXEVLEVEL) {
       if (tlev2 != -1) {
           mudstate.evalstate[mudstate.evalnum] = tlev2;
           mudstate.evaldb[mudstate.evalnum++] = target;
       }
    } else {
       if ( tlev2 != -1 ) {
          tlev2 = -2;
       }
    }
    return tlev2;
}

static int s_comp(const void *s1, const void *s2)
{
  return strcmp(*(char **) s1, *(char **) s2);
}

static const char *
time_format_2(time_t dt)
{
    register struct tm *delta;
    static char buf[64];


    if (dt < 0)
        dt = 0;

    delta = gmtime(&dt);
    if ((int)dt >= 31556926 ) {
        sprintf(buf, "%dy", (int)dt / 31556926);
    } else if ((int)dt >= 2629743 ) {
        sprintf(buf, "%dM", (int)dt / 2629743); 
    } else if ((int)dt >= 604800) {
        sprintf(buf, "%dw", (int)dt / 604800);
    } else if (delta->tm_yday > 0) {
        sprintf(buf, "%dd", delta->tm_yday);
    } else if (delta->tm_hour > 0) {
        sprintf(buf, "%dh", delta->tm_hour);
    } else if (delta->tm_min > 0) {
        sprintf(buf, "%dm", delta->tm_min);
    } else {
        sprintf(buf, "%ds", delta->tm_sec);
    }
    return(buf);
}


/*
 * dtor --
 *  convert degrees to radians
 */
double
dtor_mush(double deg)
{
   return(deg * PI_MUSH / 180.0);
}

/*
 * adj360 --
 *  adjust value so 0 <= deg <= 360
 */
void
adj360_mush(double *deg)
{
/*
   for (;;)
      if (*deg < 0)
         *deg += 360;
      else if (*deg > 360)
         *deg -= 360;
      else
         break;
*/
   *deg = fmod(*deg,360);
   if ( *deg < 0 )
      *deg += 360;
}
/*
 * potm --
 *  return phase of the moon (copyright at end of file)
 */
double
potm_mush(double days)
{
   double N, Msol, Ec, LambdaSol, l, Mm, Ev, Ac, A3, Mmprime;
   double A4, lprime, V, ldprime, D, Nm;

   N = 360 * days / 365.2422;       /* sec 42 #3 */
   adj360_mush(&N);
   Msol = N + EPSILONg - RHOg;        /* sec 42 #4 */
   adj360_mush(&Msol);
   Ec = 360 / PI_MUSH * ECCEN_MUSH * sin(dtor_mush(Msol));  /* sec 42 #5 */
   LambdaSol = N + Ec + EPSILONg;     /* sec 42 #6 */
   adj360_mush(&LambdaSol);
   l = 13.1763966 * days + lzero;     /* sec 61 #4 */
   adj360_mush(&l);
   Mm = l - (0.1114041 * days) - Pzero;     /* sec 61 #5 */
   adj360_mush(&Mm);
   Nm = Nzero - (0.0529539 * days);     /* sec 61 #6 */
   adj360_mush(&Nm);
   Ev = 1.2739 * sin(dtor_mush((2.0*(l - LambdaSol) - Mm)));/* sec 61 #7 */
   Ac = 0.1858 * sin(dtor_mush(Msol));      /* sec 61 #8 */
   A3 = 0.37 * sin(dtor_mush(Msol));
   Mmprime = Mm + Ev - Ac - A3;       /* sec 61 #9 */
   Ec = 6.2886 * sin(dtor_mush(Mmprime));   /* sec 61 #10 */
   A4 = 0.214 * sin(dtor_mush((2.0 * Mmprime)));    /* sec 61 #11 */
   lprime = l + Ev + Ec - Ac + A4;      /* sec 61 #12 */
   V = 0.6583 * sin(dtor_mush((2.0 * (lprime - LambdaSol))));/* sec 61 #13 */
   ldprime = lprime + V;        /* sec 61 #14 */
   D = ldprime - LambdaSol;       /* sec 63 #2 */

   return(50.0 * (1.0 - cos(dtor_mush(D))));    /* sec 63 #3 */
}


char *
mystrcat(char *dest, const char *src)
{
    if ((strlen(dest) + strlen(src)) > (LBUF_SIZE - 2))
       return dest;
    else
       return strcat(dest, src);
}


/* Trim off leading and trailing spaces if the separator char is a space */

char * trim_space_sep(char *str, char sep)
{
    char *p;

    if (sep != ' ')
       return str;
    while (*str && (*str == ' '))
       str++;
    for (p = str; *p; p++);
    for (p--; *p == ' ' && p > str; p--);
    p++;
    *p = '\0';
    return str;
}

/* Safe atof() function */
double safe_atof(char *input)
{
   static char safe_buff[301];
   char *safe_buffptr;
   int i_vert, i_vert2;

   memset(safe_buff, 0, sizeof(safe_buff));
   safe_buffptr = strchr(input, 'e');
   if ( safe_buffptr == NULL )
      safe_buffptr = strchr(input, 'E');
   if ( safe_buffptr != NULL ) {
      i_vert = i_vert2 = 0;
      i_vert2 = strlen(input) - strlen(safe_buffptr+1);
      if ( i_vert2 > 90 )
         i_vert2 = 90;
      i_vert = atoi(safe_buffptr+1);
      strncpy(safe_buff, input, i_vert2);
      if ( i_vert > 300 ) {
         strcat(safe_buff, "300");
      } else if ( i_vert < -300 ) {
         strcat(safe_buff, "-300");
      } else {
         strncat(safe_buff, safe_buffptr+1, 4);
      }
   } else {
      strncpy(safe_buff, input, 100);
   }
   return(atof(safe_buff));
}
/* Soundex functionality */
/* Borrowed from PENN with permission */
char *upcasestr(char *s)
{
  char *p;

  /* modifies a string in-place to be upper-case */
  for (p = s; p && *p; p++)
     *p = toupper(*p);
  return s;
}

/* Borrowed from PENN with permission */
static char soundex_val[26] =
{
   0, 1, 2, 3, 0, 1, 2, 0, 0,
   2, 2, 4, 5, 5, 0, 1, 2, 6,
   2, 3, 0, 1, 0, 2, 0, 2
};

/* Borrowed from PENN with permission */
static char *
soundex(char *str)
{
   static char tbuf1[LBUF_SIZE*2];
   char *p, *q;

   memset(tbuf1, '\0', sizeof(tbuf1));
   p = tbuf1;
   q = upcasestr(str);

   /* First character is just copied */
   *p = *q++;

   /* Special case for PH->F */
   if ((*p == 'P') && *q && (*q == 'H')) {
      *p = 'F';
      q++;
   }
   p++;

   /* Convert letters to soundex values, squash duplicates */
   while (*q) {
      if (!isalpha((int)*q)) {
      q++;
      continue;
   }
   *p = soundex_val[*q++ - 0x41] + '0';
   if (*p != *(p - 1))
      p++;
   }
   *p = '\0';

   /* Remove zeros */
   p = q = tbuf1;
   while (*q) {
     if (*q != '0')
        *p++ = *q;
     q++;
   }
   *p = '\0';

   /* Pad/truncate to 4 chars */
   if (tbuf1[1] == '\0')
      tbuf1[1] = '0';
   if (tbuf1[2] == '\0')
      tbuf1[2] = '0';
   if (tbuf1[3] == '\0')
      tbuf1[3] = '0';
   tbuf1[4] = '\0';
   return tbuf1;
}

/* next_token: Point at start of next token in string */

char *next_token(char *str, char sep)
{
    while (*str && (*str != sep))
       str++;
    if (!*str)
       return NULL;
    str++;
    if (sep == ' ') {
       while (*str == sep)
           str++;
    }
    return str;
}

/* split_token: Get next token from string as null-term string.  String is
 * destructively modified.
 */

char * split_token(char **sp, char sep)
{
    char *str, *save;

    save = str = *sp;
    if (!str) {
       *sp = NULL;
       return NULL;
    }
    while (*str && (*str != sep))
       str++;
    if (*str) {
       *str++ = '\0';
       if (sep == ' ') {
          while (*str == sep)
             str++;
       }
    } else {
       str = NULL;
    }
    *sp = str;
    return save;
}

dbref
match_thing(dbref player, char *name)
{
    init_match(player, name, NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    return (noisy_match_result());
}

dbref
match_thing_quiet(dbref player, char *name)
{
    init_match(player, name, NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    return (match_result());
}

int
return_bit(dbref player)
{
   if (God(player))
      return(7);
   if (Immortal(player))
      return(6);
   if (Wizard(player))
      return(5);
   if (Admin(player))
      return(4);
   if (Builder(player))
      return(3);
   if (Guildmaster(player))
      return(2);
   if (!(Wanderer(player) || Guest(player)))
      return(1);
   return(0);
}

/* ---------------------------------------------------------------------------
 * List management utilities.
 */

#define ALPHANUM_LIST   1
#define NUMERIC_LIST    2
#define DBREF_LIST      3
#define FLOAT_LIST      4

static int
autodetect_list(char *ptrs[], int nitems)
{
    int sort_type, i;
    char *p;

    sort_type = NUMERIC_LIST;
    for (i = 0; i < nitems; i++) {
       switch (sort_type) {
          case NUMERIC_LIST:
             if (!is_number(ptrs[i])) {

                /* If non-numeric, switch to alphanum sort.
                 * Exception: if this is the first element and
                 * it is a good dbref, switch to a dbref sort.
                 * We're a little looser than the normal
                 * 'good dbref' rules, any number following
                 # the #-sign is accepted.
                 */

                 if (i == 0) {
                    p = ptrs[i];
                    if (*p++ != NUMBER_TOKEN) {
                       return ALPHANUM_LIST;
                    } else if (is_integer(p)) {
                       sort_type = DBREF_LIST;
                    } else {
                       return ALPHANUM_LIST;
                    }
                 } else {
                    return ALPHANUM_LIST;
                 }
              } else if (index(ptrs[i], '.')) {
                 sort_type = FLOAT_LIST;
              }
              break;
           case FLOAT_LIST:
              if (!is_number(ptrs[i])) {
                 sort_type = ALPHANUM_LIST;
                 return ALPHANUM_LIST;
              }
              break;
           case DBREF_LIST:
              p = ptrs[i];
              if (*p++ != NUMBER_TOKEN)
                 return ALPHANUM_LIST;
              if (!is_integer(p))
                 return ALPHANUM_LIST;
              break;
           default:
              return ALPHANUM_LIST;
        }
    }
    return sort_type;
}

static int
get_list_type_basic(char c_char) {
   switch (ToLower((int)c_char)) {
      case 'd':
               return DBREF_LIST;
      case 'n':
               return NUMERIC_LIST;
      case 'f':
               return FLOAT_LIST;
      default:
               return ALPHANUM_LIST;
   }
   
}

static int
get_list_type(char *fargs[], int nfargs, int type_pos,
        char *ptrs[], int nitems)
{
    if (nfargs >= type_pos) {
       switch (ToLower((int)*fargs[type_pos - 1])) {
            case 'd':
               return DBREF_LIST;
            case 'n':
               return NUMERIC_LIST;
            case 'f':
               return FLOAT_LIST;
            case '\0':
               return autodetect_list(ptrs, nitems);
            default:
               return ALPHANUM_LIST;
       }
    }
    return autodetect_list(ptrs, nitems);
}

int list2arr(char *arr[], int maxlen, char *list, char sep)
{
    char *p;
    int i;

    list = trim_space_sep(list, sep);
    p = split_token(&list, sep);
    for (i = 0; p && i < maxlen; i++, p = split_token(&list, sep)) {
       arr[i] = p;
    }
    return i;
}

static void
arr2list(char *arr[], int alen, char *list, char **listcx, char sep)
{
    int i;
    int gotone = 0;

    for (i = 0; i < alen; i++) {
        if( gotone )
          safe_chr(sep, list, listcx);
        safe_str(arr[i], list, listcx);
        gotone = 1;
    }
}

static int
dbnum(char *dbr)
{
    if ((strlen(dbr) < 2) && (*dbr != '#'))
       return 0;
    else
       return atoi(dbr + 1);
}

/* ---------------------------------------------------------------------------
 * nearby_or_control: Check if player is near or controls thing
 */

int
nearby_or_control(dbref player, dbref thing)
{
    if (!Good_obj(player) || !Good_obj(thing))
       return 0;
    if (Controls(player, thing))
       return 1;
    if (!nearby(player, thing))
       return 0;
    return 1;
}

int
nearby_exam_or_control(dbref player, dbref thing)
{
    if (!Good_obj(player) || !Good_obj(thing))
       return 0;
    if (Controls(player, thing))
       return 1;
    if (!nearby(player, thing) && !Examinable(player, thing))
       return 0;
    return 1;
}
/* ---------------------------------------------------------------------------
 * ival: copy an int value into a buffer
 */

static void
ival(char *buff, char **bufcx, int result)
{
  static char tempbuff[LBUF_SIZE/2];

  sprintf(tempbuff, "%d", result);
  safe_str(tempbuff, buff, bufcx);
}

static void
uival(char *buff, char **bufcx, unsigned int result)
{
  static char tempbuffu[LBUF_SIZE/2];

  sprintf(tempbuffu, "%u", result);
  safe_str(tempbuffu, buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * dbval: copy a dbnum into a buffer
 */

static void
dbval(char *buff, char **bufcx, dbref result)
{
  static char tempbuff[MBUF_SIZE];

  sprintf(tempbuff, "#%d", (int)result);
  safe_str(tempbuff, buff, bufcx);
}


/* ---------------------------------------------------------------------------
 * fval: copy the floating point value into a buffer and make it presentable
 */

static void
fval(char *buff, char **bufcx, double result)
{
    char *p;
    static char tempbuff[LBUF_SIZE/2];

    sprintf(tempbuff, "%.*f", mudconf.float_precision, result);  /* get double val into buffer */
    p = (char *)rindex(tempbuff, '0');
    if (p == NULL) {    /* remove useless trailing 0's */
       safe_str(tempbuff, buff, bufcx);
       return;
    } else if (*(p + 1) == '\0') {
       while (*p == '0') {
          *p-- = '\0';
       }
    }
    p = (char *) rindex(tempbuff, '.'); /* take care of dangling '.' */
    if (*(p + 1) == '\0')
       *p = '\0';
    safe_str(tempbuff, buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * fn_range_check: Check # of args to a function with an optional argument
 * for validity.
 */

int fn_range_check(const char *fname, int nfargs, int minargs,
       int maxargs, char *result, char **resultcx)
{
    if ((nfargs >= minargs) && (nfargs <= maxargs))
       return 1;

    if (maxargs == (minargs + 1)) {
        safe_str((char*)"#-1 FUNCTION (", result, resultcx);
        safe_str((char*)fname, result, resultcx);
        safe_str((char*)") EXPECTS ", result, resultcx);
        ival(result, resultcx, minargs);
        safe_str((char*)" OR ", result, resultcx);
        ival(result, resultcx, maxargs);
        safe_str((char*)" ARGUMENTS [RECEIVED ", result, resultcx);
        ival(result, resultcx, nfargs);
        safe_chr(']', result, resultcx);
    } else {
        safe_str((char*)"#-1 FUNCTION (", result, resultcx);
        safe_str((char*)fname, result, resultcx);
        safe_str((char*)") EXPECTS BETWEEN ", result, resultcx);
        ival(result, resultcx, minargs);
        safe_str((char*)" AND ", result, resultcx);
        ival(result, resultcx, maxargs);
        safe_str((char*)" ARGUMENTS [RECEIVED ", result, resultcx);
        ival(result, resultcx, nfargs);
        safe_chr(']', result, resultcx);
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * delim_check: obtain delimiter
 */

int delim_check(char *fargs[], int nfargs, int sep_arg, char *sep,
    char *buff, char **bufcx, int eval, dbref player, dbref cause,
                dbref caller, char *cargs[], int ncargs)
{
    char *tstr;
    int tlen;

    if (nfargs >= sep_arg) {
       tlen = strlen(fargs[sep_arg - 1]);
       if (tlen <= 1)
          eval = 0;
       if (eval) {
          tstr = exec(player, cause, caller, EV_EVAL | EV_FCHECK,
          fargs[sep_arg - 1], cargs, ncargs);
          tlen = strlen(tstr);
          *sep = *tstr;
          free_lbuf(tstr);
       }
       if (tlen == 0) {
          *sep = ' ';
       } else if (tlen != 1) {
          safe_str("#-1 SEPARATOR MUST BE ONE CHARACTER", buff, bufcx);
          return 0;
       } else if (!eval) {
          *sep = *fargs[sep_arg - 1];
       }
    } else {
       *sep = ' ';
    }
    return 1;
}

/* ---------------------------------------------------------------------------
 * do_reverse, fun_reverse, fun_revwords: Reverse things.
 */

static void
do_reverse(char *from, char *to, char **tocx)
{
    char *fp;

    fp = from + strlen(from) - 1;
    while( fp >= from )
       safe_chr( *fp--, to, tocx);
}

FUNCTION(fun_singletime)
{
   int i_time;

   i_time = atoi(fargs[0]);
   safe_str((char *)time_format_2(i_time), buff, bufcx);
}

FUNCTION(fun_safebuff)
{
  char sep, *s_ptr, *s_tmp, *noansi_buff;
#ifdef ZENTY_ANSI
  int i_cntr;
#endif

  if (!fn_range_check("SAFEBUFF", nfargs, 1, 2, buff, bufcx)) {
    return;
  }
  if ( !*fargs[0] )
     return;


#ifdef ZENTY_ANSI
  if ( (strlen(fargs[0]) < (LBUF_SIZE - 24)) ) {
#else
  if ( (strlen(fargs[0]) < (LBUF_SIZE - 2)) ) {
#endif
    safe_str(fargs[0], buff, bufcx);
    return;
  }

  if ( (nfargs > 1) && (*fargs[1]) )
    sep = *fargs[1];
  else
    sep = ' ';

  noansi_buff = alloc_lbuf("fun_safebuff");
  strcpy(noansi_buff, strip_all_special(fargs[0]));
  s_tmp = noansi_buff;
  s_ptr = s_tmp + strlen(s_tmp) - 1;

#ifdef ZENTY_ANSI
  i_cntr = 0;
  while( ((s_ptr >= s_tmp) && (*s_ptr != sep)) || i_cntr < 24 ) {
     s_ptr--;
     i_cntr++;
  }
#else
  while( (s_ptr >= s_tmp) && (*s_ptr != sep) ) {
     s_ptr--;
  }
#endif
  if ( s_ptr > s_tmp ) {
     *s_ptr = '\0';
     *(s_ptr++) = '\0';
  }
  safe_str(s_tmp, buff, bufcx);
  free_lbuf(noansi_buff);
}

/* Borrowed from PENN with permission */
FUNCTION(fun_soundex)
{
   char *result;

   result = exec(player, cause, caller,
          EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0], cargs, ncargs);
  /* Returns the soundex code for a word. This 4-letter code is:
   * 1. The first letter of the word (exception: ph -> f)
   * 2. Replace each letter with a numeric code from the soundex table
   * 3. Remove consecutive numbers that are the same
   * 4. Remove 0's
   * 5. Truncate to 4 characters or pad with 0's.
   * It's actually a bit messier than that to make it faster.
   */
  if (!result || !*result || !isalpha((int)*result) || strchr(result, ' ')) {
    safe_str("#-1 FUNCTION (SOUNDEX) REQUIRES A SINGLE WORD ARGUMENT", buff, bufcx);
    free_lbuf(result);
    return;
  }
  safe_str(soundex(result), buff, bufcx);
  free_lbuf(result);
  return;
}

/* Borrowed from PENN with permission */
FUNCTION(fun_soundlike)
{
  char tbuf1[5], *result, *result2;
  /* Return 1 if the two arguments have the same soundex.
   * This can be optimized to go character-by-character, but
   * I deem the modularity to be more important. So there.
   */
  result = exec(player, cause, caller,
                EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0], cargs, ncargs);
  result2= exec(player, cause, caller,
                EV_STRIP | EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
  if (!*result || !*result2 || !isalpha((int)*result) || !isalpha((int)*result2) ||
               strchr(result, ' ') || strchr(result2, ' ')) {
     safe_str("#-1 FUNCTION (SOUNDLIKE) REQUIRES TWO ONE-WORD ARGUMENTS", buff, bufcx);
     free_lbuf(result);
     free_lbuf(result2);
     return;
  }
  /* soundex uses a static buffer, so we need to save it */
  strcpy(tbuf1, soundex(result));
  if (!strcmp(tbuf1, soundex(result2))) {
    safe_str("1", buff, bufcx);
  } else {
    safe_str("0", buff, bufcx);
  }
  free_lbuf(result);
  free_lbuf(result2);
  return;
}


/* This function also borrowed from Penn */
static void
do_spellnum(char *num, unsigned int len, char *buff, char **bufcx)
{
   static const char *bigones[] = {
      "", "thousand", "million", "billion", "trillion", NULL
   };
   static const char *singles[] = {
      "", "one", "two", "three", "four",
      "five", "six", "seven", "eight", "nine", NULL
   };
   static const char *special[] = {
      "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen",
      "sixteen", "seventeen", "eighteen", "nineteen", NULL
   };
   static const char *tens[] = {
      "", " ", "twenty", "thirty", "forty",
      "fifty", "sixty", "seventy", "eighty", "ninety", NULL
   };
   unsigned int ui0, ui1, ui2;
   char *tpr_buff, *tprp_buff;
   int pos, started;

   /* Something bad happened.  Reject */
   if ( (len % 3) != 0 )
      return;

   pos = len / 3;
   started = 0;
   tpr_buff = alloc_lbuf("do_spellnum");
   while (pos > 0) {
      pos--;
      ui0 = (unsigned int) (num[0] - '0');
      ui1 = (unsigned int) (num[1] - '0');
      ui2 = (unsigned int) (num[2] - '0');

      if ( ui0 ) {
         if ( started )
            safe_chr(' ', buff, bufcx);
         tprp_buff = tpr_buff;
         safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%.100s %s", singles[ui0], "hundred"), buff, bufcx);
         started = 1;
      }

      if ( ui1 == 1 ) {
         if (started)
            safe_chr(' ', buff, bufcx);
         safe_str((char *)special[ui2], buff, bufcx);
         started = 1;
      } else if ( ui1 || ui2 ) {
         if ( started )
            safe_chr(' ', buff, bufcx);
         if ( ui1 ) {
            safe_str((char *)tens[ui1], buff, bufcx);
            if ( ui2 )
               safe_chr('-', buff, bufcx);
         }
         safe_str((char *)singles[ui2], buff, bufcx);
         started = 1;
      }

      if ( pos && ( ui0 || ui1 || ui2 ) ) {
         if ( started )
            safe_chr(' ', buff, bufcx);
         safe_str((char *)bigones[pos], buff, bufcx);
         started = 1;
      }
      num += 3;
  }
  free_lbuf(tpr_buff);
}


/* fun_spellnum - return the full logical name of a given number.
 * function and code taken from Penn and MUX2 with permission.
 */
FUNCTION(fun_spellnum)
{
   static const char *tail[] = {
      "", "tenth", "hundredth", "thousandth",
      "ten-thousandth", "hundred-thousandth", "millionth",
      "ten-millionth", "hundred-millionth", "billionth",
      "ten-billionth", "hundred-billionth", "trillionth",
      "ten-trillionth", "hundred-trillionth", NULL
   };
   char *num, *pnumber, *pnum1, *pnum2,
        *s_numtoken, *tpr_buff, *tprp_buff;
   unsigned int len, m, minus, dot, len1, len2;


   if ( !fargs[0] || !*fargs[0] ) {
      safe_str("zero", buff, bufcx);
      return;
   }

   pnum1 = NULL;
   pnum2 = NULL;
   len = m = minus = dot = len1 = len2 = 0;
   pnumber = trim_space_sep(fargs[0], ' ');

   /* strip positive/negative sign */
   if ( pnumber[0] == '-' ) {
      minus = 1;
      pnumber++;
   }  else if (pnumber[0] == '+') {
      pnumber++;
   }

   /* remove leading zeros */
   if ( *pnumber ) {
      while (*pnumber && (*pnumber == '0') )
         pnumber++;
   }
   if ( !*pnumber ) {
      safe_str("zero", buff, bufcx);
      return;
   }

   /* If length is insanely large, just abort out */
   if ( (len = strlen(pnumber)) > 100 ) {
      safe_str((char *)"#-1 NOT A VALID NUMBER", buff, bufcx);
      return;
   }
   /* Sanitize pnumber, if not a valid number abort */
   for ( pnum1 = pnumber; *pnum1; pnum1++ ) {
      if ( isdigit(*pnum1) )
         continue;
      if ( *pnum1 == '.' ) {
         dot++;
         continue;
      }
      dot = 10;
      break;
   }
   if ( dot > 1 ) {
      safe_str((char *)"#-1 NOT A VALID NUMBER", buff, bufcx);
      return;
   }

   /* If decimal, break into two segments */
   if ( dot > 0 ) {
      if ( *pnumber == '.' ) {
         pnum1 = NULL;
         pnum2 = pnumber+1;
      } else {
         pnum1 = strtok_r(pnumber, ".", &s_numtoken);
         pnum2 = strtok_r(NULL, ".", &s_numtoken);
      }
   } else {
      pnum1 = pnumber;
      pnum2 = NULL;
   }

   num = alloc_lbuf("fun_spellnum_num");
   memset(num, '\0', LBUF_SIZE);
   if ( pnum1 && *pnum1 ) {
      switch(strlen(pnum1) % 3) {
         case 0: sprintf(num, "%.3900s", pnum1);
                 break;
         case 1: sprintf(num, "00%.3900s", pnum1);
                 break;
         case 2: sprintf(num, "0%.3900s", pnum1);
                 break;
      }
   }
   pnum1 = num;

   len1 = ( ((pnum1 == NULL) || !*pnum1) ? 0 : strlen(pnum1));
   len2 = ( ((pnum2 == NULL) || !*pnum2) ? 0 : strlen(pnum2));

   /* Max number is 999,999,999,999,999.999,999,999,999 */
   if ( (len1 > 15) || (len2 > 14) ) {
      safe_str((char *)"#-1 NOT A VALID NUMBER", buff, bufcx);
      free_lbuf(num);
      return;
   }

   /* before the . */

   if ( minus ) {
      safe_str("negative ", buff, bufcx);
   }
   if ( pnum1 && *pnum1 ) {
      do_spellnum(pnum1, len1, buff, bufcx);
   } else {
      len1 = 0;
   }

   /* after the . */
   if ( len2 > 0 ) {
      /* remove leading zeros */
      while ( *pnum2 && (*pnum2 == '0') )
         pnum2++;

      memset(num, '\0', LBUF_SIZE);
      if ( pnum2 && *pnum2 ) {
         switch(strlen(pnum2) % 3) {
            case 0: sprintf(num, "%.3900s", pnum2);
                    break;
            case 1: sprintf(num, "00%.3900s", pnum2);
                    break;
            case 2: sprintf(num, "0%.3900s", pnum2);
                    break;
         }
      }
      pnumber = num;

      len = strlen(pnumber);

      if ( len1 > 0 )
         safe_str(" and ", buff, bufcx);
      else if ( minus && len )
         safe_str("negative ", buff, bufcx);

      tprp_buff = tpr_buff = alloc_lbuf("fun_spellnum");
      if ( !len ) {
         safe_str(safe_tprintf(tpr_buff, &tprp_buff, "zero %ss", tail[len2]), buff, bufcx);
      } else if ( (len == 3) && (atoi(pnumber) == 1) ) {
         safe_str(safe_tprintf(tpr_buff, &tprp_buff, "one %s", tail[len2]), buff, bufcx);
      } else {
         do_spellnum(pnumber, len, buff, bufcx);
         safe_str(safe_tprintf(tpr_buff, &tprp_buff, " %ss", tail[len2]), buff, bufcx);
      }
      free_lbuf(tpr_buff);
   } else if ( len1 == 0 ) {
      safe_str("zero", buff, bufcx);
   }
   free_lbuf(num);
}


/* ---------------------------------------------------------------------------
 * fun_startsecs: Returns the time that the mush was started as a number of
 * seconds since 1/1/70 0:00.
 */
FUNCTION(fun_startsecs)
{
    ival(buff, bufcx, mudstate.start_time);
}

FUNCTION(fun_rebootsecs)
{
    ival(buff, bufcx, mudstate.reboot_time);
}

FUNCTION(fun_strcat)
{
    int i;

    safe_str(fargs[0], buff, bufcx);
    for (i = 1; i < nfargs; i++) {
        safe_str(fargs[i], buff, bufcx);
    }
}


#ifdef ZENTY_ANSI
int string_count(char* src, int numchars)
{
    int idx, ichr;
    for( idx = 0, ichr = 0; idx < numchars && src[idx]; idx++ )
    {
        if(((src[idx] == '%') || (src[idx] == '\\')) &&
           ((src[idx+1] == '%') || (src[idx+1] == '\\')))
            idx++;

        if ( (src[idx] == '%') && ((src[idx+1] == 'f') && isprint(src[idx+2])) ) {
           idx+=2;
           continue;
        }
        if ( (src[idx] == '%') && ((src[idx+1] == SAFE_CHR)
#ifdef SAFE_CHR2
                               || (src[idx+1] == SAFE_CHR2 )
#endif
#ifdef SAFE_CHR3
                               || (src[idx+1] == SAFE_CHR3 )
#endif
)) {
           if ( isAnsi[(int) src[idx+2]] ) {
              idx+=2;
              continue;
           }
           if ( (src[idx+2] == '0') && (src[idx+3] && ((src[idx+2] == 'X') || (src[idx+2] == 'x'))) ) {
              if ( src[idx+4] && src[idx+5] && isxdigit(src[idx+4]) && isxdigit(src[idx+4]) ) {
                 idx+=5;
                 continue;
              }
           }
       }
       ichr++;
    }
    return ichr;
}
#endif


/* Thorin's version */
#define JUST_LEFT   0
#define JUST_RIGHT  1
#define JUST_JUST   2
#define JUST_CENTER 3
#define JUST_IGNORE 4

struct wrapinfo {
  char *left;
  char *right;
  int just;
  int width;
  int hanging;
  int first_line;
};

void safe_substr( char* src, int numchars, char *buff, char **bufcx )
{
  int idx;

  for( idx = 0; idx < numchars && src[idx]; idx++ ) {
     safe_chr( src[idx], buff, bufcx );
  }
}

void safe_pad( char padch, int numchars, char *buff, char **bufcx )
{
  int idx;

  for( idx = 0; idx < numchars; idx++ ) {
     safe_chr( padch, buff, bufcx );
  }
}
void safe_pad2( char *padch, int numchars, char *buff, char **bufcx )
{
  int idx;
  char *ptr, *ptch;

  if ( padch && *padch )
     ptch = padch;
  else
     ptch = " ";
  ptr = ptch;
  for( idx = 0; idx < numchars; idx++ ) {
    safe_chr( *ptr, buff, bufcx );
    ptr++;
    if ( !*ptr ) {
       ptr = ptch;
    }
  }
}

void wrap_out_ansi( char* src, int numchars, struct wrapinfo *wp,
                    char *buff, char **bufcx, char *space_sep, int keyval,
                    ANSISPLIT *srcarray, ANSISPLIT *buffarray )
{
  int idx;
  int spares = 0;
  int spacecount = 0;
  int gapwidth = 0;
  int justout = 0;
  int prntchrs= 0;
  int left = LBUF_SIZE - (*bufcx - buff) - 1;
  ANSISPLIT *p_sp, *p_bp, *p_sp2;
  int i;
  char *cp;

  if ( left < 0 )
     left = 0;

  p_sp = srcarray;
  p_bp = buffarray + (*bufcx - buff);

  if( !wp->first_line && wp->hanging ) {
     safe_pad2( space_sep, wp->hanging, buff, bufcx );
     for (i = 0; i< wp->hanging; i++) {
        if ( !left || !p_bp )
           break;
        initialize_ansisplitter(p_bp, 1);
        p_bp++;
        left--;
     }
  }
  wp->first_line = 0;

  if( wp->left ) {
     safe_str( wp->left, buff, bufcx );
     cp = wp->left;
     while ( cp && *cp ) { 
        if ( !left || !p_bp )
           break;
        initialize_ansisplitter(p_bp, 1);
        p_bp++;
        cp++;
        left--;
     }
  }

#ifdef ZENTY_ANSI
  prntchrs = string_count(src, numchars);
#else
  if ( keyval )
     prntchrs = strlen(src);
  else
     prntchrs = numchars;
#endif

  switch( wp->just ) {
    case JUST_IGNORE:
      safe_substr(src, numchars, buff, bufcx);
      for ( i = 0; i < numchars; i++ ) {
         if ( !left || !p_bp || !p_sp )
            break;
         clone_ansisplitter(p_bp, p_sp);
         p_bp++;
         p_sp++;
         left--;
      }
      break;
    case JUST_LEFT:
      safe_substr(src, numchars, buff, bufcx);
      for ( i = 0; i < numchars; i++ ) {
         if ( !left || !p_bp || !p_sp )
            break;
         clone_ansisplitter(p_bp, p_sp);
         p_bp++;
         p_sp++;
         left--;
      }
      safe_pad2(space_sep, wp->width - prntchrs, buff, bufcx);
      for (i = 0; i < (wp->width - prntchrs); i++) {
         if ( !left || !p_bp )
            break;
         initialize_ansisplitter(p_bp, 1);
         p_bp++;
         left--;
      }
      break;
    case JUST_RIGHT:
      safe_pad2(space_sep, wp->width - prntchrs, buff, bufcx);
      for (i = 0; i < (wp->width - prntchrs); i++) {
         if ( !left || !p_bp )
            break;
         initialize_ansisplitter(p_bp, 1);
         p_bp++;
         left--;
      }
      safe_substr(src, numchars, buff, bufcx);
      for ( i = 0; i < numchars; i++ ) {
         if ( !left || !p_bp || !p_sp )
            break;
         clone_ansisplitter(p_bp, p_sp);
         p_bp++;
         p_sp++;
         left--;
      }
      break;
    case JUST_CENTER:
      safe_pad2(space_sep, (wp->width - prntchrs) / 2, buff, bufcx);
      for (i = 0; i < ((wp->width - prntchrs) / 2); i++) {
         if ( !left || !p_bp )
            break;
         initialize_ansisplitter(p_bp, 1);
         p_bp++;
         left--;
      }
      safe_substr(src, numchars, buff, bufcx);
      for ( i = 0; i < numchars; i++ ) {
         if ( !left || !p_bp || !p_sp )
            break;
         clone_ansisplitter(p_bp, p_sp);
         p_bp++;
         p_sp++;
         left--;
      }
      /* might need to add one due to roundoff error */
      safe_pad2(space_sep, (wp->width - prntchrs) / 2 +
                    ((wp->width - prntchrs) % 2), buff, bufcx);
      for (i = 0; i < ( ((wp->width - prntchrs) / 2) + ((wp->width - prntchrs) % 2) ); i++) {
         if ( !left || !p_bp )
            break;
         initialize_ansisplitter(p_bp, 1);
         p_bp++;
         left--;
      }
      break;
    case JUST_JUST:
      /* count spaces */
      for( idx = 0; (idx < numchars) && src[idx]; idx++ ) {
        if ( !src[idx] )
           break;
        if( src[idx] == ' ' ) {
          spacecount++;
        }
      }
      /* compute added spaces per original space */
      if( spacecount ) {
        gapwidth = (wp->width - (prntchrs - spacecount)) / spacecount;
        spares = (wp->width - (prntchrs - spacecount)) % spacecount;

        if( gapwidth < 1 ) {
          gapwidth = 1;
          spares = 0;
        }
      }
      else {
        gapwidth = 0;
        spares = 0;
      }
      /* now print out text with widened gaps */
      for( idx = 0; (idx < numchars) && src[idx]; idx++ ) {
        if( src[idx] == ' ' ) {
          safe_pad2(space_sep, gapwidth, buff, bufcx);
          for (i = 0; i < gapwidth; i++) {
             if ( !left || !p_bp )
                break;
             initialize_ansisplitter(p_bp, 1);
             p_bp++;
             left--;
          }
          justout += gapwidth;
          if( spares > 0 ) {
            safe_pad2(space_sep, 1, buff, bufcx);
            if ( !left || p_bp ) {
               initialize_ansisplitter(p_bp, 1);
               p_bp++;
               left--;
            }
            justout++;
            spares--;
          }
        }
        else {
          safe_chr(src[idx], buff, bufcx);
          if ( !left || p_bp ) {
             p_sp2 = srcarray + idx;
             clone_ansisplitter(p_bp, p_sp2);
             p_bp++;
             left--;
          }
          justout++;
        }
      }
      safe_pad2(space_sep, wp->width - justout, buff, bufcx);
      for (i = 0; i < (wp->width - justout); i++) {
         if ( !left || !p_bp )
            break;
         initialize_ansisplitter(p_bp, 1);
         p_bp++;
         left--;
      }
      break;
  }

  if( wp->right ) {
    safe_str( wp->right, buff, bufcx );
    cp = wp->right;
    while ( cp && *cp ) { 
       if ( !left || !p_bp )
          break;
       initialize_ansisplitter(p_bp, 1);
       p_bp++;
       cp++;
       left--;
    }
  }
}

void wrap_out( char* src, int numchars, struct wrapinfo *wp,
               char *buff, char **bufcx, char *space_sep, int keyval )
{
  int idx;
  int spares = 0;
  int spacecount = 0;
  int gapwidth = 0;
  int justout = 0;
  int prntchrs= 0;

  if( !wp->first_line && wp->hanging ) {
     safe_pad2( space_sep, wp->hanging, buff, bufcx );
  }
  wp->first_line = 0;

  if( wp->left ) {
     safe_str( wp->left, buff, bufcx );
  }

#ifdef ZENTY_ANSI
  prntchrs = string_count(src, numchars);
#else
  if ( keyval )
     prntchrs = strlen(src);
  else
     prntchrs = numchars;
#endif

  switch( wp->just ) {
    case JUST_IGNORE:
      safe_substr(src, numchars, buff, bufcx);
      break;
    case JUST_LEFT:
      safe_substr(src, numchars, buff, bufcx);
      safe_pad2(space_sep, wp->width - prntchrs, buff, bufcx);
      break;
    case JUST_RIGHT:
      safe_pad2(space_sep, wp->width - prntchrs, buff, bufcx);
      safe_substr(src, numchars, buff, bufcx);
      break;
    case JUST_CENTER:
      safe_pad2(space_sep, (wp->width - prntchrs) / 2, buff, bufcx);
      safe_substr(src, numchars, buff, bufcx);
      /* might need to add one due to roundoff error */
      safe_pad2(space_sep, (wp->width - prntchrs) / 2 +
                    ((wp->width - prntchrs) % 2), buff, bufcx);
      break;
    case JUST_JUST:
      /* count spaces */
      for( idx = 0; (idx < numchars) && src[idx]; idx++ ) {
        if ( !src[idx] )
           break;
        if( src[idx] == ' ' ) {
          spacecount++;
        }
      }
      /* compute added spaces per original space */
      if( spacecount ) {
        gapwidth = (wp->width - (prntchrs - spacecount)) / spacecount;
        spares = (wp->width - (prntchrs - spacecount)) % spacecount;

        if( gapwidth < 1 ) {
          gapwidth = 1;
          spares = 0;
        }
      }
      else {
        gapwidth = 0;
        spares = 0;
      }
      /* now print out text with widened gaps */
      for( idx = 0; (idx < numchars) && src[idx]; idx++ ) {
        if( src[idx] == ' ' ) {
          safe_pad2(space_sep, gapwidth, buff, bufcx);
          justout += gapwidth;
          if( spares > 0 ) {
            safe_pad2(space_sep, 1, buff, bufcx);
            justout++;
            spares--;
          }
        }
        else {
          safe_chr(src[idx], buff, bufcx);
          justout++;
        }
      }
      safe_pad2(space_sep, wp->width - justout, buff, bufcx);
      break;
  }

  if( wp->right ) {
    safe_str( wp->right, buff, bufcx );
  }
}

void tab_expand( char* dest, char* src )
{
  char *dp;

  dp = dest;
  for( ; *src; src++ ) {
    if( *src == '\t' ) {
      safe_pad( ' ', 8, dest, &dp);
    }
    else {
      safe_chr( *src, dest, &dp);
    }
  }
  *dp = '\0';
}


/* ---------------------------------------------------------------------------
 * fun_while: Evaluate a list until a termination condition is met:
 * while(EVAL_FN,CONDITION_FN,foo|flibble|baz|meep,1,|,-)
 * where EVAL_FN is "[strlen(%0)]" and CONDITION_FN is "[strmatch(%0,baz)]"
 * would result in '3-7-3' being returned.
 * The termination condition is an EXACT not wild match.
 *
 * Function taken from TinyMUSH 3.0
 * Function modified for use with RhostMUSH
 */

FUNCTION(fun_while)
{
    char sep, osep;
    dbref aowner1, thing1, aowner2, thing2;
    int aflags1, aflags2, anum1, anum2, tval;
    int is_same;
    ATTR *ap, *ap2;
    char *atext1, *atext2, *atextbuf, *condbuf, *retval;
    char *objstring, *cp, *str, *dp, *savep, *bb_p;

    svarargs_preamble("WHILE", 6);

    /* If our third arg is null (empty list), don't bother. */

    if (!fargs[2] || !*fargs[2])
        return;

    /* Our first and second args can be <obj>/<attr> or just <attr>.
     * Use them if we can access them, otherwise return an empty string.
     *
     * Note that for user-defined attributes, atr_str() returns a pointer
     * to a static, and that therefore we have to be careful about what
     * we're doing.
     */

    if (parse_attrib(player, fargs[0], &thing1, &anum1)) {
        if ((anum1 == NOTHING) || !Good_obj(thing1) || Recover(thing1) || Going(thing1))
            ap = NULL;
        else
            ap = atr_num(anum1);
    } else {
        thing1 = player;
        ap = atr_str(fargs[0]);
    }
    if (!ap)
        return;
    atext1 = atr_pget(thing1, ap->number, &aowner1, &aflags1);
    if (!atext1) {
        return;
    } else if (!*atext1 || !See_attr(player, thing1, ap, aowner1, aflags1, 0)) {
        free_lbuf(atext1);
        return;
    }

    if (parse_attrib(player, fargs[1], &thing2, &anum2)) {
        if ((anum2 == NOTHING) || !Good_obj(thing2) || Recover(thing2) || Going(thing2))
            ap2 = NULL;
        else
            ap2 = atr_num(anum2);
    } else {
        thing2 = player;
        ap2 = atr_str(fargs[1]);
    }
    if (!ap2) {
        free_lbuf(atext1);      /* we allocated this, remember? */
        return;
    }
    atext2 = atr_pget(thing2, ap2->number, &aowner2, &aflags2);
    if (!atext2) {
        free_lbuf(atext1);
        return;
    } else if (!*atext2 || !See_attr(player, thing2, ap2, aowner2, aflags2, 0)) {
        free_lbuf(atext1);
        free_lbuf(atext2);
        return;
    }

    /* If our evaluation and condition are the same, we can save ourselves
     * some time later.
     */
    if (!strcmp(atext1, atext2))
        is_same = 1;
    else
        is_same = 0;

    /* Process the list one element at a time. */

    cp = trim_space_sep(fargs[2], sep);
    atextbuf = alloc_lbuf("fun_while.eval");
    if (!is_same)
        condbuf = alloc_lbuf("fun_while.cond");
    bb_p = *bufcx;
    while (cp) {
        if (*bufcx != bb_p) {
            safe_chr(osep, buff, bufcx);
        }
        objstring = split_token(&cp, sep);
        strcpy(atextbuf, atext1);
        str = atextbuf;
        savep = *bufcx;
        if ( (mudconf.secure_functions & 2) ) {
           tval = safer_ufun(player, thing1, thing1, (ap ? ap->flags : 0), aflags1);
           if ( tval == -2 ) {
              retval = alloc_lbuf("edefault_buff");
              sprintf(retval ,"#-1 PERMISSION DENIED");
           } else {
              retval = exec(thing1, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            str, &objstring, 1);
           }
           safer_unufun(tval);
        } else {
           tval = safer_ufun(player, thing1, player, (ap ? ap->flags : 0), aflags1);
           if ( tval == -2 ) {
              retval = alloc_lbuf("edefault_buff");
              sprintf(retval ,"#-1 PERMISSION DENIED");
           } else {
              retval = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            str, &objstring, 1);
           }
           safer_unufun(tval);
        }
        safe_str(retval, buff, bufcx);
        free_lbuf(retval);
        if (is_same) {
            if (!strcmp(savep, fargs[3]))
                break;
        } else {
            strcpy(condbuf, atext2);
            dp = str = savep = condbuf;
            if ( (mudconf.secure_functions & 2) ) {
               tval = safer_ufun(player, thing2, thing2, (ap2 ? ap2->flags : 0), aflags2);
               if ( tval == -2 ) {
                  retval = alloc_lbuf("edefault_buff");
                  sprintf(retval ,"#-1 PERMISSION DENIED");
               } else {
                   retval = exec(thing2, cause, caller,
                                 EV_STRIP | EV_FCHECK | EV_EVAL, str, &objstring, 1);
               }
               safer_unufun(tval);
            } else {
               tval = safer_ufun(player, thing2, player, (ap2 ? ap2->flags : 0), aflags2);
               if ( tval == -2 ) {
                  retval = alloc_lbuf("edefault_buff");
                  sprintf(retval ,"#-1 PERMISSION DENIED");
               } else {
                  retval = exec(player, cause, caller,
                                EV_STRIP | EV_FCHECK | EV_EVAL, str, &objstring, 1);
               }
               safer_unufun(tval);
            }
            safe_str(retval, condbuf, &dp);
            free_lbuf(retval);
            if (!strcmp(savep, fargs[3]))
                break;
        }
    }
    free_lbuf(atext1);
    free_lbuf(atext2);
    free_lbuf(atextbuf);
    if (!is_same)
        free_lbuf(condbuf);
}

FUNCTION(fun_writable)
{
   int attrib;
   dbref s_thing, t_thing;
   ATTR *atr;

   s_thing = match_thing_quiet(player, fargs[0]);
   if ( !Good_chk(s_thing) || !Controls(player, s_thing) ) {
      ival(buff, bufcx, 0);
      return;
   }
   if (!parse_attrib(player, fargs[1], &t_thing, &attrib)) {
      ival(buff, bufcx, 0);
      return;
   }
   if ( !Good_chk(t_thing) ) {
      ival(buff, bufcx, 0);
      return;
   }
   if ( attrib == NOTHING ) {
      if ( Controls(s_thing, t_thing) )  {
         if ( !*fargs[1] || (strchr(fargs[1], '/') == NULL) || 
              !ok_attr_name(strchr(fargs[1], '/')+1) ||
              (!Wizard(s_thing) && *(strchr(fargs[1], '/') + 1) == '_') ) {
            ival(buff, bufcx, 0);
         } else {
            ival(buff, bufcx, 1);
         }
      } else {
         ival(buff, bufcx, 0);
      }
      return;
   }
   atr = atr_num(attrib);
   ival(buff, bufcx, Set_attr(s_thing, t_thing, atr, 0));
}

FUNCTION(fun_nslookup)
{
   struct sockaddr_in p_sock, *p_sock2;
   void *v_sock;
   struct addrinfo hints, *res, *p_res;
   char hostname[NI_MAXHOST + 1], ipstr[INET_ADDRSTRLEN + 1], *p_chk;
   int i_validip = 1, i_timechk=5;

   if ( mudstate.heavy_cpu_lockdown == 1 ) {
      safe_str("#-1 FUNCTION HAS BEEN LOCKED DOWN FOR HEAVY CPU USE.", buff, bufcx);
      return;
   }
   if ( !*fargs[0] ) {
      safe_str("#-1 EXPECTS AN IP OR HOSTNAME", buff, bufcx);
      return;
   } 
   if (mudstate.last_cmd_timestamp == mudstate.now) {
       mudstate.heavy_cpu_recurse += 1;
   }

   if ( mudconf.cputimechk < i_timechk )
      i_timechk = mudconf.cputimechk;
   /* insanely dangerous function -- only allow 5 per command */
   if ( mudstate.heavy_cpu_recurse > mudconf.heavy_cpu_max ) {
      mudstate.chkcpu_toggle = 1;
      mudstate.heavy_cpu_recurse = mudconf.heavy_cpu_max + 1;
      safe_str("#-1 HEAVY CPU RECURSION LIMIT EXCEEDED", buff, bufcx);
      return;
   }
/* nslookup should eat itself if it's over 5 seconds on a lookup */
   if ( mudstate.heavy_cpu_tmark2 > (mudstate.heavy_cpu_tmark1 + 5) ) {
      safe_str("#-1 HEAVY CPU LIMIT ON PROTECTED FUNCTION EXCEEDED", buff, bufcx);
      mudstate.heavy_cpu_recurse = mudconf.heavy_cpu_max + 1;
      mudstate.chkcpu_toggle = 1;
      if ( mudstate.heavy_cpu_tmark2 > (mudstate.heavy_cpu_tmark1 + (i_timechk * 3)) ) {
         mudstate.heavy_cpu_lockdown = 1;
      } 
      return;
   }
   memset(hostname, '\0', sizeof(hostname));
   memset(ipstr, '\0', sizeof(ipstr));
   mudstate.heavy_cpu_tmark2 = time(NULL);

   p_chk = fargs[0];
   while ( *p_chk ) {
      if ( !(isdigit(*p_chk) || (*p_chk == '.')) ) {
         i_validip = 0;
         break;
      }
      p_chk++;
   }
   if ( i_validip ) {
      memset((void*)&p_sock, 0 , sizeof(p_sock)); 
      p_sock.sin_family = AF_INET; 
      if ( *fargs[0] && (inet_aton(fargs[0], &(p_sock.sin_addr)) == 0) )  {
         safe_str("#-1 INVALID IP", buff, bufcx);
         return;
      }

      if ( getnameinfo((struct sockaddr*)&p_sock, sizeof(struct sockaddr), hostname, sizeof(hostname) - 1, NULL, 0, NI_NAMEREQD) ) { 
         safe_str("#-1 ERROR RESOLVING IP", buff, bufcx);
      } else {
         safe_str(hostname, buff, bufcx);
      }
   } else {
      memset(&hints, 0, sizeof(hints));
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_family = AF_INET;
      if ((getaddrinfo(fargs[0], NULL, &hints, &res)) != 0) {
         safe_str("#-1 INVALID HOSTNAME", buff, bufcx);
         mudstate.heavy_cpu_tmark2 = time(NULL);
         if ( mudstate.heavy_cpu_tmark2 > (mudstate.heavy_cpu_tmark1 + 5) ) {
            mudstate.chkcpu_toggle = 1;
            if ( mudstate.heavy_cpu_tmark2 > (mudstate.heavy_cpu_tmark1 + (i_timechk * 3)) ) {
               mudstate.heavy_cpu_lockdown = 1;
            } 
         }
         return;
      }
      i_validip = 0;
      for( p_res = res; p_res != NULL; p_res = p_res->ai_next) {
         p_sock2 = (struct sockaddr_in *)p_res->ai_addr;
         v_sock = &(p_sock2->sin_addr);

         inet_ntop(p_res->ai_family, v_sock, ipstr, sizeof(ipstr)-1);
         if ( i_validip ) 
            safe_chr(' ', buff, bufcx);
         safe_str(ipstr, buff, bufcx);
         i_validip = 1;
      }
      freeaddrinfo(res);
   }
   mudstate.heavy_cpu_tmark2 = time(NULL);
   if ( mudstate.heavy_cpu_tmark2 > (mudstate.heavy_cpu_tmark1 + 5) ) {
      mudstate.chkcpu_toggle = 1;
      if ( mudstate.heavy_cpu_tmark2 > (mudstate.heavy_cpu_tmark1 + (i_timechk * 3)) ) {
         mudstate.heavy_cpu_lockdown = 1;
      } 
   }
}

FUNCTION(fun_wrap) /* text, width, just, left text, right text, hanging, type */
{
  struct wrapinfo winfo;
  int buffleft, i_justifylast, i_inansi, i_firstrun;
  char* leftstart;
  char *crp;
  char *pp;
#ifdef ZENTY_ANSI
  int pchr, i_haveansi = 0;
  char *lstspc;
#endif
  char *expandbuff;

  if (!fn_range_check("WRAP", nfargs, 2, 7, buff, bufcx))
    return;

  memset(&winfo, 0, sizeof(winfo));
  winfo.first_line = 1;
  i_justifylast = 0;
  i_inansi = 0;
  i_firstrun = 0;

  winfo.width = atoi( fargs[1] );

  if( winfo.width < 1 ) {
    safe_str( "#-1 WIDTH MUST BE >= 1", buff, bufcx );
    return;
  }

  if( winfo.width > (LBUF_SIZE - 1) ) {
    safe_str( "#-1 WIDTH ARGUMENT OUT OF BOUNDS", buff, bufcx );
    return;
  }
  if( nfargs >= 3 ) {
    switch( toupper(*fargs[2]) ) {
      case 'L':
        winfo.just = JUST_LEFT;
        break;
      case 'R':
        winfo.just = JUST_RIGHT;
        break;
      case 'J':
        winfo.just = JUST_JUST;
        break;
      case 'C':
        winfo.just = JUST_CENTER;
        break;
      case 'I':
        winfo.just = JUST_IGNORE;
        break;
      default:
        safe_str("#-1 INVALID JUSTIFICATION SPECIFIED", buff, bufcx);
        return;
    }
  }
  if( nfargs >= 4 ) {
    winfo.left = fargs[3];
  }
  if( nfargs >= 5 ) {
    winfo.right = fargs[4];
  }
  if( nfargs >= 6 ) {
    winfo.hanging = atoi(fargs[5]);
  }
  if ( (nfargs >= 7) && *fargs[6]) {
    i_justifylast = atoi(fargs[6]);
    if ( i_justifylast < 1 )
       i_justifylast = 0;
  }

  /* setup phase done */

  expandbuff = alloc_lbuf("fun_wrap");
  *expandbuff = '\0';

  tab_expand( expandbuff, fargs[0] );

  buffleft = strlen(expandbuff);

  if( (buffleft < winfo.width) && !strchr(expandbuff, '\r') ) {
      if ( i_justifylast && (winfo.just == JUST_JUST) )
         winfo.just = JUST_LEFT;
      wrap_out( expandbuff, winfo.width, &winfo, buff, bufcx, " ", 1 );
      if ( i_justifylast )
         winfo.just = JUST_JUST;
      buffleft = 0;
  }

  for(leftstart = expandbuff; buffleft > 0; ) {
    crp = strchr(leftstart, '\r');
    if( crp && 
        crp <= leftstart + winfo.width ) { /* split here and start over */
#ifdef ZENTY_ANSI
      if ( i_firstrun )
         safe_str( "\r\n", buff, bufcx );
      wrap_out( leftstart, crp - leftstart, &winfo, buff, bufcx, " ", 0 );
#else
      wrap_out( leftstart, crp - leftstart, &winfo, buff, bufcx, " ", 0 );
      safe_str( "\r\n", buff, bufcx );
      if ( i_firstrun && i_inansi );
#endif
      buffleft -= (crp - leftstart) + 2;
      leftstart = crp + 2;
      i_firstrun = 1;
    } else { /* no \r\n in interesting proximity */
      i_firstrun = 1;
        /* start at leftstart + width and backtrack to find where to split the
           line */
        /* two cases could happen here, either we find a space, then we break
           there, or we hit the front of the buffer and we have to split
           the line at exactly 'width' characters thereby breaking a word */
#ifdef ZENTY_ANSI
          /* instead of going backwards, we go forward... no pain */
          // We want to find the true length in the string, without ansi
          for(lstspc=pp=leftstart,pchr=0;
              (*pp != '\0') && (pchr < winfo.width);pp++) {
              // Skip over the first char of a comment. count the second.
              if(((*pp == '%')|| (*pp == '\\')) &&
                 ((*(pp+1) == '%') || (*(pp+1) == '\\')))
                  continue;

              if( (*pp == '%') && ((*(pp+1) == 'f') && isprint(*(pp+2))) ) {
                 pp+=2;
                 continue;
              }
              // Skip over ansi
              if( (*pp == '%') && ((*(pp+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                               || (*(pp+1) == SAFE_CHR2 )
#endif
#ifdef SAFE_CHR3
                               || (*(pp+1) == SAFE_CHR3 )
#endif
)) {
                 if ( isAnsi[(int) *(pp+2)] ) {
                    i_haveansi=1;
                    i_inansi=1;
                    if ( *(pp+2) == 'n' ) {
                       i_haveansi=0;
                    }
                    pp+=2;
                    continue;
                 }
                 if ( (*(pp+2) == '0') && (*(pp+3) && ((*(pp+3) == 'X') || (*(pp+3) == 'x'))) ) {
                    if ( *(pp+4) && *(pp+5) && isxdigit(*(pp+4)) && isxdigit(*(pp+5)) ) {
                       pp+=5;
                       continue;
                    }
                 }
              }
              if ( (*pp == '%') && (*(pp+1) == '<') && *(pp+2) && *(pp+3) && *(pp+4) &&
                   isdigit(*(pp+2)) && isdigit(*(pp+3)) && isdigit(*(pp+4)) &&
                   (*(pp+5) == '>') ) {
                 
                 pp+=5;
              }
              // This should let us grab any future %c codes.

              pchr++;

              if(*pp == ' ')
                 lstspc=pp;
          }

          if(pchr == 0) {
              if ( i_inansi && !i_haveansi ) {
                 safe_str((char *)SAFE_ANSI_NORMAL, buff, bufcx);
              }
              free_lbuf(expandbuff);
              return;
          }

          if(leftstart != expandbuff)
              safe_str("\r\n", buff, bufcx );

          if((lstspc == leftstart) || (pchr != winfo.width)) {
              if ( i_justifylast && (winfo.just == JUST_JUST) )
                 winfo.just = JUST_LEFT;
              wrap_out(leftstart, pp - leftstart, &winfo, buff, bufcx, " ", 0);
              if ( i_justifylast )
                 winfo.just = JUST_JUST;
              buffleft -= (pp - leftstart);
              leftstart += pp - leftstart;
          } else {
             wrap_out(leftstart, lstspc - leftstart, &winfo, buff, bufcx, " ", 0);
             buffleft -= (lstspc - leftstart + 1);
             leftstart += lstspc - leftstart + 1;
          }
#else
      if( buffleft <= winfo.width ) { /* last line of output */
        if ( i_justifylast && (winfo.just == JUST_JUST) )
           winfo.just = JUST_LEFT;
        wrap_out( leftstart, buffleft, &winfo, buff, bufcx, " ", 0 );
        if ( i_justifylast )
           winfo.just = JUST_JUST;
        leftstart += buffleft;
        buffleft = 0;
      } else {
        for( pp = leftstart + winfo.width; pp > leftstart; pp-- ) {
          if( *pp == ' ' ) { /* got a space, split here */
            break;
          }
        }
        if( pp == leftstart ) { /* we hit the front of the buffer */
          wrap_out( leftstart, winfo.width, &winfo, buff, bufcx, " ", 0 );
          safe_str( "\r\n", buff, bufcx );
          leftstart += winfo.width;
          buffleft -= winfo.width;
        }
        else { /* we hit a space, chop it there */
          wrap_out( leftstart, pp - leftstart, &winfo, buff, bufcx, " ", 0 );
          safe_str( "\r\n", buff, bufcx );
          buffleft -= pp - leftstart + 1;
          leftstart += pp - leftstart + 1; /* eat the space */
        }
      }
#endif
    }
  }
  free_lbuf(expandbuff);
}

/* Display configuration option */
FUNCTION(fun_config)
{
   if ( *fargs[0] ) {
      cf_display(player, fargs[0], 0, buff, bufcx);
   } else {
      cf_display(player, fargs[0], 1, buff, bufcx);
   }
}


FUNCTION(fun_columns)
{
  struct wrapinfo winfo;
  int buffleft, pos, maxw, wtype, cut, cut2, rnorem, bufstr;
  int count, ncols, remorig, rows, x, y, z, scount, bufcols;
  char *leftstart, *pp, *holdbuff, *hbpt, *pt2, *between;
  char delim, *spacer_sep, *string;
  ANSISPLIT outsplit[LBUF_SIZE], outsplit2[LBUF_SIZE], *p_bp, *p_sp, *p_leftstart; 

  if (!fn_range_check("COLUMNS", nfargs, 3, 13, buff, bufcx))
    return;


  count = 0;
  delim = '\0';
  bufcols = bufstr = 0;
  x = 0;
  memset(&winfo, 0, sizeof(winfo));
  winfo.first_line = 1;

  maxw = atoi( fargs[1] );

  if( maxw < 1 ) {
     safe_str( "#-1 WIDTH MUST BE >= 1", buff, bufcx );
     return;
  }

  if( maxw > (LBUF_SIZE - 1) ) {
     safe_str( "#-1 WIDTH ARGUMENT OUT OF BOUNDS", buff, bufcx );
     return;
  }
  ncols = atoi(fargs[2]);
  if ((ncols < 1) || (ncols > (LBUF_SIZE-1))) {
     safe_str( "#-1 COLUMNS ARGUMENT OUT OF BOUNDS", buff, bufcx );
     return;
  }
  if( nfargs >= 4 ) {
     switch( toupper(*fargs[3]) ) {
        case 'L': /* Left Justification */
           winfo.just = JUST_LEFT;
           break;
        case 'R': /* Right Justification */
           winfo.just = JUST_RIGHT;
           break;
        case 'J': /* Spread Justification */
           winfo.just = JUST_JUST;
           break;
        case 'C': /* Center Justification */
           winfo.just = JUST_CENTER;
           break;
        default:
           safe_str("#-1 INVALID JUSTIFICATION SPECIFIED", buff, bufcx);
           return;
           break;
     }
  }
  if (nfargs >= 5) {
     cut = atoi(fargs[4]);
     if (cut != 1)
        cut = 0;
  } else {
     cut = 0;
  }
  if (nfargs >= 6) {
     cut2 = atoi(fargs[5]);
     if (cut2 != 1)
        cut2 = 0;
  } else {
     cut2 = 0;
  }
  if (nfargs >= 10) {
     wtype = atoi(fargs[9]);
     if (wtype != 1)
        wtype = 0;
  } else {
     wtype = 0;
  }
  if (nfargs >= 11) {
     delim = *fargs[10];
  }
  spacer_sep = (char *)" ";
  if (nfargs >= 12) {
     if ( strlen(fargs[11]) > 0 ) {
        spacer_sep = (char *)strip_returntab(fargs[11],3);
     }
  }

  initialize_ansisplitter(outsplit, LBUF_SIZE);
  initialize_ansisplitter(outsplit2, LBUF_SIZE);
  string = alloc_lbuf("fun_columns_String");
  memset(string, '\0', LBUF_SIZE);
  split_ansi(strip_ansi(fargs[0]), string, outsplit);

  /* setup phase done */

  holdbuff = alloc_lbuf("fun_columns");
  memset(holdbuff, '\0', LBUF_SIZE);
  hbpt = holdbuff;

  if (cut) {
     winfo.width = 0;
     for( buffleft = strlen(string), leftstart = string; buffleft > 0; ) {
        pp = leftstart;
        if (delim) {
           while (*pp && (*pp == delim))
              pp++;
        } else {
           while (*pp && isspace((int)*pp))
              pp++;
        }
        if (!*pp)
           break;
        leftstart = pp++;
        if (delim) {
           while (*pp && (*pp != delim) && ((pp - leftstart) < maxw))
              pp++;
        } else {
           while (*pp && !isspace((int)*pp) && ((pp - leftstart) < maxw))
              pp++;
        }
        if ((pp-leftstart) > winfo.width)
           winfo.width = pp-leftstart;
        if (cut2) {
           if (delim) {
              while (*pp && (*pp != delim))
                 pp++;
           } else {
              while (*pp && !isspace((int)*pp))
                 pp++;
           }
        }
        buffleft -= (pp - leftstart);
        leftstart = pp;
     }
     scount = (LBUF_SIZE - 2) / (winfo.width+1);
     if ((LBUF_SIZE - 2) % (winfo.width+1))
        scount++;
  } else {
     winfo.width = maxw;
     scount = (LBUF_SIZE - 2) / (winfo.width+1);
     if ((LBUF_SIZE - 2) % (winfo.width+1))
        scount++;
  }
  for( buffleft = strlen(string), leftstart = string, p_leftstart = outsplit; buffleft > 0; ) {
     pp = leftstart;
     p_sp = p_leftstart;
     if (delim) {
        while (*pp && (*pp == delim)) {
           pp++;
           p_sp++;
        }
     } else {
        while (*pp && isspace((int)*pp)) {
           pp++;
           p_sp++;
        }
     }
     if (!*pp)
        break;
     leftstart = pp++;
     p_leftstart = p_sp++;
     if (delim) {
        while (*pp && (*pp != delim) && ((pp - leftstart) < maxw)) {
           pp++;
           p_sp++;
        }
     } else {
        while (*pp && !isspace((int)*pp) && ((pp - leftstart) < maxw)) {
           pp++;
           p_sp++;
        }
     }
     count++;
     wrap_out_ansi(leftstart, pp - leftstart, &winfo, holdbuff, &hbpt, spacer_sep, 0,
                   p_leftstart, outsplit2);
  
/*   wrap_out( leftstart, pp - leftstart, &winfo, holdbuff, &hbpt, spacer_sep, 0 ); */
     if (cut2) {
        if (delim) {
           while (*pp && (*pp != delim)) {
              pp++;
              p_sp++;
           }
        } else {
           while (*pp && !isspace((int)*pp)) {
              pp++;
              p_sp++;
           }
        }
     }
     buffleft -= (pp - leftstart);
     leftstart = pp;
     p_leftstart = p_sp;
     if (*pp) {
        safe_str("\n", holdbuff, &hbpt);
        p_bp = outsplit2 + (hbpt - holdbuff);
        initialize_ansisplitter(p_bp, 1);
     }
  }
  if (count > scount)
     count = scount;
  if( nfargs >= 7 ) {
     winfo.left = fargs[6];
  }
  if (nfargs >= 8) {
     between = fargs[7];
  } else {
     between = NULL;
  }
  if( nfargs >= 9 ) {
     winfo.right = fargs[8];
  }
  free_lbuf(string);
  string = rebuild_ansi(holdbuff, outsplit2);
  if (wtype) {
     hbpt = string;
     while (count > 0) {
        if (winfo.left) {
#ifdef ZENTY_ANSI
           safe_chr('%', buff, bufcx);
           safe_chr(SAFE_CHR, buff, bufcx);
           safe_chr('n', buff, bufcx);
#endif
           safe_str(winfo.left, buff, bufcx);
        }
        for (x = 0; x < ncols; x++) {
           count--;
           pt2 = strchr(hbpt,'\n');
           if (pt2)
              *pt2 = '\0';
           safe_str(hbpt, buff, bufcx);
           if (pt2)
              hbpt = pt2+1;
           if (!count)
              break;
           if (x < (ncols - 1)) {
              if (between) {
#ifdef ZENTY_ANSI
                 safe_chr('%', buff, bufcx);
                 safe_chr(SAFE_CHR, buff, bufcx);
                 safe_chr('n', buff, bufcx);
#endif
                 safe_str(between, buff, bufcx);
              }
           }
        }
        bufcols = x + 1;
        if ( bufcols > ncols )
           bufcols = ncols;
        bufstr = 0;
        if ( nfargs >= 13 )
           bufstr = atoi(fargs[12]);
        if ( (bufstr > 0) && (bufcols > 0) ) {
           while ( bufcols < ncols ) {
              if ( bufcols < ncols ) {
                 if ( *between ) {
#ifdef ZENTY_ANSI
                    safe_chr('%', buff, bufcx);
                    safe_chr(SAFE_CHR, buff, bufcx);
                    safe_chr('n', buff, bufcx);
#endif
                    safe_str(between, buff, bufcx);
                 }
                 safe_pad2(spacer_sep, winfo.width, buff, bufcx);
              }
              bufcols++;
           }
        }
        if (winfo.right) {
#ifdef ZENTY_ANSI
           safe_chr('%', buff, bufcx);
           safe_chr(SAFE_CHR, buff, bufcx);
           safe_chr('n', buff, bufcx);
#endif
           safe_str(winfo.right, buff, bufcx);
        }
        if (count)
           safe_str("\r\n", buff, bufcx);
     }
  } else {
     rows = count / ncols;
     rnorem = rows;
     remorig = count % ncols;
     if (remorig)
        rows++;
     for (x = 0; x < rows; x++) {
        if (winfo.left) {
           safe_str(winfo.left, buff, bufcx);
        }
        for (y = 0; y < ncols; y++) {
           pos = x;
           for (z = 0; z < y; z++) {
              pos += rnorem;
              if ((remorig - z) > 0)
                 pos++;
           }
           hbpt = string;
           for (z = 0; z < pos; z++) {
              pt2 = strchr(hbpt,'\n');
              if ( pt2 ) 
                 hbpt = pt2+1;
              else
                 hbpt = string + LBUF_SIZE - 1; 
           }
           pt2 = strchr(hbpt,'\n');
           if (pt2)
              *pt2 = '\0';
           if ( *hbpt && (strlen(hbpt) >= (winfo.width-1))) {
              safe_str(hbpt, buff, bufcx);
           } else {
              safe_pad2(spacer_sep, winfo.width, buff, bufcx);
           }
           if (pt2)
              *pt2 = '\n';
           if ((x == (rows - 1)) && remorig) {
              if ((remorig - y) == 1) {
                 break;
              } else if (between) {
                 safe_str(between, buff, bufcx);
              }
           } else if (y < (ncols - 1)) {
              if (between)
                 safe_str(between, buff, bufcx);
           }
        }
        bufcols = y + 1;
        if ( bufcols > ncols )
           bufcols = ncols;
        bufstr = 0;
        if ( nfargs >= 13 )
           bufstr = atoi(fargs[12]);
        if ( (bufstr > 0) && (bufcols > 0) ) {
           while ( bufcols < ncols ) {
              if ( bufcols < ncols ) {
                 if ( *between )
                    safe_str(between, buff, bufcx);
                 safe_pad2(spacer_sep, winfo.width, buff, bufcx);
              }
              bufcols++;
           }
        }
        if (winfo.right)
           safe_str(winfo.right, buff, bufcx);
        if (x < (rows - 1))
           safe_str("\r\n", buff, bufcx);
     }
  }
  free_lbuf(holdbuff);
  free_lbuf(string);
}

FUNCTION(fun_wrapcolumns)
{
  struct wrapinfo winfo;
  int buffleft, pos, wtype, cut, cut2, maxw, scount, bufstr, bufcols;
  int count, ncols, remorig, rnorem, rows, x, y, z;
  char *leftstart, *crp, *pp, *expandbuff, *string;
  char *holdbuff, *hbpt, *pt2, *between;
  ANSISPLIT outsplit[LBUF_SIZE], outsplit2[LBUF_SIZE], *p_bp, *p_sp, *p_leftstart; 

  if (!fn_range_check("WRAPCOLUMNS", nfargs, 3, 11, buff, bufcx))
     return;

  count = 0;
  memset(&winfo, 0, sizeof(winfo));
  winfo.first_line = 1;
  bufcols = bufstr = 0;

  maxw = atoi( fargs[1] );

  if( maxw < 1 ) {
     safe_str( "#-1 WIDTH MUST BE >= 1", buff, bufcx );
     return;
  }

  if( maxw > (LBUF_SIZE - 1) ) {
     safe_str( "#-1 WIDTH ARGUMENT OUT OF BOUNDS", buff, bufcx );
     return;
  }
  ncols = atoi(fargs[2]);
  if ((ncols < 1) || (ncols > (LBUF_SIZE-1))) {
     safe_str( "#-1 COLUMNS ARGUMENT OUT OF BOUNDS", buff, bufcx );
     return;
  }
  if( nfargs >= 4 ) {
     switch( toupper(*fargs[3]) ) {
        case 'L':
           winfo.just = JUST_LEFT;
           break;
        case 'R':
           winfo.just = JUST_RIGHT;
           break;
        case 'J':
           winfo.just = JUST_JUST;
           break;
        case 'C':
           winfo.just = JUST_CENTER;
           break;
        default:
           safe_str("#-1 INVALID JUSTIFICATION SPECIFIED", buff, bufcx);
           return;
     }
  }
  if (nfargs >= 5) {
     cut = atoi(fargs[4]);
     if (cut != 1)
        cut = 0;
  } else
     cut = 0;
  if (nfargs >= 6) {
     cut2 = atoi(fargs[5]);
     if (cut2 != 1)
        cut2 = 0;
  } else
     cut2 = 0;
  if (nfargs >= 10) {
     wtype = atoi(fargs[9]);
     if (wtype != 1)
        wtype = 0;
  } else
     wtype = 0;

  /* setup phase done */

  initialize_ansisplitter(outsplit, LBUF_SIZE);
  initialize_ansisplitter(outsplit2, LBUF_SIZE);

  string = alloc_lbuf("fun_wrapcolumns");
  memset(string, '\0', LBUF_SIZE);
  holdbuff = alloc_lbuf("fun_wrapcolumns");
  memset(holdbuff, '\0', LBUF_SIZE);
  hbpt = holdbuff;

  tab_expand( string, strip_ansi(fargs[0]) );
  expandbuff = alloc_lbuf("fun_wrapcolumns_String");
  memset(expandbuff, '\0', LBUF_SIZE);
  split_ansi(string, expandbuff, outsplit);
  free_lbuf(string);
  
  if (cut) {
     winfo.width = 0;
     for( buffleft = strlen(expandbuff), leftstart = expandbuff; buffleft > 0; ) {
        crp = strchr(leftstart, '\r');
        if( crp && crp <= leftstart + maxw ) { /* split here and start over */
           if ((crp - leftstart) > winfo.width)
              winfo.width = crp - leftstart;
           buffleft -= (crp - leftstart) + 2;
           leftstart = crp + 2;
        } else { /* no \r\n in interesting proximity */
           if( buffleft <= maxw ) { /* last line of output */
              if (buffleft > winfo.width)
                 winfo.width = buffleft;
              leftstart += buffleft;
              buffleft = 0;
           } else {
              /* start at leftstart + width and backtrack to find where to split the line
                 two cases could happen here, either we find a space, then we break
                 there, or we hit the front of the buffer and we have to split
                 the line at exactly 'width' characters thereby breaking a word */
              for( pp = leftstart + maxw; pp > leftstart; pp-- ) {
                 if( *pp == ' ' ) { /* got a space, split here */
                    break;
                 }
              }
              if( pp == leftstart ) { /* we hit the front of the buffer */
                 winfo.width = maxw;
                 if (cut2) {
                    pp = leftstart + maxw;
                    while (*pp && !isspace((int)*pp))
                       pp++;
                    if (*pp)
                    pp++;
                    buffleft -= pp - leftstart;
                    leftstart += pp - leftstart;
                 } else {
                    leftstart += maxw;
                    buffleft -= maxw;
                 }
              } else { /* we hit a space, chop it there */
                 if ((pp - leftstart) > winfo.width)
                    winfo.width = pp - leftstart;
                    buffleft -= pp - leftstart + 1;
                    leftstart += pp - leftstart + 1; /* eat the space */
              }
           }
        }
     }
     scount = (LBUF_SIZE - 2) / (winfo.width + 1);
     if ((LBUF_SIZE - 2) % (winfo.width+1))
        scount++;
  } else {
     winfo.width = maxw;
     scount = (LBUF_SIZE - 2) / (winfo.width + 1);
     if ((LBUF_SIZE - 2) % (winfo.width+1))
        scount++;
  }
  for( buffleft = strlen(expandbuff), leftstart = expandbuff, p_leftstart = outsplit; buffleft > 0; ) {
     crp = strchr(leftstart, '\r');
     if( crp && crp <= leftstart + maxw ) { /* split here and start over */
        wrap_out_ansi(leftstart, crp - leftstart, &winfo, holdbuff, &hbpt, " ", 0,
                      p_leftstart, outsplit2);
        count++;
        safe_str( "\r\n", holdbuff, &hbpt );
        p_bp = outsplit2 + (hbpt - holdbuff);
        initialize_ansisplitter(p_bp, 2);
        buffleft -= (crp - leftstart) + 2;
        leftstart = crp + 2;
        p_leftstart = p_leftstart + (crp - leftstart + 2);
     } else { /* no \r\n in interesting proximity */
        if( buffleft <= maxw ) { /* last line of output */
           wrap_out_ansi(leftstart, buffleft, &winfo, holdbuff, &hbpt, " ", 0,
                         p_leftstart, outsplit2);
           count++;
           leftstart += buffleft;
           p_leftstart += buffleft;
           buffleft = 0;
        } else {
           /* start at leftstart + width and backtrack to find where to split the line
              two cases could happen here, either we find a space, then we break
              there, or we hit the front of the buffer and we have to split
              the line at exactly 'width' characters thereby breaking a word */
           for( pp = leftstart + maxw, p_sp = p_leftstart + maxw; pp > leftstart; pp--, p_sp-- ) {
              if( *pp == ' ' ) { /* got a space, split here */
                 break;
              }
           }
           if( pp == leftstart ) { /* we hit the front of the buffer */
              wrap_out_ansi(leftstart, maxw, &winfo, holdbuff, &hbpt, " ", 0,
                            p_leftstart, outsplit2);
              count++;
              safe_str( "\r\n", holdbuff, &hbpt );
              p_bp = outsplit2 + (hbpt - holdbuff);
              initialize_ansisplitter(p_bp, 2);
              if (cut2) {
                 pp = leftstart + maxw;
                 p_sp = p_leftstart + maxw;
                 while (*pp && !isspace((int)*pp)) {
                    pp++;
                    p_sp++;
                 }
                 if (*pp) {
                    pp++;
                    p_sp++;
                 }
                 buffleft -= pp - leftstart;
                 leftstart += pp - leftstart;
                 p_leftstart += p_sp - p_leftstart;
              } else {
                 leftstart += maxw;
                 p_leftstart += maxw;
                 buffleft -= maxw;
              }
           } else { /* we hit a space, chop it there */
              wrap_out_ansi(leftstart, pp - leftstart, &winfo, holdbuff, &hbpt, " ", 0,
                            p_leftstart, outsplit2);
              count++;
              safe_str( "\r\n", holdbuff, &hbpt );
              p_bp = outsplit2 + (hbpt - holdbuff);
              initialize_ansisplitter(p_bp, 2);
              buffleft -= pp - leftstart + 1;
              leftstart += pp - leftstart + 1; /* eat the space */
              p_leftstart += p_sp - p_leftstart + 1;
           }
        }
     }
  }
  if (count > scount)
     count = scount;
  if( nfargs >= 7 ) {
     winfo.left = fargs[6];
  } else
     winfo.left = NULL;
  if (nfargs >= 8) {
     between = fargs[7];
  } else {
     between = NULL;
  }
  if( nfargs >= 9 ) {
     winfo.right = fargs[8];
  } else
     winfo.right = NULL;

  string = rebuild_ansi(holdbuff, outsplit2);

  if (wtype) {
     hbpt = string;
     while (count > 0) {
        if (winfo.left) {
#ifdef ZENTY_ANSI
           safe_chr('%', buff, bufcx);
           safe_chr(SAFE_CHR, buff, bufcx);
           safe_chr('n', buff, bufcx);
#endif
           safe_str(winfo.left, buff, bufcx);
        }
        for (x = 0; x < ncols; x++) {
           count--;
           pt2 = strchr(hbpt,'\r');
           if (pt2)
              *pt2 = '\0';
           safe_str(hbpt, buff, bufcx);
           if (pt2 && (pt2+1))
              hbpt = pt2+2;
           if (!count)
              break;
           if (x < (ncols - 1)) {
              if (between) {
#ifdef ZENTY_ANSI
                 safe_chr('%', buff, bufcx);
                 safe_chr(SAFE_CHR, buff, bufcx);
                 safe_chr('n', buff, bufcx);
#endif
                 safe_str(between, buff, bufcx);
              }
           }
        }
        bufcols = x + 1;
        if ( bufcols > ncols )
           bufcols = ncols;
        bufstr = 0;

        if ( nfargs >= 11 )
           bufstr = atoi(fargs[10]);
        if ( (bufstr > 0) && (bufcols > 0) ) {
           while ( bufcols < ncols ) {
              if ( bufcols < ncols ) {
                 if ( *between )
                    safe_str(between, buff, bufcx);
                 safe_pad2((char *)" ", winfo.width, buff, bufcx);
              }
              bufcols++;
           }
        }

        if (winfo.right) {
#ifdef ZENTY_ANSI
           safe_chr('%', buff, bufcx);
           safe_chr(SAFE_CHR, buff, bufcx);
           safe_chr('n', buff, bufcx);
#endif
           safe_str(winfo.right, buff, bufcx);
        }
        if (count)
           safe_str("\r\n", buff, bufcx);
     }
  } else {
     rows = count / ncols;
     rnorem = rows;
     remorig = count % ncols;
     if (remorig)
        rows++;
     for (x = 0; x < rows; x++) {
        if (winfo.left) {
#ifdef ZENTY_ANSI
           safe_chr('%', buff, bufcx);
           safe_chr(SAFE_CHR, buff, bufcx);
           safe_chr('n', buff, bufcx);
#endif
           safe_str(winfo.left, buff, bufcx);
        }
        for (y = 0; y < ncols; y++) {
           pos = x;
           for (z = 0; z < y; z++) {
              pos += rnorem;
              if ((remorig - z) > 0)
                 pos++;
           }
           hbpt = string;
           for (z = 0; z < pos; z++) {
              pt2 = strchr(hbpt,'\r');
              if ( pt2 && (pt2+1))
                 hbpt = pt2+2;
              else if ( *hbpt ) {
                 pt2 = strchr(hbpt,'\0');
                 if ( pt2 )
                    hbpt = pt2;
              }
           }
           pt2 = strchr(hbpt,'\r');
           if (pt2)
              *pt2 = '\0';
           safe_str(hbpt, buff, bufcx);
           if (pt2)
              *pt2 = '\r';
           if ((x == (rows - 1)) && remorig) {
              if ((remorig - y) == 1)
                 break;
              else if (between) {
#ifdef ZENTY_ANSI
                 safe_chr('%', buff, bufcx);
                 safe_chr(SAFE_CHR, buff, bufcx);
                 safe_chr('n', buff, bufcx);
#endif
                 safe_str(between, buff, bufcx);
              }
           } else if (y < (ncols - 1)) {
              if (between) {
#ifdef ZENTY_ANSI
                 safe_chr('%', buff, bufcx);
                 safe_chr(SAFE_CHR, buff, bufcx);
                 safe_chr('n', buff, bufcx);
#endif
                 safe_str(between, buff, bufcx);
              }
           }
        }
        bufcols = y + 1;
        if ( bufcols > ncols )
           bufcols = ncols;
        bufstr = 0;

        if ( nfargs >= 11 )
           bufstr = atoi(fargs[10]);
        if ( (bufstr > 0) && (bufcols > 0) ) {
           while ( bufcols < ncols ) {
              if ( bufcols < ncols ) {
                 if ( *between )
                    safe_str(between, buff, bufcx);
                 safe_pad2((char *)" ", winfo.width, buff, bufcx);
              }
              bufcols++;
           }
        }

        if (winfo.right) {
#ifdef ZENTY_ANSI
           safe_chr('%', buff, bufcx);
           safe_chr(SAFE_CHR, buff, bufcx);
           safe_chr('n', buff, bufcx);
#endif
           safe_str(winfo.right, buff, bufcx);
        }
        if (x < (rows - 1))
           safe_str("\r\n", buff, bufcx);
     }
  }
  free_lbuf(expandbuff);
  free_lbuf(holdbuff);
  free_lbuf(string);
}

/* ---------------------------------------------------------------------------
 * fun_bitlevel: Returns a level number for a player based on what bit
 *               they have
 *
 */
FUNCTION(fun_bittype)
{
  dbref target;
  static char *levs[] = { "#-1",    /* bad player */
                          "0",      /* Wanderer */
                          "1",      /* Citizen */
                          "2",      /* Guildmaster */
                          "3",      /* Architect */
                          "4",      /* Councilor */
                          "5",      /* Royalty */
                          "6",      /* Super-royalty */
                          "7" };    /* God */
  int got   = 0,
      i_chk = 0;

  if (!fn_range_check("BITTYPE", nfargs, 1, 2, buff, bufcx))
     return;

  target = match_thing(player, fargs[0]);

  if ( (nfargs > 1) && *fargs[1] )
     i_chk = atoi(fargs[1]);

  if( !Good_obj(target) ) {
    got = 0;
  } else {
    if ( i_chk == 0 ) {
       target = Owner(target);
    }
    if( God(target) ) {
      got = 8;
    }
    else if( Immortal(target) ) {
      got = 7;
    }
    else if( Wizard(target) ) {
      got = 6;
    }
    else if( Admin(target) ) {
      got = 5;
    }
    else if( Builder(target) ) {
      got = 4;
    }
    else if( Guildmaster(target) ) {
      got = 3;
    }
    else if( Wanderer(target) || Guest(target) ) {
      got = 1;
    }
    else {
      got = 2;
    }
    if ( Guildmaster(target) && HasPriv(target, player, POWER_HIDEBIT, POWER5, NOTHING) &&
         (Owner(player) != Owner(target) || mudstate.objevalst) ) {
       if( Wanderer(target) || Guest(target) )
          got = 1;
       else
          got = 2;
    }

  }

  safe_str(levs[got], buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * fun_words: Returns number of words in a string.
 * Added 1/28/91 Philip D. Wasson
 */

static int
countwords(char *str, char sep)
{
    int n;

    str = trim_space_sep(str, sep);
    if (!*str)
       return 0;
    for (n = 0; str; str = next_token(str, sep), n++);
    return n;
}

FUNCTION(fun_art)
{
    int i_key;

    if (!fn_range_check("ART", nfargs, 1, 2, buff, bufcx))
       return;

    i_key = 0;
    if ( (nfargs > 1) && *fargs[1] )
       i_key = atoi(fargs[1]);

    switch (tolower(*strip_all_special(fargs[0]))) {
       case 'a':
       case 'e':
       case 'i':
       case 'o':
       case 'u':
           safe_str("an", buff, bufcx);
           break;
       default:
           safe_chr('a', buff, bufcx);
    }
    if ( i_key == 0 ) {
       safe_chr(' ', buff, bufcx);
       safe_str(fargs[0], buff, bufcx);
    }
}

FUNCTION(fun_textfile)
{
   int t_val;
   dbref it;
   char *t_buff, *t_bufptr;
   CMDENT *cmdp;

   if (mudstate.last_cmd_timestamp == mudstate.now) {
      mudstate.heavy_cpu_recurse += 1;
   }
   if ( mudstate.heavy_cpu_recurse > mudconf.heavy_cpu_max ) {
      safe_str("#-1 HEAVY CPU RECURSION LIMIT EXCEEDED", buff, bufcx);
      return;
   }
   cmdp = (CMDENT *)hashfind((char *)"@dynhelp", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@dynhelp") ||
        cmdtest(Owner(player), "@dynhelp") || zonecmdtest(player, "@dynhelp") ) {
      notify(player, "Permission denied.");
      return;
   }

   if (!fn_range_check("TEXTFILE", nfargs, 2, 3, buff, bufcx))
       return;

   it = player;
   t_val = 0;
   if ( nfargs > 2 && *fargs[2] ) {
      t_val = atoi(fargs[2]);
      t_val = (((t_val < 0) || (t_val > 2)) ? 0 : t_val);
   }
   t_bufptr = t_buff = alloc_lbuf("fun_textfile");
   parse_dynhelp(it, cause, t_val, fargs[0], fargs[1], t_buff, t_bufptr, 1);

   safe_str(t_buff, buff, bufcx);
   free_lbuf(t_buff);
}

FUNCTION(fun_dynhelp)
{
   char *tpr_buff, *tprp_buff;
   int retval, t_val;
   dbref it;
   CMDENT *cmdp;

   if (mudstate.last_cmd_timestamp == mudstate.now) {
      mudstate.heavy_cpu_recurse += 1;
   }
   if ( mudstate.heavy_cpu_recurse > mudconf.heavy_cpu_max ) {
      safe_str("#-1 HEAVY CPU RECURSION LIMIT EXCEEDED", buff, bufcx);
      return;
   }
   cmdp = (CMDENT *)hashfind((char *)"@dynhelp", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@dynhelp") ||
        cmdtest(Owner(player), "@dynhelp") || zonecmdtest(player, "@dynhelp") ) {
      notify(player, "Permission denied.");
      return;
   }

   if (!fn_range_check("DYNHELP", nfargs, 2, 4, buff, bufcx))
       return;

   if ( nfargs < 3 || !*fargs[2] ) {
      it = player;
   } else {
      it = lookup_player(player, fargs[2], 1);
      if (it == NOTHING || !Good_obj(it) || !isPlayer(it) || Going(it) || Recover(it) ) {
         tprp_buff = tpr_buff = alloc_lbuf("fun_dynhelp");
         notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Unknown player '%s'.", fargs[2]));
         free_lbuf(tpr_buff);
         return;
      }
   }
   t_val = 0;
   if ( nfargs > 3 && *fargs[3] ) {
      t_val = atoi(fargs[3]);
      t_val = (((t_val < 0) || (t_val > 2)) ? 0 : t_val);
   }

   retval = parse_dynhelp(it, cause, t_val, fargs[0], fargs[1], (char *)NULL, (char *)NULL, 0);

   if ( retval ) {
      safe_str("#-1", buff, bufcx);
   }
}

FUNCTION(fun_lrand)
{
    char sep;
    int n_times, r_bot, r_top, i, first;
    double n_range;

    /* Special: the delim is really an output delim. */

    if (!fn_range_check("LRAND", nfargs, 3, 4, buff, bufcx))
       return;

    if ( (nfargs > 3) && (*fargs[3]) )
       sep = *fargs[3];
    else
       sep = ' ';

    /* If we're generating no numbers, since this is a list function,
     * we return empty, rather than returning 0.
     */

    n_times = atoi(fargs[2]);
    if (n_times < 1) {
       return;
    }
    if (n_times > LBUF_SIZE) {
       n_times = LBUF_SIZE;
    }
    r_bot = atoi(fargs[0]);
    r_top = atoi(fargs[1]);

    if (r_top < r_bot) {
       return;
    } else if (r_bot == r_top) {
        first = 0;
        for (i = 0; i < n_times; i++) {
           if (first)
              safe_chr(sep, buff, bufcx);
           ival(buff, bufcx, r_bot);
           first = 1;
        }
    } else {
  /* This will never be above the Float limit */
        n_range = (double) r_top - r_bot + 1;
        first = 0;
        for (i = 0; i < n_times; i++) {
           if (first)
              safe_chr(sep, buff, bufcx);
           ival(buff, bufcx, (r_bot + (random() % (int)n_range)));
           first = 1;
        }
    }
}

FUNCTION(fun_dice)
{
    int i, j, x, tot, botval, modval, i_type, i_save, i_crit;
    int arry[101], farry[101], carry[101];
    char sep;

    if (!fn_range_check("DICE", nfargs, 2, 7, buff, bufcx))
       return;

    if (!is_number(fargs[0]) || !is_number(fargs[1])) {
       safe_str("#-1 INVALID ARGUMENT", buff, bufcx);
       return;
    }
    i = atoi(fargs[0]);
    j = atoi(fargs[1]);
    i_type = 0;
    if ( (nfargs > 2) && *fargs[2] )
       i_type = atoi(fargs[2]);
    botval = 0;
    if ( nfargs > 3 && *fargs[3] )
       botval = atoi(fargs[3]);
    if ( botval <= 0 )
       botval = 1;
    if ( botval >= j )
       botval = j;
    if ( nfargs > 4 && *fargs[4] )
       sep = *fargs[4];
    else
       sep = ' ';
    if ( nfargs > 5 && *fargs[5] )
       i_save = atoi(fargs[5]);
    else
       i_save = 0;
    if ( nfargs > 6 && *fargs[6] )
       i_crit = atoi(fargs[6]);
    else
       i_crit = 0;

    if ((i > 100) || (i < 1) || (j > 100) || (j < 2) ||
        (i_save > 100) || (i_save < 0) ||
        (i_crit > 100) || (i_crit < 0)) {
       safe_str("#-1 ARGUMENT OUT OF RANGE", buff, bufcx);
       return;
    }
    tot = 0;
/* This is safe as above arry can't be above 100 */
    modval = (j - botval + 1);
    for (x = 0; x < i; x++) {
        arry[x] = (random() % modval) + botval;
        if ( (arry[x] <= (1 + i_save)) ) {
           farry[x] = arry[x];
           carry[x] = -1;
        } else if ( arry[x] >= (j - i_crit)) {
           carry[x] = arry[x];
           farry[x] = -1;
        } else
           farry[x] = carry[x] = -1;
        tot += arry[x];
    }
    if ( ((nfargs > 2) && (i_type != 2) && (i_type != 4)) || (nfargs == 2) ) {
       ival(buff, bufcx, tot);
       tot = 1;
    } else
       tot = 0;
    if ( (nfargs > 2) && ((i_type != 0) && (i_type != 5)) ) {
       for (x = 0; x < i; x++) {
          if ( !(((i_type <= 5) && (i_type >=3)) &&
                 ((arry[x] <= (1 + i_save)) || (arry[x] >= (j - i_crit)))) ) {
             if ( tot )
                safe_chr(sep, buff, bufcx);
             ival(buff, bufcx, arry[x]);
             tot = 1;
          }
       }
    }
    if ( (i_type == 3) || (i_type == 4) || (i_type == 5) ) {
       if ( sep != '|' )
          safe_chr('|', buff, bufcx);
       else
          safe_chr(':', buff, bufcx);
       tot=0;
       for (x = 0; x < i; x++) {
          if ( farry[x] != -1 ) {
            if ( tot )
               safe_chr(sep, buff, bufcx);
            ival(buff, bufcx, farry[x]);
            tot = 1;
          }
       }
       if ( sep != '|' )
          safe_chr('|', buff, bufcx);
       else
          safe_chr(':', buff, bufcx);
       tot=0;
       for ( x = 0; x < i; x++) {
          if ( carry[x] != -1 ) {
            if ( tot )
               safe_chr(sep, buff, bufcx);
            ival(buff, bufcx, carry[x]);
            tot = 1;
          }
       }
    }
}

FUNCTION(fun_checkpass)
{
   dbref thing;
   char *arg_ptr;

   // Try a DBREF first.
   if( *fargs[0] == '#' )
   {
      thing = match_thing( player, fargs[0] );
   }
   else
   {
      // Maybe we're doing an absolute player match.
      if( *fargs[0] == '*' )
      {
         arg_ptr = fargs[0]+1;
      } else {
         arg_ptr = fargs[0];
      }

      thing = lookup_player( player, arg_ptr, 1 );
   }

   // Let's see if we have anything worthwhile.
   if( !Good_obj( thing ) || ( thing == AMBIGUOUS || thing == NOTHING ) ||
       Recover( thing ) || !isPlayer( thing ) ||
      ( !Immortal( player ) && Cloak( thing ) && SCloak( thing ) ) ||
      ( !Wizard( player ) && Cloak( thing ) ) )
   {
      // That's a no. Kick it back.
      safe_str( "#-1 NO MATCH", buff, bufcx );
      return;
   }

   // Hey, we've got a player. Let's check the password.
   ival(buff, bufcx, (int) check_pass(thing, fargs[1], 0));
}

FUNCTION(fun_digest)
{
#ifdef HAS_OPENSSL
  EVP_MD_CTX ctx;
  const EVP_MD *mp;
  unsigned char md[EVP_MAX_MD_SIZE];
  unsigned int n, len = 0;
  const char *digits = "0123456789abcdef";
  int len2;

  len2 = strlen(fargs[1]);
  if ( !fargs[0] || !*fargs[0] || ((mp = EVP_get_digestbyname(fargs[0])) == NULL) ) {
    safe_str("#-1 UNSUPPORTED DIGEST TYPE", buff, bufcx);
    return;
  }

  EVP_DigestInit(&ctx, mp);
  EVP_DigestUpdate(&ctx, fargs[1], len2);
  EVP_DigestFinal(&ctx, md, &len);

  for (n = 0; n < len; n++) {
     safe_chr(digits[md[n] >> 4], buff, bufcx);
     safe_chr(digits[md[n] & 0x0F], buff, bufcx);
  }

#else
  int len2;

  len2 = strlen(fargs[1]);
  if ( fargs[0] && *fargs[0] && (strcmp(fargs[0], "sha") == 0) ) {
    safe_sha0(fargs[1], len2, buff, bufcx);
  } else {
    safe_str("#-1 UNSUPPORTED DIGEST TYPE", buff, bufcx);
  }
#endif
}

FUNCTION(fun_shuffle)
{
    int x, pic, num, numleft;
    char *pt1, *numpt, sep, osep, *outbuff, *s_output;
    ANSISPLIT outsplit[LBUF_SIZE], *p_sp;

    if (!fn_range_check("SHUFFLE", nfargs, 1, 3, buff, bufcx))
       return;

    initialize_ansisplitter(outsplit, LBUF_SIZE);
    outbuff = alloc_lbuf("fun_shuffle");
    split_ansi(strip_ansi(fargs[0]), outbuff, outsplit);
    pt1 = outbuff;
    p_sp = outsplit;
    if ( (nfargs > 1) && *fargs[1] )
       sep = *fargs[1];
    else
       sep = ' ';
    if ( (nfargs > 2) && *fargs[2] )
       osep = *fargs[2];
    else
       osep = sep;

    num = 0;
    while (*pt1) {
        while (*pt1 && (*pt1 != sep))
           pt1++;
        num++;
        while (*pt1 && (*pt1 == sep)) {
           *pt1 = '\0';
           pt1++;
        }
    }
    if (!num) {
       free_lbuf(outbuff);
       return;
    }
    if (num == 1) {
       safe_str(fargs[0], buff, bufcx);
       free_lbuf(outbuff);
       return;
    }
    numleft = num;
    numpt = (char *) malloc(num);
    for (x = 0; x < num; x++)
       *(numpt + x) = 0;
    while (numleft > 1) {
       do {
          pic = random() % num;
       } while (*(numpt + pic));
       *(numpt + pic) = 1;
       pt1 = outbuff;
       p_sp = outsplit;
       while (pic) {
          while (*pt1) {
             pt1++;
             p_sp++;
          }
          while (!*pt1) {
             pt1++;
             p_sp++;
          }
          pic--;
       }
       s_output = rebuild_ansi(pt1, p_sp);
       safe_str(s_output, buff, bufcx);
       free_lbuf(s_output);
       safe_chr(osep, buff, bufcx);
       numleft--;
    }
    pic = 0;
    while (*(numpt + pic) && (pic < num))
       pic++;
    pt1 = outbuff;
    p_sp = outsplit;
    while (pic) {
       while (*pt1) {
          pt1++;
          p_sp++;
       }
       while (!*pt1) {
          pt1++;
          p_sp++;
       }
       pic--;
    }
    s_output = rebuild_ansi(pt1, p_sp);
    safe_str(s_output, buff, bufcx);
    free_lbuf(s_output);
    free_lbuf(outbuff);
    free(numpt);
}

FUNCTION(fun_scramble)
{
    int x, pic, num, numleft;
    char *numpt, *outbuff, *s_output, *mybuff, *pmybuff;
    ANSISPLIT outsplit[LBUF_SIZE], outsplit2[LBUF_SIZE], *p_sp;

    initialize_ansisplitter(outsplit, LBUF_SIZE);
    initialize_ansisplitter(outsplit2, LBUF_SIZE);
    outbuff = alloc_lbuf("fun_scramble");
    split_ansi(strip_ansi(fargs[0]), outbuff, outsplit);

    num = strlen(outbuff);
    if (!num) {
       free_lbuf(outbuff);
       return;
    }
    if (num == 1) {
       safe_str(fargs[0], buff, bufcx);
       free_lbuf(outbuff);
       return;
    }

    pmybuff = mybuff = alloc_lbuf("fun_scramble2");

    numleft = num;
    numpt = (char *) malloc(num);
    for (x = 0; x < num; x++)
       *(numpt + x) = 0;
    p_sp = outsplit2;
    while (numleft > 1) {
       do {
          pic = random() % num;
       } while (*(numpt + pic));
       *(numpt + pic) = 1;
       safe_chr(*(outbuff + pic), mybuff, &pmybuff);
       clone_ansisplitter(p_sp, outsplit+pic);
       p_sp++;
       numleft--;
    }
    pic = 0;
    while (*(numpt + pic) && (pic < num))
       pic++;
    safe_chr(*(outbuff + pic), mybuff, &pmybuff);
    clone_ansisplitter(p_sp, outsplit+pic);
    s_output = rebuild_ansi(mybuff, outsplit2);
    safe_str(s_output, buff, bufcx);
    free_lbuf(mybuff);
    free_lbuf(s_output);
    free_lbuf(outbuff);
    free(numpt);
}

FUNCTION(fun_grab)
{
    char *pt1, toks[6], *pt2, *s_tok;

    if (!fn_range_check("GRAB", nfargs, 2, 3, buff, bufcx)) {
      return;
    }
    if ((nfargs == 2) || ((nfargs == 3) && !*fargs[2])) {
      pt2 = toks;
      strcpy(toks," \t\r\n,");
    }
    else
      pt2 = fargs[2];
    pt1 = strtok_r(fargs[0], pt2, &s_tok);
    while (pt1) {
       if (quick_wild(fargs[1], pt1))
          break;
       else
          pt1 = strtok_r(NULL, pt2, &s_tok);
    }
    if (pt1)
       safe_str(pt1, buff, bufcx);
}

FUNCTION(fun_graball)
{
    char *pt1, toks[6], *pt2, osep, *s_tok;
    int cntr;

    if (!fn_range_check("GRABALL", nfargs, 2, 4, buff, bufcx)) {
      return;
    }
    if ((nfargs == 2) || ((nfargs >= 3) && !*fargs[2])) {
      pt2 = toks;
      strcpy(toks," \t\r\n,");
    }
    else
      pt2 = fargs[2];
    if ( (nfargs > 3) && *fargs[3] )
       osep = *fargs[3];
    else if (nfargs > 3)
       osep = '\0';
    else
       osep = ' ';
    pt1 = strtok_r(fargs[0], pt2, &s_tok);
    cntr = 0;
    while (pt1) {
       if (quick_wild(fargs[1], pt1)) {
          if ( cntr && osep )
             safe_chr(osep, buff, bufcx);
          safe_str(pt1, buff, bufcx);
          cntr++;
       }
       pt1 = strtok_r(NULL, pt2, &s_tok);
    }
}

FUNCTION(fun_logstatus)
{
   ival(buff, bufcx, internal_logstatus());
}

FUNCTION(fun_logtofile)
{
    char *s_logroom;

    if ( mudstate.log_maximum >= mudconf.log_maximum ) {
       safe_str("#-1 MAXIMUM LOGS PER COMMAND EXCEEDED", buff, bufcx);
       return;
    }
    if ( !*fargs[0] ) {
       safe_str("#-1 EMPTY LOGFILE NAME SPECIFIED", buff, bufcx);
       return;
    }
    if ( !*fargs[1] ) {
       safe_str("#-1 EMPTY STRING TO LOG", buff, bufcx);
       return;
    }
    mudstate.log_maximum++;
    s_logroom = alloc_mbuf("fun_logfile");
    sprintf(s_logroom, "%.160s", fargs[0]);
    do_log(player, cause, (MLOG_FILE), s_logroom, fargs[1]);
    free_mbuf(s_logroom);
}


FUNCTION(fun_lockencode)
{
   struct boolexp *okey;
   char *s_instr;
   int len;

   if ( !fargs[0] || !*fargs[0] ) {
      safe_str("#-1 UNDEFINED KEY", buff, bufcx);
      return;
   }
   okey = parse_boolexp(player, strip_returntab(fargs[0],3), 0);
   if (okey == TRUE_BOOLEXP) {
      safe_str("#-1 UNDEFINED KEY", buff, bufcx);
   } else {
      s_instr = alloc_lbuf("fun_lockencode");
      memset(s_instr, '\0', LBUF_SIZE);
      sprintf(s_instr, "%s", unparse_boolexp_quiet(player, okey));
      len = strlen(s_instr);
      encode_base64((const char*)s_instr, len, buff, bufcx);
      free_lbuf(s_instr);
   }
   free_boolexp(okey);
}

FUNCTION(fun_lockdecode)
{
   struct boolexp *okey;
   char *s_instr, *s_instrptr, *tbuf;
   int len;

   if ( !fargs[0] || !*fargs[0] ) {
      safe_str("#-1 UNDEFINED KEY", buff, bufcx);
      return;
   }
   s_instrptr = s_instr = alloc_lbuf("fun_lockdecode");
   memset(s_instr, '\0', LBUF_SIZE);
   len = strlen(fargs[0]);
   decode_base64((const char*)strip_all_special(fargs[0]), len, s_instr, &s_instrptr, 0);
   okey = parse_boolexp(player, s_instr, 1);
   if (okey == TRUE_BOOLEXP) {
      safe_str("#-1 UNDEFINED KEY", buff, bufcx);
   } else {
      tbuf = (char *) unparse_boolexp_function(player, okey);
      safe_str(tbuf, buff, bufcx);
   }
   free_boolexp(okey);
   free_lbuf(s_instr);
}

FUNCTION(fun_lockcheck)
{
   struct boolexp *okey;
   char *s_instr, *s_instrptr;
   int len, i_locktype;
   dbref victim;

   if (!fn_range_check("LOCKCHECK", nfargs, 2, 3, buff, bufcx)) 
      return;

   if ( !fargs[0] || !*fargs[0] ) {
      safe_str("#-1 UNDEFINED KEY", buff, bufcx);
      return;
   }
   if ( !fargs[1] || !*fargs[1] ) {
      safe_str("#-1 UNDEFINED TARGET", buff, bufcx);
      return;
   }
   i_locktype = 0;
   if ( (nfargs > 2) && *fargs[2] ) {
      i_locktype = atoi(fargs[2]);
      if ( (i_locktype < 0) || (i_locktype > 2) )
         i_locktype = 0;
   }
   victim = match_thing(player, fargs[1]);
   if (!Good_chk(victim)) {
      safe_str("#-1 UNDEFINED TARGET", buff, bufcx);
      return;
   }

   s_instrptr = s_instr = alloc_lbuf("fun_lockdecode");
   memset(s_instr, '\0', LBUF_SIZE);
   len = strlen(fargs[0]);
   decode_base64((const char*)strip_all_special(fargs[0]), len, s_instr, &s_instrptr, 0);
   okey = parse_boolexp(player, s_instr, 1);
   if (okey == TRUE_BOOLEXP) {
      notify_quiet(player, "Warning: UNDEFINED KEY");
      ival(buff, bufcx, 0);
   } else {
      ival(buff, bufcx, eval_boolexp(victim, victim, victim, okey, i_locktype));
   }
   free_boolexp(okey);
   free_lbuf(s_instr);
}

/* Encode/Decode require OpenSSL or will return an error */
FUNCTION(fun_encode64)
{
   int len;

   len = strlen(strip_all_special(fargs[0]));
   if ( len <= 0) 
      return;

   encode_base64((const char*)strip_all_special(fargs[0]), len, buff, bufcx);
}

FUNCTION(fun_decode64)
{
   int len;

   len = strlen(strip_all_special(fargs[0]));
   if ( len <= 0) 
      return;

   decode_base64((const char*)strip_all_special(fargs[0]), len, buff, bufcx, 0);
}

/* --------------------------------------------------------
 * Entrances() works like @entrances but just returns
 * the list of dbref#'s.  You may specify type
 */

FUNCTION(fun_entrances)
{
    dbref thing, i;
    char *p, *tpr_buff, *tprp_buff;
    int control_thing, count, low_bound, high_bound, type_flg;

    if (!fn_range_check("ENTRANCES", nfargs, 1, 4, buff, bufcx))
       return;

    if (mudstate.last_cmd_timestamp == mudstate.now) {
       mudstate.heavy_cpu_recurse += 1;
    }
    if ( mudstate.heavy_cpu_recurse > mudconf.heavy_cpu_max ) {
       safe_str("#-1 HEAVY CPU RECURSION LIMIT EXCEEDED", buff, bufcx);
       return;
    }
    if (!fargs[0] || !*fargs[0]) {
       if (Has_location(player))
          thing = Location(player);
       else
          thing = player;
       if (!Good_obj(thing)) {
          return;
       }
    } else {
       init_match(player, fargs[0], NOTYPE);
       match_everything(MAT_EXIT_PARENTS);
       thing = noisy_match_result();
       if (thing == NOTHING || !Good_obj(thing))
          return;
    }
    type_flg = 0;
    if (nfargs > 1) {
       if (!fargs[1] || !*fargs[1]) {
          safe_str("#-1 INVALID SECOND ARGUMENT", buff, bufcx);
          return;
       }
       p = fargs[1];
       while (*p) {
          switch (*p) {
             case 'a':
             case 'A':
                type_flg = 15;
                break;
             case 'e':
             case 'E':
                type_flg |= 1;
                break;
             case 't':
             case 'T':
                type_flg |= 2;
                break;
             case 'p':
             case 'P':
                type_flg |= 4;
                break;
             case 'r':
             case 'R':
                type_flg |= 8;
                break;
             default:
                safe_str("#-1 INVALID SECOND ARGUMENT", buff, bufcx);
                return;
          }
          p++;
       }
    } else {
       type_flg = 15;
    }

    low_bound = 0;
    high_bound = mudstate.db_top - 1;
    if ( nfargs > 2 ) {
       if ( !fargs[2] || !*fargs[2] )
          low_bound = 0;
       else
          low_bound = atoi(fargs[2]);
    }
    if ( low_bound < 0 )
       low_bound = 0;
    if ( nfargs > 3 ) {
       if ( !fargs[3] || !*fargs[3] )
          high_bound = mudstate.db_top - 1;
       else
          high_bound = atoi(fargs[3]);
    }
    if (high_bound >= mudstate.db_top)
       high_bound = mudstate.db_top - 1;

    if (!payfor(player, mudconf.searchcost)) {
        tprp_buff = tpr_buff = alloc_lbuf("fun_entrances");
        notify(player,
               safe_tprintf(tpr_buff, &tprp_buff, "You don't have enough %s.",
        mudconf.many_coins));
        free_lbuf(tpr_buff);
        return;
    }

    control_thing = Examinable(player, thing);
    count = 0;
    for (i = low_bound; i <= high_bound; i++) {
       if (control_thing || (!Going(i) && !Recover(i) && Examinable(player, i))) {
          switch (Typeof(i)) {
             case TYPE_EXIT:
                if ( type_flg & 1 ) {
                   if (Location(i) == thing) {
                      if ( count )
                         safe_chr(' ', buff, bufcx);
                      dbval(buff, bufcx, i);
                      count++;
                   }
                }
                break;
             case TYPE_ROOM:
                if ( type_flg & 8 ) {
                   if (Dropto(i) == thing) {
                      if ( count )
                         safe_chr(' ', buff, bufcx);
                      dbval(buff, bufcx, i);
                      count++;
                   }
                }
                break;
             case TYPE_THING:
                if ( type_flg & 2 ) {
                   if (Home(i) == thing) {
                      if ( count )
                         safe_chr(' ', buff, bufcx);
                      dbval(buff, bufcx, i);
                      count++;
                   }
                 }
                 break;
              case TYPE_PLAYER:
                 if ( type_flg & 4 ) {
                    if (Home(i) == thing) {
                       if ( count )
                          safe_chr(' ', buff, bufcx);
                       dbval(buff, bufcx, i);
                       count++;
                    }
                  }
                  break;
          } /* Switch */
       } /* If */
    } /* For */
}

/* --------------------------------------------------------
 * elist returns list in english-language format
 * Idea was borrowed from Elendor with permission
 * elist(string,<string other than and>,<seperator>)
 */
FUNCTION(fun_elist)
{
    char *sep_buf, *curr, *objstring, *result, *cp, *r_munge, *r_store, *s_array[3],
         *curr_buf, sep, sop[LBUF_SIZE+1], sepfil, *sep_buf2, *sepptr;
    int first, t_words, t_last, cntr, commsep, i_munge;

    if (!fn_range_check("ELIST", nfargs, 1, 6, buff, bufcx))
       return;

    commsep = 0;
    curr_buf = NULL;
    memset(sop, 0, sizeof(sop));
    if (nfargs > 1 && *fargs[1] ) {
       sep_buf = exec(player, cause, caller,
          EV_STRIP | EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
       if ( *sep_buf )
          strcpy(sop, sep_buf);
       else
          strcpy(sop, "and");
       free_lbuf(sep_buf);
    } else {
       strcpy(sop, "and");
    }
    if (nfargs > 2 && *fargs[2] ) {
       sep_buf = exec(player, cause, caller,
          EV_STRIP | EV_FCHECK | EV_EVAL, fargs[2], cargs, ncargs);
       if ( *sep_buf )
          sep = *sep_buf;
       else
          sep = ' ';
       free_lbuf(sep_buf);
    } else {
       sep = ' ';
    }
    if (nfargs > 3 && *fargs[3] ) {
       sep_buf = exec(player, cause, caller,
          EV_STRIP | EV_FCHECK | EV_EVAL, fargs[3], cargs, ncargs);
       sep_buf2 = alloc_lbuf("fun_elist2");
       sepptr = sep_buf2;
       safe_str(strip_ansi(sep_buf),sep_buf2,&sepptr);
       sepptr = sep_buf2;
       free_lbuf(sep_buf);
       if ( *sep_buf2 )
          sepfil = *sep_buf2;
       else
          sepfil = ' ';
       free_lbuf(sep_buf2);
    } else {
       sepfil = ' ';
    }
    if (nfargs > 4 && *fargs[4] ) {
       sep_buf = exec(player, cause, caller,
          EV_STRIP | EV_FCHECK | EV_EVAL, fargs[4], cargs, ncargs);
       if ( *sep_buf ) {
          sepptr = sep_buf2 = alloc_lbuf("fun_elist3");
          safe_str(strip_ansi(sep_buf),sep_buf2,&sepptr);
          commsep = 1;
       }
       free_lbuf(sep_buf);
    }
    i_munge = 0;
    if ( nfargs > 5 && *fargs[5] ) {
       r_store = alloc_lbuf("munge_elist");
       s_array[0] = alloc_lbuf("munge_elist_num");
       s_array[1] = alloc_lbuf("munge_elist_num2");
       s_array[2] = NULL;
       i_munge = 1;
    }

    curr = alloc_lbuf("fun_elist");
    cp = curr;

    curr_buf = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0],
                    cargs, ncargs);
    safe_str(strip_ansi(curr_buf),curr,&cp);
    free_lbuf(curr_buf);

    cp = curr;
    cp = trim_space_sep(cp, sep);
    if (!*cp) {
       free_lbuf(curr);
       if ( commsep )
          free_lbuf(sep_buf2);
       if ( i_munge ) {
          free_lbuf(r_store);
          free_lbuf(s_array[0]);
          free_lbuf(s_array[1]);
       }
       return;
    }
    first = 1;
    t_words = countwords(curr, sep);
    t_last = t_words - 2;
    cntr = 1;
    while (cp) {
       objstring = split_token(&cp, sep);
       if (!first)
          safe_chr(sepfil, buff, bufcx);
       if ( mudconf.old_elist == 1 ) {
          result = exec(player, cause, caller,
                        EV_STRIP | EV_FCHECK | EV_EVAL, objstring, cargs, ncargs);
          if ( i_munge ) {
             memcpy(r_store, fargs[5], LBUF_SIZE-2);
             memcpy(s_array[0], result, LBUF_SIZE-2);
             sprintf(s_array[1], "%d", cntr);
             r_munge = exec(player, cause, caller,
                            EV_STRIP | EV_FCHECK | EV_EVAL, r_store, s_array, 2);
             
             safe_str(r_munge, buff, bufcx);
             free_lbuf(r_munge);
          } else {
             safe_str(result, buff, bufcx);
          }
          free_lbuf(result);
       } else {
          if ( i_munge ) {
             memcpy(r_store, fargs[5], LBUF_SIZE-2);
             memcpy(s_array[0], objstring, LBUF_SIZE-2);
             sprintf(s_array[1], "%d", cntr);
             r_munge = exec(player, cause, caller,
                            EV_STRIP | EV_FCHECK | EV_EVAL, r_store, s_array, 2);
             
             safe_str(r_munge, buff, bufcx);
             free_lbuf(r_munge);
          } else {
             safe_str(objstring, buff, bufcx);
          }
       }
       first = 0;
       if ( cntr <= t_last ) {
          if ( commsep )
             safe_str(sep_buf2, buff, bufcx);
          else
             safe_chr(',', buff, bufcx);
       } else if ( cntr < t_words ) {
          if ( t_words != 2 ) {
             if ( commsep )
                safe_str(sep_buf2, buff, bufcx);
             else
                safe_chr(',', buff, bufcx);
          }
          safe_chr(sepfil, buff, bufcx);
          safe_str(sop, buff, bufcx);
       }
       cntr++;
    }
    if ( i_munge ) {
       free_lbuf(s_array[0]);
       free_lbuf(s_array[1]);
       free_lbuf(r_store);
    }
    free_lbuf(curr);
    if ( commsep )
       free_lbuf(sep_buf2);
}

/* ----------------------------------------------------------------------
 * Vector Math Additions
 */

FUNCTION(fun_vmag)
{
   double num, sum;
   char *p1, sep;

   if (!fargs[0])
      return;
   varargs_preamble("VMAG", 2);
   p1 = trim_space_sep(fargs[0], sep);
   if (!*p1)
      return;

   /* sum the squares */
   num = safe_atof(split_token(&p1, sep));
   sum = num * num;
   while (p1) {
      num = safe_atof(split_token(&p1, sep));
      sum += num * num;
   }
   fval(buff, bufcx, sqrt(sum));

}

FUNCTION(fun_vadd)
{
   char *p1, *p2, sep, osep;

  /* return if a list is empty */
   if (!fargs[0] || !fargs[1])
       return;

   svarargs_preamble("VADD", 4);
   if ( countwords(fargs[0],sep) != countwords(fargs[1],sep) ) {
      safe_str("#-1 VECTORS MUST BE SAME DIMENSIONS", buff, bufcx);
      return;
   }
   p1 = trim_space_sep(fargs[0], sep);
   p2 = trim_space_sep(fargs[1], sep);

  /* return if a list is empty */
   if (!*p1 || !*p2)
      return;

  /* add the vectors */
   fval(buff, bufcx, safe_atof(split_token(&p1, sep)) + safe_atof(split_token(&p2, sep)));
   while (p1 && p2) {
      safe_chr(osep, buff, bufcx);
      fval(buff, bufcx, safe_atof(split_token(&p1, sep)) + safe_atof(split_token(&p2, sep)));
   }

}

FUNCTION(fun_vsub)
{
   char *p1, *p2, sep, osep;

  /* return if a list is empty */
   if (!fargs[0] || !fargs[1])
       return;

   svarargs_preamble("VSUB", 4);
   if ( countwords(fargs[0],sep) != countwords(fargs[1],sep) ) {
      safe_str("#-1 VECTORS MUST BE SAME DIMENSIONS", buff, bufcx);
      return;
   }
   p1 = trim_space_sep(fargs[0], sep);
   p2 = trim_space_sep(fargs[1], sep);

  /* return if a list is empty */
   if (!*p1 || !*p2)
      return;

  /* add the vectors */
   fval(buff, bufcx, safe_atof(split_token(&p1, sep)) - safe_atof(split_token(&p2, sep)));
   while (p1 && p2) {
      safe_chr(osep, buff, bufcx);
      fval(buff, bufcx, safe_atof(split_token(&p1, sep)) - safe_atof(split_token(&p2, sep)));
   }

}

FUNCTION(fun_vdim)
{
    char sep;

    if (nfargs == 0) {
       safe_str("0", buff, bufcx);
       return;
    }
    varargs_preamble("VDIM", 2);
    ival(buff, bufcx, countwords(fargs[0], sep));
}

FUNCTION(fun_vdot)
{
   double product;
   char *p1, *p2, sep;

  /* return if a list is empty */
   if (!fargs[0] || !fargs[1])
      return;
   varargs_preamble("VDOT", 3);
   if ( countwords(fargs[0],sep) != countwords(fargs[1],sep) ) {
      safe_str("#-1 VECTORS MUST BE SAME DIMENSIONS", buff, bufcx);
      return;
   }
   p1 = trim_space_sep(fargs[0], sep);
   p2 = trim_space_sep(fargs[1], sep);

  /* return if a list is empty */
   if (!*p1 || !*p2)
      return;

  /* multiply the vectors */
   product = 0;
   while (p1 && p2) {
      product += safe_atof(split_token(&p1, sep)) * safe_atof(split_token(&p2, sep));
   }
   fval(buff, bufcx, product);
}
/***************************************************
 * basic code and idea borrowed from TinyMUX 2.0
 */
FUNCTION(fun_vcross)
{
   double a[2][3];
   int i;
   char *p1, *p2, sep, osep;

   /* return if a list is empty */
   if ( !fargs[0] || !fargs[1])
      return;
   svarargs_preamble("VCROSS", 4);
   if ( (countwords(fargs[0],sep) != 3) ) {
      safe_str("#-1 VECTORS MUST BE DIMENSION OF 3", buff, bufcx);
      return;
   }
   if (countwords(fargs[0],sep) != countwords(fargs[1],sep)) {
      safe_str("#-1 VECTORS MUST BE DIMENSION OF 3", buff, bufcx);
      return;
   }
   p1 = trim_space_sep(fargs[0], sep);
   p2 = trim_space_sep(fargs[1], sep);
   for (i = 0; i < 3; i++)
   {
      a[0][i] = safe_atof(split_token(&p1,sep));
      a[1][i] = safe_atof(split_token(&p2,sep));
   }
   fval(buff, bufcx, (a[0][1] * a[1][2]) - (a[0][2] * a[1][1]));
   safe_chr(osep, buff, bufcx);
   fval(buff, bufcx, (a[0][2] * a[1][0]) - (a[0][0] * a[1][2]));
   safe_chr(osep, buff, bufcx);
   fval(buff, bufcx, (a[0][0] * a[1][1]) - (a[0][1] * a[1][0]));
}

FUNCTION(fun_vmul)
{
   double e1, e2;
   char *p1, *p2, sep, osep;
   int wrd1, wrd2;

  /* return if a list is empty */
   if (!fargs[0] || !fargs[1])
      return;

   svarargs_preamble("VMUL", 4);
   wrd1 = countwords(fargs[0],sep);
   wrd2 = countwords(fargs[1],sep);
   if ( wrd1 != wrd2 && wrd1 != 1 && wrd2 != 1 ) {
      safe_str("#-1 VECTORS MUST BE SAME DIMENSIONS", buff, bufcx);
      return;
   }
   p1 = trim_space_sep(fargs[0], sep);
   p2 = trim_space_sep(fargs[1], sep);

  /* return if a list is empty */
   if (!*p1 || !*p2)
      return;

  /* multiply the vectors */
   e1 = safe_atof(split_token(&p1, sep));
   e2 = safe_atof(split_token(&p2, sep));
   if (!p1) {
    /* scalar * vector */
      fval(buff, bufcx, e1 * e2);
      while (p2) {
         safe_chr(osep, buff, bufcx);
         fval(buff, bufcx, e1 * safe_atof(split_token(&p2, sep)));
      }
   } else if (!p2) {
    /* vector * scalar */
      fval(buff, bufcx, e1 * e2);
      while (p1) {
         safe_chr(osep, buff, bufcx);
         fval(buff, bufcx, safe_atof(split_token(&p1, sep)) * e2);
      }
   } else {
    /* vector * vector elementwise product */
      fval(buff, bufcx, e1 * e2);
      while (p1 && p2) {
         safe_chr(osep, buff, bufcx);
         fval(buff, bufcx, safe_atof(split_token(&p1, sep)) * safe_atof(split_token(&p2, sep)));
      }
   }

}

FUNCTION(fun_vunit)
{
   double num, sum;
   char tbuf[LBUF_SIZE*2], *p1, sep, osep;

  /* return if a list is empty */
   if (!fargs[0])
      return;

   svarargs_preamble("VUNIT", 3);
   p1 = trim_space_sep(fargs[0], sep);

  /* return if a list is empty */
   if (!*p1)
      return;

  /* copy the vector, since we have to walk it twice... */
   strncpy(tbuf, p1, (LBUF_SIZE*2)-1);

  /* find the magnitude */
   num = safe_atof(split_token(&p1, sep));
   sum = num * num;
   while (p1) {
      num = safe_atof(split_token(&p1, sep));
      sum += num * num;
   }
   sum = sqrt(sum);

   if (!sum) {
    /* zero vector */
      p1 = tbuf;
      safe_chr('0', buff, bufcx);
      while (split_token(&p1, sep), p1) {
         safe_chr(osep, buff, bufcx);
         safe_chr('0', buff, bufcx);
      }
      return;
   }
  /* now make the unit vector */
   p1 = tbuf;
   fval(buff, bufcx, safe_atof(split_token(&p1,sep)) / sum);
   while (p1) {
      safe_chr(osep, buff, bufcx);
      fval(buff, bufcx, safe_atof(split_token(&p1,sep)) / sum);
   }

}

/* ----------------------------------------------------------------------
 * End of vector math additions
 */

FUNCTION(fun_valid)
{
/*
 * Checks to see if a given <something> is valid as a parameter of a
 * * given type (such as an object name).
 */

   char *mbuf;
   CMDENT *cmdp;
   FLAGENT *flgp;
   FUN *fp;
   UFUN *ufp;
   PENNANSI *cm;
   TOGENT *tp;

  if (!fargs[0] || !*fargs[0]) {
      safe_str("#-1", buff, bufcx);
  } else if (!fargs[1] || !*fargs[1]) {
      ival(buff, bufcx, 0);
  } else if ( !stricmp(fargs[0], "name") ||
              !stricmp(fargs[0], "thingname") ||
              !stricmp(fargs[0], "roomname") ||
              !stricmp(fargs[0], "exitname") ||
              !stricmp(fargs[0], "zonename") ) {
      ival(buff, bufcx, ok_name(fargs[1]));
  } else if (!stricmp(fargs[0], "attrname")) {
      ival(buff, bufcx, ok_attr_name(fargs[1]));
  } else if (!stricmp(fargs[0], "playername")) {
      ival(buff, bufcx, (ok_player_name(fargs[1]) &&
                        (lookup_player(NOTHING, fargs[1], 0) == NOTHING)));
  } else if (!stricmp(fargs[0], "password")) {
      ival(buff, bufcx, ok_password(fargs[1], player, 1));
  } else if (!stricmp(fargs[0], "command")) {
     cmdp = lookup_command(fargs[1]);
     if ( cmdp && (cmdp->cmdtype & CMD_BUILTIN_e || cmdp->cmdtype & CMD_LOCAL_e) &&
           check_access(player, cmdp->perms, cmdp->perms2, 0)) {
       if ( (!strcmp(cmdp->cmdname, "@snoop") || !strcmp(cmdp->cmdname, "@depower")) &&
             !Immortal(player)) {
          cmdp = NULL;
       }
     } else {
        cmdp = NULL;
     }
     ival(buff, bufcx, ((cmdp != NULL) ? 1 : 0));
  } else if (!stricmp(fargs[0], "function")) {
     fp = (FUN *) hashfind(fargs[1], &mudstate.func_htab);
     if ( !fp ) {
        ival(buff, bufcx, 0);
     } else {
        ival(buff, bufcx, check_access(player, fp->perms, fp->perms2, 0));
     }
  } else if (!stricmp(fargs[0], "ufunction")) {
     ufp = (UFUN *) hashfind(fargs[1], &mudstate.ufunc_htab);
     if ( !ufp ) {
        mbuf = alloc_mbuf("valid");
        sprintf(mbuf, "%d_%.100s", Owner(player), fargs[1]);
        ufp = (UFUN *) hashfind(mbuf, &mudstate.ulfunc_htab);
        if ( ufp && (!Good_chk(ufp->obj) || (ufp->orig_owner != Owner(ufp->obj))) ) {
           ufp = NULL;
        }
        free_mbuf(mbuf);
     }
     if ( !ufp ) {
        ival(buff, bufcx, 0);
     } else {
        ival(buff, bufcx, check_access(player, ufp->perms, ufp->perms2, 0));
     }
  } else if (!stricmp(fargs[0], "flag")) {
     flgp = find_flag(player, fargs[1]);
     if ( flgp ) {
        if ((flgp->listperm & CA_BUILDER) && !Builder(player))
            flgp = NULL;
        else if ((flgp->listperm & CA_ADMIN) && !Admin(player))
            flgp = NULL;
        else if ((flgp->listperm & CA_WIZARD) && !Wizard(player))
            flgp = NULL;
        else if ((flgp->listperm & CA_IMMORTAL) && !Immortal(player))
            flgp = NULL;
        else if ((flgp->listperm & CA_GOD) && !God(player))
            flgp = NULL;
     }
     ival(buff, bufcx, ((flgp != NULL) ? 1 : 0));
  } else if (!stricmp(fargs[0], "toggle")) {
     tp = find_toggle(player, fargs[1]);
     if ( tp ) {
        if ((tp->listperm & CA_BUILDER) && !Builder(player))
            tp = NULL;
        else if ((tp->listperm & CA_ADMIN) && !Admin(player))
            tp = NULL;
        else if ((tp->listperm & CA_WIZARD) && !Wizard(player))
            tp = NULL;
        else if ((tp->listperm & CA_IMMORTAL) && !Immortal(player))
            tp = NULL;
        else if ((tp->listperm & CA_GOD) && !God(player))
            tp = NULL;
     }
     ival(buff, bufcx, ((tp != NULL) ? 1 : 0));
  } else if (!stricmp(fargs[0], "qreg")) {
     ival(buff, bufcx, (((strlen(fargs[1]) < SBUF_SIZE) && (strlen(fargs[1]) > 0)) ? 1 : 0));
  } else if (!stricmp(fargs[0], "colorname")) {
     cm = (PENNANSI *)hashfind(fargs[1], &mudstate.ansi_htab);
     ival(buff, bufcx, ((cm != NULL) ? 1 : 0));
  } else if (!stricmp(fargs[0], "ansicodes")) {
     ival(buff, bufcx, 1);
  } else {
     safe_str("#-1", buff, bufcx);
  }

}

FUNCTION(fun_visible)
{
  dbref thing;

  init_match(player, fargs[0], NOTYPE);
  match_everything(MAT_EXIT_PARENTS);
  thing = match_result();
  if ((thing == NOTHING) || (thing == AMBIGUOUS))
    safe_str("0", buff, bufcx);
  else if ( !Examinable(player,thing) )
    safe_str("0", buff, bufcx);
  else
    safe_str("1", buff, bufcx);
}

FUNCTION(fun_visiblemux)
{
   char ch, *atr_gotten;
   dbref thing, it, aowner;
   int atr, aflags;
   ATTR *ap = NULL;

   ch = '0';
   init_match(player, fargs[0], NOTYPE);
   match_everything(MAT_EXIT_PARENTS);
   it = match_result();
   atr = NOTHING;

   if ((it != NOTHING) && (it != AMBIGUOUS)) {
      if ( !Controls(player, it) )
         it = player;
      if (parse_attrib(player, fargs[1], &thing, &atr)) {
        if ((atr == NOTHING) || !Good_obj(thing) || Recover(thing) || Going(thing))
            ap = NULL;
        else
            ap = atr_num(atr);
      }
      if ( atr == NOTHING ) {
         init_match(player, fargs[1], NOTYPE);
         match_everything(MAT_EXIT_PARENTS);
         thing = match_result();
      }
      if (Good_obj(thing) && !Recover(thing) && !Going(thing)) {
         if ((atr == NOTHING) || !ap) {
            if (Examinable(it, thing)) {
               ch = '1';
            }
         } else {
            atr_gotten = atr_get(thing, atr, &aowner, &aflags);
            if (ap && See_attr(it, thing, ap, aowner, aflags, 0))
               ch = '1';
            free_lbuf(atr_gotten);
         }
      }
   }
   safe_chr(ch, buff, bufcx);
}


FUNCTION(fun_grep)
{
    dbref object;
    char *ret;
    int i_key;

    if (!fn_range_check("GREP", nfargs, 3, 4, buff, bufcx))
       return;

    init_match(player, fargs[0], NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    object = noisy_match_result();
    if (object == NOTHING)
       safe_str("#-1", buff, bufcx);
    else if (!Examinable(player, object))
       safe_str("#-1", buff, bufcx);
    else {
       if ( (nfargs > 3) && *fargs[3] ) {
          i_key = atoi(fargs[3]);
          if ( i_key ) 
             i_key = 2;
       } else {
          i_key = 0;
       }
       ret = grep_internal(player, object, fargs[2], fargs[1], i_key);
       safe_str(ret, buff, bufcx);
       free_lbuf(ret);
    }
}

FUNCTION(fun_pgrep)
{
    dbref object, parent;
    int i_first, loop, i_showdbref, i_key;
    char *ret, *s_tmpbuf, *sep;

    if (!fn_range_check("PGREP", nfargs, 3, 6, buff, bufcx))
       return;

    if ( (nfargs > 3) && *fargs[3] ) {
       i_showdbref = atoi(fargs[3]);
    } else {
       i_showdbref = 0;
    }
    if ( i_showdbref ) {
       s_tmpbuf = alloc_mbuf("fun_pgrep");
    }

    if ( (nfargs > 4) && *fargs[4] ) {
       sep = fargs[4];
    } else {
       sep = NULL;
    }
    if ( (nfargs > 5) && *fargs[5] ) {
       i_key = atoi(fargs[5]);
       if ( i_key )
          i_key = 2;
    } else {
       i_key = 0;
    }
    init_match(player, fargs[0], NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    object = noisy_match_result();
    if (object == NOTHING)
       safe_str("#-1", buff, bufcx);
    else if (!Examinable(player, object))
       safe_str("#-1", buff, bufcx);
    else {
       i_first = 0;
       ITER_PARENTS(object, parent, loop) {
          if ( !mudstate.chkcpu_toggle && Good_chk(parent) && Examinable(player, parent) ) {
             ret = grep_internal(player, parent, fargs[2], fargs[1], (i_key | ((i_showdbref == 2) ? 1 : 0)) );
             if ( *ret ) {
                if ( i_showdbref == 1 ) {
                   if ( i_first )
                      safe_chr(' ', buff, bufcx);
                   if ( sep ) {
                      safe_str(sep, buff, bufcx);
                      sprintf(s_tmpbuf, "#%d", parent);
                   } else {
                      sprintf(s_tmpbuf, "[#%d]", parent);
                   }
                   i_first = 1;
                   safe_str(s_tmpbuf, buff, bufcx);
                }
                if ( i_first )
                   safe_chr(' ', buff, bufcx);
                i_first = 1;
                safe_str(ret, buff, bufcx);
             }
             free_lbuf(ret);
          }
       }
    }
    if ( i_showdbref ) {
       free_mbuf(s_tmpbuf);
    }
}

int
convnum(char *source)
{
    int rval;
    char *pt1;
    int x;

    rval = 0;
    if ((strlen(source) > 1) && (tolower(*(source + 1)) == 'b')) {
       pt1 = source + 2;
       while (*pt1 == '0')
          pt1++;
       if (strlen(pt1) > 32)
          rval = ~rval;
       else {
          for ( pt1 = source + strlen(source) - 1, x = 0;
                pt1 > (source + 1); x++, pt1-- ) {
              if ( *pt1 == '1' ) {
                 rval = rval | (0x01 << x);
              }
          }
       }
    } else
       sscanf(source, "%i", &rval);
    return rval;
}

int
process_tr(char *s_instr, char *s_outstr, char *s_outptr)
{
   int i_cntr, i_range, i;
   char *s_holdstr, *s_ptr, c_lastchr;

   s_holdstr = alloc_lbuf("process_tr");
   i_cntr = i_range = 0;
   strcpy(s_holdstr, strip_ansi(s_instr));
   s_ptr = s_holdstr;
   c_lastchr = '\0';
   while ( (i_cntr < (LBUF_SIZE - 1000)) && *s_ptr ) {
      if ( (((int)*s_ptr >= 32) && ((int)*s_ptr <= 126)) || (*s_ptr == '\n') ) {
         if ( c_lastchr && *s_ptr == '-' ) {
            i_range = 1;
         } else {
            if ( i_range ) {
               if ( (int)*s_ptr > (int)c_lastchr ) {
                  for ( i = (int)c_lastchr + 1; i <= (int)*s_ptr; i++ ) {
                     if ( ((i >= 32) && (i <= 126)) || ((char)i == '\n') ) {
                        safe_chr((char)i, s_outstr, &s_outptr);
                        i_cntr++;
                     }
                  }
               } else {
                  for ( i = (int)c_lastchr - 1; i >= (int)*s_ptr; i-- ) {
                     if ( ((i >= 32) && (i <= 126)) || ((char)i == '\n') ) {
                        safe_chr((char)i, s_outstr, &s_outptr);
                        i_cntr++;
                     }
                  }
               }
               c_lastchr = '\0';
            } else {
               safe_chr(*s_ptr, s_outstr, &s_outptr);
               c_lastchr = *s_ptr;
            }
            i_range = 0;
         }
      }
      s_ptr++;
      i_cntr++;
   }
   free_lbuf(s_holdstr);
   return(i_cntr);
}

FUNCTION(fun_tr)
{
   char *s_instr1, *s_instr2, *s_holdstr, *s_ptr, s_chrmap[256], 
        *s_inptr, *outbuff, *outarg2, *s_output;
   int i_cntr, i, i_noansi;
   ANSISPLIT outsplit[LBUF_SIZE], outsplit2[LBUF_SIZE], *p_sp, *p_sp2,
             s_splitmap[256], s_splitarg2[LBUF_SIZE];

   if (!fn_range_check("TR", nfargs, 3, 4, buff, bufcx)) {
      return;
   }

   if ( !*fargs[0] ) {
      return;
   }
   if ( !*fargs[1] || !*fargs[2] ) {
      safe_str(fargs[0], buff, bufcx);
      return;
   }

   i_cntr = 0;
   /* Sanitize the lists */
   s_inptr = s_instr1 = alloc_lbuf("fun_tr_str1");
   i_cntr = process_tr(fargs[1], s_instr1, s_inptr);
   if ( i_cntr > (LBUF_SIZE - 1001) ) {
      free_lbuf(s_instr1);
      safe_str("#-1 INPUT FIND LIST TOO LARGE.", buff, bufcx);
      return;
   }
   s_inptr = s_instr2 = alloc_lbuf("fun_tr_str2");
   i_cntr = process_tr(fargs[2], s_instr2, s_inptr);
   if ( i_cntr > (LBUF_SIZE - 1001) ) {
      free_lbuf(s_instr1);
      free_lbuf(s_instr2);
      safe_str("#-1 OUTPUT REPLACE LIST TOO LARGE.", buff, bufcx);
      return;
   }

   i_noansi = 0;
   if ( (nfargs > 3) && *fargs[3] )
      i_noansi = atoi(fargs[3]);

   if ( i_noansi != 0 ) 
      i_noansi = 1;

   if ( (!i_noansi && (strlen(s_instr1) != strlen(strip_all_special(s_instr2)))) ||
        ( i_noansi && (strlen(s_instr1) != strlen(s_instr2))) ) {
      free_lbuf(s_instr1);
      free_lbuf(s_instr2);
      safe_str("#-1 FIND AND REPLACE LISTS MUST BE OF SAME LENGTH.", buff, bufcx);
      return;
   }

   s_ptr = s_instr1;

   for ( i = 0; i < 256; i++ ) {
      s_chrmap[i] = (char)i;
   }

   if ( !i_noansi ) {
      initialize_ansisplitter(outsplit, LBUF_SIZE);
      initialize_ansisplitter(outsplit2, LBUF_SIZE);
      initialize_ansisplitter(s_splitarg2, LBUF_SIZE);
      initialize_ansisplitter(s_splitmap, 256);
      outbuff = alloc_lbuf("fun_tr");
      outarg2 = alloc_lbuf("fun_tr2");
      memset(outbuff, '\0', LBUF_SIZE);
      split_ansi(strip_ansi(fargs[0]), outbuff, outsplit);
      split_ansi(strip_ansi(s_instr2), outarg2, s_splitarg2);
      s_inptr = outarg2;
      p_sp = s_splitarg2;
      while ( *s_ptr ) {
         s_chrmap[(int)*s_ptr]=*s_inptr;
         clone_ansisplitter(&(s_splitmap[(int)*s_ptr]), p_sp);
         s_ptr++;
         s_inptr++;
         p_sp++;
      }
      free_lbuf(outarg2);
   } else {
      s_inptr = s_instr2;
      while ( *s_ptr ) {
         s_chrmap[(int)*s_ptr]=*s_inptr;
         s_ptr++;
         s_inptr++;
      }
   }

   if ( i_noansi ) {
      s_ptr = fargs[0];
   } else {
      s_ptr = outbuff;
   }
   p_sp  = outsplit;
   p_sp2 = outsplit2;   
   s_inptr = s_holdstr = alloc_lbuf("fun_tr_str3");
   while ( *s_ptr ) {
      if ( s_chrmap[(int)*s_ptr] == '\n' ) {
         safe_chr('\r', s_holdstr, &s_inptr);
         if ( !i_noansi ) {
            clone_ansisplitter_two(p_sp2, &(s_splitmap[(int)*s_ptr]), p_sp);
         }
         p_sp2++;
      }
      if ( !i_noansi )
         clone_ansisplitter_two(p_sp2, &(s_splitmap[(int)*s_ptr]), p_sp);
      safe_chr(s_chrmap[(int)*s_ptr], s_holdstr, &s_inptr);
      s_ptr++;
      p_sp++;
      p_sp2++;
   }
   if ( !i_noansi ) {
      s_output = rebuild_ansi(s_holdstr, outsplit2);
      safe_str(s_output, buff, bufcx);
      free_lbuf(s_output);
      free_lbuf(outbuff);
   } else {
      safe_str(s_holdstr, buff, bufcx);
   }
   free_lbuf(s_holdstr);
   free_lbuf(s_instr1);
   free_lbuf(s_instr2);
}

FUNCTION(fun_t)
{
    char *bp;
    bp = fargs[0];

    if (!bp || !*bp) {
      ival(buff, bufcx, 0);
      return;
    }
    if (*bp == '#' && *(bp+1) == '-') {
      ival(buff, bufcx, 0);
      return;
    }
    if ( (*bp == '0') && !*(bp+1)) {
      ival(buff, bufcx, 0);
      return;
    }
    while (*bp == ' ' && *bp)
      bp++;
    if (*bp)
       ival(buff, bufcx, 1);
    else
       ival(buff, bufcx, 0);

}

FUNCTION(fun_tohex)
{
    char tempbuff[SBUF_SIZE];
    sprintf(tempbuff, "%X", convnum(fargs[0]));
    safe_str(tempbuff, buff, bufcx);
}

FUNCTION(fun_todec)
{
    ival(buff, bufcx, convnum(fargs[0]));
}

FUNCTION(fun_tooct)
{
    char tempbuff[SBUF_SIZE];
    sprintf(tempbuff, "%o", convnum(fargs[0]));
    safe_str(tempbuff, buff, bufcx);
}

FUNCTION(fun_tobin)
{
    unsigned int val;
    int x;

    val = convnum(fargs[0]);

    for( x = 31; x >= 0; x--) {
      if( (val >> x) & 0x01 ) {
        safe_chr('1', buff, bufcx);
      }
      else {
        safe_chr('0', buff, bufcx);
      }
    }
}

/* ---------------------------------------------------------------------------
 * fun_translate: Takes a string and a second argument. If the second argument
 * is 0 or s, control characters are converted to spaces. If it's 1 or p,
 * they're converted to percent substitutions.
 * Function borrowed from MUX 2.0 with permission.
 * Function modified for RhostMUSH
 */

FUNCTION(fun_translate)
{
    int type = 0;
    char *result;

    if (fargs[0] && fargs[1]) {
        if (*fargs[1] && ((*fargs[1] == 's') || (*fargs[1] == '0')))
            type = 0;
        else if (*fargs[1] && ((*fargs[1] == 'p') || (*fargs[1] == '1')))
            type = 1;
        result = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0], cargs, ncargs);
        safe_str(translate_string(result, type), buff, bufcx);
        free_lbuf(result);
    }
}

FUNCTION(fun_mwords)
{
   char *sep, *pos, *poschk;
   int i_count = 0;

   if (!fn_range_check("MWORDS", nfargs, 0, 2, buff, bufcx))
      return;
   
   if ( (nfargs < 1) || !*fargs[0] ) {
      ival(buff, bufcx, i_count);
      return;
   } 

   if ( (nfargs > 1) && *fargs[1] ) {
      sep = fargs[1];
   } else {
      sep = (char *)" ";
   }

   pos = strip_all_ansi(fargs[0]);
   i_count++;
   while ( (poschk = strstr(pos, sep)) != NULL ) {
      pos = poschk + strlen(sep);
      i_count++;
      if ( !pos || !*pos )
         break;
   }

   ival(buff, bufcx, i_count);
}

FUNCTION(fun_words)
{
    char sep;

    if (nfargs == 0) {
       safe_str("0", buff, bufcx);
       return;
    }
    varargs_preamble("WORDS", 2);
    ival(buff, bufcx, countwords(fargs[0], sep));
}

FUNCTION(fun_totcmds)
{
   DESC *d;
   dbref target, aowner;
   int tot_cmds, aflags;
   char *buf;

   tot_cmds=0;
   target = lookup_player(player, fargs[0], 0);
   if ( !Good_obj(target) || target == NOTHING || target == AMBIGUOUS ) {
      safe_str("-1", buff, bufcx);
   }
   else {
      buf = atr_get(target, A_TOTCMDS, &aflags, &aowner);
      if (*buf)
         tot_cmds = atoi(buf);
      free_lbuf(buf);
      if (Connected(target) && (!Cloak(target) || Wizard(player)) &&
          (!(SCloak(target) && Cloak(target)) || Immortal(player)) ) {
         DESC_ITER_CONN(d) {
            if (d->player == target)
               tot_cmds += d->command_count;
         }
         if ( tot_cmds > 2000000000 )
            tot_cmds = 2000000000;
      }
      else if ( !(!Connected(target) && (!Cloak(target) || Wizard(player)) &&
          (!(SCloak(target) && Cloak(target)) || Immortal(player))) ) {
         tot_cmds = -1;
      }
      ival(buff, bufcx, tot_cmds);
   }
}

FUNCTION(fun_doing)
{
   DESC *d;
   dbref target;
   int portid;

   portid = 0;
   if (!fn_range_check("DOING", nfargs, 0, 2, buff, bufcx)) {
      return;
   } else if ( nfargs == 2 ) {
      portid = atoi(fargs[1]);
   }

   if ( !stricmp(fargs[0],"/HEADER") || !*fargs[0] ) {
      if (mudconf.who_default)
         safe_str(mudstate.ng_doing_hdr, buff, bufcx);
      else
         safe_str(mudstate.doing_hdr, buff, bufcx);
      return;
   }

   target = lookup_player(player, fargs[0], 0);
   if ( !Good_obj(target) || target == NOTHING || target == AMBIGUOUS ) {
      return;
   } else if (Connected(target) && (!Cloak(target) || Wizard(player)) &&
             (!(SCloak(target) && Cloak(target)) || Immortal(player)) ) {
      DESC_ITER_CONN(d) {
         if (d->player == target) {
            if ( (d->descriptor == portid) && (nfargs == 2) ) {
               safe_str(d->doing, buff, bufcx);
               break;
            } else if (nfargs != 2) {
               safe_str(d->doing, buff, bufcx);
               break;
            }
         }
      }
   } else {
      return;
   }
}

/* Work on charin and charout... needs work and thought */
FUNCTION(fun_charin)
{
   DESC *d;
   dbref target, aowner;
   int cmd, aflags, i_type, i_loop;
   char *buf, tmp_buff[LBUF_SIZE+1];

   if (!fn_range_check("CHARIN", nfargs, 1, 2, buff, bufcx)) {
      return;
   } else if (nfargs == 2) {
      i_type = atoi(fargs[1]);
   } else {
      i_type = 0;
   }
   cmd=0;
   target = lookup_player(player, fargs[0], 0);
   if ( !Good_obj(target) || target == NOTHING || target == AMBIGUOUS ) {
      safe_str("-1", buff, bufcx);
   }
   else {
      if (Connected(target) && (!Cloak(target) || Wizard(player)) &&
          (!(SCloak(target) && Cloak(target)) || Immortal(player)) ) {
         i_loop = 0;
         DESC_ITER_CONN(d) {
            if (d->player == target) {
               if ( i_type == 0 )
                  cmd += d->input_tot;
               else {
                  if (i_loop)
                     safe_chr(' ', buff, bufcx);
                  sprintf(tmp_buff, "%d:%d", d->descriptor, d->input_tot);
                  safe_str(tmp_buff, buff, bufcx);
                  i_loop=1;
               }
            }
         }
      }
      else if ( !Connected(target) && (!Cloak(target) || Wizard(player)) &&
          (!(SCloak(target) && Cloak(target)) || Immortal(player)) ) {
         buf = atr_get(target, A_TOTCHARIN, &aflags, &aowner);
         if (*buf)
            cmd = atoi(buf);
         free_lbuf(buf);
         i_type = 0;
      }
      else {
         cmd = -1;
         i_type = 0;
      }

      if ( i_type == 0 )
         ival(buff, bufcx, cmd);
   }
}

FUNCTION(fun_charout)
{
   DESC *d;
   dbref target, aowner;
   int cmd, aflags, i_type, i_loop;
   char *buf, tmp_buff[LBUF_SIZE + 1];

   if (!fn_range_check("CHAROUT", nfargs, 1, 2, buff, bufcx)) {
      return;
   } else if (nfargs == 2) {
      i_type = atoi(fargs[1]);
   } else {
      i_type = 0;
   }
   cmd=0;
   target = lookup_player(player, fargs[0], 0);
   if ( !Good_obj(target) || target == NOTHING || target == AMBIGUOUS ) {
      safe_str("-1", buff, bufcx);
   }
   else {
      if (Connected(target) && (!Cloak(target) || Wizard(player)) &&
          (!(SCloak(target) && Cloak(target)) || Immortal(player)) ) {
         i_loop = 0;
         DESC_ITER_CONN(d) {
            if (d->player == target) {
               if ( i_type == 0 )
                  cmd += d->output_tot;
               else {
                  if (i_loop)
                     safe_chr(' ', buff, bufcx);
                  sprintf(tmp_buff, "%d:%d", d->descriptor, d->output_tot);
                  safe_str(tmp_buff, buff, bufcx);
                  i_loop=1;
               }
            }
         }
      }
      else if ( !Connected(target) && (!Cloak(target) || Wizard(player)) &&
          (!(SCloak(target) && Cloak(target)) || Immortal(player)) ) {
         buf = atr_get(target, A_TOTCHAROUT, &aflags, &aowner);
         if (*buf)
            cmd = atoi(buf);
         free_lbuf(buf);
         i_type = 0;
      }
      else {
         cmd = -1;
         i_type = 0;
      }

      if ( i_type == 0 )
         ival(buff, bufcx, cmd);
   }
}


FUNCTION(fun_cmds)
{
   DESC *d;
   dbref target, aowner;
   int cmd, aflags, i_type, i_loop, d_type, found;
   char *buf, tmp_buff[LBUF_SIZE + 1];

   if (!fn_range_check("CMDS", nfargs, 1, 3, buff, bufcx)) {
      return;
   } else if (nfargs >= 2) {
      i_type = atoi(fargs[1]);
   } else {
      i_type = 0;
   }
   if ( i_type != 0 )
      i_type = 1;
   if ( (nfargs >= 3) && *fargs[2]) {
      d_type = atoi(fargs[2]);
   } else {
      d_type = -1;
   }
   cmd=0;
   target = lookup_player(player, fargs[0], 0);
   if ( !Good_obj(target) || target == NOTHING || target == AMBIGUOUS ) {
      safe_str("-1", buff, bufcx);
   } else {
      if (Connected(target) && (!Cloak(target) || Wizard(player)) &&
          (!(SCloak(target) && Cloak(target)) || Immortal(player)) &&
          ((target == player) || Wizard(player)) ) {
         i_loop=0;
         found = 0;
         DESC_ITER_CONN(d) {
            if (d->player == target) {
               if ( i_type == 0 ) {
                  found = 1;
                  cmd += d->command_count;
               } else {
                  if ( (d_type == d->descriptor) ) {
                     cmd = d->command_count;
                     /* Fake single mode */
                     i_type = 0;
                     found = 1;
                     break;
                  } else if ( (i_type == 1) && (d_type == -1) ) {
                     if (i_loop)
                        safe_chr(' ', buff, bufcx);
                     sprintf(tmp_buff, "%d:%d", d->descriptor, d->command_count);
                     safe_str(tmp_buff, buff, bufcx);
                     i_loop=1;
                     found = 1;
                  }
               }
            }
         }
      } else if ( !Connected(target) && (!Cloak(target) || Wizard(player)) &&
          (!(SCloak(target) && Cloak(target)) || Immortal(player)) ) {
         buf = atr_get(target, A_LSTCMDS, &aflags, &aowner);
         if (*buf)
            cmd = atoi(buf);
         free_lbuf(buf);
         i_type=0;
         found = 1;
      } else {
         cmd = -1;
         i_type=0;
         found = 1;
      }
      if ( (i_type == 0) && (found) )
         ival(buff, bufcx, cmd);
      if ( !found )
         ival(buff, bufcx, -1);
   }
}

/* This function derivived from the pom.c code by Berkley */
FUNCTION(fun_moon)
{
   struct tm *GMT, *ttm;
   double days, today, tomorrow, dd_now;
   long l_offset;
   char *tpr_buff, *tprp_buff;
   int cnt;


   if (!fn_range_check("MOON", nfargs, 1, 2, buff, bufcx))
      return;

   if ( *fargs[0] )
      dd_now = safe_atof(fargs[0]);
   else
      dd_now = (double)mudstate.now;

   ttm = localtime(&mudstate.now);
   l_offset = (long) mktime(ttm) - (long) mktime64(ttm);
   dd_now -= l_offset;
   GMT = gmtime64_r(&dd_now, ttm);
   days = (GMT->tm_yday + 1.0) + ((GMT->tm_hour +
          (GMT->tm_min / 60.0) + (GMT->tm_sec / 3600.0)) / 24.0);
   for (cnt = EPOCH_MUSH; cnt < GMT->tm_year; ++cnt)
      days += isleap(cnt) ? 366 : 365;
   today = potm_mush(days + 0.5);
   tomorrow = potm_mush((days + 1.0));
   if ( (nfargs > 1) && *fargs[1] && (atoi(fargs[1]) > 0) ) {
       tpr_buff = tprp_buff = alloc_lbuf("fun_moon");
       if ( tomorrow > today )
          safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%1.0f", today), buff, bufcx);
       else
          safe_str(safe_tprintf(tpr_buff, &tprp_buff, "-%1.0f", today), buff, bufcx);
       free_lbuf(tpr_buff);
       return;
   }
   safe_str("The Moon is ", buff, bufcx);
   if (today > 99.4)
      safe_str("Full", buff, bufcx);
   else if (today < 0.5)
      safe_str("New", buff, bufcx);
   else {
      /* if (today == 50.0) */
      tpr_buff = tprp_buff = alloc_lbuf("fun_moon");
      if ( (today > 49.7) && (today < 50.3) )
          safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%s (%1.0f%% of Full)", tomorrow > today ?
                           "Waxing Half" : "Waning Half", today), buff, bufcx);
      /*
       * This WAS "First Quarter" : "Last Quarter" and didn't display percentages
       * at all for half moons
       * safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%s", tomorrow > today ?
       *                 "at the First Quarter" : "at the Last Quarter"), buff, bufcx);
       */
      else {
         safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%s ", tomorrow > today ?  "Waxing" : "Waning"), buff, bufcx);
         if (today > 50.0)
            safe_str(safe_tprintf(tpr_buff, &tprp_buff, "Gibbous (%1.0f%% of Full)", today), buff, bufcx);
         else if (today < 50.0)
            safe_str(safe_tprintf(tpr_buff, &tprp_buff, "Crescent (%1.0f%% of Full)", today), buff, bufcx);
      }
      free_lbuf(tpr_buff);
   }
}

/************************************************************************************
 * fun_pack   - pack into 64 base number - taken from MUX2 with permission and
 *              heavilly modified for Rhost
 * fun_unpack - unpack from 64 base number - taken from MUX2 with permission
 * fun_crc32  - use crc32 bitmath - taken from MUX2 with permission
 ************************************************************************************/
static char aRadixTable[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz@$";
static char aRadixTablePenn[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static char aRadixTablePenn32[]="0123456789abcdefghijklmnopqrstuvwxyz";
FUNCTION(fun_unpack)
{
   int iRadix, c, LeadingCharacter, iValue, i, i_penn;
   double sum;
   char MatchTable[256], *pString, cutoff[65];

   if (!fn_range_check("UNPACK", nfargs, 1, 3, buff, bufcx))
      return;
   iRadix = 64;
   if ( nfargs >= 2 ) {
      if ( !is_integer(fargs[1]) ||
           ((iRadix = atoi(fargs[1])) < 2) ||
           (64 < iRadix) ) {
         safe_str("#-1 RADIX MUST BE A NUMBER BETWEEN 2 and 64", buff, bufcx);
         return;
      }
   }
 
   i_penn = 0;
   if ( nfargs >= 3 ) {
      if ( *fargs[2] ) {
         i_penn = atoi(fargs[2]);
         if ( i_penn != 0 ) {
            i_penn = 1;
         }
      }
   }

   memset(MatchTable, 0, sizeof(MatchTable));
   if ( i_penn ) {
      if ( iRadix <= 36 ) {
         for (i = 0; i < iRadix; i++) {
            MatchTable[(unsigned int) aRadixTablePenn32[i]] = i+1;
         }
      } else {
         for (i = 0; i < iRadix; i++) {
            MatchTable[(unsigned int) aRadixTablePenn[i]] = i+1;
         }
      }
   } else {
      for (i = 0; i < iRadix; i++) {
         MatchTable[(unsigned int) aRadixTable[i]] = i+1;
      }
   }
   memset(cutoff, 0, sizeof(cutoff));
   sprintf(cutoff, "%64.64s", fargs[0]);
   pString = cutoff;
   while ( isspace((int)*pString) ) {
      pString++;
   }
   LeadingCharacter = c = *pString++;
   if (c == '-' || c == '+') {
      c = *pString++;
   }
   sum = 0;
   while ((iValue = MatchTable[(unsigned int)c])) {
      sum = (double)iRadix * sum + (double)iValue - (double)1;
      c = *pString++;
   }

   if (LeadingCharacter == '-') {
      sum = -sum;
   }

   fval(buff, bufcx, sum);
}

FUNCTION(fun_pack)
{
   int iRadix, i_penn;
   double val, iDiv, iTerm;
   char TempBuffer[80], *p, *q, temp, cutoff[64];

   if (!fn_range_check("PACK", nfargs, 1, 3, buff, bufcx))
      return;
   if ( !is_integer(fargs[0]) ||
        ((nfargs >= 2) && !is_integer(fargs[1])) ) {
      safe_str("#-1 ARGUMENTS MUST BE NUMBERS", buff, bufcx);
      return;
   }
   memset(cutoff, 0, sizeof(cutoff));
   sprintf(cutoff, "%19.19s", fargs[0]);
   val = safe_atof(cutoff);
   iRadix = 64;
   if (nfargs >= 2) {
      iRadix = atoi(fargs[1]);
      if ((iRadix < 2) || (64 < iRadix)) {
         safe_str("#-1 RADIX MUST BE A NUMBER BETWEEN 2 and 64", buff, bufcx);
         return;
      }
   }
   i_penn = 0;
   if ( nfargs >= 3 ) {
      if ( *fargs[2] ) {
         i_penn = atoi(fargs[2]);
         if ( i_penn != 0 ) {
            i_penn = 1;
            if ( iRadix > 36 ) {
               i_penn = 2;
            }
         }
      }
   }

   memset(TempBuffer, 0, sizeof(TempBuffer));
   p = TempBuffer;
   if (val < 0) {
      *p++ = '-';
      val = -val;
   }
   q = p;
   while (val > iRadix-1) {
      iDiv  = floor(val / (double)iRadix);
      iTerm = val - iDiv * (double)iRadix;
      val = iDiv;
      if ( iTerm > (iRadix - 1))
         iTerm = iRadix - 1;
      if ( iTerm < 0 )
         iTerm = 0;
      if ( i_penn >= 1 ) {
         if ( iRadix <= 36 ) 
            *p++ = aRadixTablePenn32[(int)iTerm];
         else
            *p++ = aRadixTablePenn[(int)iTerm];
      } else {
         *p++ = aRadixTable[(int)iTerm];
      }
   }
   if ( val < 0 )
      val = 0;
   if ( val > (iRadix - 1) )
      val = iRadix - 1;
   if ( i_penn == 2 ) {
      *p++ = aRadixTablePenn[(int)val];
   } else {
      *p++ = aRadixTable[(int)val];
   }
   *p-- = '\0';
   while (q < p) {
      temp = *p;
      *p = *q;
      *q = temp;

      --p;
      ++q;

  }
  if ( (i_penn == 1) ) {
     p = TempBuffer;
     while ( p && *p ) {
        safe_chr(ToLower(*p), buff, bufcx);
        p++;
     }
  } else {
     safe_str(TempBuffer, buff, bufcx);
  }
}

static unsigned int CRC32_Table[256] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
    0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
    0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
    0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
    0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
    0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
    0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
    0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
    0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
    0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
    0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
    0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
    0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
    0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
    0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

unsigned int CRC32_ProcessBuffer( unsigned int ulCrc, const void *arg_pBuffer, unsigned int nBuffer)
{
    unsigned char *pBuffer = (unsigned char *)arg_pBuffer;

    ulCrc = ~ulCrc;
    while (nBuffer) {
       nBuffer--;
       ulCrc  = CRC32_Table[((unsigned char)*pBuffer++) ^ (unsigned char)ulCrc] ^ (ulCrc >> 8);
    }
    return ~ulCrc;
}

FUNCTION(fun_crc32)
{
   unsigned int ulCRC32;
   int i, n;

   ulCRC32 = 0;
   for (i = 0; i < nfargs; i++) {
      n = strlen(fargs[i]);
      ulCRC32 = CRC32_ProcessBuffer(ulCRC32, fargs[i], n);
   }
   uival(buff, bufcx, ulCRC32);
}

FUNCTION(fun_pfind)
{
   dbref thing;

   if (*fargs[0] == '#') {
      thing = match_thing(player, fargs[0]);
      if ( !Good_obj(thing) || (thing == AMBIGUOUS || thing == NOTHING) || Recover(thing) ||
          (!Immortal(player) && Cloak(thing) && SCloak(thing)) ||
           (!Wizard(player) && Cloak(thing)) ) {
          safe_str("#-1 NO MATCH", buff, bufcx);
          return;
      } else {
          dbval(buff, bufcx, thing);
          return;
      }
   }
   if (!((thing = lookup_player(player, fargs[0], 1)) == NOTHING)) {
      if ( !Good_obj(thing) || (thing == AMBIGUOUS || thing == NOTHING) || Recover(thing) ||
          (!Immortal(player) && Cloak(thing) && SCloak(thing)) ||
           (!Wizard(player) && Cloak(thing)) ) {
          safe_str("#-1 NO MATCH", buff, bufcx);
          return;
      } else {
          dbval(buff, bufcx, thing);
          return;
      }
   } else {
      safe_str("#-1 NO MATCH", buff, bufcx);
   }
}

FUNCTION(fun_pmatch)
{
   dbref thing;
   char *arg_ptr;

   if (*fargs[0] == '#') {
      thing = match_thing(player, fargs[0]);
      if ( !Good_obj(thing) || (thing == AMBIGUOUS || thing == NOTHING) ||
           Recover(thing) || !isPlayer(thing) ||
          (!Immortal(player) && Cloak(thing) && SCloak(thing)) ||
           (!Wizard(player) && Cloak(thing)) ) {
          safe_str("#-1 NO MATCH", buff, bufcx);
          return;
      } else {
          dbval(buff, bufcx, thing);
          return;
      }
   }
   if (*fargs[0] == '*') {
      arg_ptr = fargs[0]+1;
   } else {
      arg_ptr = fargs[0];
   }
   if (!((thing = lookup_player(player, arg_ptr, 1)) == NOTHING)) {
      if ( !Good_obj(thing) || (thing == AMBIGUOUS || thing == NOTHING) || Recover(thing) ||
          !isPlayer(thing) || (!Immortal(player) && Cloak(thing) && SCloak(thing)) ||
           (!Wizard(player) && Cloak(thing)) ) {
          safe_str("#-1 NO MATCH", buff, bufcx);
          return;
      } else {
          dbval(buff, bufcx, thing);
          return;
      }
   } else {
      safe_str("#-1 NO MATCH", buff, bufcx);
   }
}

FUNCTION(fun_pid)
{
   int i_keyval;
   char sep, c_type;

   if (!fn_range_check("PID", nfargs, 1, 4, buff, bufcx)) 
      return;

   i_keyval = atoi(fargs[0]);
   
   if ( (nfargs > 1) && *fargs[1] )
      sep = *fargs[1];
   else
      sep = ' ';
   if ( (nfargs > 2) && *fargs[2] )
      c_type = tolower(*fargs[2]);
   else
      c_type = 'q';
   if ( nfargs > 3 )
      show_que_func(player, fargs[3], i_keyval, c_type, buff, bufcx, sep);
   else
      show_que_func(player, (char *)NULL, i_keyval, c_type, buff, bufcx, sep);
}

FUNCTION(fun_port)
{
    DESC *d;
    dbref it;
    int gotone = 0;

    it = lookup_player(player, fargs[0], 0);
    if (Good_obj(it) && Controls(player, it)) {
       DESC_ITER_CONN(d) {
          if (d->player == it) {
             if (gotone) {
                safe_chr(' ', buff, bufcx);
             }
             gotone = 1;
             ival(buff, bufcx, d->descriptor);
          }
       }
    }
    if ( !gotone )
       safe_str("-1", buff, bufcx);
}

FUNCTION(fun_chkgarbage)
{
   int i, retval;
   dbref thing;

   thing = -1;
   retval = 0;
   if ( (*fargs[0] == '#') && is_number(fargs[0]+1) ) {
      thing=atoi(fargs[0]+1);
   } else {
      ival(buff, bufcx, retval);
      return;
   }
   if ( (*fargs[1] == 'r') || (*fargs[1] == 'R') ) {
      i = mudstate.recoverlist;
   } else {
      i = mudstate.freelist;
   }   
   while ( i != NOTHING ) {
      if ( i == thing ) {
         retval = 1;
         break;
      }
      i = Link(i);
   }
   ival(buff, bufcx, retval);
}

FUNCTION(fun_lookup_site)
{
   DESC *d;
   dbref it, aowner;
   int mush_port = -1,
       gotone = 0,
       c_type = 0,
       aflags;
   char *s_str;

   if (!fn_range_check("LOOKUP_SITE", nfargs, 1, 3, buff, bufcx)) {
      return;
   }

   it = lookup_player(player, fargs[0], 0);
   if ( (nfargs > 1) && is_number(fargs[1]) ) {
      mush_port = atoi(fargs[1]);
      if ( mush_port < 1 ) {
         mush_port = -1;
      }
   } else {
      mush_port = -1;
   }
   if ( (nfargs > 2) && *fargs[2] ) {
      c_type=atoi(fargs[2]);
   }
   if ( (c_type < 0) || (c_type > 1) ) {
      c_type = 0;
   }
   if ( Good_chk(it) && Controls(player, it)) {
      DESC_ITER_CONN(d) {
         if ( d->player == it ) {
            if ( (mush_port == -1) || (d->descriptor == mush_port) ) {
               if ( gotone ) {
                  safe_chr(' ', buff, bufcx);
               }
               gotone = 1;
               if ( c_type == 1 ) {
                  safe_str(inet_ntoa(d->address.sin_addr), buff, bufcx);
               } else {
                  safe_str(d->addr, buff, bufcx);
               }
            }
         }
      }
      if ( !gotone && (mush_port == -1)) {
         if ( c_type == 1 ) {
            s_str=atr_get(it, A_LASTIP, &aowner, &aflags);
         } else {
            s_str=atr_get(it, A_LASTSITE, &aowner, &aflags);
         }
         if ( *s_str ) {
            gotone = 1;
            safe_str(s_str, buff, bufcx);
            free_lbuf(s_str);
         }
      }
   } 
   if ( !gotone ) {
      safe_str("#-1", buff, bufcx);
   }
}

FUNCTION(fun_children)
{
    dbref i, it;
    int gotone, i_max, i_ceil, i_type, i_cntr, i_start, i_startcnt;
    char c_type, *tbuf;

    if (!fn_range_check("CHILDREN", nfargs, 1, 2, buff, bufcx)) {
       return;
    }
    i_type = 0;
    i_cntr = 0;
    c_type = '\0';
    tbuf = NULL;

    if ( (nfargs > 1) && *fargs[1] ) {
       i_type = atoi(fargs[1]);
       c_type = *fargs[1];
    }
    if ( i_type < 0 )
       i_type = 0;

    i_start = i_startcnt = 0;
    if ( (c_type == '~') && (strchr(fargs[1], '-') != NULL) ) {
       i_start = atoi(fargs[1]+1);
       i_startcnt = i_start + atoi(strchr(fargs[1], '-')+1) - 1;
    }
    i_max = (i_type - 1) * 400;
    i_ceil = i_max + 400;
    gotone = 0;
    it = match_thing(player, fargs[0]);
    if (Good_obj(it) && Controls(player, it)) {
       DO_WHOLE_DB(i) {
          if (Parent(i) == it) {
             i_cntr++;
             if ( (i_type && (i_cntr < i_max)) || (c_type == 'l'))
                continue;
             if ( i_type && (i_cntr >= i_ceil) )
                break;
             if ( (i_start > 0) && ((i_cntr < i_start) || (i_cntr > i_startcnt)) )
                continue;
             if (gotone) {
                safe_chr(' ', buff, bufcx);
             } else {
                gotone = 1;
             }
             dbval(buff, bufcx, i);
          }
       }
       if ( c_type == 'l' ) {
          gotone = 1;
          tbuf = alloc_sbuf("fun_lzone");
          sprintf(tbuf, "%d %d", ((i_cntr/400)+1), i_cntr);
          safe_str(tbuf, buff, bufcx);
          free_sbuf(tbuf);
       }
    }
    if (!gotone && (mudconf.mux_child_compat == 0) ) {
         safe_str("#-1", buff, bufcx);
    }
    
}

/*---------------------------------------------------------------------------
 * fun_orflags, fun_andflags:  check multiple flags at the same time.
 * Based off the PennMUSH 1.50 code (and the TinyMUX derivative of it).
 * Modified for use with the new RhostMUSH permission structure
 * type is '0' for orflags '1' for andflags
 */

static int handle_flaglists(dbref player, dbref cause, char *name, char *fstr, int type)
{
    char *s;
    char flag_low[MBUF_SIZE+1], flag_high[MBUF_SIZE+1];
    int high_bit, esc_chr, negate, temp, ret;
    dbref it;

    memset(flag_low, 0, sizeof(flag_low));
    memset(flag_high, 0, sizeof(flag_high));
    it  = match_thing(player, name);

    if ((it != NOTHING) && (it != AMBIGUOUS) && (!Cloak(it) || (Cloak(it) && (Examinable(player, it) || Wizard(player)))) &&
        (!(SCloak(it) && Cloak(it)) || (SCloak(it) && Cloak(it) && Immortal(player))) &&
        (mudconf.pub_flags || Examinable(player, it) || (it == cause))) {
        unparse_fun_flags(player, it, flag_low, flag_high);
    } else
        return 0;

    ret = type;
    negate = esc_chr = temp = high_bit = 0;

    for (s = fstr; *s; s++) {

        /* Check for a negation sign. If we find it, we note it and
         * increment the pointer to the next character.
         */

        if (!*s)
           return 0;

        if (*s == '\\' || *s == '%') {
           esc_chr = 1;
           continue;
        }
        if (*s == '!' && !esc_chr ) {
           negate = 1;
           continue;
        }
        if (*s == '2' && !esc_chr) {
           high_bit = 1;
           continue;
        }
        if (*s == '1' && !esc_chr) {
           high_bit = 0;
           continue;
        }

        if ( type ) {
           if ( negate ) {
              if ( high_bit )
                 ret = !((pmath2)strchr(flag_high, *s));
              else
                 ret = !((pmath2)strchr(flag_low, *s));
            } else {
              if ( high_bit )
                 ret = (pmath2)strchr(flag_high, *s);
              else
                 ret = (pmath2)strchr(flag_low, *s);
            }
            if ( !ret )
               return 0;
        } else {
           if ( negate ) {
              if ( high_bit )
                 ret = !((pmath2)strchr(flag_high, *s));
              else
                 ret = !((pmath2)strchr(flag_low, *s));
            } else {
              if ( high_bit )
                 ret = (pmath2)strchr(flag_high, *s);
              else
                 ret = (pmath2)strchr(flag_low, *s);
            }
            if ( ret )
               return 1;
        }
  esc_chr = negate = 0;
    }
    return (ret ? 1 : 0);
}

FUNCTION(fun_orflags)
{
    fval(buff, bufcx, handle_flaglists(player, cause, fargs[0], fargs[1], 0));
}

FUNCTION(fun_andflags)
{
    fval(buff, bufcx, handle_flaglists(player, cause, fargs[0], fargs[1], 1));
}

FUNCTION(fun_keepflags)
{
   char *pt1, *pt2, *s_tok, *s_tprintf, *s_tprintfptr, sep, osep, sep_buf[2];
   int first, i_type, obj;

   svarargs_preamble("KEEPFLAGS", 4);

   if ( !fargs[0] || !fargs[1] || !*fargs[0] || !*fargs[1] )
      return;

   first = i_type = 0;
   if ( *fargs[1] == '&' ) {
      i_type = 1;
   } else if ( *fargs[1] == '|' ) {
      i_type = 0;
   } else {
      safe_str("#-1 FUNCTION REQUIRES '&' OR '|' FOR FLAG MATCHES", buff, bufcx);
      return;
   }
   if ( sep == '\0' )
      sep = ' ';
   if ( osep == '\0' )
      osep = sep;
   sep_buf[0] = sep;
   sep_buf[1] = '\0';
   pt2 = fargs[1]+1;
   pt1 = strtok_r(fargs[0], sep_buf, &s_tok);
   s_tprintfptr = s_tprintf = alloc_lbuf("safe_tprintf_keepflags");
   while (pt1) {
        init_match(player, pt1, NOTYPE);
        match_everything(MAT_EXIT_PARENTS);
        obj = noisy_match_result();
        if ( !Good_obj(obj) ) {
           pt1 = strtok_r(NULL, sep_buf, &s_tok);
           continue;
        }
        if ( handle_flaglists(player, cause, safe_tprintf(s_tprintf, &s_tprintfptr, "#%d", obj), pt2, i_type) ) {
           if ( first )
              safe_chr(osep, buff, bufcx);
           safe_str(safe_tprintf(s_tprintf, &s_tprintfptr, "#%d", obj), buff, bufcx);
           first = 1;
        }
        pt1 = strtok_r(NULL, sep_buf, &s_tok);
   }
   free_lbuf(s_tprintf);
}

FUNCTION(fun_remflags)
{
   char *pt1, *pt2, *s_tok, *s_tprintf, *s_tprintfptr, sep, osep, sep_buf[2];
   int first, i_type, obj;

   svarargs_preamble("REMFLAGS", 4);

   if ( !fargs[0] || !fargs[1] || !*fargs[0] || !*fargs[1] )
      return;

   first = i_type = 0;
   if ( *fargs[1] == '&' ) {
      i_type = 1;
   } else if ( *fargs[1] == '|' ) {
      i_type = 0;
   } else {
      safe_str("#-1 FUNCTION REQUIRES '&' OR '|' FOR FLAG MATCHES", buff, bufcx);
      return;
   }
   if ( sep == '\0' )
      sep = ' ';
   if ( osep == '\0' )
      osep = sep;
   sep_buf[0] = sep;
   sep_buf[1] = '\0';
   pt2 = fargs[1]+1;
   pt1 = strtok_r(fargs[0], sep_buf, &s_tok);
   s_tprintfptr = s_tprintf = alloc_lbuf("safe_tprintf_keepflags");
   while (pt1) {
        init_match(player, pt1, NOTYPE);
        match_everything(MAT_EXIT_PARENTS);
        obj = noisy_match_result();
        if ( !Good_obj(obj) ) {
           pt1 = strtok_r(NULL, sep_buf, &s_tok);
           continue;
        }
        if ( !handle_flaglists(player, cause, safe_tprintf(s_tprintf, &s_tprintfptr, "#%d", obj), pt2, i_type) ) {
           if ( first )
              safe_chr(osep, buff, bufcx);
           safe_str(safe_tprintf(s_tprintf, &s_tprintfptr, "#%d", obj), buff, bufcx);
           first = 1;
        }
        pt1 = strtok_r(NULL, sep_buf, &s_tok);
   }
   free_lbuf(s_tprintf);
}

/* ---------------------------------------------------------------------------
 * fun_foreach - works like map() in args it takes
 * Taken from MUX 2.0 with permission
 */

FUNCTION(fun_foreach)
{
    dbref aowner, thing;
    int aflags, anum, tval, flag = 0;
    ATTR *ap;
    char *atext, *atextbuf, *str, *cp, *bp, *result;
    char cbuf[2], prev = '\0';

    if (!fn_range_check("FOREACH", nfargs, 2, 4, buff, bufcx)) {
        return;
    }

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
        if ((anum == NOTHING) || !Good_obj(thing))
            ap = NULL;
        else
            ap = atr_num(anum);
    } else {
        thing = player;
        ap = atr_str(fargs[0]);
    }

    if (!ap) {
        return;
    }
    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
        return;
    } else if (!*atext || !See_attr(player, thing, ap, aowner, aflags, 0)) {
        free_lbuf(atext);
        return;
    }
    atextbuf = alloc_lbuf("fun_foreach");
    cp = trim_space_sep(strip_all_special(fargs[1]), ' ');

    bp = cbuf;

    cbuf[1] = '\0';

    if (nfargs >= 3) {
        while (cp && *cp) {
            cbuf[0] = *cp++;

            if (flag) {
                if ( (nfargs >= 4) && (cbuf[0] == *fargs[3]) && (prev != '\\') && (prev != '%')) {
                    flag = 0;
                    continue;
                }
            } else {
                if ((cbuf[0] == *fargs[2]) && (prev != '\\') && (prev != '%')) {
                    flag = 1;
                    continue;
                } else {
                    safe_chr(cbuf[0], buff, bufcx);
                    continue;
                }
            }

            strcpy(atextbuf, atext);
            str = atextbuf;
            if ( (mudconf.secure_functions & 1) ) {
               tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
               if ( tval == -2 ) {
                  result = alloc_lbuf("edefault_buff");
                  sprintf(result ,"#-1 PERMISSION DENIED");
               } else {
                  result = exec(thing, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, str, &bp, 1);
               }
            } else {
               tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
               if ( tval == -2 ) {
                  result = alloc_lbuf("edefault_buff");
                  sprintf(result ,"#-1 PERMISSION DENIED");
               } else {
                  result = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, str, &bp, 1);
               }
            }
            safer_unufun(tval);
            safe_str(result, buff, bufcx);
            free_lbuf(result);
            prev = cbuf[0];
        }
    } else {
        while (cp && *cp) {
            cbuf[0] = *cp++;

            strcpy(atextbuf, atext);
            str = atextbuf;
            if ( (mudconf.secure_functions & 1) ) {
               tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
               if ( tval == -2 ) {
                  result = alloc_lbuf("edefault_buff");
                  sprintf(result ,"#-1 PERMISSION DENIED");
               } else {
                  result = exec(thing, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, str, &bp, 1);
               }
            } else {
               tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
               if ( tval == -2 ) {
                  result = alloc_lbuf("edefault_buff");
                  sprintf(result ,"#-1 PERMISSION DENIED");
               } else {
                  result = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, str, &bp, 1);
               }
            }
            safer_unufun(tval);
            safe_str(result, buff, bufcx);
            free_lbuf(result);
        }
    }

    free_lbuf(atextbuf);
    free_lbuf(atext);
}



/* ---------------------------------------------------------------------------
 * fun_flags: Returns the flags on an object.
 * Because @switch is case-insensitive, not quite as useful as it could be.
 */
FUNCTION(fun_flags)
{
    dbref it;
    char *buff2;
    OBLOCKMASTER master;

    olist_init(&master);
    if (parse_attrib_wild(player, fargs[0], &it, 0, 1, 0, &master, 0, 0, 0)) {
       if ((it != NOTHING) && (it != AMBIGUOUS) && (!Cloak(it) || (Cloak(it) && Examinable(player, it))) &&
                 (mudconf.pub_flags || Examinable(player, it) || (it == cause))) {
          parse_aflags(player, it, olist_first(&master), buff, bufcx, 0);
       }
       olist_cleanup(&master);
       return;
    }
    olist_cleanup(&master);
    init_match(player, fargs[0], NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    it = match_result();
    if ((it != NOTHING) && (it != AMBIGUOUS) && (!Cloak(it) || (Cloak(it) && (Examinable(player, it) || Wizard(player)))) &&
            (!(SCloak(it) && Cloak(it)) || (SCloak(it) && Cloak(it) && Immortal(player))) &&
            (mudconf.pub_flags || Examinable(player, it) || (it == cause))) {
       buff2 = unparse_flags(player, it);
       safe_str(buff2, buff, bufcx);
       free_mbuf(buff2);
    } else
       safe_str("#-1", buff, bufcx);
    return;
}

FUNCTION(fun_garble)
{
  char sep, sup, *pt1, *pt2, *s_retval, *s_tmpvalue;
  int num, x, sip, end, i_punct, i_space, i_string, i_count, i_numchars;

  if (!fn_range_check("GARBLE", nfargs, 2, 7, buff, bufcx)) {
     return;
  } else {
     i_string = 0;
     sep = ' ';
     sup = 0;
     sip = 0;
     i_count = i_numchars = 0;
     num = atoi(fargs[1]);
     if ( nfargs >= 5 )
        sip = atoi(fargs[4]);
     if ( nfargs >= 6 ) {
        i_space = atoi(fargs[5]);
        if ( i_space > 4 ) {
           i_count = 1;
           i_space -= 4;
        }
     } else
        i_space = 0;
     pt1 = fargs[0];
     if (nfargs > 2) {
        if (*fargs[2])
           sep = *fargs[2];
        if (nfargs >= 4)
           sup = *fargs[3];
     }
     if ( i_space > 3) {
        if ( (nfargs > 6) && *fargs[6] )
           i_space = 4;
        else
           i_space = 0;
     }
     if ( i_space == 4 ) {
        i_string = 1;
        i_space = 0;
        s_tmpvalue = alloc_lbuf("garble_tmp");
     }
     if ( i_space & 2 )
        i_punct = 1;
     else
        i_punct = 0;
     i_space = (i_space & 1);
     while (*pt1 && (num > 0)) {
        pt2 = pt1+1;
        while (*pt2 && (*pt2 != sep)) pt2++;
        if (*pt2) {
           end = 0;
           *pt2 = '\0';
        } else
           end = 1;

      /* Modified so that original 1 in X still works, but also added percent mod
       * if sip is equal to '1'.
       */
      if ( mudstate.chkcpu_toggle )
         break;
      if ( (i_string == 1) ) {
         if ( ((!(random() % num) && !sip) || (((random() % 100) < num) && sip)) ) {
            strcpy(s_tmpvalue, fargs[6]);
            s_retval = exec(player, cause, player, EV_STRIP | EV_FCHECK | EV_EVAL, s_tmpvalue,
                            &pt1, 1);
            safe_str(s_retval, buff, bufcx);
            free_lbuf(s_retval);
         } else
            safe_str(pt1, buff, bufcx);
      } else if ( (!(random() % num) && !sip) || (((random() % 100) < num) && sip) )  {
         if (sup) {
            i_numchars = 0;
            for (x = 0; x < (pt2 - pt1); x++) {
               if ( (i_space && (*(pt1+x) == ' ')) || (i_punct && (*(pt1+x) && index("?!.,", *(pt1+x)))) ) {
                  if ( i_count && i_numchars ) {
                     ival(buff, bufcx, i_numchars);
                     i_numchars = 0;
                  }
                  safe_chr(*(pt1+x),buff,bufcx);
               } else {
                  if ( i_count )
                     i_numchars++;
                  else
                     safe_chr(sup,buff,bufcx);
               }
            }
            if ( i_count && i_numchars)
               ival(buff, bufcx, i_numchars);
         } else {
            i_numchars = 0;
            for (x = 0; x < (pt2 - pt1); x++) {
               if ( (i_space && (*(pt1+x) == ' ')) || (i_punct && (*(pt1+x) && index("?!.,", *(pt1+x)))) ) {
                  if ( i_count && i_numchars ) {
                     ival(buff, bufcx, i_numchars);
                     i_numchars = 0;
                  }
                  safe_chr(*(pt1+x),buff,bufcx);
               } else {
                  if ( i_count )
                     i_numchars++;
                  else
                     safe_chr(33 + (random() % 94),buff,bufcx);
               }
            }
            if ( i_count && i_numchars)
               ival(buff, bufcx, i_numchars);
         }
      } else
         safe_str(pt1,buff,bufcx);
      if (!end) {
         safe_chr(' ',buff,bufcx);
         pt1 = pt2+1;
      } else
         pt1 = pt2;
   }
   if ( (num == 0) && (sip == 1) && *pt1 ) {
      safe_str(pt1, buff, bufcx);
   }
   if ( i_string == 1 )
      free_lbuf(s_tmpvalue);
   }
}


/* ---------------------------------------------------------------------------
 * fun_rand: Return a random number from 0 to arg1-1
 */

FUNCTION(fun_rand)
{
    int i_num, i_num2;
    double d_num, d_num2, d_num3;
    static char tempbuff[LBUF_SIZE/2], *fakerand, *fakerandptr;

    if (!fn_range_check("RAND", nfargs, 1, 3, buff, bufcx)) {
      return;
    }

    i_num = i_num2 = 0;
    d_num = d_num2 = d_num3 = 0;
    if ( (nfargs > 2) && *fargs[2] && (atoi(fargs[2]) == 1) ) {
       d_num = safe_atof(fargs[0]);

       if ( (nfargs > 1) && *fargs[1] ) {
          fakerandptr = fakerand = alloc_lbuf("double_rand");
          d_num  = safe_atof(fargs[0]);
          d_num2 = safe_atof(fargs[1]);
          if ( (d_num2 < d_num) || (d_num < 0) ) {
             safe_str("0", buff, bufcx);
             return;
          }
          while ( strlen(fakerand) < 300 ) {
             i_num=random();
             sprintf(tempbuff, "%ld", (long)i_num);
             safe_str(tempbuff, fakerand, &fakerandptr);
             if ( i_num == 0 )
                break;
          }
          d_num3 = safe_atof(fakerand);
          free_lbuf(fakerand);
          if ( d_num2 == d_num ) {
             sprintf(tempbuff, "%.0f", (double)d_num);
          } else {
             sprintf(tempbuff, "%.0f", (double)(fmod(d_num3, ((d_num2 + 1) - d_num)) + d_num) );
          }
       } else {
          if (d_num < 1) {
             safe_str("0", buff, bufcx);
             return;
          } else {
             sprintf(tempbuff, "%.0f", (double)(fmod(d_num3, d_num)) );
          }
       }
       safe_str(tempbuff, buff, bufcx);
    } else {
       i_num = atoi(fargs[0]);

       if ( (nfargs > 1) && *fargs[1] ) {
          i_num2 = atoi(fargs[1]);
          if ( (i_num2 < i_num) || (i_num < 0) ) {
             safe_str("0", buff, bufcx);
             return;
          }
          if ( i_num2 == i_num ) {
             sprintf(tempbuff, "%ld", (long)i_num);
          } else {
             sprintf(tempbuff, "%ld", (long)((random() % ((i_num2 + 1) - i_num)) + i_num) );
          }
       } else {
          if (i_num < 1) {
             safe_str("0", buff, bufcx);
             return;
          } else {
             sprintf(tempbuff, "%ld", (long)(random() % i_num));
          }
       }
       safe_str(tempbuff, buff, bufcx);
    }
}

/* ---------------------------------------------------------------------------
 * fun_abs: Returns the absolute value of its argument.
 */

FUNCTION(fun_abs)
{
    double num;

    num = safe_atof(fargs[0]);
    if (num == 0.0) {
       safe_str("0", buff, bufcx);
    } else if (num < 0.0) {
       fval(buff, bufcx, -num);
    } else {
       fval(buff, bufcx, num);
    }
}

FUNCTION(fun_strip)
{
    char *pt1, *s_tok;
    int i_type = 0;

    if (!fn_range_check("STRIP", nfargs, 2, 3, buff, bufcx))
       return;

    if ( !*fargs[1] ) {
       safe_str(fargs[0], buff, bufcx);
       return;
    }

    if ( (nfargs > 2) && *fargs[2] )
       i_type = atoi(fargs[2]);
   

    if ( !i_type ) {
       pt1 = strtok_r(fargs[0], fargs[1], &s_tok);
       while (pt1) {
          safe_str(pt1, buff, bufcx);
          pt1 = strtok_r(NULL, fargs[1], &s_tok);
       }
    } else {
       pt1 = fargs[0];
       while ( *pt1 ) {
          if ( strchr(fargs[1], *pt1) != NULL )
             safe_chr(*pt1, buff, bufcx);
          pt1++;
       }
    }
}

/* ---------------------------------------------------------------------------
 * fun_sign: Returns -1, 0, or 1 based on the the sign of its argument.
 */

FUNCTION(fun_sign)
{
    double num;

    num = safe_atof(fargs[0]);
    if (num < 0)
       safe_str("-1", buff, bufcx);
    else if (num > 0)
       safe_str("1", buff, bufcx);
    else
       safe_str("0", buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * fun_time: Returns nicely-formatted time.
 */

FUNCTION(fun_time)
{
    char *temp;

    temp = (char *) ctime(&mudstate.now);
    temp[strlen(temp) - 1] = '\0';
    safe_str(temp, buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * fun_timefmt: Returns time formatted any way that the user asks for it
 *
 * timefmt(<fmt>, <secs>)
 *
 * Where <fmt> is a string containing a sequence of any of the following:
 *
 * normal text
 * '$' - format sequence start
 *
 * Stuff that is interpreted following a '$':
 * FOMATTING:
 * '$' - don't treat the previous $ as the start of a format
 * '-' - left justify field
 * Num - Width of next field, numbers begining in '0' denote zero padded fields
 * '!' - Omit field print if 0 value. No further literal strings will be
 *       printed until a literal '!' is encountered.
 *
 * '~' - Omit field as with '!' but preserve field width, ie. blank pad the
 *       field.
 *       No further literal strings will be printed until a literal '~' is
 *       encountered.
 * '@' - Omit field as in '!' but if ANY previous field was 0.
 *       No further literal strings will be printed until a literal '!' is
 *       encountered.
 * '#' - Omit field as in '~' but if ANY previous field was 0.
 *       No further literal strings will be printed until a literal '#' is
 *       encountered.
 *
 * NOTE: Any of the ! ~ @ # characters can be escaped in literal text by
 *       repeating them such as !!
 *
 * TIME:
 * 'H' - hour of day (1..12)
 * 'h' - hour of day (0..23)
 * 'T' - minute in hour (0..59)
 * 'P' - AM/PM
 * 'p' - am/pm
 * 'S' - second in minute (0..59)
 *
 * DATE:
 * 'A' - Weekday name (Sunday..Saturday)
 * 'a' - Abbrev Weekday name (Sun..Sat)
 * 'W' - Day of week index (0..6)
 * 'B' - Month name (January..December)
 * 'b' - Abbrev Month name (Jan..Dec)
 * 'M' - Month number (1..12)
 * 'D' - Day of month (1..31)
 * 'J' - Day of year (1..366)
 * 'Y' - Year number (four digit)
 * 'y' - Year number (two digit)
 * 'z' - '1' if daylight savings is in effect
 * 'i' - Timezone
 * 'I' - offset of timezone in seconds from GMT
 *
 * ELAPSED TIME:
 * 'm' - Elapsed milleniums (0..??)
 * 'U' - Elapsed century in millinium (0..9)
 * 'u' - Elapsed year in century (0..99)
 * 'Z' - Elapsed years (0..??)
 * 'E' - Elapsed months within year (0..12)
 * 'e' - Elapsed months (0..??)
 * 'w' - Elapsed weeks within month (0..5)
 * 'd' - Elapsed days within week (0..6)
 * 'C' - Elapsed days within month (0..31)
 * 'c' - Elapsed days (0..??)
 * 'X' - Elapsed hours within day (0..24)
 * 'x' - Elapsed hours (0..??)
 * 'F' - Elapsed minutes within hour (0..59)
 * 'f' - Elapsed minutes (0..??)
 * 'G' - Elapsed seconds within minute (0..59)
 * 'g' - Elapsed seconds (0..??)
 */

struct timefmt_format {
  int lastval;
  int sawnonzeroval;
  int fieldsupress1; /* 0 val handling, don't show anything */
  int fieldsupress2; /* 0 val handling, show spaces */
  int fieldsupress3; /* any prev 0 val handling, don't show anything */
  int fieldsupress4; /* any prev 0 val handling, show spaces */
  int insup1;        /* for nailing trailing string literals after suppression */
  int insup2;
  int insup3;
  int insup4;
  int supressing;
  int zeropad;       /* pad fieldwidth with zeros */
  int fieldwidth;
  int leftjust;      /* left justify the field */
  int nocutval;	     /* Don't cut the value off */
  int breakonreturn;	/* Break on return identification */
  char format_padch;	/* Character to pad */
  char format_padst[LBUF_SIZE]; /* String for padding -- ansi aware */
  int format_padstsize;
  int formatting;
  int forcebreakonreturn;
  int morepadd;
  int cutatlength;
};

void safe_chr_fm( char ch, char* buff, char** bufcx,
                  struct timefmt_format* fm )
{
  char t_buff[8];

  memset(t_buff, '\0', sizeof(t_buff));
  if ( ((unsigned int)ch >= 160) || 
       ((unsigned int)ch == 28) ||
       ((unsigned int)ch == 29) ) {
     switch( (unsigned int)ch % 256 ) {
        case 28: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)92);
                 break;
        case 29: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)37);
                 break;
        default: sprintf(t_buff, "%c<%03u>", '%', ((unsigned int)ch) % 256);
                 break;
     }
  }
  if( !fm->fieldsupress1 &&
      !fm->fieldsupress2 &&
      !fm->fieldsupress3 &&
      !fm->fieldsupress4 ) {
    if ( *t_buff ) {
       safe_str( t_buff, buff, bufcx );
    } else {
       safe_chr( ch, buff, bufcx );
    }
  }
  else if( fm->fieldsupress1 ) {
    if( fm->lastval ) {
      if ( *t_buff ) {
         safe_str( t_buff, buff, bufcx );
      } else {
         safe_chr( ch, buff, bufcx );
      }
    }
    else {
      fm->supressing = 1;
    }
  }
  else if( fm->fieldsupress3 ) {
    if( fm->sawnonzeroval ) {
      if ( *t_buff ) {
         safe_str( t_buff, buff, bufcx );
      } else {
         safe_chr( ch, buff, bufcx );
      }
    }
    else {
      fm->supressing = 1;
    }
  }
  else if( fm->fieldsupress2 ) {
    if( fm->lastval ) {
      if ( *t_buff ) {
         safe_str( t_buff, buff, bufcx );
      } else {
         safe_chr( ch, buff, bufcx );
      }
    }
    else {
      fm->supressing = 1;
      safe_chr( ' ', buff, bufcx );
    }
  }
  else if( fm->fieldsupress4 ) {
    if( fm->sawnonzeroval ) {
      if ( *t_buff ) {
         safe_str( t_buff, buff, bufcx );
      } else {
         safe_chr( ch, buff, bufcx );
      }
    }
    else {
      fm->supressing = 1;
      safe_chr( ' ', buff, bufcx );
    }
  }
}

void safe_str_fm( char* str, char* buff, char** bufcx,
                 struct timefmt_format* fm )
{
  int idx;

  if( !fm->fieldsupress1 &&
      !fm->fieldsupress2 &&
      !fm->fieldsupress3 &&
      !fm->fieldsupress4 ) {
    safe_str( str, buff, bufcx );
  }
  else if( fm->fieldsupress1 ) {
    if( fm->lastval ) {
      safe_str( str, buff, bufcx );
    }
    else {
      fm->supressing = 1;
    }
  }
  else if( fm->fieldsupress3 ) {
    if( fm->sawnonzeroval ) {
      safe_str( str, buff, bufcx );
    }
    else {
      fm->supressing = 1;
    }
  }
  else if( fm->fieldsupress2 ) {
    if( fm->lastval ) {
      safe_str( str, buff, bufcx );
    }
    else {
      fm->supressing = 1;
      for( idx = strlen(strip_all_special(str)); idx > 0; idx-- ) {
        safe_chr( ' ', buff, bufcx );
      }
    }
  }
  else if( fm->fieldsupress4 ) {
    if( fm->sawnonzeroval ) {
      safe_str( str, buff, bufcx );
    }
    else {
      fm->supressing = 1;
      for( idx = strlen(strip_all_special(str)); idx > 0; idx-- ) {
        safe_chr( ' ', buff, bufcx );
      }
    }
  }
}

void setupfield(struct timefmt_format *fm, int numeric)
{
  if( fm->lastval ) {
    fm->sawnonzeroval = 1;
  }
  if( fm->fieldsupress1 || fm->fieldsupress2 || fm->fieldsupress3 || fm->fieldsupress4 ) {
     fm->supressing = 1;
  }
}

void showfield(char* fmtbuff, char* buff, char** bufcx,
               struct timefmt_format* fm, int numeric)
{   
  int padwidth = 0;
  int currwidth = 0;
  char padch = ' ';
  int idx; 

  if( fm->lastval ) {
    fm->sawnonzeroval = 1;
  }    
       
  if( !fm->fieldwidth ) {
    safe_str_fm( fmtbuff, buff, bufcx, fm );
  }    
  else {
    padwidth = fm->fieldwidth - strlen(fmtbuff);
    if ( fm->format_padch )
       padch=fm->format_padch;
    if( numeric &&
        fm->zeropad ) {
      padch = '0';
    }  
    if( !fm->leftjust ) {
      for( idx = 0; idx < padwidth; idx++, currwidth++ ) {
        safe_chr_fm( padch, buff, bufcx, fm );
      }
    }
    for( ; *fmtbuff && currwidth < fm->fieldwidth; fmtbuff++, currwidth++ ) {
      safe_chr_fm( *fmtbuff, buff, bufcx, fm );
    }
    if( fm->leftjust ) {
      if ( !fm->format_padch )
         padch = ' '; /* don't right-pad with zeros ever */
      for( idx = 0; idx < padwidth; idx++ ) {
        safe_chr_fm( padch, buff, bufcx, fm );
      }
    }
  }
}

void snarfle_special_characters(char *s_instring, char *s_outstring)
{
    char *s, *t;

    s = s_instring;
    t = s_outstring;
    while ( *s ) {
       if ( (*s == '%') && (*(s+1) == '<') && *(s+2) && *(s+3) && *(s+4) &&
            isdigit(*(s+2)) && isdigit(*(s+3)) && isdigit(*(s+4)) &&
            (*(s+5) == '>') ) {
          switch ( atoi(s+2) ) {
             case 92: *t++ = (char) 28;
                      break;
             case 37: *t++ = (char) 29;
                      break;
             default: *t++ = (char) atoi(s+2);
                      break;
          }
          s+=6;
          continue;
       }
       *t++ = *s++;
    }
}

void showfield_printf(char* fmtbuff, char* buff, char** bufcx,
                      struct timefmt_format* fm, int numeric, char *shold, char **sholdptr, int morepadd)
{
  int padwidth = 0;
  int currwidth = 0;
  char padch = ' ', *s_justbuff, *s_pp, *s_padbuf, *s_padbufptr, x1, x2, x3, x4,
       *s_special, *s_normal, *s_accent, s_padstring[LBUF_SIZE], s_padstring2[LBUF_SIZE], *s, *t, *u;
  int idx, idy, i_stripansi, i_nostripansi, i_inansi, i_spacecnt, gapwidth, i_padme, i_padmenow, i_padmecurr, i_chk,
      center_width, spares, i_breakhappen, i_usepadding, i_savejust, i_lastspace;

  i_breakhappen = i_usepadding = 0;
  if( fm->lastval ) {
    fm->sawnonzeroval = 1;
  }


  x1 = x2 = x3 = x4 = '\0';
  i_stripansi = i_nostripansi = i_inansi = 0;
  s_padbuf = s_padbufptr = alloc_lbuf("printf_buffering_crap");
  s_special = alloc_mbuf("printf_mbuf_1");
  s_normal = alloc_mbuf("printf_mbuf_2");
  s_accent = alloc_mbuf("printf_mbuf_3");
  memset(s_padstring, '\0', sizeof(s_padstring));
  i_chk = i_lastspace = 0;
  i_savejust = -1;

  if( !fm->fieldwidth ) {
    safe_str_fm( fmtbuff, buff, bufcx, fm );
  }
  else {
/*
#ifdef ZENTY_ANSI
*/
    i_stripansi = strlen(strip_all_special(fmtbuff)) - count_extended(fmtbuff);
    idy = idx = i_chk = 0;
    snarfle_special_characters(fmtbuff, s_padstring);
    strcpy(fmtbuff, s_padstring);
    memset(s_padstring, '\0', sizeof(s_padstring));
    if ( fm->forcebreakonreturn ) {
       s = fmtbuff;
       t = s_padstring;
       idy = idx = i_chk = 0;
       while ( *s ) {
          if ( (*s == '\n') || (*s == '\r')) {
             i_chk = 0;
             i_lastspace = 0;
          }
          if ( idx > (LBUF_SIZE - 12) )
             break;
          if ( (fm->cutatlength != 0) && (idy >= fm->cutatlength) )
             break;
          if ( ((i_chk+1) > (fm->fieldwidth + morepadd)) && (i_stripansi > (fm->fieldwidth + morepadd))) {
             if ( i_lastspace > 0 ) { 
                t = s_padstring + i_lastspace;
                if ( *t ) 
                   t++;
                i_chk = strlen(strip_all_special(t));
                memcpy(s_padstring2, t, LBUF_SIZE-10);
                *t++ = '\r';
                *t++ = '\n';
                u = s_padstring2;
                while ( *u ) {
                   *t++ = *u++;
                }
             } else {
                *t++ = '\r';
                *t++ = '\n';
                i_chk = 0;
             }
             idx+=2;
             i_lastspace = 0;
          }
#ifdef ZENTY_ANSI
          if( (*s == '%') && ((*(s+1) == 'f') && isprint(*(s+2))) ) {
             *t++ = *s++;
             *t++ = *s++;
             *t++ = *s++;
             idx+=3;
             continue;
          }
          if( (*s == '%') && ((*(s+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                          || (*(s+1) == SAFE_CHR2 )
#endif
#ifdef SAFE_CHR3
                          || (*(s+1) == SAFE_CHR3 )
#endif
)) {
             if ( isAnsi[(int) *(s+2)] ) {
                *t++ = *s++;
                *t++ = *s++;
                *t++ = *s++;
                idx+=3;
                continue;
             }
             if ( (*(s+2) == '0') && ((*(s+3) == 'x') || (*(s+3) == 'X')) &&
                  *(s+4) && *(s+5) && isxdigit(*(s+4)) && isxdigit(*(s+5)) ) {
                *t++ = *s++;
                *t++ = *s++;
                *t++ = *s++;
                *t++ = *s++;
                *t++ = *s++;
                *t++ = *s++;
                idx+=6;
                continue;
             }
          }
#endif
          if ( isspace(*s) && (fm->morepadd & 4) ) {
             i_lastspace = idx;
          }
          idx++;
          idy++;
          i_chk++;
          *t++ = *s++;
          if ( idx > (LBUF_SIZE - 10) )
             break;
       }
       strcpy(fmtbuff, s_padstring);
       memset(s_padstring, '\0', sizeof(s_padstring));
    } 
    x1 = x2 = '\0';
    i_stripansi = i_chk = idx = 0;
    if ( fm->breakonreturn ) {
       s_justbuff = strip_all_special(fmtbuff);
       i_nostripansi=0;
       s = s_padstring;
       while ( *s_justbuff && *s_justbuff != '\n' ) {
          if ( *s_justbuff != '\r' )
             i_nostripansi++;
          *s = *s_justbuff;
          s_justbuff++;
          s++;
       } 
       i_stripansi = i_nostripansi - count_extended(s_padstring);
       memset(s_padstring, '\0', sizeof(s_padstring));
    } else {
       i_stripansi = strlen(strip_all_special(fmtbuff)) - count_extended(fmtbuff);
    }
    if ( (i_stripansi == 0) && (*(fm->format_padst) == '!') ) {
       fm->format_padstsize = 0;
       fm->format_padch = ' ';
    } else if ( *(fm->format_padst) == '!') {
/*     fm->format_padstsize = strlen(strip_all_special(fm->format_padst)); */
       snarfle_special_characters(fm->format_padst+1, s_padstring);
       fm->format_padstsize = strlen(strip_all_special(s_padstring));
    } else {
       snarfle_special_characters(fm->format_padst, s_padstring);
    }
    i_nostripansi = strlen(fmtbuff);
    if ( fm->nocutval && *fmtbuff && ((fm->fieldwidth + morepadd) < i_stripansi) )
       fm->fieldwidth = i_stripansi;
    padwidth = (fm->fieldwidth + morepadd) - i_stripansi;
    if ( fm->format_padch )
       padch = fm->format_padch;
    if ( (fm->format_padstsize == 0) && (*(s_padstring) == '%') ) {
       padch = ' ';
    }
    if( numeric && fm->zeropad ) {
      padch = '0';
    }
    center_width = 0;
    if ( fm->leftjust == 3 ) {
        s_justbuff = fmtbuff;
        i_spacecnt = 0;
        while ( *s_justbuff ) {
           if ( *s_justbuff == ' ')
              i_spacecnt++;
           if ( fm->breakonreturn && (*s_justbuff == '\n') )
              break;
           if ( fm->forcebreakonreturn && ((int)(s_justbuff - fmtbuff) >= i_stripansi) )
              break;
           s_justbuff++;
        }
        if ( i_spacecnt > 0 ) {
           gapwidth = ((fm->fieldwidth + morepadd) - (i_stripansi - i_spacecnt)) / i_spacecnt;
           spares = ((fm->fieldwidth + morepadd) - (i_stripansi - i_spacecnt)) % i_spacecnt;
           if( gapwidth < 1 ) {
             gapwidth = 1;
             spares = 0;
           }
        } else {
           gapwidth = spares = 0;
           fm->leftjust = 2;
           i_savejust = 3;
        }
    }
    if( !fm->leftjust ) {
      if ( fm->format_padstsize > 0 ) {
         s_pp = s_padstring;
         for( idx = 0; idx < padwidth; idx++, currwidth++ ) {
           if ( !s_pp || !*s_pp ) 
              s_pp = s_padstring;
           i_chk = 0;
#ifdef ZENTY_ANSI
           while ( *s_pp ) {
              if ( ((*s_pp == '%') && ((*(s_pp+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                                   || (*(s_pp+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                                   || (*(s_pp+1) == SAFE_CHR3)
#endif
) && isAnsi[(int) *(s_pp+2)]) ||
                   ((*s_pp == '%') && (*(s_pp+1) == 'f') && isprint(*(s_pp+2))) ) {
                 safe_chr(*s_pp, buff, bufcx);
                 safe_chr(*(s_pp+1), buff, bufcx);
                 safe_chr(*(s_pp+2), buff, bufcx);
                 i_usepadding = 1;
                 s_pp+=3;
              } else if ( (*s_pp == '%') && ((*(s_pp+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                                         || (*(s_pp+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                                         || (*(s_pp+1) == SAFE_CHR3)
#endif
) && (*(s_pp+2) == '0') && 
                          ((*(s_pp+3) == 'X') || (*(s_pp+3) == 'x')) &&
                           *(s_pp+4) && isxdigit(*(s_pp+4)) && *(s_pp+5) && isxdigit(*(s_pp+5)) ) {
                 safe_chr(*s_pp, buff, bufcx);
                 safe_chr(*(s_pp+1), buff, bufcx);
                 safe_chr(*(s_pp+2), buff, bufcx);
                 safe_chr(*(s_pp+3), buff, bufcx);
                 safe_chr(*(s_pp+4), buff, bufcx);
                 safe_chr(*(s_pp+5), buff, bufcx);
                 i_usepadding = 1;
                 s_pp+=6;
              } else if (*s_pp ) {
                 break;
              }
              if ( !s_pp || !*s_pp ) {
                 i_chk++;
                 s_pp = s_padstring;
                 if ( i_chk > 2 ) {
                    break;
                 }
              }
           }
#else
           while ( *s_pp ) {
              if ( *s_pp ) { 
                 break;
              }
              if ( !s_pp || !*s_pp ) {
                 s_pp = s_padstring;
                 i_chk++;
              }
              if ( i_chk > 2 ) 
                 break;
           }
#endif
           if ( i_chk > 2 )
              safe_chr_fm( (char)' ', buff, bufcx, fm );
           else
              safe_chr_fm( *s_pp, buff, bufcx, fm );
           s_pp++;
         }
#ifdef ZENTY_ANSI
         safe_str((char *)SAFE_ANSI_NORMAL, buff, bufcx);
         safe_str("%fn", buff, bufcx);
#endif
      } else {
         for( idx = 0; idx < padwidth; idx++, currwidth++ ) {
           safe_chr_fm( padch, buff, bufcx, fm );
         }
      }
    } else if ( fm->leftjust == 2 ) {
      center_width = padwidth / 2;
      if ( fm->format_padstsize > 0 ) {
         s_pp = s_padstring;
         for( idx = 0; idx < center_width; idx++, currwidth++ ) {
           if ( !s_pp || !*s_pp )
              s_pp = s_padstring;
           i_chk = 0;
#ifdef ZENTY_ANSI
           while ( *s_pp ) {
              if ( ((*s_pp == '%') && ((*(s_pp+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                                   || (*(s_pp+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                                   || (*(s_pp+1) == SAFE_CHR3)
#endif
) && isAnsi[(int) *(s_pp+2)]) ||
                   ((*s_pp == '%') && (*(s_pp+1) == 'f') && isprint(*(s_pp+2))) ) {
                 
                 safe_chr(*s_pp, buff, bufcx);
                 safe_chr(*(s_pp+1), buff, bufcx);
                 safe_chr(*(s_pp+2), buff, bufcx);
                 i_usepadding = 1;
                 s_pp+=3;
              } else if ( (*s_pp == '%') && ((*(s_pp+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                                         || (*(s_pp+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                                         || (*(s_pp+1) == SAFE_CHR3)
#endif
) && (*(s_pp+2) == '0') && 
                          ((*(s_pp+3) == 'X') || (*(s_pp+3) == 'x')) &&
                           *(s_pp+4) && isxdigit(*(s_pp+4)) && *(s_pp+5) && isxdigit(*(s_pp+5)) ) {
                 safe_chr(*s_pp, buff, bufcx);
                 safe_chr(*(s_pp+1), buff, bufcx);
                 safe_chr(*(s_pp+2), buff, bufcx);
                 safe_chr(*(s_pp+3), buff, bufcx);
                 safe_chr(*(s_pp+4), buff, bufcx);
                 safe_chr(*(s_pp+5), buff, bufcx);
                 i_usepadding = 1;
                 s_pp+=6;
              } else if (*s_pp ) {
                 break;
              }
              if ( !s_pp || !*s_pp ) {
                 i_chk++;
                 s_pp = s_padstring;
                 if ( i_chk > 2 ) {
                    break;
                 }
              }
           }
#else
           while ( *s_pp ) {
              if ( *s_pp ) { 
                 i_chk = 0;
                 break;
              }
              if ( !s_pp || !*s_pp ) {
                 s_pp = s_padstring;
                 i_chk++;
              }
              if ( i_chk > 2 ) 
                 break;
           }
#endif
           if ( i_chk > 2 )
              safe_chr_fm( (char)' ', buff, bufcx, fm );
           else
              safe_chr_fm( *s_pp, buff, bufcx, fm );
           s_pp++;
         }
#ifdef ZENTY_ANSI
         safe_str((char *)SAFE_ANSI_NORMAL, buff, bufcx);
         safe_str("%fn", buff, bufcx);
#endif
      } else {
         for( idx = 0; idx < center_width; idx++, currwidth++ ) {
           safe_chr_fm( padch, buff, bufcx, fm );
         }
      }
    }
    i_padme = 0;
    i_padmenow = 0;
    if ( (i_stripansi == 0) && (i_nostripansi > 0) )
       currwidth--;
    while ( *fmtbuff && (currwidth < (fm->fieldwidth + morepadd)) ) {
#ifdef ZENTY_ANSI
         if ( (*fmtbuff == '%') && ((*(fmtbuff+1) == 'f') && isprint(*(fmtbuff+2))) ) {
            safe_chr( *fmtbuff, buff, bufcx );
            safe_chr( *(fmtbuff+1), buff, bufcx );
            safe_chr( *(fmtbuff+2), buff, bufcx );
            safe_chr( *fmtbuff, s_padbuf, &s_padbufptr );
            safe_chr( *(fmtbuff+1), s_padbuf, &s_padbufptr );
            safe_chr( *(fmtbuff+2), s_padbuf, &s_padbufptr );
            if ( fm->breakonreturn && shold ) {
               safe_chr( *fmtbuff, shold, sholdptr );
               safe_chr( *(fmtbuff+1), shold, sholdptr );
               safe_chr( *(fmtbuff+2), shold, sholdptr );
            }
            fmtbuff+=3;
            i_inansi=1;
            continue;
         }
         if ( (*fmtbuff == '%') && ((*(fmtbuff+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                                || (*(fmtbuff+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                                || (*(fmtbuff+1) == SAFE_CHR3)
#endif
)) {
            if ( isAnsi[(int) *(fmtbuff+2)] ) {
               safe_chr( *fmtbuff, buff, bufcx );
               safe_chr( *(fmtbuff+1), buff, bufcx );
               safe_chr( *(fmtbuff+2), buff, bufcx );
               safe_chr( *fmtbuff, s_padbuf, &s_padbufptr );
               safe_chr( *(fmtbuff+1), s_padbuf, &s_padbufptr );
               safe_chr( *(fmtbuff+2), s_padbuf, &s_padbufptr );
               if ( fm->breakonreturn && shold ) {
                   safe_chr( *fmtbuff, shold, sholdptr );
                   safe_chr( *(fmtbuff+1), shold, sholdptr );
                   safe_chr( *(fmtbuff+2), shold, sholdptr );
               }
               fmtbuff+=3;
               i_inansi=1;
               continue;
            }
            if ( (*(fmtbuff+2) == '0') && (*(fmtbuff+3) && ((*(fmtbuff+3) == 'X') || (*(fmtbuff+3) == 'x'))) ) {
               if ( *(fmtbuff+4) && *(fmtbuff+5) && isxdigit(*(fmtbuff+4)) && isxdigit(*(fmtbuff+5)) ) {
                  safe_chr( *fmtbuff, buff, bufcx );
                  safe_chr( *(fmtbuff+1), buff, bufcx );
                  safe_chr( *(fmtbuff+2), buff, bufcx );
                  safe_chr( *(fmtbuff+3), buff, bufcx );
                  safe_chr( *(fmtbuff+4), buff, bufcx );
                  safe_chr( *(fmtbuff+5), buff, bufcx );
                  safe_chr( *fmtbuff, s_padbuf, &s_padbufptr );
                  safe_chr( *(fmtbuff+1), s_padbuf, &s_padbufptr );
                  safe_chr( *(fmtbuff+2), s_padbuf, &s_padbufptr );
                  safe_chr( *(fmtbuff+3), s_padbuf, &s_padbufptr );
                  safe_chr( *(fmtbuff+4), s_padbuf, &s_padbufptr );
                  safe_chr( *(fmtbuff+5), s_padbuf, &s_padbufptr );
                  if ( fm->breakonreturn && shold ) {
                      safe_chr( *fmtbuff, shold, sholdptr );
                      safe_chr( *(fmtbuff+1), shold, sholdptr );
                      safe_chr( *(fmtbuff+2), shold, sholdptr );
                      safe_chr( *(fmtbuff+3), shold, sholdptr );
                      safe_chr( *(fmtbuff+4), shold, sholdptr );
                      safe_chr( *(fmtbuff+5), shold, sholdptr );
                  }
                  fmtbuff+=6;
                  i_inansi=1;
                  continue;
               }
            }
         }

#endif
       if ( fm->breakonreturn && (*fmtbuff == '\n') && (*(fmtbuff+1) != '\0') ) {
          safe_str(fmtbuff+1, shold, sholdptr);
          i_breakhappen = 1;
          break;
       }
       if ( fm->breakonreturn && (*fmtbuff == '\r') ) {
          fmtbuff++;
          continue;
       }
       if ( (fm->leftjust == 3) && (*fmtbuff == ' ') ) {
          if ( fm->format_padstsize > 0 ) {
             s_pp = s_padstring;
             i_padme = 0;
             i_padmecurr = i_padmenow;
#ifdef ZENTY_ANSI
             safe_str((char *)SAFE_ANSI_NORMAL, buff, bufcx);
             safe_str("%fn", buff, bufcx);
#endif
             for ( idx = 0; idx < gapwidth; idx++ ) {
               if ( !s_pp || !*s_pp )
                  s_pp = s_padstring;
               i_chk = 0;
#ifdef ZENTY_ANSI
               x1 = x2 = '\0';
               while ( *s_pp ) {
                  if ( ((*s_pp == '%') && ((*(s_pp+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                                       || (*(s_pp+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                                       || (*(s_pp+1) == SAFE_CHR3)
#endif
) && isAnsi[(int) *(s_pp+2)]) ||
                       ((*s_pp == '%') && (*(s_pp+1) == 'f') && isprint(*(s_pp+2))) ) {
                     if ( *(s_pp+1) == 'f' ) {
                        memset(s_accent, '\0', MBUF_SIZE);
                        sprintf(s_accent, "%cf%c", (char)'%', *(s_pp+2));
                     } else {
                        switch (*(s_pp+2)) {
                           case 'f':
                           case 'h':
                           case 'i':
                           case 'u':
                                 memset(s_special, '\0', MBUF_SIZE);
                                 sprintf(s_special, "%c%c%c", (char)'%', (char)SAFE_CHR, *(s_pp+2));
                              break;
                           case 'n':
                                 memset(s_special, '\0', MBUF_SIZE);
                                 memset(s_normal, '\0', MBUF_SIZE);
                                 sprintf(s_special, "%c%c%c", (char)'%', (char)SAFE_CHR, *(s_pp+2));
                              break;
                           default:
                                 memset(s_normal, '\0', MBUF_SIZE);
                                 sprintf(s_normal, "%c%c%c", (char)'%', (char)SAFE_CHR, *(s_pp+2));
                              break;
                        }
                     }
                     s_pp+=3;
                     i_usepadding = 1;
                  } else if ( (*s_pp == '%') && ((*(s_pp+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                                             || (*(s_pp+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                                             || (*(s_pp+1) == SAFE_CHR3)
#endif
) && (*(s_pp+2) == '0') && 
                              ((*(s_pp+3) == 'X') || (*(s_pp+3) == 'x')) &&
                               *(s_pp+4) && isxdigit(*(s_pp+4)) && *(s_pp+5) && isxdigit(*(s_pp+5)) ) {
                     memset(s_normal, '\0', MBUF_SIZE);
                     sprintf(s_normal, "%c%c0%c%c%c", (char)'%', (char)SAFE_CHR, *(s_pp+3), *(s_pp+4), *(s_pp+5));
                     s_pp+=6;
                     i_usepadding = 1;
                  } else if (*s_pp ) {
                     if ( i_padme < i_padmecurr ) {
                        s_pp++;
                        i_padme++;
                        i_chk = 0;
                     } else {
                        if ( spares ) {
                           if ( !x1 ) {
                              x1 = *s_pp;
                              if ( *s_special)
                                 safe_str(s_special, buff, bufcx);
                              if ( *s_normal)
                                 safe_str(s_normal, buff, bufcx);
                              if ( *s_accent)
                                 safe_str(s_accent, buff, bufcx);
                              safe_chr_fm( x1, buff, bufcx, fm );
                              memset(s_special, '\0', MBUF_SIZE);
                              memset(s_accent, '\0', MBUF_SIZE);
                              memset(s_normal, '\0', MBUF_SIZE);
                              s_pp++;
                           } else {
                              i_chk = 0;
                              x2 = *s_pp;
                              break;
                           }
                        } else {
                           i_chk = 0;
                           x1 = *s_pp;
                           s_pp++;
                           break;
                        } 
                     }
                  }
                  if ( !s_pp || !*s_pp ) {
                     i_chk++;
                     s_pp = s_padstring;
                     if ( i_chk > 2 ) {
                        break;
                     }
                  }
               }
#else
               while ( *s_pp ) {
                  if ( *s_pp ) { 
                     if ( i_padme < i_padmecurr ) {
                        s_pp++;
                        i_padme++;
                        i_chk = 0;
                     } else {
                        if ( spares ) {
                           if ( !x1 ) {
                              x1 = *s_pp;
                              s_pp++;
                           } else {
                              x2 = *s_pp;
                              break;
                           }
                        } else {
                           x1 = *s_pp;
                           s_pp++;
                           break;
                        } 
                     }
                  }
                  if ( !s_pp || !*s_pp ) {
                     s_pp = s_padstring;
                     i_chk++;
                  }
                  if ( i_chk > 2 ) 
                     break;
               }
#endif
#ifdef ZENTY_ANSI
               if ( !spares ) {
                  if ( *s_special)
                     safe_str(s_special, buff, bufcx);
                  if ( *s_normal)
                     safe_str(s_normal, buff, bufcx);
                  if ( *s_accent)
                     safe_str(s_accent, buff, bufcx);
                  if ( i_chk > 2 )
                     safe_chr_fm( (char)' ', buff, bufcx, fm );
                  else
                     safe_chr_fm( x1, buff, bufcx, fm );
                  memset(s_special, '\0', MBUF_SIZE);
                  memset(s_accent, '\0', MBUF_SIZE);
                  memset(s_normal, '\0', MBUF_SIZE);
               } 
#else
               if ( i_chk > 2 )
                  safe_chr_fm( (char)' ', buff, bufcx, fm );
               else
                  safe_chr_fm( x1, buff, bufcx, fm );
#endif
               i_padmenow++;
               x1 = '\0';
             }
             if ( spares ) {
                if ( *s_special)
                   safe_str(s_special, buff, bufcx);
                if ( *s_normal)
                   safe_str(s_normal, buff, bufcx);
                if ( *s_accent)
                   safe_str(s_accent, buff, bufcx);
                if ( i_chk > 2 )
                   safe_chr_fm( (char)' ', buff, bufcx, fm );
                else
                   safe_chr_fm( x2, buff, bufcx, fm );
                i_padmenow++;
                x1 = x2 = '\0';
                spares--;
             }
#ifdef ZENTY_ANSI
             i_usepadding = 1;
             safe_str((char *)SAFE_ANSI_NORMAL, buff, bufcx);
             safe_str("%fn", buff, bufcx);
#endif
          } else {
#ifdef ZENTY_ANSI
             i_usepadding = 1;
             safe_str((char *)SAFE_ANSI_NORMAL, buff, bufcx);
             safe_str("%fn", buff, bufcx);
#endif
              for ( idx = 0; idx < gapwidth; idx++ ) {
                 safe_chr_fm( padch, buff, bufcx, fm );
              }
              if ( spares ) {
                 safe_chr_fm( padch, buff, bufcx, fm );
                 spares--;
              }
          }
       } else {
          if ( i_usepadding ) {
             safe_str(s_padbuf, buff, bufcx);
             i_usepadding = 0;
          }
          if ( fm->breakonreturn && !((*fmtbuff == '\r') || (*fmtbuff == '\n')) ) {
             safe_chr_fm( *fmtbuff, buff, bufcx, fm );
             i_padmenow++;
          } else if ( !fm->breakonreturn ) {
             safe_chr_fm( *fmtbuff, buff, bufcx, fm );
             i_padmenow++;
          }
       }
       fmtbuff++;
       currwidth++;
    } 
#ifdef ZENTY_ANSI
    while ( *fmtbuff ) {
       if ( !fm->leftjust && fm->breakonreturn && shold ) {
          if ( (*fmtbuff == '%') && ((*(fmtbuff+1) == 'f') && isprint(*(fmtbuff+2))) ) {
             safe_chr( *fmtbuff, shold, sholdptr );
             safe_chr( *(fmtbuff+1), shold, sholdptr );
             safe_chr( *(fmtbuff+2), shold, sholdptr );
             fmtbuff+=3;
          } else if ( (*fmtbuff == '%') && ((*(fmtbuff+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                                        || (*(fmtbuff+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                                        || (*(fmtbuff+1) == SAFE_CHR3)
#endif
)) {
             if ( isAnsi[(int) *(fmtbuff+2)] ) {
                safe_chr( *fmtbuff, shold, sholdptr );
                safe_chr( *(fmtbuff+1), shold, sholdptr );
                safe_chr( *(fmtbuff+2), shold, sholdptr );
                fmtbuff+=3;
             } else if ( (*(fmtbuff+2) == '0') && (*(fmtbuff+3) && ((*(fmtbuff+3) == 'X') || (*(fmtbuff+3) == 'x'))) ) {
                if ( *(fmtbuff+4) && *(fmtbuff+5) && isxdigit(*(fmtbuff+4)) && isxdigit(*(fmtbuff+5)) ) {
                   safe_chr( *fmtbuff, shold, sholdptr );
                   safe_chr( *(fmtbuff+1), shold, sholdptr );
                   safe_chr( *(fmtbuff+2), shold, sholdptr );
                   safe_chr( *(fmtbuff+3), shold, sholdptr );
                   safe_chr( *(fmtbuff+4), shold, sholdptr );
                   safe_chr( *(fmtbuff+5), shold, sholdptr );
                   fmtbuff+=6;
                }
             } else {
                break;
             }
          } else {
             break;
          }
       } else {
          break;
       }
    }
#endif
    if ( fm->breakonreturn && shold ) {
       if ( *fmtbuff && !i_breakhappen ) {
          while ( *fmtbuff && (*fmtbuff != '\n') ) 
             fmtbuff++;
          safe_str(fmtbuff+1, shold, sholdptr);
       } else if (!i_breakhappen) {
          memset(shold, '\0', LBUF_SIZE);
          *sholdptr = shold;
       }
    }
#ifdef ZENTY_ANSI
    if ( i_inansi ) {
       safe_str((char *)SAFE_ANSI_NORMAL, buff, bufcx);
       safe_str("%fn", buff, bufcx);
    }
#endif
    if( fm->leftjust == 1 ) {
      if ( !fm->format_padch )
         padch = ' '; /* don't right-pad with zeros ever */
      if ( fm->format_padstsize > 0 ) {
         s_pp = s_padstring;
         i_padmenow = i_stripansi;
         i_padme = 0;
         for( idx = 0; idx < padwidth; idx++ ) {
           if ( !s_pp || !*s_pp )
              s_pp = s_padstring;
           i_chk = 0;
#ifdef ZENTY_ANSI
           while ( *s_pp ) {
              if ( ((*s_pp == '%') && ((*(s_pp+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                                   || (*(s_pp+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                                   || (*(s_pp+1) == SAFE_CHR3)
#endif
) && isAnsi[(int) *(s_pp+2)]) ||
                   ((*s_pp == '%') && (*(s_pp+1) == 'f') && isprint(*(s_pp+2))) ) {
                 if ( *(s_pp+1) == 'f' ) {
                    memset(s_accent, '\0', MBUF_SIZE);
                    sprintf(s_accent, "%cf%c", (char)'%', *(s_pp+2));
                 } else {
                    switch (*(s_pp+2)) {
                       case 'f':
                       case 'h':
                       case 'i':
                       case 'u':
                             memset(s_special, '\0', MBUF_SIZE);
                             sprintf(s_special, "%c%c%c", (char)'%', (char)SAFE_CHR, *(s_pp+2));
                          break;
                       case 'n':
                             memset(s_special, '\0', MBUF_SIZE);
                             memset(s_normal, '\0', MBUF_SIZE);
                             sprintf(s_special, "%c%c%c", (char)'%', (char)SAFE_CHR, *(s_pp+2));
                          break;
                       default:
                             memset(s_normal, '\0', MBUF_SIZE);
                             sprintf(s_normal, "%c%c%c", (char)'%', (char)SAFE_CHR, *(s_pp+2));
                          break;
                    }
                 }
                 i_usepadding = 1;
                 s_pp+=3;
              } else if ( (*s_pp == '%') && ((*(s_pp+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                                         || (*(s_pp+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                                         || (*(s_pp+1) == SAFE_CHR3)
#endif
) && (*(s_pp+2) == '0') && 
                          ((*(s_pp+3) == 'X') || (*(s_pp+3) == 'x')) &&
                           *(s_pp+4) && isxdigit(*(s_pp+4)) && *(s_pp+5) && isxdigit(*(s_pp+5)) ) {
                 memset(s_normal, '\0', MBUF_SIZE);
                 sprintf(s_normal, "%c%c0%c%c%c", (char)'%', (char)SAFE_CHR, *(s_pp+3), *(s_pp+4), *(s_pp+5));
                 i_usepadding = 1;
                 s_pp+=6;
              } else if (*s_pp ) {
                 if ( i_padme < i_padmenow ) {
                    i_padme++;
                    s_pp++;
                    i_chk = 0;
                 } else {
                    break;
                 }
              }
              if ( !s_pp || !*s_pp ) {
                 i_chk++;
                 s_pp = s_padstring;
                 if ( i_chk > 2 ) {
                    break;
                 }
              }
           }
#else
           while ( *s_pp ) {
              if ( *s_pp ) { 
                 if ( i_padme < i_padmenow ) {
                    i_padme++;
                    s_pp++;
                    i_chk = 0;
                 } else {
                    break;
                 }
              }
              if ( !s_pp || !*s_pp ) {
                 s_pp = s_padstring;
                 i_chk++;
              }
              if ( i_chk > 2 ) 
                 break;
           }
#endif
           if ( *s_special) 
              safe_str(s_special, buff, bufcx);
           if ( *s_normal) 
              safe_str(s_normal, buff, bufcx);
           if ( *s_accent) 
              safe_str(s_accent, buff, bufcx);
           if ( i_chk > 2 )
              safe_chr_fm( (char)' ', buff, bufcx, fm );
           else
              safe_chr_fm( *s_pp, buff, bufcx, fm );
           memset(s_special, '\0', MBUF_SIZE);
           memset(s_accent, '\0', MBUF_SIZE);
           memset(s_normal, '\0', MBUF_SIZE);
           s_pp++;
         }
#ifdef ZENTY_ANSI
         safe_str((char *)SAFE_ANSI_NORMAL, buff, bufcx);
         safe_str("%fn", buff, bufcx);
#endif
      } else {
         for( idx = 0; idx < padwidth; idx++ ) {
           safe_chr_fm( padch, buff, bufcx, fm );
         }
      }
    } else if ( fm->leftjust == 2 ) {
      if ( !fm->format_padch )
         padch = ' '; /* don't right-pad with zeros ever */
      i_padmenow = (padwidth / 2) + i_stripansi;
      i_padme = 0;
      center_width = padwidth - center_width;
      if ( fm->format_padstsize > 0 ) {
         s_pp = s_padstring;
         for( idx = 0; idx < center_width; idx++ ) {
           if ( !s_pp || !*s_pp )
              s_pp = s_padstring;
           i_chk = 0;
#ifdef ZENTY_ANSI
           while ( *s_pp ) {
              if ( ((*s_pp == '%') && ((*(s_pp+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                                   || (*(s_pp+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                                   || (*(s_pp+1) == SAFE_CHR3)
#endif
) && isAnsi[(int) *(s_pp+2)]) ||
                   ((*s_pp == '%') && (*(s_pp+1) == 'f') && isprint(*(s_pp+2))) ) {
                 if ( *(s_pp+1) == 'f' ) {
                    memset(s_accent, '\0', MBUF_SIZE);
                    sprintf(s_accent, "%cf%c", (char)'%', *(s_pp+2));
                 } else {
                    switch (*(s_pp+2)) {
                       case 'f':
                       case 'h':
                       case 'i':
                       case 'u':
                             memset(s_special, '\0', MBUF_SIZE);
                             sprintf(s_special, "%c%c%c", (char)'%', (char)SAFE_CHR, *(s_pp+2));
                          break;
                       case 'n':
                             memset(s_special, '\0', MBUF_SIZE);
                             memset(s_normal, '\0', MBUF_SIZE);
                             sprintf(s_special, "%c%c%c", (char)'%', (char)SAFE_CHR, *(s_pp+2));
                          break;
                       default:
                             memset(s_normal, '\0', MBUF_SIZE);
                             sprintf(s_normal, "%c%c%c", (char)'%', (char)SAFE_CHR, *(s_pp+2));
                          break;
                    }
                 }
                 i_usepadding = 1;
                 s_pp+=3;
              } else if ( (*s_pp == '%') && ((*(s_pp+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                                         || (*(s_pp+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                                         || (*(s_pp+1) == SAFE_CHR3)
#endif
) && (*(s_pp+2) == '0') && 
                          ((*(s_pp+3) == 'X') || (*(s_pp+3) == 'x')) &&
                           *(s_pp+4) && isxdigit(*(s_pp+4)) && *(s_pp+5) && isxdigit(*(s_pp+5)) ) {
                 memset(s_normal, '\0', MBUF_SIZE);
                 sprintf(s_normal, "%c%c0%c%c%c", (char)'%', (char)SAFE_CHR, *(s_pp+3), *(s_pp+4), *(s_pp+5));
                 i_usepadding = 1;
                 s_pp+=6;
              } else if (*s_pp ) {
                 if ( i_padme < i_padmenow ) {
                    i_padme++;
                    s_pp++;
                    i_chk = 0;
                 } else {
                    break;
                 }
                 if ( !s_pp || !*s_pp ) {
                    i_chk++;
                    s_pp = s_padstring;
                 }
              }
              if ( !s_pp || !*s_pp ) {
                 i_chk++;
                 s_pp = s_padstring;
                 if ( i_chk > 2 ) {
                    break;
                 }
              }
           }
#else
           while ( *s_pp ) {
              if ( *s_pp ) { 
                 i_chk = 0;
                 if ( i_padme < i_padmenow ) {
                    i_padme++;
                    s_pp++;
                 } else {
                    break;
                 }
              }
              if ( !s_pp || !*s_pp ) {
                 s_pp = s_padstring;
                 i_chk++;
              }
              if ( i_chk > 2 ) 
                 break;
           }
#endif
           if ( *s_special)
              safe_str(s_special, buff, bufcx);
           if ( *s_normal)
              safe_str(s_normal, buff, bufcx);
           if ( *s_accent)
              safe_str(s_accent, buff, bufcx);
           if ( i_chk > 2 )
              safe_chr_fm( (char)' ', buff, bufcx, fm );
           else
              safe_chr_fm( *s_pp, buff, bufcx, fm );
           s_pp++;
           memset(s_special, '\0', MBUF_SIZE);
           memset(s_accent, '\0', MBUF_SIZE);
           memset(s_normal, '\0', MBUF_SIZE);
         }
#ifdef ZENTY_ANSI
         safe_str((char *)SAFE_ANSI_NORMAL, buff, bufcx);
         safe_str("%fn", buff, bufcx);
#endif
      } else {
         for( idx = 0; idx < center_width; idx++ ) {
           safe_chr_fm( padch, buff, bufcx, fm );
         }
      }
    }
  }
  free_lbuf(s_padbuf);
  free_mbuf(s_special);
  free_mbuf(s_normal);
  free_mbuf(s_accent);
  if ( i_savejust != -1 )
     fm->leftjust = i_savejust;
}

FUNCTION(fun_printf)
{
   char *pp = NULL, *s_pp = NULL;
   int formatpass = 0;
   int fmterror = 0;
   int fmtdone = 0;
   int fmtcurrarg = 1;
   int i_arrayval, i_totwidth, i, j, i_loopydo;
   int i_breakarray[30], i_widtharray[30];
   int morepadd = 0;
   int i_outbuff = 0;
   char *s_strarray[30], *s_strptr, *s_tmpbuff, *outbuff, *outbuff2, *o_p1, *o_p2;
   ANSISPLIT outsplit[LBUF_SIZE], outsplit2[LBUF_SIZE], *p_sp, *p_sp2;
   struct timefmt_format fm, fm_array[30];

   if ( (nfargs < 2) || (nfargs > 28) ) {
      safe_str("#-1 FUNCTION (PRINTF) EXPECTS BETWEEN 2 AND 28 ARGUMENTS [RECEIVED ", buff, bufcx);
      ival(buff, bufcx, nfargs);
      safe_chr(']', buff, bufcx);
      return;
   }

   memset( &fm, 0, sizeof(fm) );
   for ( i = 0; i < 30; i++ ) {
      s_strarray[i] = NULL;
      i_breakarray[i] = i_widtharray[i] = 0;
   }
   i_arrayval = i_totwidth = i = i_loopydo = 0;
 
   fm.format_padch = '\0';
   memset(fm.format_padst, '\0', sizeof(fm.format_padst));
   fm.format_padstsize = 0;
   fm.formatting = 0;
   fm.cutatlength = 0;
   for( pp = fargs[0]; !fmterror && pp && *pp; pp++ ) {
      switch( *pp ) {
         case '!': /* end of fieldsuppress1 */
            if( fm.insup1 ) {
               if( *(pp + 1) == '!' ) {
                  if( !fm.supressing ) {
                     safe_chr(*pp, buff, bufcx);
                     i_totwidth++;
                  } else if( fm.insup2 || fm.insup4 ) {
                     safe_chr(' ', buff, bufcx);
                     i_totwidth++;
                  }
                  pp++;
               } else {
                  fm.supressing = 0;
                  fm.insup1 = 0;
               }
            } else {
               if( !fm.supressing ) {
                  safe_chr(*pp, buff, bufcx);
                  i_totwidth++;
               } else if( fm.insup2 || fm.insup4 ) {
                  safe_chr(' ', buff, bufcx);
                  i_totwidth++;
               }
            }
            break;
         case '~': /* end of fieldsuppress2 */
            if( fm.insup2 ) {
               if( *(pp + 1) == '~' ) {
                  if( !fm.supressing ) {
                     safe_chr(*pp, buff, bufcx);
                     i_totwidth++;
                  } else if( fm.insup2 || fm.insup4 ) {
                     safe_chr(' ', buff, bufcx);
                     i_totwidth++;
                  }
                  pp++;
               } else {
                  fm.supressing = 0;
                  fm.insup2 = 0;
               }
            } else {
               if( !fm.supressing ) {
                  safe_chr(*pp, buff, bufcx);
                  i_totwidth++;
               } else if( fm.insup2 || fm.insup4 ) {
                  safe_chr(' ', buff, bufcx);
                  i_totwidth++;
               }
            }
            break;
         case '@': /* end of fieldsuppress3 */
            if( fm.insup3 ) {
               if( *(pp + 1) == '@' ) {
                  if( !fm.supressing ) {
                     i_totwidth++;
                     safe_chr(*pp, buff, bufcx);
                  } else if( fm.insup2 || fm.insup4 ) {
                     i_totwidth++;
                     safe_chr(' ', buff, bufcx);
                  }
                  pp++;
               } else {
                  fm.supressing = 0;
                  fm.insup3 = 0;
               }
            } else {
               if( !fm.supressing ) {
                  i_totwidth++;
                  safe_chr(*pp, buff, bufcx);
               } else if( fm.insup2 || fm.insup4 ) {
                  i_totwidth++;
                  safe_chr(' ', buff, bufcx);
               }
            }
            break;
         case '#': /* end of fieldsuppress4 */
            if( fm.insup4 ) {
               if( *(pp + 1) == '#' ) {
                  if( !fm.supressing ) {
                     i_totwidth++;
                     safe_chr(*pp, buff, bufcx);
                  } else if( fm.insup2 || fm.insup4 ) {
                     i_totwidth++;
                     safe_chr(' ', buff, bufcx);
                  }
                  pp++;
               } else {
                  fm.supressing = 0;
                  fm.insup4 = 0;
               }
            } else {
               if( !fm.supressing ) {
                  i_totwidth++;
                  safe_chr(*pp, buff, bufcx);
               } else if( fm.insup2 || fm.insup4 ) {
                  i_totwidth++;
                  safe_chr(' ', buff, bufcx);
               }
            }
            break;
         case '$': /* start of format */
            fm.format_padch = '\0';
            memset(fm.format_padst, '\0', sizeof(fm.format_padst));
            fm.format_padstsize = 0;
            fm.formatting = 0;
            if( *(pp + 1) == '$' ) { /* eat a '$' and don't process format */
               if( !fm.supressing ) {
                  safe_chr('$', buff, bufcx);
                  i_totwidth++;
               } else if( fm.insup2 || fm.insup4 ) {
                  safe_chr(' ', buff, bufcx);
                  i_totwidth++;
               }
               pp++;
            } else {
               fm.supressing = 0;
               fm.insup1 = 0;
               fm.insup2 = 0;
               fm.insup3 = 0;
               fm.insup4 = 0;
               /* process the characters following as format string */
               for( fmtdone = 0, pp++; !fmterror && !fmtdone && *pp; pp++ ) {
                  formatpass = 0;
                  switch( *pp ) {
                     case '/': /* Cut-off value if using '|' option */
                        if ( (strchr(pp+1, '/') != NULL ) ) {
                           fm.cutatlength = atoi(pp+1);
                           if ( fm.cutatlength < 0 )
                              fm.cutatlength = 0;
                           if ( fm.cutatlength > (LBUF_SIZE - 2) )
                              fm.cutatlength = (LBUF_SIZE - 2);
                           pp++;
                           while ( *pp && (*pp != '/') )
                              pp++;
                        }
                        formatpass = 1;
                        break;
                     case ':': /* Filler Character */
                        if ( *(pp+1) && (*(pp+2) == ':') ) {
                           if( fm.formatting ) {
                              safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                              fmterror = 1;
                              break;
                           }
                           pp++;
                           fm.format_padch=(char)*pp;
                           pp++;
                           fm.formatting = 1;
                        } else if ( *(pp+1) && (strchr(pp+1, ':') != NULL) ) {
                           if( fm.formatting ) {
                              safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                              fmterror = 1;
                              break;
                           }
                           s_pp = fm.format_padst;
                           pp++;
                           while ( *pp && (*pp != ':') ) {
                              if ( ((*pp == '%') && ((*(pp+1) == 'r') || *(pp+1) == 'R')) ) {
                                 pp+=2;
                                 continue;
                              }
                              if ( (*pp == '\n') || (*pp == '\r') ) {
                                 pp++;
                                 continue;
                              }
                              *s_pp = *pp;
                              s_pp++;
                              pp++;
                           }
#ifndef ZENTY_ANSI
                           s_tmpbuff = alloc_lbuf("fun_printf_tempbuff");
                           memcpy(s_tmpbuff, strip_all_special(fm.format_padst), LBUF_SIZE - 1);
                           memcpy(fm.format_padst, s_tmpbuff, LBUF_SIZE - 1);
                           free_lbuf(s_tmpbuff);
#endif
                           fm.format_padstsize = strlen(strip_all_special(fm.format_padst));
                           fm.format_padch=(char)*(fm.format_padst);
                           initialize_ansisplitter(outsplit, LBUF_SIZE);
                           initialize_ansisplitter(outsplit2, LBUF_SIZE);
                           outbuff = alloc_lbuf("fun_printf_ansi");
                           outbuff2 = alloc_lbuf("fun_printf_ansi2");
                           memset(outbuff2, '\0', LBUF_SIZE);
                           split_ansi(fm.format_padst, outbuff, outsplit);
                           o_p1 = outbuff;
                           o_p2 = outbuff2;
                           p_sp = outsplit;
                           p_sp2 = outsplit2;
                           for (i_outbuff = 0; i_outbuff < LBUF_SIZE - 10; i_outbuff++) {
                             *o_p2++ = *o_p1++;
                              clone_ansisplitter(p_sp2, p_sp);
                              p_sp++;
                              p_sp2++;
                              if ( !*o_p1 ) {
                                 if ( *outbuff == '!' ) {
                                    o_p1 = outbuff+1;
                                    p_sp = outsplit+1;
                                 } else {
                                    o_p1 = outbuff;
                                    p_sp = outsplit;
                                 }
                              }
                           }
                           free_lbuf(outbuff);
                           outbuff = rebuild_ansi(outbuff2, outsplit2);
                           free_lbuf(outbuff2);
                           strcpy(fm.format_padst, outbuff);
                           free_lbuf(outbuff);
                           fm.formatting = 1;
                        }
                        formatpass = 1;
                        break;
                     case '!': /* fieldsuppress type 1 */
                        if( fm.fieldsupress1 || fm.fieldsupress2 || fm.fieldsupress3 || fm.fieldsupress4 ) {
                           safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                           fmterror = 1;
                           break;
                        }
                        fm.insup1 = 1;
                        fm.fieldsupress1 = 1;
                        formatpass = 1;
                        break;
                     case '\'': /* padd to left if left string empty */
                        fm.morepadd = (fm.morepadd | 1);
                        formatpass = 1;
                        break;
                     case '`': /* padd to right if right string empty */
                        fm.morepadd = (fm.morepadd | 2);
                        formatpass = 1;
                        break;
                     case '"': /* Enforce word separation with '|' usage */
                        fm.morepadd = (fm.morepadd | 4);
                        formatpass = 1;
                        break;
                     case '@': /* fieldsuppress type 3 */
                        if( fm.fieldsupress1 || fm.fieldsupress2 || fm.fieldsupress3 || fm.fieldsupress4 ) {
                           safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                           fmterror = 1;
                           break;
                        }
                        fm.insup3 = 1;
                        fm.fieldsupress3 = 1;
                        formatpass = 1;
                        break;
                     case '+': /* don't cut the fields */
                        if( fm.nocutval ) {
                           safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                           fmterror = 1;
                           break;
                        }
                        fm.nocutval = 1;
                        formatpass = 1;
                        break;
                     case '-': /* left justify field */
                        if( fm.leftjust ) {
                           safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                           fmterror = 1;
                           break;
                        }
                        fm.leftjust = 1;
                        formatpass = 1;
                        break;
                     case '^': /* center justify field */
                        if( fm.leftjust ) {
                           safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                           fmterror = 1;
                           break;
                        }
                        fm.leftjust = 2;
                        formatpass = 1;
                        break;
                     case '_': /* wrap justify field */
                        if( fm.leftjust ) {
                           safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                           fmterror = 1;
                           break;
                        }
                        fm.leftjust = 3;
                        formatpass = 1;
                        break;
                     case '~': /* fieldsuppress type 2 */
                        if( fm.fieldsupress1 || fm.fieldsupress2 || fm.fieldsupress3 || fm.fieldsupress4 ) {
                           safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                           fmterror = 1;
                           break;
                        }
                        fm.insup2 = 1;
                        fm.fieldsupress2 = 1;
                        formatpass = 1;
                        break;
                     case '&': /* breakonreturn */
                        if( fm.breakonreturn || fm.forcebreakonreturn ) {
                           safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                           fmterror = 1;
                           break;
                        }
                        if ( i_arrayval > 30 ) {
                           safe_str("#-1 EXCEEDED MAXIMUM BUFFERING FOR RETURN BREAKS", buff, bufcx);
                           fmterror = 1;
                           break;
                        }
                        fm.breakonreturn = 1;
                        formatpass = 1;
                        break;
                     case '|': /* Force break on return */
                        if( fm.forcebreakonreturn || fm.breakonreturn ) {
                           safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                           fmterror = 1;
                           break;
                        }
                        if ( i_arrayval > 30 ) {
                           safe_str("#-1 EXCEEDED MAXIMUM BUFFERING FOR RETURN BREAKS", buff, bufcx);
                           fmterror = 1;
                           break;
                        }
                        fm.forcebreakonreturn = 1;
                        fm.breakonreturn = 1;
                        formatpass = 1;
                        break;
                     case '#': /* fieldsuppress type 4 */
                        if( fm.fieldsupress1 || fm.fieldsupress2 || fm.fieldsupress3 || fm.fieldsupress4 ) {
                           safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                           fmterror = 1;
                           break;
                        }
                        fm.insup4 = 1;
                        fm.fieldsupress4 = 1;
                        formatpass = 1;
                        break;
                     case '0': /* field width argument start */
                     case '1':
                     case '2':
                     case '3':
                     case '4':
                     case '5':
                     case '6':
                     case '7':
                     case '8':
                     case '9':
                        if( fm.fieldwidth || fm.zeropad ) {
                           safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                           fmterror = 1;
                           break;
                        }
                        if( *pp == '0' ) {
                           fm.zeropad = 1;
                        }
                        fm.fieldwidth = *pp - '0';
                        for( pp++; *pp && isdigit((int)*pp); pp++ ) {
                           fm.fieldwidth *= 10;
                           fm.fieldwidth += *pp - '0';
                        }
                        pp--;
                        /* this next check isn't to protect the buffer, but rather keep
                           people from lagging the mush with huge field width processing */
                        if( fm.fieldwidth > LBUF_SIZE - 100 ) {
                           safe_str( "#-1 FIELD WIDTH OUT OF RANGE", buff, bufcx );
                           fmterror = 1;
                           break;
                        }
                        i_totwidth += fm.fieldwidth;
                        formatpass = 1;
                        break;
                     case 'S': /* The string */
                     case 's':
                        fm.lastval = 0;
                        morepadd = 0;
                        if ( fmtcurrarg < nfargs ) {
                           if ( *fargs[fmtcurrarg] )
                              fm.lastval = 1;
                           if ( (fm.forcebreakonreturn && ((strlen(strip_all_special(fargs[fmtcurrarg])) - count_extended(fargs[fmtcurrarg])) > fm.fieldwidth)) || 
                                (fm.breakonreturn && (strchr(fargs[fmtcurrarg], '\n') != NULL)) ) {
                              s_strptr = s_strarray[i_arrayval] = alloc_lbuf("showfield_printf_cr");
                              if ( i_totwidth > 0 )
                                 i_breakarray[i_arrayval] = i_totwidth - fm.fieldwidth;
                              else
                                 i_breakarray[i_arrayval] = 0;
                              i_widtharray[i_arrayval] = fm.fieldwidth;
                              i_totwidth = 0;
                              showfield_printf(fargs[fmtcurrarg], buff, bufcx, &fm, 1, s_strarray[i_arrayval], &s_strptr, morepadd);
                              fm.forcebreakonreturn = 0;
                              fm_array[i_arrayval] = fm;
                              i_arrayval++;
                              i_loopydo = 1;
                           } else {
                              showfield_printf(fargs[fmtcurrarg], buff, bufcx, &fm, 1, (char *)NULL, (char **)NULL, morepadd);
                              if ( fm.supressing )
                                 i_totwidth -= fm.fieldwidth;
                           }
                        } else {
                           setupfield(&fm, 1);
                           if ( fm.supressing )
                              i_totwidth -= fm.fieldwidth;
                        }
                        fmtdone = 1;
                        fmtcurrarg++;
                        fm.nocutval = 0;
                        fm.breakonreturn = 0;
                        fm.forcebreakonreturn = 0;
                        fm.morepadd = 0;
                        break;
                  } /* Case */
                  if( !formatpass ) {
                     fm.fieldsupress1 = 0;
                     fm.fieldsupress2 = 0;
                     fm.fieldsupress3 = 0;
                     fm.fieldsupress4 = 0;
                     fm.fieldwidth = 0;
                     fm.zeropad = 0;
                     fm.leftjust = 0;
                     fm.nocutval = 0;
                     fm.forcebreakonreturn = 0;
                     fm.breakonreturn = 0;
                     fm.morepadd = 0;
                  }
               } /* For */
               pp--;
            } /* Else-If */
            break; 
         default: /* literal character */
            if( !fm.supressing ) {
               i_totwidth++;
               safe_chr(*pp, buff, bufcx);
               if ( (*pp == '\n') || (*pp == '\r') )
                  i_totwidth = 0;
#ifdef ZENTY_ANSI
               if ( *pp == '%' && ((((*(pp+1) == 'c') || (*(pp+1) == 'x')) && (isAnsi[(int) *(pp+2)])) ||
                                    ((*(pp+1) == 'f') && isprint(*(pp+2)))) ) {
                  i_totwidth-=3;
               }
               if ( *pp == '%' && ((*(pp+1) == 'c') || (*(pp+1) == 'x')) && 
                    (*(pp+1) == '0') && ((*(pp+2) == 'x') || (*(pp+2) == 'X')) && 
                    (*(pp+3) && isxdigit(*(pp+3))) && (*(pp+4) && isxdigit(*(pp+4))) ) {
                  i_totwidth-=6;
               }
#endif
            } else if( fm.insup2 || fm.insup4 ) {
               safe_chr(' ', buff, bufcx);
            }
            break;
      } /* Case */
   } /* For */
   if ( i_arrayval > 0 ) {
      s_tmpbuff = alloc_lbuf("fun_printf_tempbuff");
      while ( i_loopydo ) {
         i_loopydo = 0;
         i_totwidth = 0;
         fmtdone = 0;
         fmterror = 0;
         formatpass = 0;
         safe_str("\r\n", buff, bufcx);
         for ( i = 0; i < i_arrayval; i++ ) {
            morepadd = 0;
            if ( (i > 0) && (fm_array[i-1].morepadd & 1) && !*s_strarray[i-1]  ) {
               if ( !fmtdone ) 
                  morepadd += fm_array[i-1].fieldwidth;
               fmtdone = 0;
            } else if ( ((fm_array[i].morepadd & 1) || (fm_array[i].morepadd & 2)) && !*s_strarray[i] ) {
               if ( !(((fm_array[i].morepadd & 2) && (i == 0)) ||
                     ((fm_array[i].morepadd & 1) && ((i+1) >= i_arrayval))) ) {
                  formatpass = i_breakarray[i];
                  fmterror = 1;
                  if ( ((i > 1) && !((fm_array[i-2].morepadd & 1) && !*s_strarray[i-2])) || (i <= 1)) {
                     morepadd -= fm_array[i].fieldwidth;
                  }
               }
            } else if ( ((i+1) < i_arrayval) && (fm_array[i+1].morepadd & 2) && !*s_strarray[i+1]  ) {
                  formatpass = i_breakarray[i];
                  fmterror = 1;
                  i_breakarray[i] += fm_array[i].fieldwidth;
                  morepadd = fm_array[i+1].fieldwidth - fm_array[i].fieldwidth;
            }
            for ( j = 0; j < i_breakarray[i]; j++ )
               safe_chr(' ', buff, bufcx);
            if ( fmterror ) {
               i_breakarray[i] = formatpass;
               formatpass = fmterror = 0;
            }
            if ( strchr(s_strarray[i], '\n') != NULL ) {
               s_strptr = s_tmpbuff;
               showfield_printf(s_strarray[i], buff, bufcx, &fm_array[i], 1, s_tmpbuff, &s_strptr, morepadd);
               strcpy(s_strarray[i], s_tmpbuff);
               i_loopydo = 1;
               i_widtharray[i] = i_widtharray[i] + i_totwidth;
               i_totwidth = 0;
            } else {
               showfield_printf(s_strarray[i], buff, bufcx, &fm_array[i], 1, (char *)NULL, (char **)NULL, morepadd);
               fmtdone = 0;
               if ( *s_strarray[i] )
                  fmtdone = 1;
               memset(s_strarray[i], '\0', LBUF_SIZE);
               fm_array[i].morepadd |= 256;
               if ( !fm_array[i].supressing  )
                  i_totwidth = i_widtharray[i];
            }
         }
      }
      for ( i = 0; i < i_arrayval; i++ ) 
         free_lbuf(s_strarray[i]);
      free_lbuf(s_tmpbuff);
   }
}

#define MUSH_TDAY	30.416667
FUNCTION(fun_timefmt)
{
  char* pp;
  time_t secs, i_frell;
  double secs2;
  char* fmtbuff;
  int formatpass = 0;
  int fmterror = 0;
  int fmtdone = 0;
  long l_offset = 0;
  TZONE_MUSH *tzmush;
  struct tm *tms, *tms2, *tms3;

  static char* shortdayname[] = { "Sun", "Mon", "Tue", "Wed",
                                  "Thu", "Fri", "Sat" };
  static char* longdayname[] = { "Sunday", "Monday", "Tuesday", "Wednesday",
                                 "Thursday", "Friday", "Saturday" };
  static char* shortmonthname[] = { "Jan", "Feb", "Mar", "Apr", "May",
                                    "Jun", "Jul", "Aug", "Sep", "Oct",
                                    "Nov", "Dec" };
  static char* longmonthname[] = { "January", "February", "March", "April",
                                   "May", "June", "July", "August",
                                   "September", "October", "November",
                                   "December" };

  struct timefmt_format fm;

  if (!fn_range_check("TIMEFMT", nfargs, 2, 3, buff, bufcx))
    return;

  memset( &fm, 0, sizeof(fm) );

  tzset();
  secs = atol(fargs[1]);
  secs2 = safe_atof(fargs[1]);
  /* tms2 = localtime(&secs); */
  tms2 = localtime(&mudstate.now);

  tzmush = NULL;
  if ( (nfargs > 2) && *fargs[2] ) {
     for ( tzmush = timezone_list; tzmush->mush_tzone != NULL; tzmush++ ) {
        if ( stricmp((char *)tzmush->mush_tzone, (char *)fargs[2]) == 0 ) {
           break;
        }
     }
     if ( tzmush->mush_tzone ) {
        secs = secs + timezone + tzmush->mush_offset;
/*      secs2 = secs2 + timezone + tzmush->mush_offset; */
        secs2 = secs2 + (double)(int)timezone + (double)(tzmush->mush_offset);
        i_frell = mudstate.now + timezone + tzmush->mush_offset;
        tms2 = localtime(&i_frell);
     }
  }
 
  if ( !tzmush ) {
     for ( tzmush = timezone_list; tzmush->mush_tzone != NULL; tzmush++ ) {
/*      if ( tzmush->mush_offset == -(timezone) ) { */
        if ( (int)(tzmush->mush_offset) == -((int)timezone) ) {
           break;
        }
     }
  }

  l_offset = (long) mktime(tms2) - (long) mktime64(tms2);
  secs2 -= l_offset;
  tms = gmtime64_r(&secs2, tms2); 
  secs2 += l_offset;
  
  fmtbuff = alloc_mbuf("timefmt");

  for( pp = fargs[0]; !fmterror && pp && *pp; pp++ ) {
    switch( *pp ) {
      case '!': /* end of fieldsuppress1 */
        if( fm.insup1 ) {
          if( *(pp + 1) == '!' ) {
            if( !fm.supressing ) {
              safe_chr(*pp, buff, bufcx);
            }
            else if( fm.insup2 ||
                     fm.insup4 ) {
              safe_chr(' ', buff, bufcx);
            }
            pp++;
          }
          else {
            fm.supressing = 0;
            fm.insup1 = 0;
          }
        }
        else {
          if( !fm.supressing ) {
            safe_chr(*pp, buff, bufcx);
          }
          else if( fm.insup2 ||
                   fm.insup4 ) {
            safe_chr(' ', buff, bufcx);
          }
        }
        break;
      case '~': /* end of fieldsuppress2 */
        if( fm.insup2 ) {
          if( *(pp + 1) == '~' ) {
            if( !fm.supressing ) {
              safe_chr(*pp, buff, bufcx);
            }
            else if( fm.insup2 ||
                     fm.insup4 ) {
              safe_chr(' ', buff, bufcx);
            }
            pp++;
          }
          else {
            fm.supressing = 0;
            fm.insup2 = 0;
          }
        }
        else {
          if( !fm.supressing ) {
            safe_chr(*pp, buff, bufcx);
          }
          else if( fm.insup2 ||
                   fm.insup4 ) {
            safe_chr(' ', buff, bufcx);
          }
        }
        break;
      case '@': /* end of fieldsuppress3 */
        if( fm.insup3 ) {
          if( *(pp + 1) == '@' ) {
            if( !fm.supressing ) {
              safe_chr(*pp, buff, bufcx);
            }
            else if( fm.insup2 ||
                     fm.insup4 ) {
              safe_chr(' ', buff, bufcx);
            }
            pp++;
          }
          else {
            fm.supressing = 0;
            fm.insup3 = 0;
          }
        }
        else {
          if( !fm.supressing ) {
            safe_chr(*pp, buff, bufcx);
          }
          else if( fm.insup2 ||
                   fm.insup4 ) {
            safe_chr(' ', buff, bufcx);
          }
        }
        break;
      case '#': /* end of fieldsuppress4 */
        if( fm.insup4 ) {
          if( *(pp + 1) == '#' ) {
            if( !fm.supressing ) {
              safe_chr(*pp, buff, bufcx);
            }
            else if( fm.insup2 ||
                     fm.insup4 ) {
              safe_chr(' ', buff, bufcx);
            }
            pp++;
          }
          else {
            fm.supressing = 0;
            fm.insup4 = 0;
          }
        }
        else {
          if( !fm.supressing ) {
            safe_chr(*pp, buff, bufcx);
          }
          else if( fm.insup2 ||
                   fm.insup4 ) {
            safe_chr(' ', buff, bufcx);
          }
        }
        break;
      case '$': /* start of format */
        if( *(pp + 1) == '$' ) { /* eat a '$' and don't process format */
          if( !fm.supressing ) {
            safe_chr('$', buff, bufcx);
          }
          else if( fm.insup2 ||
                   fm.insup4 ) {
            safe_chr(' ', buff, bufcx);
          }
          pp++;
        }
        else {
          fm.supressing = 0;
          fm.insup1 = 0;
          fm.insup2 = 0;
          fm.insup3 = 0;
          fm.insup4 = 0;
          /* process the characters following as format string */
          for( fmtdone = 0, pp++; !fmterror && !fmtdone && *pp; pp++ ) {
            formatpass = 0;
            switch( *pp ) {
              case '!': /* fieldsuppress type 1 */
                if( fm.fieldsupress1 ||
                    fm.fieldsupress2 ||
                    fm.fieldsupress3 ||
                    fm.fieldsupress4 ) {
                  safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                  fmterror = 1;
                  break;
                }
                fm.insup1 = 1;
                fm.fieldsupress1 = 1;
                formatpass = 1;
                break;
              case '@': /* fieldsuppress type 3 */
                if( fm.fieldsupress1 ||
                    fm.fieldsupress2 ||
                    fm.fieldsupress3 ||
                    fm.fieldsupress4 ) {
                  safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                  fmterror = 1;
                  break;
                }
                fm.insup3 = 1;
                fm.fieldsupress3 = 1;
                formatpass = 1;
                break;
              case '-': /* left justify field */
                if( fm.leftjust ) {
                  safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                  fmterror = 1;
                  break;
                }
                fm.leftjust = 1;
                formatpass = 1;
                break;
              case '~': /* fieldsuppress type 2 */
                if( fm.fieldsupress1 ||
                    fm.fieldsupress2 ||
                    fm.fieldsupress3 ||
                    fm.fieldsupress4 ) {
                  safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                  fmterror = 1;
                  break;
                }
                fm.insup2 = 1;
                fm.fieldsupress2 = 1;
                formatpass = 1;
                break;
              case '#': /* fieldsuppress type 4 */
                if( fm.fieldsupress1 ||
                    fm.fieldsupress2 ||
                    fm.fieldsupress3 ||
                    fm.fieldsupress4 ) {
                  safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                  fmterror = 1;
                  break;
                }
                fm.insup4 = 1;
                fm.fieldsupress4 = 1;
                formatpass = 1;
                break;
              case '0': /* field width argument start */
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                if( fm.fieldwidth ||
                    fm.zeropad ) {
                  safe_str( "#-1 FIELD SPECIFIER EXPECTED", buff, bufcx );
                  fmterror = 1;
                  break;
                }
                if( *pp == '0' ) {
                  fm.zeropad = 1;
                }
                fm.fieldwidth = *pp - '0';
                for( pp++; *pp && isdigit((int)*pp); pp++ ) {
                  fm.fieldwidth *= 10;
                  fm.fieldwidth += *pp - '0';
                }
                pp--;
                /* this next check isn't to protect the buffer, but rather keep
                   people from lagging the mush with huge field width processing */
                if( fm.fieldwidth > LBUF_SIZE - 1 ) {
                  safe_str( "#-1 FIELD WIDTH OUT OF RANGE", buff, bufcx );
                  fmterror = 1;
                  break;
                }
                formatpass = 1;
                break;
              case 'I': /* Offset from GMT */
                if ( tzmush && tzmush->mush_offset ) {
                   fm.lastval = tzmush->mush_offset; 
                } else {
                   fm.lastval = 0;
                }
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'i': /* The Time Zone Itself */
                if ( tzmush && tzmush->mush_tzone) {
                   sprintf(fmtbuff, "%s", tzmush->mush_tzone);
                } else {
                   sprintf(fmtbuff, "N/A");
                }
                showfield(fmtbuff, buff, bufcx, &fm, 0);
                fmtdone = 1;
                break;
              case 'n': /* Short name of timezone */
                if ( tzmush && tzmush->mush_ttype ) {
                   sprintf(fmtbuff, "%s", tzmush->mush_ttype);
                } else {
                   sprintf(fmtbuff, "Unknown");
                }
                showfield(fmtbuff, buff, bufcx, &fm, 0);
                fmtdone = 1;
                break;
              case 'N': /* Long name of timezone */
                if ( tzmush && tzmush->mush_tname ) {
                   sprintf(fmtbuff, "%s", tzmush->mush_tname);
                } else {
                   sprintf(fmtbuff, "Unknown");
                }
                showfield(fmtbuff, buff, bufcx, &fm, 0);
                fmtdone = 1;
                break;
              case 't': /* Timezone */
                fm.lastval = timezone;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'z': /* is daylight savings */
                tms3 = localtime64_r(&secs2, tms2);
                fm.lastval = tms3->tm_isdst;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'H': /* hour 1..12 */
                fm.lastval = (tms->tm_hour % 12);
                if( fm.lastval == 0 ) {
                  fm.lastval = 12;
                }
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'h': /* hour 0..23 */
                fm.lastval = tms->tm_hour;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'T': /* minute 0..59 */
                fm.lastval = tms->tm_min;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'S': /* second 0..61 */
                fm.lastval = tms->tm_sec;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'P': /* AM/PM */
                if( tms->tm_hour <= 11 ) {
                  sprintf(fmtbuff, "AM");
                }
                else {
                  sprintf(fmtbuff, "PM");
                }
                showfield(fmtbuff, buff, bufcx, &fm, 0);
                fmtdone = 1;
                break;
              case 'p': /* am/pm */
                if( tms->tm_hour <= 11 ) {
                  sprintf(fmtbuff, "am");
                }
                else {
                  sprintf(fmtbuff, "pm");
                }
                showfield(fmtbuff, buff, bufcx, &fm, 0);
                fmtdone = 1;
                break;
              case 'A': /* Sunday..Saturday */
                sprintf(fmtbuff, "%s", longdayname[tms->tm_wday]);
                showfield(fmtbuff, buff, bufcx, &fm, 0);
                fmtdone = 1;
                break;
              case 'a': /* Sun..Sat */
                sprintf(fmtbuff, "%s", shortdayname[tms->tm_wday]);
                showfield(fmtbuff, buff, bufcx, &fm, 0);
                fmtdone = 1;
                break;
              case 'B': /* January..December */
                sprintf(fmtbuff, "%s", longmonthname[tms->tm_mon]);
                showfield(fmtbuff, buff, bufcx, &fm, 0);
                fmtdone = 1;
                break;
              case 'b': /* Jan..Dec */
                sprintf(fmtbuff, "%s", shortmonthname[tms->tm_mon]);
                showfield(fmtbuff, buff, bufcx, &fm, 0);
                fmtdone = 1;
                break;
              case 'W': /* Day of week 0..6 */
                fm.lastval = tms->tm_wday;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'M': /* Month number 1..12 */
                fm.lastval = tms->tm_mon + 1;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'D': /* Day of month 1..31 */
                fm.lastval = tms->tm_mday;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'J': /* Day of year 1..366 */
                fm.lastval = tms->tm_yday + 1;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'Y': /* year 0..? (4 digit)*/
                fm.lastval = tms->tm_year + 1900;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'y': /* year 0..99 (2 digit)*/
                fm.lastval = (tms->tm_year + 1900) % 100;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'Z': /* elapsed years (365 day year) */
                fm.lastval = (int) (secs2 / 60 / 60 / 24 / 365);
                sprintf(fmtbuff, "%.0f", floor(secs2 / 60 / 60 / 24 / 365));
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'E': /* elapsed months in year (31 day month) */
                fm.lastval = (int) fmod( (secs2 / 60 / 60 / 24 / MUSH_TDAY), 12);
                if ( ((int) fmod((secs2/60/60/24), MUSH_TDAY)) < 0 )
                   fm.lastval--;
                if ( fm.lastval < 0 )
                   fm.lastval = 12 - fm.lastval;
                if ( (int)(secs2 / 60 / 60 / 24 / 365) != (int)((secs2 - 1) / 60 / 60 / 24 / 365) )
                   fm.lastval = 0;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'e': /* elapsed months (31 day month) */
                fm.lastval = (int)(secs2 / 60 / 60 / 24 / MUSH_TDAY);
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
             case 'C': /* elapsed days in month (31 day month) */
                fm.lastval = (int) fmod( (secs2 / 60 / 60 / 24), MUSH_TDAY);
                if ( ((int) fmod( (secs2 / 60 / 60) , 24 )) < 0 )
                   fm.lastval--;
                if ( fm.lastval < 0 )
                   fm.lastval = 31 - fm.lastval;
                if ( (int)(secs2 / 60 / 60 / 24 / 365) != (int)((secs2 - 1) / 60 / 60 / 24 / 365) )
                   fm.lastval = 0; 
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
             case 'm': /* elapsed milleniums (365 day year) */
                fm.lastval = (int) (secs2 / 60 / 60 / 24 / 365 / 1000);
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
             case 'U': /* elapsed century in millenium (365 day year) */
                fm.lastval = (int) (fmod( (secs2 / 60 / 60 / 24 / 365 / 100), 10));
                if ( ((int) fmod( (secs2 / 60 / 60 / 24 / 365) , 100)) < 0 )
                   fm.lastval--;
                if ( fm.lastval < 0 )
                   fm.lastval = 10 - fm.lastval;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
             case 'u': /* elapsed years in century (365 day week) */
                fm.lastval = (int) (fmod( (secs2 / 60 / 60 / 24/ 365), 100));
                if ( ((int) fmod( (secs2 / 60 / 60 / 24) , 365 )) < 0 )
                   fm.lastval--;
                if ( fm.lastval < 0 )
                   fm.lastval = 100 - fm.lastval;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
             case 'w': /* elapsed weeks in month (7 day week) */
                fm.lastval = (int) (fmod( (secs2 / 60 / 60 / 24), MUSH_TDAY) / 7);
                if ( (int)(secs2 / 60 / 60 / 24 / 365) != (int)((secs2 - 1) / 60 / 60 / 24 / 365) )
                   fm.lastval = 0; 
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'd': /* elapsed days in week (7 day month) */
                fm.lastval = (int) fmod( (secs2 / 60 / 60 / 24), 7);
                if ( ((int) fmod( (secs2 / 60 / 60) , 24)) < 0 )
                   fm.lastval--;
                if ( fm.lastval < 0 )
                   fm.lastval = 7 + fm.lastval;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'c': /* elapsed days */
                fm.lastval = (int)(secs2 / 60 / 60 / 24);
                if ( (int)secs2 < 0 )
                   sprintf(fmtbuff, "%.0f", 0.0 - floor(fabs(secs2) / 60 / 60 / 24));
                else
                   sprintf(fmtbuff, "%.0f", floor(secs2 / 60 / 60 / 24));
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'X': /* elapsed hours within a day */
                fm.lastval = (int) fmod( (secs2 / 60 / 60), 24);
                if ( ((int) fmod( (secs / 60), 60)) < 0)
                   fm.lastval--;
                if ( fm.lastval < 0 )
                   fm.lastval = 24 - fm.lastval;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'x': /* elapsed hours */
                fm.lastval = (int)(secs2 / 60 / 60);
                if ( (int)secs2 < 0 )
                   sprintf(fmtbuff, "%.0f", 0.0 - floor(fabs(secs2) / 60 / 60));
                else
                   sprintf(fmtbuff, "%.0f", floor(secs2 / 60 / 60));
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'F': /* elapsed minutes within hour */
                fm.lastval = (int) fmod( (secs2 / 60), 60);
                if ( ((int) fmod(secs2,60)) < 0 )
                   fm.lastval--;
                if ( fm.lastval < 0 )
                   fm.lastval = 60 + fm.lastval;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'f': /* elapsed minutes */
                fm.lastval = (int)(secs2 / 60);
                if ( (int)secs2 < 0 )
                   sprintf(fmtbuff, "%.0f", 0.0 - floor(fabs(secs2) / 60));
                else
                   sprintf(fmtbuff, "%.0f", floor(secs2 / 60));
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'G': /* elapsed seconds within minute */
                fm.lastval = (int) fmod(secs2, 60);
                if ( fm.lastval < 0 )
                   fm.lastval = 60 + fm.lastval;
                sprintf(fmtbuff, "%d", fm.lastval);
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              case 'g': /* elapsed seconds */
                fm.lastval = (int)(secs2);
                if ( fm.lastval < 0 )
                   sprintf(fmtbuff, "%.0f", 0.0 - floor(fabs(secs2)));
                else
                   sprintf(fmtbuff, "%.0f", floor(secs2));
                showfield(fmtbuff, buff, bufcx, &fm, 1);
                fmtdone = 1;
                break;
              default:
                safe_str( "#-1 MISSING FORMAT CHARACTER", buff, bufcx );
                fmterror = 1;
                break;
            } /* switch */
            if( !formatpass ) {
              fm.fieldsupress1 = 0;
              fm.fieldsupress2 = 0;
              fm.fieldsupress3 = 0;
              fm.fieldsupress4 = 0;
              fm.fieldwidth = 0;
              fm.zeropad = 0;
              fm.leftjust = 0;
            }
          }
          pp--;
        }
        break;
      default: /* literal character */
        if( !fm.supressing ) {
          safe_chr(*pp, buff, bufcx);
        }
        else if( fm.insup2 ||
                 fm.insup4 ) {
          safe_chr(' ', buff, bufcx);
        }
        break;
    } /* switch */
  }
  free_mbuf(fmtbuff);
}
/***********************************************************************
 * PENN's implementation of timefmt().  It uses the hardcoded strftime()
 * Code borrowed from PENN with permission
 */
FUNCTION(fun_ptimefmt)
{
  char *s_buff, *s;
  struct tm *ttm;
  time_t tt;
  int len, n;
  unsigned int val;

  if (!fn_range_check("PTIMEFMT", nfargs, 0, 2, buff, bufcx))
    return;

  if (!fargs[0] || !*fargs[0])
    return;     /* No field? Bad user. */

  val = 0;
  if (nfargs == 2) {
    val = (unsigned int) atoi(fargs[1]);
    if (!is_integer(fargs[1]) || (val < 0)) {
      safe_str("#-1 VALUE MUST BE A POSITIVE INTEGER", buff, bufcx);
      return;
    }
    tt = (time_t) val;
  } else
    tt = mudstate.now;

  ttm = localtime(&tt);
  len = strlen(fargs[0]);
  if ( len > LBUF_SIZE )
     len = LBUF_SIZE;
  s_buff = alloc_lbuf("fun_ptimefmt");
  s = alloc_lbuf("fun_ptimefmt2");
  memset(s_buff, 0, LBUF_SIZE);
  memset(s, 0, LBUF_SIZE);
  strncpy(s_buff, fargs[0], LBUF_SIZE-1);
  for (n = 0; n < len; n++) {
    if (s_buff[n] == '%')
      s_buff[n] = 0x5;
    else if (s_buff[n] == '$') {
      s_buff[n] = '%';
      if ( s_buff[n+1] && !index("aAbBcdHIJmMpSUwWxXyYzZ$",s_buff[n+1]) ) {
         safe_str("#-1 INVALID ESCAPE CODE", buff, bufcx);
         free_lbuf(s_buff);
         free_lbuf(s);
         return;
      }
    }
  }
  len = strftime(s, (LBUF_SIZE - 100), s_buff, ttm);
  free_lbuf(s_buff);
  if (len == 0) {
    /* Problem. Either the output from strftime would be over
     * LBUF_SIZE characters, or there wasn't any output at all.
     * In the former case, what's in s is indeterminate. Instead of
     * trying to figure out which of the two cases happened, just
     * return an empty string.
     */
    free_lbuf(s);
    return;
  }
  for (n = 0; n < len; n++) {
    if (s[n] == '%')
      s[n] = '$';
    else if (s[n] == 0x5)
      s[n] = '%';
  }
  safe_str(s, buff, bufcx);
  free_lbuf(s);
}

/* ---------------------------------------------------------------------------
 * fun_secs: Seconds since 0:00 1/1/70
 */

FUNCTION(fun_secs)
{
    ival(buff, bufcx, mudstate.now);
}

/* ---------------------------------------------------------------------------
 * fun_convsecs: converts seconds to time string, based off 0:00 1/1/70
 */

FUNCTION(fun_convsecs)
{
    char *s_wday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", NULL };
    char *s_mon[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL };
    char *s_format;
    double tt2;
    long l_offset;
    struct tm *ttm2;

    tt2 = safe_atof(fargs[0]);
    ttm2 = localtime(&mudstate.now);
    l_offset = (long) mktime(ttm2) - (long) mktime64(ttm2);
    tt2 -= l_offset;
    gmtime64_r(&tt2, ttm2);
    ttm2->tm_year += 1900;
    s_format = alloc_mbuf("convsecs");
    sprintf(s_format, "%s %s %2d %02d:%02d:%02d %d", s_wday[ttm2->tm_wday % 7], s_mon[ttm2->tm_mon % 12], 
                      ttm2->tm_mday, ttm2->tm_hour, ttm2->tm_min, ttm2->tm_sec, ttm2->tm_year);
    safe_str(s_format, buff, bufcx);
    free_mbuf(s_format);
}

/* ---------------------------------------------------------------------------
 * fun_convtime: converts time string to seconds, based off 0:00 1/1/70
 *    additional auxiliary function and table used to parse time string,
 *    since no ANSI standard function are available to do this.
 */

static const char *monthtab[] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char daystab[] = {
  31, 29, 31, 30, 31, 30,
  31, 31, 30, 31, 30, 31
};

/* converts time string to a struct tm. Returns 1 on success, 0 on fail.
 * Time string format is always 24 characters long, in format
 * Ddd Mmm DD HH:MM:SS YYYY
 */

#define get_substr(buf, p) { \
  p = (char *)index(buf, ' '); \
  if (p) { \
    *p++ = '\0'; \
    while (*p == ' ') p++; \
  } \
}

int
do_convtime(char *str, struct tm *ttm)
{
    char *buf, *p, *q;
    int i;

    if (!str || !ttm)
       return 0;
    while (*str == ' ')
       str++;
    buf = p = alloc_sbuf("do_convtime");  /* make a temp copy of arg */
    safe_sb_str(str, buf, &p);
    *p = '\0';

    get_substr(buf, p);   /* day-of-week or month */
    if (!p || strlen(buf) != 3) {
       free_sbuf(buf);
       return 0;
    }
    for (i = 0; (i < 12) && string_compare(monthtab[i], p); i++);
    if (i == 12) {
       get_substr(p, q); /* month */
       if (!q || strlen(p) != 3) {
          free_sbuf(buf); /* bad length */
          return 0;
       }
       for (i = 0; (i < 12) && string_compare(monthtab[i], p); i++);
       if (i == 12) {
          free_sbuf(buf); /* not found */
          return 0;
       }
       p = q;
    }
    ttm->tm_mon = i;

    get_substr(p, q);   /* day of month */
    if (!q || (ttm->tm_mday = atoi(p)) < 1 || ttm->tm_mday > daystab[i]) {
       free_sbuf(buf);
       return 0;
    }
    p = (char *) index(q, ':'); /* hours */
    if (!p) {
       free_sbuf(buf);
       return 0;
    }
    *p++ = '\0';
    if ((ttm->tm_hour = atoi(q)) > 23 || ttm->tm_hour < 0) {
       free_sbuf(buf);
       return 0;
    }
    if (ttm->tm_hour == 0) {
       while (isspace((int)*q))
           q++;
       if (*q != '0') {
           free_sbuf(buf);
           return 0;
       }
    }
    q = (char *) index(p, ':'); /* minutes */
    if (!q) {
       free_sbuf(buf);
       return 0;
    }
    *q++ = '\0';
    if ((ttm->tm_min = atoi(p)) > 59 || ttm->tm_min < 0) {
       free_sbuf(buf);
       return 0;
    }
    if (ttm->tm_min == 0) {
       while (isspace((int)*p))
           p++;
       if (*p != '0') {
           free_sbuf(buf);
           return 0;
       }
    }
    get_substr(q, p);   /* seconds */
    if (!p || (ttm->tm_sec = atoi(q)) > 59 || ttm->tm_sec < 0) {
       free_sbuf(buf);
       return 0;
    }
    if (ttm->tm_sec == 0) {
       while (isspace((int)*q))
           q++;
       if (*q != '0') {
           free_sbuf(buf);
           return 0;
       }
    }
    get_substr(p, q);   /* year */
    if ((ttm->tm_year = atoi(p)) == 0) {
       while (isspace((int)*p))
           p++;
       if (*p != '0') {
           free_sbuf(buf);
           return 0;
       }
    }
/*
    if (ttm->tm_year > 100)
*/
       ttm->tm_year -= 1900;
    free_sbuf(buf);
/*
    if (ttm->tm_year < 0)
       return 0;
*/

    /* Ignore daylight savings and other such time */
    ttm->tm_isdst = -1;
#define LEAPYEAR_1900(yr) ((yr)%400==100||((yr)%100!=0&&(yr)%4==0))
    return (ttm->tm_mday != 29 || i != 1 || LEAPYEAR_1900(ttm->tm_year));
#undef LEAPYEAR_1900
}

FUNCTION(fun_convtime)
{
    struct tm *ttm;
    long l_offset;

    ttm = localtime(&mudstate.now);
    l_offset = (long) mktime(ttm) - (long) mktime64(ttm);
    if (do_convtime(fargs[0], ttm)) {
       fval(buff, bufcx, (double)(mktime64(ttm) + l_offset));
    }
    else
       safe_str("-1", buff, bufcx);
}


/* ---------------------------------------------------------------------------
 * fun_starttime: What time did this system last reboot?
 */

FUNCTION(fun_starttime)
{
    char *temp;

    temp = (char *) ctime(&mudstate.start_time);
    temp[strlen(temp) - 1] = '\0';
    safe_str(temp, buff, bufcx);
}

FUNCTION(fun_reboottime)
{
    char *temp;

    temp = (char *) ctime(&mudstate.reboot_time);
    temp[strlen(temp) - 1] = '\0';
    safe_str(temp, buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * fun_get, fun_get_eval: Get attribute from object.
 */

int check_read_perms(dbref player, dbref thing, ATTR * attr,
                     int aowner, int aflags, char *buff, char **bufcx)
{
    int see_it;

    /* If we have explicit read permission to the attr, return it */

    if (See_attr_explicit(player, thing, attr, aowner, aflags))
       return 1;

    /* If we are nearby or have examine privs to the attr and it is
     * visible to us, return it.
     */

    if ( thing == GOING || thing == AMBIGUOUS || !Good_obj(thing))
        return 0;
    see_it = See_attr(player, thing, attr, aowner, aflags, 0);
    if ((Examinable(player, thing) || nearby(player, thing))) {
        if ( ((attr)->flags & (AF_INTERNAL)) ||
             (((attr)->flags & (AF_DARK)) && !Immortal(player)) ||
             (((attr)->flags & (AF_MDARK)) && !Wizard(player)) )
           return 0;
        else
           return 1;
    }
    /* For any object, we can read its visible attributes, EXCEPT
     * for descs, which are only visible if read_rem_desc is on.
     */

    if (see_it) {
       if (!mudconf.read_rem_desc && (attr->number == A_DESC)) {
          safe_str((char *) "#-1 TOO FAR AWAY TO SEE", buff, bufcx);
          return 0;
       } else {
          return 1;
       }
    }
    safe_str((char *) "#-1 PERMISSION DENIED", buff, bufcx);
    return 0;
}

static int
check_read_perms2(dbref player, dbref thing, ATTR * attr,
                  int aowner, int aflags)
{
    int see_it;

    /* If we have explicit read permission to the attr, return it */

    if (See_attr_explicit(player, thing, attr, aowner, aflags))
       return 1;

    /* If we are nearby or have examine privs to the attr and it is
     * visible to us, return it.
     */

    if ( thing == GOING || thing == AMBIGUOUS || !Good_obj(thing))
        return 0;
    see_it = See_attr(player, thing, attr, aowner, aflags, 0);
    if ((Examinable(player, thing) || nearby(player, thing))) /* && see_it) */
       return 1;
    /* For any object, we can read its visible attributes, EXCEPT
     * for descs, which are only visible if read_rem_desc is on.
     */

    if (see_it) {
       if (!mudconf.read_rem_desc && (attr->number == A_DESC)) {
          return 0;
       } else {
          return 1;
       }
    }
    return 0;
}

#ifdef USECRYPT
static char *
crunch_code(code)
    char *code;
{
    char *in;
    char *out;
    static char output[LBUF_SIZE];

    out = output;
    in = code;
    while (*in) {
       if ((*in >= 32) && (*in <= 126)) {
          *out++ = *in;
       }
       in++;
    }
    *out = '\0';
    return (output);
}

static char *
crypt_code(char *code, char *text, int type)
{
    static char textbuff[LBUF_SIZE];
    char codebuff[LBUF_SIZE];
    int start = 32;
    int end = 126;
    int mod = end - start + 1;
    char *p, *q, *r;

    if (!text && !*text)
       return ((char *) "");
    strcpy(codebuff, crunch_code(code));
    if (!code || !*code || !*codebuff)
       return (text);
    strcpy(textbuff, "");

    p = text;
    q = codebuff;
    r = textbuff;
    /* Encryption: Simply go through each character of the text, get its ascii
       value, subtract start, add the ascii value (less start) of the
       code, mod the result, add start. Continue  */
    while (*p) {
       if ((*p < start) || (*p > end)) {
          p++;
          continue;
       }
       if (type)
          *r++ = (((*p++ - start) + (*q++ - start)) % mod) + start;
       else
          *r++ = (((*p++ - *q++) + 2 * mod) % mod) + start;
       if (!*q)
          q = codebuff;
    }
    *r = '\0';
    return (textbuff);
}

FUNCTION(fun_encrypt)
{
    safe_str(crypt_code(fargs[1], fargs[0], 1), buff, bufcx);
}

FUNCTION(fun_decrypt)
{
    safe_str(crypt_code(fargs[1], fargs[0], 0), buff, bufcx);
}

#endif

FUNCTION(fun_default)
{
    dbref thing, aowner;
    int attrib, free_buffer, aflags;
    ATTR *attr;
    char *atr_gotten, *s_fargs0, *s_fargs1;
    struct boolexp *bool;

    s_fargs0 = exec(player, cause, caller,  EV_FCHECK | EV_EVAL | EV_STRIP, fargs[0], cargs, ncargs);
    if (!parse_attrib(player, s_fargs0, &thing, &attrib)) {
       s_fargs1 = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
       safe_str(s_fargs1, buff, bufcx);
       free_lbuf(s_fargs1);
       free_lbuf(s_fargs0);
       return;
    }
    free_lbuf(s_fargs0);
    if (attrib == NOTHING) {
       s_fargs1 = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
       safe_str(s_fargs1, buff, bufcx);
       free_lbuf(s_fargs1);
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    free_buffer = 1;
    attr = atr_num(attrib); /* We need the attr's flags for this: */
    if (!attr) {
       s_fargs1 = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
       safe_str(s_fargs1, buff, bufcx);
       free_lbuf(s_fargs1);
       return;
    }
    if (attr->flags & AF_IS_LOCK) {
       atr_gotten = atr_get(thing, attrib, &aowner, &aflags);
       if (Read_attr(player, thing, attr, aowner, aflags, 0)) {
          bool = parse_boolexp(player, atr_gotten, 1);
          free_lbuf(atr_gotten);
          atr_gotten = unparse_boolexp(player, bool);
          free_boolexp(bool);
       } else {
          free_lbuf(atr_gotten);
          atr_gotten = (char *) "#-1 PERMISSION DENIED";
       }
       free_buffer = 0;
    } else {
       atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);
    }

    /* Perform access checks.  c_r_p fills buff with an error message
     * if needed.
     */
    if (check_read_perms(player, thing, attr, aowner, aflags, buff, bufcx)) {
       if (*atr_gotten)
          safe_str(atr_gotten, buff, bufcx);
       else {
          s_fargs1 = exec(player, caller, cause, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
          safe_str(s_fargs1, buff, bufcx);
          free_lbuf(s_fargs1);
       }
    } else {
          s_fargs1 = exec(player, caller, cause, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
          safe_str(s_fargs1, buff, bufcx);
          free_lbuf(s_fargs1);
    }
    if (free_buffer)
       free_lbuf(atr_gotten);
    return;
}

FUNCTION(fun_xget)
{
    dbref thing, aowner;
    int attrib, free_buffer, aflags;
    ATTR *attr;
    char *atr_gotten, *tpr_buff, *tprp_buff;
    struct boolexp *bool;

    tprp_buff = tpr_buff = alloc_lbuf("fun_xget");
    if (!parse_attrib(player, safe_tprintf(tpr_buff, &tprp_buff, "%s/%s",fargs[0], fargs[1]), &thing, &attrib)) {
       safe_str("#-1 NO MATCH", buff, bufcx);
       free_lbuf(tpr_buff);
       return;
    }
    free_lbuf(tpr_buff);
    if (attrib == NOTHING) {
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    free_buffer = 1;
    attr = atr_num(attrib); /* We need the attr's flags for this: */
    if (!attr) {
       return;
    }
    if (attr->flags & AF_IS_LOCK) {
       atr_gotten = atr_get(thing, attrib, &aowner, &aflags);
       if (Read_attr(player, thing, attr, aowner, aflags, 0)) {
          bool = parse_boolexp(player, atr_gotten, 1);
          free_lbuf(atr_gotten);
          atr_gotten = unparse_boolexp(player, bool);
          free_boolexp(bool);
       } else {
          free_lbuf(atr_gotten);
          atr_gotten = (char *) "#-1 PERMISSION DENIED";
       }
       free_buffer = 0;
    } else {
       atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);
    }

    /* Perform access checks.  c_r_p fills buff with an error message
     * if needed.
     */
    if (check_read_perms(player, thing, attr, aowner, aflags, buff, bufcx))
       safe_str(atr_gotten, buff, bufcx);
    if (free_buffer)
       free_lbuf(atr_gotten);
    return;
}

#ifdef PASSWD_FUNC
FUNCTION(fun_passwd)
{
    dbref target, aowner;
    int aflags;
    char *buf;

    if ( !Immortal(player) ) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    target = lookup_player(player, fargs[0], 0);
    if ( !Good_obj(target) || Going(target) || Recover(target) ) {
       safe_str("#-1 NOT FOUND", buff, bufcx);
       return;
    }
    buf = atr_get(target, A_PASS, &aflags, &aowner);
    safe_str(buf, buff, bufcx);
    free_lbuf(buf);
}
#endif

FUNCTION(fun_get)
{
    dbref thing, aowner;
    int attrib, free_buffer, aflags;
    ATTR *attr;
    char *atr_gotten;
    struct boolexp *bool;

    if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
       safe_str("#-1 NO MATCH", buff, bufcx);
       return;
    }
    if (attrib == NOTHING) {
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    free_buffer = 1;
    attr = atr_num(attrib); /* We need the attr's flags for this: */
    if (!attr) {
       return;
    }
    if (attr->flags & AF_IS_LOCK) {
       atr_gotten = atr_get(thing, attrib, &aowner, &aflags);
       if (Read_attr(player, thing, attr, aowner, aflags, 0)) {
           bool = parse_boolexp(player, atr_gotten, 1);
           free_lbuf(atr_gotten);
           atr_gotten = unparse_boolexp(player, bool);
           free_boolexp(bool);
       } else {
           free_lbuf(atr_gotten);
           atr_gotten = (char *) "#-1 PERMISSION DENIED";
       }
       free_buffer = 0;
    } else {
       atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);
    }

    /* Perform access checks.  c_r_p fills buff with an error message
     * if needed.
     */
    if (check_read_perms(player, thing, attr, aowner, aflags, buff, bufcx))
       safe_str(atr_gotten, buff, bufcx);
    if (free_buffer)
       free_lbuf(atr_gotten);
    return;
}

#ifdef REALITY_LEVELS
FUNCTION(fun_chkreality)
{
   dbref thing, target;

   if ( !*fargs[0] || !*fargs[1] ) {
      safe_str("#-1", buff, bufcx);
      return;
   }

   init_match(player, fargs[0], NOTYPE);
   match_everything(MAT_EXIT_PARENTS);
   thing = match_result();
   init_match(player, fargs[1], NOTYPE);
   match_everything(MAT_EXIT_PARENTS);
   target = match_result();
   if ( !Good_obj(target) || !Good_obj(thing) || Going(target) || Going(thing) ||
        Recover(target) || Recover(thing) || !Controls(player, target) || !Controls(player, thing) )
      safe_str("#-1", buff, bufcx);
   else
      ival(buff, bufcx, IsReal(thing, target));
}
#endif

FUNCTION(fun_hasattr)
{
    dbref thing, aowner;
    int attrib, aflags;
    ATTR *attr;
    char *atr_gotten, *tpr_buff, *tprp_buff;

    if (!fn_range_check("HASATTR", nfargs, 1, 2, buff, bufcx))
        return;

    if ( nfargs == 2 ) {
       tprp_buff = tpr_buff = alloc_lbuf("fun_hasattr");
       if (!parse_attrib(player, safe_tprintf(tpr_buff, &tprp_buff, "%s/%s", fargs[0], fargs[1]), &thing, &attrib)) {
          safe_str("#-1", buff, bufcx);
          free_lbuf(tpr_buff);
          return;
       }
       free_lbuf(tpr_buff);
    } else {
       if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
          safe_str("#-1", buff, bufcx);
          return;
       }
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1", buff, bufcx);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1", buff, bufcx);
       return;
    }
    if (attrib == NOTHING) {
       safe_str("0", buff, bufcx);
       return;
    }
    attr = atr_num(attrib); /* We need the attr's flags for this: */
    if (!attr) {
       safe_str("0", buff, bufcx);
       return;
    }
    atr_gotten = atr_get(thing, attrib, &aowner, &aflags);

    /* Perform access checks.  c_r_p fills buff with an error message
     * if needed.
     */

    if (check_read_perms2(player, thing, attr, aowner, aflags)) {
       if (*atr_gotten)
          safe_str("1", buff, bufcx);
       else
          safe_str("0", buff, bufcx);
    }
    else
       safe_str("#-1", buff, bufcx);
    free_lbuf(atr_gotten);
    return;
}

FUNCTION(fun_hasattrp)
{
    dbref thing, aowner;
    int attrib, aflags;
    ATTR *attr;
    char *atr_gotten, *tpr_buff, *tprp_buff;

    if (!fn_range_check("HASATTRP", nfargs, 1, 2, buff, bufcx))
        return;

    if ( nfargs == 2 ) {
       tprp_buff = tpr_buff = alloc_lbuf("fun_hasattrp");
       if (!parse_attrib(player, safe_tprintf(tpr_buff, &tprp_buff, "%s/%s", fargs[0], fargs[1]), &thing, &attrib)) {
          safe_str("#-1", buff, bufcx);
          free_lbuf(tpr_buff);
          return;
       }
       free_lbuf(tpr_buff);
    } else {
       if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
          safe_str("#-1", buff, bufcx);
          return;
       }
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1", buff, bufcx);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1", buff, bufcx);
       return;
    }

    if (!mudconf.hasattrp_compat) {
       thing = Parent(thing);
    }

    if (thing == NOTHING) {
       safe_str("#-1", buff, bufcx);
       return;
    }
    if (attrib == NOTHING) {
       safe_str("0", buff, bufcx);
       return;
    }
    attr = atr_num(attrib); /* We need the attr's flags for this: */
    if (!attr) {
       safe_str("0", buff, bufcx);
       return;
    }
    atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);

    /* Perform access checks.  c_r_p fills buff with an error message
     * if needed.
     */

    if (check_read_perms2(player, thing, attr, aowner, aflags)) {
       if (*atr_gotten)
          safe_str("1", buff, bufcx);
       else
          safe_str("0", buff, bufcx);
    } else
       safe_str("#-1", buff, bufcx);
    free_lbuf(atr_gotten);
    return;
}

FUNCTION(fun_haspower)
{
  dbref target;
  POWENT *pent;
  int t_work, t_retval, t_level, t_break;

  init_match(player, fargs[0], NOTYPE);
  match_everything(MAT_EXIT_PARENTS);
  target = match_result();
  if ((target == NOTHING) || (target == AMBIGUOUS) || (!Good_obj(target)) ||
      !Controls(player,target)) {
     safe_str("#-1",buff,bufcx);
  } else {
    pent = find_power(target,fargs[1]);
    if (pent != NULL) {
      if ( pent->powerflag & POWER3 ) {
         t_retval = Toggles3(target);
      } else if ( pent->powerflag & POWER4 ) {
         t_retval = Toggles4(target);
      } else {
         t_retval = Toggles5(target);
      }
      t_work = t_retval;
      t_work >>= (pent->powerpos);
      t_level = t_work & POWER_LEVEL_COUNC;
      t_break = 0;
      if (t_level) {
         if ((pent->powerperm & CA_GUILDMASTER) && !Guildmaster(player))
             t_break = 1;
         if ((pent->powerperm & CA_BUILDER) && !Builder(player))
             t_break = 1;
         if ((pent->powerperm & CA_ADMIN) && !Admin(player))
             t_break = 1;
         if ((pent->powerperm & CA_WIZARD) && !Wizard(player))
             t_break = 1;
         if ((pent->powerperm & CA_IMMORTAL) && !Immortal(player))
             t_break = 1;
         if ((pent->powerperm & CA_GOD) && !God(player))
             t_break = 1;
         if (pent->powerlev == POWER_LEVEL_OFF)
             t_break = 1;
      }
      if ( t_break || !t_level)
         safe_str("0",buff,bufcx);
      else
         safe_str("1",buff,bufcx);
    } else
      safe_str("0",buff,bufcx);
  }
}

FUNCTION(fun_hasdepower)
{
  dbref target;
  POWENT *pent;
  int t_work, t_retval, t_level, t_break;

  init_match(player, fargs[0], NOTYPE);
  match_everything(MAT_EXIT_PARENTS);
  target = match_result();
  if (target == NOTHING || target == AMBIGUOUS || !Good_obj(target)) {
    safe_str("#-1",buff,bufcx);
  }
  else {
    pent = find_depower(target,fargs[1]);
    if (pent != NULL) {
      if ( pent->powerflag & POWER6 ) {
         t_retval = Toggles6(target);
      } else if ( pent->powerflag & POWER7 ) {
         t_retval = Toggles7(target);
      } else {
         t_retval = Toggles8(target);
      }
      t_work = t_retval;
      t_work >>= (pent->powerpos);
      t_level = t_work & POWER_LEVEL_COUNC;
      t_break = 0;
      if (t_level) {
         if ((pent->powerperm & CA_IMMORTAL) && !Immortal(player))
             t_break = 1;
         if ((pent->powerperm & CA_GOD) && !God(player))
             t_break = 1;
         if (pent->powerlev == POWER_REMOVE)
             t_break = 1;
      }
      if ( t_break || !t_level)
         safe_str("0",buff,bufcx);
      else
      safe_str("1",buff,bufcx);
    } else
      safe_str("0",buff,bufcx);
  }
}


FUNCTION(fun_iscluster) {
  dbref target;
  char *t_buff;
  TOGENT *pent;

  init_match(player, fargs[0], NOTYPE);
  match_everything(MAT_EXIT_PARENTS);
  target = match_result();
  t_buff = alloc_lbuf("fun_iscluster");
  strcpy(t_buff, "cluster");
  if ( !Good_chk(target) || !Controls(player,target)) {
     safe_str("#-1",buff,bufcx);
  } else {
    pent = find_toggle(target, t_buff);
    if (pent != NULL) {
        if ( Toggles(target) & pent->togglevalue )
           ival(buff, bufcx, 1);
        else
           ival(buff, bufcx, 0);
    } else {
        ival(buff, bufcx, 0);
    }
  }
  free_lbuf(t_buff);
}

FUNCTION(fun_hastoggle)
{
  dbref target;
  TOGENT *pent;
  int t_retval, t_break;

  init_match(player, fargs[0], NOTYPE);
  match_everything(MAT_EXIT_PARENTS);
  target = match_result();
  if ((target == NOTHING) || (target == AMBIGUOUS) || !Good_obj(target) ||
       !Controls(player,target)) {
     safe_str("#-1",buff,bufcx);
  } else {
    pent = find_toggle(target,fargs[1]);
    if (pent != NULL) {
        if (pent->toggleflag & TOGGLE2)
            t_retval = Toggles2(target);
        else
            t_retval = Toggles(target);
        t_break = 0;
        if (t_retval & pent->togglevalue) {
            if ((pent->listperm & CA_GUILDMASTER) && !Guildmaster(player))
                t_break = 1;
            if ((pent->listperm & CA_BUILDER) && !Builder(player))
                t_break = 1;
            if ((pent->listperm & CA_ADMIN) && !Admin(player))
                t_break = 1;
            if ((pent->listperm & CA_WIZARD) && !Wizard(player))
                t_break = 1;
            if ((pent->listperm & CA_IMMORTAL) && !Immortal(player))
                t_break = 1;
            if ((pent->listperm & CA_GOD) && !God(player))
                t_break = 1;
        }
        if ( t_break || !(pent->togglevalue & t_retval) )
           safe_str("0",buff,bufcx);
        else
           safe_str("1",buff,bufcx);
   } else
      safe_str("0",buff,bufcx);
  }
}

FUNCTION(fun_listprotection)
{
   char *tstr, *tstr2, *s_strtokr, *s_strtok, *s_matchstr, c_buf;
   int aflags, i_key, i_matchint, i_first;
   dbref thing, aowner;

   if (!fn_range_check("LISTPROTECTION", nfargs, 1, 3, buff, bufcx))
      return;
   i_first =  0;

   thing = lookup_player(player, fargs[0], 0);
   if ( !Good_chk(thing) ) {
      safe_str("#-1 NO MATCH", buff, bufcx);
      return;
   }
   if ( !Wizard(player) && (thing != player) ) {
      thing = player;
   }
   if ( (nfargs > 1) && *fargs[1] ) {
      i_key = atoi(fargs[1]);
   } else { 
      i_key = 0;
   }
   if ( (nfargs > 2) && *fargs[2] ) {
      c_buf = *fargs[2];
   } else {
      c_buf = ':';
   }

   tstr = alloc_lbuf("listprotection_1");
   tstr2 = alloc_lbuf("listprotection_2");
   if (H_Protect(thing)) {
      (void) atr_get_str(tstr, thing, A_PROTECTNAME,
                         &aowner, &aflags);
      if ( *tstr ) {
         strcpy(tstr2, tstr);
         s_strtok = strtok_r(tstr2, "\t", &s_strtokr);
         while ( s_strtok ) {
            s_matchstr = strchr(s_strtok, ':');
            if ( s_matchstr ) {
               *s_matchstr = '\0';
               i_matchint = atoi(s_matchstr+1);
               if ( (!i_key || (i_key == 1)) && (i_matchint == 1) ) {
                  if ( i_first )
                     safe_chr(c_buf, buff, bufcx);
                  safe_str(s_strtok, buff, bufcx);
                  i_first = 1;
               } else if ( (!i_key || (i_key == 2)) && (i_matchint == 0) ) {
                  if ( i_first )
                     safe_chr(c_buf, buff, bufcx);
                  safe_str(s_strtok, buff, bufcx);
                  i_first = 1;
               }
            } else if ( !i_key || (i_key == 2) ) {
               if ( i_first )
                  safe_chr(c_buf, buff, bufcx);
               safe_str(s_strtok, buff, bufcx);
               i_first = 1;
            }
            s_strtok = strtok_r(NULL, "\t", &s_strtokr);
         }
      }
   }
   free_lbuf(tstr);
   free_lbuf(tstr2);
}

FUNCTION(fun_listfunctions)
{
   int keyval;

   if (!fn_range_check("LISTFUNCTIONS", nfargs, 0, 1, buff, bufcx)) {
      return;
   }
   keyval = 0;
   if ( (nfargs == 1) && fargs[0] && *fargs[0] )
      keyval = atoi(fargs[0]);
   if (keyval > 2)
      keyval = 0;
   list_functable2(player, buff, bufcx, keyval);
}

FUNCTION(fun_listcommands)
{
  CMDENT *cmdp;
  const char *ptrs[LBUF_SIZE / 2];
  int nptrs = 0, i;

  for (cmdp = (CMDENT *) hash_firstentry(&mudstate.command_htab);
       cmdp;
       cmdp = (CMDENT *) hash_nextentry(&mudstate.command_htab)) {

    if ( (cmdp->cmdtype & CMD_BUILTIN_e || cmdp->cmdtype & CMD_LOCAL_e) &&
          check_access(player, cmdp->perms, cmdp->perms2, 0)) {
      if ( (!strcmp(cmdp->cmdname, "@snoop") || !strcmp(cmdp->cmdname, "@depower")) &&
            !Immortal(player))
         continue;
      if ( !(cmdp->perms & CF_DARK) ) {
         ptrs[nptrs] = cmdp->cmdname;
         nptrs++;
      }
    }
  }
  qsort(ptrs, nptrs, sizeof(CMDENT *), s_comp);
  safe_str((char *)ptrs[0], buff, bufcx);
  for (i = 1; i < nptrs; i++) {
    safe_chr(' ', buff, bufcx);
    safe_str((char *)ptrs[i], buff, bufcx);
  }
}

FUNCTION(fun_listnewsgroups)
{
   char *retptr;
   dbref target;

   if (!fn_range_check("LISTNEWSGROUPS", nfargs, 0, 1, buff, bufcx)) {
      return;
   }
   if ( nfargs == 0 ) {
      retptr = news_function_group(player, cause, 0);
      safe_str(retptr, buff, bufcx);
      free_lbuf(retptr);
   } else {
      target = lookup_player(player, fargs[0], 0);
      if ((target == NOTHING) || !Controls(player,target)) {
         notify(player, "#-1 PERMISSION DENIED");
         return;
      } else
         retptr = news_function_group(target, cause, 1);
         safe_str(retptr, buff, bufcx);
         free_lbuf(retptr);
   }

}

FUNCTION(fun_listflags)
{
   display_flagtab2(player, buff, bufcx);
}

FUNCTION(fun_listtoggles)
{
   display_toggletab2(player, buff, bufcx);
}


FUNCTION(fun_lit)
{
   safe_str(fargs[0], buff, bufcx);
}

FUNCTION(fun_lrooms)
{
  static dbref rlist[1000];
  static int dlist[1000];
  dbref start, eloop;
  int index, got, depth, d2, showall, index2;

  if (mudstate.last_cmd_timestamp == mudstate.now) {
     mudstate.heavy_cpu_recurse += 1;
  }
  if ( mudstate.heavy_cpu_recurse > mudconf.heavy_cpu_max ) {
     safe_str("#-1 HEAVY CPU RECURSION LIMIT EXCEEDED", buff, bufcx);
     return;
  }
  if (!fn_range_check("LROOMS", nfargs, 1, 3, buff, bufcx)) {
    return;
  }

  if ((nfargs >= 2) && *fargs[1]) {
    depth = atoi(fargs[1]);
    if (depth < 0)
      depth = 1;
  }
  else
    depth = 1;
  if ( (nfargs == 3) && *fargs[2] ) {
     showall = atoi(fargs[2]);
  } else
     showall = 1;
  if ( (showall < 0) || (showall > 1) )
     showall = 1;
  init_match(player, fargs[0], NOTYPE);
  match_everything(0);
  start = match_result();
  if ((start == NOTHING) || (start == AMBIGUOUS) || !isRoom(start) || (!Examinable(player,start) && Dark(start)))
    safe_str("#-1", buff, bufcx);
  else {
     index = 0;
     DO_WHOLE_DB(eloop) {
        c_Lrused(eloop);
     }
     s_Lrused(start);
     DOLIST(eloop, Exits(start)) {
        if (!Dark(eloop) || Immortal(player) || Controls(player,Owner(eloop))) {
           if ( Good_obj(Location(eloop)) && isRoom(Location(eloop)) && (index < 1000) && !Lrused(Location(eloop)) &&
                (Examinable(player,Location(eloop)) || Abode(Location(eloop) ) ||
              Link_ok(Location(eloop)) || Jump_ok(Location(eloop)))) {
              dlist[index] = 1;
              rlist[index++] = Location(eloop);
              s_Lrused(Location(eloop));
           }
        }
     }
     got = 0;
     d2 = 1;
     while (index > 0) {
        start = rlist[--index];
        if ( showall || (!showall && (dlist[index] == depth)) ) {
           if (got)
              safe_chr(' ',buff,bufcx);
           safe_chr('#',buff,bufcx);
           safe_str(myitoa(start),buff,bufcx);
           got = 1;
        }
        d2 = dlist[index];
        if (!depth || (d2 < depth)) {
           DOLIST(eloop, Exits(start)) {
              if (!Dark(eloop) || Immortal(player) || Controls(player,Owner(eloop))) {
                 if ( Good_obj(Location(eloop)) && isRoom(Location(eloop)) &&
                      (index < 1000) && !Lrused(Location(eloop)) &&
                      (Examinable(player,Location(eloop)) || Abode(Location(eloop)) ||
                       Link_ok(Location(eloop)) || Jump_ok(Location(eloop)))) {
                    dlist[index] = d2 + 1;
                    rlist[index++] = Location(eloop);
                    s_Lrused(Location(eloop));
                 } else { /* If */
                    if ( !showall && ((d2+1) == depth) &&
                         Good_obj(Location(eloop)) && isRoom(Location(eloop)) &&
                         (index < 1000) && 
                         (Examinable(player,Location(eloop)) || Abode(Location(eloop)) ||
                          Link_ok(Location(eloop)) || Jump_ok(Location(eloop))) ) {
                       for ( index2 = 0; index2 < index; index2++ ) {
                          if ( rlist[index2] == Location(eloop) )
                             dlist[index2] = d2+1;
                       }
                    }
                 }
              } /* If */
           } /* Dolist */
        } /* If */
     } /* While */
  } /* Else */
}

FUNCTION(fun_edefault)
{
    dbref thing, aowner;
    int attrib, free_buffer, aflags, eval_it, tval;
    ATTR *attr;
    char *atr_gotten, *atr_gotten2, *s_fargs0, *s_fargs1;
    struct boolexp *bool;

    
    s_fargs0 = exec(player, cause, caller,  EV_FCHECK | EV_EVAL | EV_STRIP, fargs[0], cargs, ncargs);
    if (!parse_attrib(player, s_fargs0, &thing, &attrib)) {
       s_fargs1 = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
       safe_str(s_fargs1, buff, bufcx);
       free_lbuf(s_fargs0);
       free_lbuf(s_fargs1);
       return;
    }
    free_lbuf(s_fargs0);
    if (attrib == NOTHING) {
       s_fargs1 = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
       safe_str(s_fargs1, buff, bufcx);
       free_lbuf(s_fargs1);
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    free_buffer = 1;
    eval_it = 1;
    attr = atr_num(attrib); /* We need the attr's flags for this: */
    if (!attr) {
       s_fargs1 = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
       safe_str(s_fargs1, buff, bufcx);
       free_lbuf(s_fargs1);
       return;
    }
    if (attr->flags & AF_IS_LOCK) {
       atr_gotten = atr_get(thing, attrib, &aowner, &aflags);
       if (Read_attr(player, thing, attr, aowner, aflags, 0)) {
          bool = parse_boolexp(player, atr_gotten, 1);
          free_lbuf(atr_gotten);
          atr_gotten = unparse_boolexp(player, bool);
          free_boolexp(bool);
       } else {
          free_lbuf(atr_gotten);
          atr_gotten = (char *) "#-1 PERMISSION DENIED";
       }
       free_buffer = 0;
       eval_it = 0;
    } else {
       atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);
    }
    if (!check_read_perms(player, thing, attr, aowner, aflags, buff, bufcx)) {
       s_fargs1 = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
       safe_str(s_fargs1, buff, bufcx);
       free_lbuf(s_fargs1);
       if (free_buffer)
          free_lbuf(atr_gotten);
       return;
    }
    if (eval_it) {
       tval = safer_ufun(player, thing, thing, (attr ? attr->flags : 0), aflags);
       if ( tval == -2 ) {
          atr_gotten2 = alloc_lbuf("edefault_buff");
          sprintf(atr_gotten2 ,"#-1 PERMISSION DENIED");
       } else {
          atr_gotten2 = exec(thing, player, player, EV_FIGNORE | EV_EVAL, atr_gotten,
                             (char **) NULL, 0);
       }
       safer_unufun(tval);
       if (*atr_gotten2)
          safe_str(atr_gotten2, buff, bufcx);
       else {
          s_fargs1 = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
          safe_str(s_fargs1, buff, bufcx);
          free_lbuf(s_fargs1);
       }
       free_lbuf(atr_gotten2);
    } else {
      if (*atr_gotten)
          safe_str(atr_gotten, buff, bufcx);
      else {
          s_fargs1 = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
          safe_str(s_fargs1, buff, bufcx);
          free_lbuf(s_fargs1);
      }
    }
    if (free_buffer)
       free_lbuf(atr_gotten);
    return;
}

FUNCTION(fun_get_eval)
{
    dbref thing, aowner;
    int attrib, free_buffer, aflags, eval_it, tval;
    ATTR *attr;
    char *atr_gotten;
    char *atr_gotten2;
    struct boolexp *bool;

    if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
       safe_str("#-1 NO MATCH", buff, bufcx);
       return;
    }
    if (attrib == NOTHING) {
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    free_buffer = 1;
    eval_it = 1;
    attr = atr_num(attrib); /* We need the attr's flags for this: */
    if (!attr) {
       return;
    }
    if (attr->flags & AF_IS_LOCK) {
       atr_gotten = atr_get(thing, attrib, &aowner, &aflags);
       if (Read_attr(player, thing, attr, aowner, aflags, 0)) {
          bool = parse_boolexp(player, atr_gotten, 1);
          free_lbuf(atr_gotten);
          atr_gotten = unparse_boolexp(player, bool);
          free_boolexp(bool);
       } else {
          free_lbuf(atr_gotten);
          atr_gotten = (char *) "#-1 PERMISSION DENIED";
       }
       free_buffer = 0;
       eval_it = 0;
    } else {
       atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);
    }
    if (!check_read_perms(player, thing, attr, aowner, aflags, buff, bufcx)) {
       if (free_buffer)
          free_lbuf(atr_gotten);
       return;
    }
    if (eval_it) {
       tval = safer_ufun(player, thing, thing, (attr ? attr->flags : 0), aflags);
       if ( tval == -2 ) {
          atr_gotten2 = alloc_lbuf("edefault_buff");
          sprintf(atr_gotten2 ,"#-1 PERMISSION DENIED");
       } else {
          atr_gotten2 = exec(thing, player, player, EV_FIGNORE | EV_EVAL, atr_gotten,
                             (char **) NULL, 0);
       }
       safe_str(atr_gotten2, buff, bufcx);
       free_lbuf(atr_gotten2);
       safer_unufun(tval);
    } else {
       safe_str(atr_gotten, buff, bufcx);
    }
    if (free_buffer)
       free_lbuf(atr_gotten);
    return;
}

FUNCTION(fun_shl)
{
  unsigned long val;
  char *tpr_buff, *tprp_buff;
  int num;

  num = atoi(fargs[1]);
  if (!*fargs[0])
     ;
  else if (num < 1)
     safe_str(fargs[0],buff,bufcx);
  else {
     if (is_number(fargs[0])) {
        if (num > (sizeof(long) * 8)) {
           safe_str("0",buff,bufcx);
        } else {
           val = atol(fargs[0]);
           if (val > 0)
              val <<= num;
           tprp_buff = tpr_buff = alloc_lbuf("fun_shl");
           safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%lu",val),buff,bufcx);
           free_lbuf(tpr_buff);
        }
     } else {
        if (num < strlen(fargs[0]))
           safe_str(fargs[0]+num,buff,bufcx);
     }
  }
}

FUNCTION(fun_roman) {
   char *s_romanmill[]={"", "m", "mm", "mmm", NULL};
   char *s_romanhundthous[]={"", "c", "cc", "ccc", "cd", "d", "dc", "dcc", "dccc", "cm", NULL};
   char *s_romantenthous[]={"", "x", "xx", "xxx", "xl", "xl", "lx", "lxx", "lxxx", "xc", NULL};
   char *s_romanthous[]={"", "M", "MM", "MMM", "Mv", "v", "vM", "vMM", "vMMM", "Mx", NULL};
   char *s_romanhund[]={"", "C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC", "CM", NULL};
   char *s_romantens[]={"", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC", NULL};
   char *s_romanones[]={"", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", NULL};
   char *s_romanbuff;
   int i_roman;

   if ( !fargs[0] || !*fargs[0] ) {
      safe_str("#-1 NUMBER EXPECTED", buff, bufcx);
      return;
   }

   i_roman = atoi(fargs[0]);

   if ( (i_roman <= 0) || (i_roman > 3999999) ) {
      if ( is_integer(fargs[0]) )
         safe_str("#-1 VALUE MUST BE GREATER THAN 0 OR LESS THAN 4000000", buff, bufcx);
      else
         safe_str("#-1 NUMBER EXPECTED", buff, bufcx);
      return;
   }

   s_romanbuff = alloc_lbuf("fun_roman");
   sprintf(s_romanbuff, "%s%s%s%s%s%s%s",
           s_romanmill[i_roman / 1000000],
           s_romanhundthous[(i_roman % 1000000) / 100000],
           s_romantenthous[(i_roman % 100000) / 10000],
           s_romanthous[(i_roman % 10000) / 1000],
           s_romanhund[(i_roman % 1000) / 100],
           s_romantens[(i_roman % 100) / 10],
           s_romanones[i_roman % 10]);
   safe_str(s_romanbuff, buff, bufcx);
   free_lbuf(s_romanbuff);
}

FUNCTION(fun_rotl)
{
  int num;

  num = atoi(fargs[1]);
  if (!*fargs[0])
    ;
  else if (num < 1)
    safe_str(fargs[0],buff,bufcx);
  else {
    num %= strlen(fargs[0]);
    safe_str(fargs[0]+num,buff,bufcx);
    if (num > 0) {
      *(fargs[0]+num) = '\0';
      safe_str(fargs[0],buff,bufcx);
    }
  }
}

FUNCTION(fun_rotr)
{
  int num, len;

  num = atoi(fargs[1]);
  if (!*fargs[0])
    ;
  else if (num < 1)
    safe_str(fargs[0],buff,bufcx);
  else {
    len = strlen(fargs[0]);
    num %= len;
    num = len - num;
    safe_str(fargs[0]+num,buff,bufcx);
    if (num > 0) {
      *(fargs[0]+num) = '\0';
      safe_str(fargs[0],buff,bufcx);
    }
  }
}

FUNCTION(fun_shr)
{
  unsigned long val;
  char *tpr_buff, *tprp_buff;
  int num;

  num = atoi(fargs[1]);
  if (!*fargs[0])
    ;
  else if (num < 1) {
    safe_str(fargs[0],buff,bufcx);
  } else {
    if (is_number(fargs[0])) {
       if (num > (sizeof(long) * 8)) {
         safe_str("0",buff,bufcx);
       } else {
          val = atol(fargs[0]);
          if (val > 0)
             val >>= num;
          tprp_buff = tpr_buff = alloc_lbuf("fun_shr");
          safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%lu",val),buff,bufcx);
          free_lbuf(tpr_buff);
       }
    } else {
       if (num < strlen(fargs[0]))  {
          *(fargs[0] + strlen(fargs[0]) - num) = '\0';
          safe_str(fargs[0],buff,bufcx);
       }
    }
  }
}

FUNCTION(fun_hastype)
{
  dbref obj;

  init_match(player, fargs[0], NOTYPE);
  match_everything(0);
  obj = match_result();
  if ((obj == NOTHING) || (obj == AMBIGUOUS) || !Good_obj(obj)) {
    safe_str("#-1", buff, bufcx);
  } else if ( (Cloak(obj) && !Wizard(player)) ||
        (Cloak(obj) && SCloak(obj) && !Immortal(player)) ) {
    safe_str("#-1", buff, bufcx);
  } else {
    switch (tolower(*fargs[1])) {
      case 'p':
  if (isPlayer(obj))
    safe_str("1",buff,bufcx);
  else
    safe_str("0",buff,bufcx);
  break;
      case 'r':
  if (isRoom(obj))
    safe_str("1",buff,bufcx);
  else
    safe_str("0",buff,bufcx);
  break;
      case 't':
  if (isThing(obj))
    safe_str("1",buff,bufcx);
  else
    safe_str("0",buff,bufcx);
  break;
      case 'e':
  if (isExit(obj))
    safe_str("1",buff,bufcx);
  else
    safe_str("0",buff,bufcx);
  break;
      default:
  safe_str("0",buff,bufcx);
    }
  }
}

FUNCTION(fun_cloak)
{
  dbref obj, target;

  init_match(player, fargs[0], NOTYPE);
  match_everything(0);
  obj = match_result();
  if ((obj == NOTHING) || (obj == AMBIGUOUS) || (Immortal(obj) && SCloak(obj) && !Immortal(player)))
    safe_str("#-1",buff,bufcx);
  else {
    init_match(player, fargs[1], NOTYPE);
    match_everything(0);
    target = match_result();
    if ((target == NOTHING) || (target == AMBIGUOUS) || (Immortal(target) && SCloak(target) && !Immortal(player)))
      safe_str("#-1",buff,bufcx);
    else {
      if (Immortal(target) && SCloak(target) && Immortal(obj)) {
         safe_str("0",buff,bufcx);
         return;
      } else if (Immortal(target) && SCloak(target)) {
         safe_str("1",buff,bufcx);
         return;
      }
      if (Wizard(target) && Cloak(target) && Wizard(obj)) {
         safe_str("0",buff,bufcx);
         return;
      } else if (Wizard(target) && Cloak(target)) {
         safe_str("1",buff,bufcx);
         return;
      }
      safe_str("0",buff,bufcx);
    }
  }
}

/*
 * Borrowed from PennMUSH 1.50
 */
FUNCTION(fun_isword)
{
    char *p;
    int i_val;

    if ( *fargs[0] == '\0' ) {
       ival(buff, bufcx, 0);
       return;
    }
    i_val = 1;
    for (p = fargs[0]; *p; p++)
    {
        if (!isalpha((int)*p) && (*p != '-'))
        {
            i_val = 0;
            break;
        }
    }
    ival(buff, bufcx, i_val);
}

FUNCTION(fun_ishidden)
{
  dbref target;

  target = lookup_player(player, fargs[0], 0);
  if ((target == NOTHING) || !Controls(player,target))
    safe_str("#-1",buff,bufcx);
  else {
    if (Unfindable(target))
      safe_str("1",buff,bufcx);
    else
      safe_str("0",buff,bufcx);
  }
}

FUNCTION(fun_programmer)
{
  char *t_buf, tmpstr[20], *tmpstrptr, *t_bufptr;
  dbref target, aowner2, it;
  int aflags2, overflowchk;

  memset(tmpstr, 0, sizeof(tmpstr));
  it = lookup_player(player, fargs[0], 0);
  if ( (it == NOTHING) || !Controls(player, it) || !InProgram(it)) {
     safe_str("#-1", buff, bufcx);
     return;
  }
  t_buf = atr_get(it, A_PROGBUFFER, &aowner2, &aflags2);
  if ( !*t_buf || !isdigit((int)*t_buf) ) {
     safe_str("#-1", buff, bufcx);
     free_lbuf(t_buf);
     return;
  }

  tmpstrptr = tmpstr;
  t_bufptr = t_buf;

  overflowchk = 1;
  while ( *t_bufptr && *t_bufptr != ':' ) {
     *tmpstrptr = *t_bufptr;
     t_bufptr++;
     tmpstrptr++;
     overflowchk++;
     if ( overflowchk >= 19 )
        break;
  }
  target = atoi(tmpstr);

  if ( target != NOTHING && Good_obj(target) && !Recover(target) &&
       !Going(target) && Controls(player, target) ) {
     dbval(buff, bufcx, target);
  } else {
     safe_str("#-1", buff, bufcx);
  }
  free_lbuf(t_buf);
}

FUNCTION(fun_inprogram)
{
  dbref target;

  target = lookup_player(player, fargs[0], 0);
  if ( (target == NOTHING) || !Controls(player, target))
     safe_str("#-1", buff, bufcx);
  else {
     if ( InProgram(target) )
        safe_chr('1', buff, bufcx);
     else
        safe_chr('0', buff, bufcx);
  }
}

extern NAMETAB evaltab_sw[];
extern NAMETAB type_sw[];

FUNCTION(fun_remtype)
{
    dbref obj;
    int otype, stype, do_count, bit_type, loop_cntr;
    char *pt1, sep, osep, sep_buf[2], *pt2, *s_tok;
    int chk_types[]={1, 2, 4, 8, 16, 32, 64, 128};

    svarargs_preamble("REMTYPE", 4);
    bit_type = loop_cntr = 0;
    pt2 = strtok_r(fargs[1], " ", &s_tok);
    while (pt2 && (loop_cntr <= 7)) {
       otype = search_nametab(player, type_sw, pt2);
       if ( otype >= 0 && otype <= 7 ) {
          bit_type |= chk_types[otype];
       }
       pt2 = strtok_r(NULL, " ", &s_tok);
       loop_cntr++;
    }
    if (bit_type == 0) {
       safe_str(fargs[0], buff, bufcx);
       return;
    }
    if ( sep == '\0' )
       sep = ' ';
    if ( osep == '\0' )
       osep = sep;
    sep_buf[0] = sep;
    sep_buf[1] = '\0';
    pt1 = strtok_r(fargs[0], sep_buf, &s_tok);
    do_count = 0;
    while (pt1) {
       init_match(player, pt1, NOTYPE);
       match_everything(MAT_EXIT_PARENTS);
       obj = noisy_match_result();
       if (obj == NOTHING) {
          stype = TYPE_MASK;
       } else if (Going(obj) || Recover(obj)) {
          stype = TYPE_MASK;
       } else if (Cloak(obj) && (SCloak(obj)) && !Immortal(player)) {
          stype = TYPE_MASK;
       } else if (Cloak(obj) && !Wizard(player)) {
          stype = TYPE_MASK;
       } else {
          stype = Typeof(obj);
       }
       if ( stype < 0 || stype > 7 )
          stype = TYPE_MASK;
       if ( !(bit_type & chk_types[stype]) ) {
          if ( do_count )
             safe_chr(osep, buff, bufcx);
          do_count = 1;
          safe_str(pt1, buff, bufcx);
       }
       pt1 = strtok_r(NULL, sep_buf, &s_tok);
    }
}

FUNCTION(fun_keeptype)
{
    dbref obj;
    int otype, stype, do_count, bit_type, loop_cntr;
    char *pt1, sep, osep, sep_buf[2], *pt2, *s_tok;
    int chk_types[]={1, 2, 4, 8, 16, 32, 64, 128};

    svarargs_preamble("KEEPTYPE", 4);
    bit_type = loop_cntr = 0;
    pt2 = strtok_r(fargs[1], " ", &s_tok);
    while (pt2 && (loop_cntr <= 7)) {
       otype = search_nametab(player, type_sw, pt2);
       if ( otype >= 0 && otype <= 7 ) {
          bit_type |= chk_types[otype];
       }
       pt2 = strtok_r(NULL, " ", &s_tok);
       loop_cntr++;
    }
    if (bit_type == 0) {
       safe_str(fargs[0], buff, bufcx);
       return;
    }
    if ( sep == '\0' )
       sep = ' ';
    if ( osep == '\0' )
       osep = sep;
    sep_buf[0] = sep;
    sep_buf[1] = '\0';
    pt1 = strtok_r(fargs[0], sep_buf, &s_tok);
    do_count = 0;
    while (pt1) {
       init_match(player, pt1, NOTYPE);
       match_everything(MAT_EXIT_PARENTS);
       obj = noisy_match_result();
       if (obj == NOTHING) {
          stype = TYPE_MASK;
       } else if (Going(obj) || Recover(obj)) {
          stype = TYPE_MASK;
       } else if (Cloak(obj) && (SCloak(obj)) && !Immortal(player)) {
          stype = TYPE_MASK;
       } else if (Cloak(obj) && !Wizard(player)) {
          stype = TYPE_MASK;
       } else {
          stype = Typeof(obj);
       }
       if ( stype < 0 || stype > 7 )
          stype = TYPE_MASK;
       if ( bit_type & chk_types[stype] ) {
          if ( do_count )
             safe_chr(osep, buff, bufcx);
          do_count = 1;
          safe_str(pt1, buff, bufcx);
       }
       pt1 = strtok_r(NULL, sep_buf, &s_tok);
    }
}

FUNCTION(fun_mailalias)
{
   char *retval;
   int keyval;

   if (!fn_range_check("MAILALIAS", nfargs, 1, 2, buff, bufcx))
      return;

   if ( mudstate.mail_state != 1 ) {
      safe_str("#-1 MAIL SYSTEM IS CURRENTLY OFF", buff, bufcx);
      return;
   }
   if ( strlen(fargs[0]) > 16 ) {
      safe_str("#-1 ALIAS TOO LONG", buff, bufcx);
      return;
   }
   if ( nfargs == 2 ) {
      keyval = atoi(fargs[1]);
      if ( keyval < 0 || keyval > 1 )
         keyval = 0;
   } else {
      keyval = 0;
   }
   retval = (char *)mail_alias_function(player, keyval, fargs[0], NULL);
   safe_str(retval, buff, bufcx);
   free_lbuf(retval);
}

FUNCTION(fun_mailquota)
{
   dbref target;
   int i_key, i_user, i_umax, i_saved, i_samax, i_sent, i_semax;
   char *m_buff;

   if (!fn_range_check("MAILQUOTA", nfargs, 1, 2, buff, bufcx))
      return;

   if ( !*fargs[0] ) {
      safe_str((char *)"-1 -1 -1 -1 -1 -1", buff, bufcx); 
      return;
   }
   target = lookup_player(player, fargs[0], 0);
   if ( !Good_chk(target) ) {
      safe_str((char *)"-1 -1 -1 -1 -1 -1", buff, bufcx); 
      return;
   }
   i_key = 0;
   if ( (nfargs > 1) && *fargs[1] ) {
      i_key = atoi(fargs[1]);
   }
   i_user = i_umax = i_saved = i_samax = i_sent = i_semax = -1;
   mail_quota(player, fargs[0], 1, &i_user, &i_saved, &i_sent, &i_umax, &i_samax, &i_semax);
   m_buff = alloc_mbuf("fun_mailquota");
   switch (i_key) {
      case 0: sprintf(m_buff, "%d %d %d %d %d %d", i_user, i_umax, i_saved, i_samax, i_sent, i_semax);
              break;
      case 1: sprintf(m_buff, "%d %d", i_user, i_umax);
              break;
      case 2: sprintf(m_buff, "%d %d", i_saved, i_samax);
              break;
      case 3: sprintf(m_buff, "%d %d", i_sent, i_semax);
              break;
      default: strcpy(m_buff, (char *)"-1 -1 -1 -1 -1 -1");
              break;
   }
   safe_str(m_buff, buff, bufcx);
   free_mbuf(m_buff);
}

FUNCTION(fun_mailquick)
{
   char *retval;
   dbref it;
   int keyval;

   keyval = 0;
   if (!fn_range_check("MAILQUICK", nfargs, 1, 3, buff, bufcx))
      return;
   if ( mudstate.mail_state != 1 ) {
      safe_str("#-1 MAIL SYSTEM IS CURRENTLY OFF", buff, bufcx);
      return;
   }
   it = match_thing(player, fargs[0]);
   if (!Good_obj(it) || Recover(it) || Going(it) || !isPlayer(it)) {
      safe_str("#-1 NOT FOUND", buff, bufcx);
      return;
   }
   if ( !(Controls(player, it) || Immortal(player)) ) {
      safe_str("-1 -1 -1 -1 -1 -1", buff, bufcx);
      return;
   }
   if ( nfargs == 3 ) {
      keyval = atoi(fargs[2]);
      if ( keyval > 3 || keyval < 0 )
         keyval = 0;
   }
   if ( nfargs == 1 )
      retval = (char *)mail_quick_function(it, "", keyval);
   else
      retval = (char *)mail_quick_function(it, fargs[1], keyval);
   safe_str(retval, buff, bufcx);
   free_lbuf(retval);
}

FUNCTION(fun_folderlist)
{
   char *tbuff, *tbuffptr;

   if ( !fargs[0] || !*fargs[0] ) {
      safe_str("#-1 NOT FOUND", buff, bufcx);
      return;
   }
   tbuffptr = tbuff = alloc_lbuf("folderlist");
   folder_plist_function(player, fargs[0], tbuff, tbuffptr);
   safe_str(tbuff, buff, bufcx);
   free_lbuf(tbuff);
}

FUNCTION(fun_foldercurrent)
{
   int i_key;
   char *tbuff, *tbuffptr;

   if (!fn_range_check("FOLDERCURRENT", nfargs, 1, 2, buff, bufcx))
      return;

   if ( !fargs[0] || !*fargs[0] ) {
      safe_str("#-1 NOT FOUND", buff, bufcx);
      return;
   }
   i_key = 0;
   if ( (nfargs > 1) && fargs[1] && *fargs[1] )
      i_key = ((atoi(fargs[1]) > 0) ? 1 : 0);
   tbuffptr = tbuff = alloc_lbuf("folderlist");
   folder_current_function(player, i_key, fargs[0], tbuff, tbuffptr);
   safe_str(tbuff, buff, bufcx);
   free_lbuf(tbuff);
}

FUNCTION(fun_mailstatus)
{
   char *r_buff, *r_buffptr, *tbuff;
   dbref it;

   if (!fn_range_check("MAILSTATUS", nfargs, 1, 2, buff, bufcx))
      return;
   if ( mudstate.mail_state != 1 ) {
      safe_str("#-1 MAIL SYSTEM IS CURRENTLY OFF", buff, bufcx);
      return;
   }
   it = match_thing(player, fargs[0]);
   if (!Good_obj(it) || Recover(it) || Going(it) || !isPlayer(it)) {
      safe_str("#-1 NOT FOUND", buff, bufcx);
      return;
   }
   if ( !(Controls(player, it) || Immortal(player)) ) {
        safe_str("#-1 PERMISSION DENIED", buff, bufcx);
        return;
   }
   r_buffptr = r_buff = alloc_lbuf("fun_mailstatus");
   tbuff = alloc_lbuf("fun_mailstatus_temp");
   if ( nfargs > 1 )
      strcpy(tbuff, fargs[1]);
   mail_status(it, tbuff, NOTHING, 1, 0, r_buff, r_buffptr);
   free_lbuf(tbuff);
   safe_str(r_buff, buff, bufcx);
   free_lbuf(r_buff);
}

FUNCTION(fun_zfuneval)
{
    dbref aowner, thing;
    int aflags, anum, tlev, goodzone, tval;
    ATTR *ap;
    ZLISTNODE *ptr;
    char *atext, *result, *pt1, *pt2, *lbuf;

    /* We need at least one argument */

    if (nfargs < 1) {
       safe_str("#-1 FUNCTION (ZFUNEVAL) EXPECTS 1 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    pt1 = index(fargs[0], '/');
    if (pt1) {
       lbuf = alloc_lbuf("ueval");
       pt2 = index(pt1 + 1, '/');
       if (!pt2)
           pt2 = pt1;
       *pt2 = '\0';
       strcpy(lbuf, pt2 + 1);
    } else {
       safe_str("#-1 NO LEVEL SPECIFIED", buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    if (parse_attrib_zone(player, fargs[0], &thing, &anum)) {
        if ((anum == NOTHING) || (!Good_obj(thing)))
            ap = NULL;
        else
            ap = atr_num(anum);
    } else {
        ptr = db[player].zonelist;
        if ( ptr == NULL ) {
           safe_str("#-1 INVALID ZONE", buff, bufcx);
     free_lbuf(lbuf);
           return;
        }
        thing = ptr->object;
        ap = atr_str(fargs[0]);
    }

    /* Make sure it's a valid zone on the target */
    goodzone = 0;
    for( ptr = db[player].zonelist; ptr; ptr = ptr->next ) {
       if ( ptr->object == thing && thing != NOTHING ) {
          goodzone = 1;
          break;
       }
    }
    if ( !goodzone ) {
       safe_str("#-1 INVALID ZONE", buff, bufcx);
       free_lbuf(lbuf);
       return;
    }

    /* Make sure we got a good attribute */

    if (!ap) {
       free_lbuf(lbuf);
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       return;
    }
    /* Use it if we can access it, otherwise return an error. */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
       free_lbuf(lbuf);
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(lbuf);
       free_lbuf(atext);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(lbuf);
       free_lbuf(atext);
       return;
    }
    if (!*atext) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       free_lbuf(lbuf);
       return;
    }
    if (!check_read_perms2(player, thing, ap, aowner, aflags) &&
        !could_doit(Owner(player), thing, A_LZONEWIZ, 0, 0) ) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       free_lbuf(lbuf);
       return;
    }
    if (mudstate.evalnum < MAXEVLEVEL) {
       tlev = search_nametab(player, evaltab_sw, lbuf);
       free_lbuf(lbuf);
       if (God(player)) {
           if (tlev > 5)
               tlev = -1;
       } else if (Immortal(player)) {
           if (tlev > 4)
               tlev = -1;
       } else if (Wizard(player)) {
           if (tlev > 3)
               tlev = -1;
       } else if (Admin(player)) {
           if (tlev > 2)
              tlev = -1;
       } else if (Builder(player)) {
           if (tlev > 1)
              tlev = -1;
       } else if (Guildmaster(player)) {
           if (tlev > 0)
              tlev = -1;
       } else
           tlev = -1;
       if (tlev != -1) {
           mudstate.evalstate[mudstate.evalnum] = tlev;
           mudstate.evaldb[mudstate.evalnum++] = player;
       }
    } else {
       tlev = -1;
       free_lbuf(lbuf);
    }
    /* Evaluate it using the rest of the passed function args */

    tval = -1;
    if ( tlev == -1 ) {
       tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
    }
    result = exec(player, cause, player, EV_FCHECK | EV_EVAL, atext,
                  &(fargs[1]), nfargs - 1);
    safer_unufun(tval);
    free_lbuf(atext);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
    if (tlev != -1) {
       mudstate.evalnum--;
    }
}

FUNCTION(fun_streval)
{
    int tlev;
    char *result;

    /* We need at least one argument */

    if (nfargs < 2) {
       safe_str("#-1 FUNCTION (streval) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }

    result = exec(player, cause, player, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
    if (mudstate.evalnum < MAXEVLEVEL) {
       tlev = search_nametab(player, evaltab_sw, result);
       if (God(player)) {
           if (tlev > 5)
              tlev = -1;
       } else if (Immortal(player)) {
           if (tlev > 4)
              tlev = -1;
       } else if (Wizard(player)) {
           if (tlev > 3)
              tlev = -1;
       } else if (Admin(player)) {
           if (tlev > 2)
              tlev = -1;
       } else if (Builder(player)) {
           if (tlev > 1)
              tlev = -1;
       } else if (Guildmaster(player)) {
           if (tlev > 0)
              tlev = -1;
       } else
           tlev = -1;
       if (tlev != -1) {
           mudstate.evalstate[mudstate.evalnum] = tlev;
           mudstate.evaldb[mudstate.evalnum++] = player;
       }
    } else {
       tlev = -1;
    }
    free_lbuf(result);

    /* Evaluate it using the rest of the passed function args */

    result = exec(player, cause, player, EV_FCHECK | EV_EVAL, fargs[0],
                  &(fargs[2]), nfargs - 2);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
    if (tlev != -1) {
       mudstate.evalnum--;
    }
}

void
do_ueval(char *buff, char **bufcx, dbref player, dbref cause, dbref caller,
             char *fargs[], int nfargs, char *cargs[], int ncargs, int i_type)
{
    dbref aowner, thing;
    int aflags, anum, tlev, tval;
    ATTR *ap;
    char *atext, *result, *pt1, *pt2, *lbuf;

    /* We need at least one argument */

    if (nfargs < 1) {
       safe_str("#-1 FUNCTION (UEVAL) EXPECTS 1 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    pt1 = index(fargs[0], '/');
    if (pt1) {
       lbuf = alloc_lbuf("ueval");
       pt2 = index(pt1 + 1, '/');
       if (!pt2)
           pt2 = pt1;
       *pt2 = '\0';
       strcpy(lbuf, pt2 + 1);
    } else {
       safe_str("#-1 NO LEVEL SPECIFIED", buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
       thing = player;
       ap = atr_str(fargs[0]);
    }

    /* Make sure we got a good attribute */

    if (!ap) {
       free_lbuf(lbuf);
       return;
    }
    /* Use it if we can access it, otherwise return an error. */

    /* if it's a cluster, need to do cluster lookup and identification */
    if ( i_type ) {
       atext = find_cluster(thing, player, ap->number);
       thing = match_thing(player, atext);
       free_lbuf(atext);
       if ( !Good_chk(thing) || !Cluster(thing) ) {
          safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
          free_lbuf(lbuf);
          return;
       }
    }
    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
       free_lbuf(lbuf);
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       free_lbuf(lbuf);
       free_lbuf(atext);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       free_lbuf(lbuf);
       free_lbuf(atext);
       return;
    }
    if (!*atext) {
       free_lbuf(atext);
       free_lbuf(lbuf);
       return;
    }
    if (!check_read_perms(player, thing, ap, aowner, aflags, buff, bufcx)) {
       free_lbuf(atext);
       free_lbuf(lbuf);
       return;
    }
    if (mudstate.evalnum < MAXEVLEVEL) {
       tlev = search_nametab(player, evaltab_sw, lbuf);
       free_lbuf(lbuf);
       if (God(player)) {
           if (tlev > 5)
              tlev = -1;
       } else if (Immortal(player)) {
           if (tlev > 4)
              tlev = -1;
       } else if (Wizard(player)) {
           if (tlev > 3)
              tlev = -1;
       } else if (Admin(player)) {
           if (tlev > 2)
              tlev = -1;
       } else if (Builder(player)) {
           if (tlev > 1)
              tlev = -1;
       } else if (Guildmaster(player)) {
           if (tlev > 0)
              tlev = -1;
       } else
           tlev = -1;
       if (tlev != -1) {
           mudstate.evalstate[mudstate.evalnum] = tlev;
           mudstate.evaldb[mudstate.evalnum++] = player;
       }
    } else {
       free_lbuf(lbuf);
       tlev = -1;
    }
    /* Evaluate it using the rest of the passed function args */

    tval = -1;
    if ( tlev == -1 ) {
       tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
    }
    result = exec(player, cause, player, EV_FCHECK | EV_EVAL, atext,
                  &(fargs[1]), nfargs - 1);

    safer_unufun(tval);
    free_lbuf(atext);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
    if (tlev != -1) {
       mudstate.evalnum--;
    }
}

FUNCTION(fun_ueval) {
    do_ueval(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, 0);
}

FUNCTION(fun_cluster_ueval) {
    do_ueval(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, 1);
}

char *
strip_escapes(char *atext)
{
    char *retbuff, *atextptr, *retbuffptr;
    int esc_val;

    atextptr = atext;
    esc_val = 0;
    retbuffptr = retbuff = alloc_lbuf("strip_escapes");
    while ( *atextptr ) {
       if ( *atextptr == '\\' || *atextptr == '%' )
          esc_val = !esc_val;
       if ( !esc_val ) {
          if (*atextptr == '\\' || *atextptr == '%')
             safe_chr('X', retbuff, &retbuffptr);
          else
             safe_chr(*atextptr, retbuff, &retbuffptr);
       } else {
          safe_chr('X', retbuff, &retbuffptr);
       }
       if ( *atextptr != '\\' && *atextptr != '%' )
          esc_val = 0;
       atextptr++;
    }
    return retbuff;
}

int
paren_match2(char *atext)
{
    char *atextptr;
    int tcnt, i_err, i_pos;
    static int iarr[LBUF_SIZE];

    atextptr = atext;
    tcnt = i_err = i_pos = 0;
    while ( *atextptr ) {
          switch(*atextptr) {
             case ']' : iarr[tcnt] = 1;
                        tcnt++;
                        break;
             case ')' : iarr[tcnt] = 2;
                        tcnt++;
                        break;
             case '}' : iarr[tcnt] = 3;
                        tcnt++;
                        break;
             case '[' : if (tcnt == 0)
                           i_err = 1;
                        else if (iarr[tcnt-1] != 1)
                           i_err = 1;
                        tcnt--;
                        break;
             case '(' : if (tcnt == 0)
                           i_err = 1;
                        else if (iarr[tcnt-1] != 2)
                           i_err = 1;
                        tcnt--;
                        break;
             case '{' : if (tcnt == 0)
                           i_err = 1;
                        else if (iarr[tcnt-1] != 3)
                           i_err = 1;
                        tcnt--;
                        break;
       }
       if ( i_err || (tcnt < 0 ) ) {
          i_pos++;
          return i_pos;
       } else {
          atextptr++;
       }
       i_pos++;
    }
    return (tcnt > 0 ? i_pos : 0);
}

int
paren_match(char *atext, char *buff, char **bptr, int key, int i_type)
{
    char *atextptr;
    int oldtcnt, tcnt, i_clr, i_err, esc_val, i_pos;
    static int iarr[LBUF_SIZE];
    static char *s_clrs[] = {ANSI_MAGENTA, ANSI_GREEN, ANSI_YELLOW, ANSI_CYAN, ANSI_BLUE};

    i_pos = i_err = i_clr = tcnt = oldtcnt = esc_val = 0;
    atextptr = atext;
    while ( *atextptr ) {
       if ( (*atextptr == '%') || (*atextptr == '\\') )
          esc_val = !esc_val;
       if (!esc_val) {
          switch(*atextptr) {
             case '[' : iarr[tcnt] = 1;
                        tcnt++;
                        i_clr = tcnt;
                        break;
             case '(' : iarr[tcnt] = 2;
                        tcnt++;
                        i_clr = tcnt;
                        break;
             case '{' : iarr[tcnt] = 3;
                        tcnt++;
                        i_clr = tcnt;
                        break;
             case ']' : if (tcnt == 0)
                           i_err = 1;
                        else if (iarr[tcnt-1] != 1)
                           i_err = 1;
                        i_clr = tcnt;
                        tcnt--;
                        break;
             case ')' : if (tcnt == 0)
                           i_err = 1;
                        else if (iarr[tcnt-1] != 2)
                           i_err = 1;
                        i_clr = tcnt;
                        tcnt--;
                        break;
             case '}' : if (tcnt == 0)
                           i_err = 1;
                        else if (iarr[tcnt-1] != 3)
                           i_err = 1;
                        i_clr = tcnt;
                        tcnt--;
                        break;
          }
       }
       if ( (*atextptr != '%') && (*atextptr != '\\') )
          esc_val = 0;
       if ( i_err || (tcnt < 0) || (key == i_pos) ) {
          if ( i_type == 0 ) {
             safe_strmax(ANSI_RED, buff, bptr);
             safe_strmax(ANSI_HILITE, buff, bptr);
             safe_chr(*atextptr, buff, bptr);
             safe_str(ANSI_NORMAL, buff, bptr);
             atextptr++;
             safe_str(atextptr, buff, bptr);
             return 0;
          } else {
             atextptr = atext;
             *bptr = buff;
             tcnt = 0;
             while ( *atextptr && tcnt < i_pos ) {
                safe_chr(*atextptr, buff, bptr);
                atextptr++;
                tcnt++;
             }
             safe_strmax(ANSI_RED, buff, bptr);
             safe_strmax(ANSI_HILITE, buff, bptr);
             safe_chr(*atextptr, buff, bptr);
             safe_str(ANSI_NORMAL, buff, bptr);
             atextptr++;
             safe_str(atextptr, buff, bptr);
             return 0;
          }
       } else {
          if ( tcnt != oldtcnt ) {
             safe_strmax(s_clrs[i_clr % 5], buff, bptr);
             safe_chr(*atextptr, buff, bptr);
             safe_str(ANSI_NORMAL, buff, bptr);
          } else {
             safe_chr(*atextptr, buff, bptr);
          }
          oldtcnt = tcnt;
          atextptr++;
       }
       i_pos++;
    }
    return (tcnt > 0 ? i_pos : 0);
}

FUNCTION(fun_parenmatch)
{
    dbref aowner, thing;
    int aflags, anum, tcnt, i_type;
    ATTR *ap;
    char *atext, *atextptr, *revatext, *revatextptr;
    char *tbuff, *tbuffptr;

    if (mudstate.last_cmd_timestamp == mudstate.now) {
       mudstate.heavy_cpu_recurse += 1;
    }
    if ( mudstate.heavy_cpu_recurse > mudconf.heavy_cpu_max ) {
       safe_str("#-1 HEAVY CPU RECURSION LIMIT EXCEEDED", buff, bufcx);
       return;
    }
    if (!fn_range_check("PARENMATCH", nfargs, 1, 2, buff, bufcx))
        return;
    if (nfargs == 2) {
       i_type = atoi(fargs[1]);
    } else
       i_type = 0;
    if ( i_type < 0 || i_type > 1)
       i_type = 0;

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
       thing = player;
       ap = atr_str(fargs[0]);
    }
    if (!ap) {
       return;
    }
    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (!*atext) {
       free_lbuf(atext);
       return;
    }
    /* String is too long to do anything with anwyay.  Just return it */
    if ( strlen(atext) > (LBUF_SIZE - 14) ) {
        safe_str(atext, buff, bufcx);
        free_lbuf(atext);
        return;
    }
    if (!check_read_perms(player, thing, ap, aowner, aflags, buff, bufcx)) {
       free_lbuf(atext);
       return;
    }
    tcnt = 0;
    tbuffptr = tbuff = alloc_lbuf("fun_parenmatch");
    tcnt = paren_match(atext, tbuff, &tbuffptr, -1, i_type);
    if ( tcnt > 0 ) {
       revatextptr = revatext = alloc_lbuf("fun_parentmatch_rev");
       atextptr = strip_escapes(atext);
       do_reverse(atextptr, revatext, &revatextptr);
       free_lbuf(atextptr);
       tbuffptr = tbuff;
       tcnt = paren_match2(revatext);
       free_lbuf(revatext);
       tcnt = paren_match(atext, tbuff, &tbuffptr, (tcnt > 0 ? (strlen(atext)-tcnt) : -1), i_type);
       free_lbuf(atext);
       safe_str(tbuff, buff, bufcx);
       free_lbuf(tbuff);
    } else {
       free_lbuf(atext);
       safe_str(tbuff, buff, bufcx);
       free_lbuf(tbuff);
    }
}

FUNCTION(fun_u)
{
    dbref aowner, thing;
    int aflags, anum, tval;
    ATTR *ap;
    char *atext, *result;

    /* We need at least one argument */

    if (nfargs < 1) {
       safe_str("#-1 FUNCTION (U) EXPECTS 1 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
       thing = player;
       ap = atr_str(fargs[0]);
    }

    /* Make sure we got a good attribute */

    if (!ap) {
       return;
    }
    /* Use it if we can access it, otherwise return an error. */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (!*atext) {
       free_lbuf(atext);
       return;
    }
    if (!check_read_perms(player, thing, ap, aowner, aflags, buff, bufcx)) {
       free_lbuf(atext);
       return;
    }
    /* Evaluate it using the rest of the passed function args */

    tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
    if ( tval == -2 ) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
    } else {
       result = exec(player, cause, player, EV_FCHECK | EV_EVAL, atext,
                     &(fargs[1]), nfargs - 1);
    }
    free_lbuf(atext);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
    safer_unufun(tval);
}

FUNCTION(fun_zfun)
{
    dbref aowner, thing;
    int aflags, anum, goodzone, tval;
    ATTR *ap;
    ZLISTNODE *ptr;
    char *atext, *result;

    /* We need at least one argument */

    if (nfargs < 1) {
       safe_str("#-1 FUNCTION (ZFUN) EXPECTS 1 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */
    /* If <attr> assume the first zone in zone list */

    if (parse_attrib_zone(player, fargs[0], &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
        ptr = db[player].zonelist;
        if ( ptr == NULL ) {
           safe_str("#-1 INVALID ZONE", buff, bufcx);
           return;
        }
        thing = ptr->object;
        ap = atr_str(fargs[0]);
    }

    /* Make sure it's a valid zone on the target */
    goodzone = 0;
    for( ptr = db[player].zonelist; ptr; ptr = ptr->next ) {
       if ( ptr->object == thing && thing != NOTHING ) {
          goodzone = 1;
          break;
       }
    }
    if ( !goodzone ) {
       safe_str("#-1 INVALID ZONE", buff, bufcx);
       return;
    }

    /* Make sure we got a good attribute */
    if (!ap) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       return;
    }
    /* Use it if we can access it, otherwise return an error. */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (!*atext) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (!check_read_perms2(player, thing, ap, aowner, aflags) &&
        !could_doit(Owner(player), thing, A_LZONEWIZ, 0, 0) ) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    /* Evaluate it using the rest of the passed function args */

    tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
    if ( tval == -2 ) {
       result = alloc_lbuf("edefault_buff");
       sprintf(result ,"#-1 PERMISSION DENIED");
    } else {
       result = exec(player, cause, player, EV_FCHECK | EV_EVAL, atext,
                     &(fargs[1]), nfargs - 1);
    }
    safer_unufun(tval);
    free_lbuf(atext);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
}

FUNCTION(fun_zfun2)
{
    dbref aowner, thing;
    int aflags, anum, goodzone, tval;
    ATTR *ap;
    ZLISTNODE *ptr;
    char *atext, *result;

    /* We need at least one argument */

    if (nfargs < 1) {
       safe_str("#-1 FUNCTION (ZFUN2) EXPECTS 1 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */
    /* If <attr> assume the first zone in zone list */

    if (parse_attrib_zone(player, fargs[0], &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
        ptr = db[player].zonelist;
        if ( ptr == NULL ) {
           safe_str("#-1 INVALID ZONE", buff, bufcx);
           return;
        }
        thing = ptr->object;
        ap = atr_str(fargs[0]);
    }

    /* Make sure it's a valid zone on the target */
    goodzone = 0;
    for( ptr = db[player].zonelist; ptr; ptr = ptr->next ) {
       if ( ptr->object == thing && thing != NOTHING ) {
          goodzone = 1;
          break;
       }
    }
    if ( !goodzone ) {
       safe_str("#-1 INVALID ZONE", buff, bufcx);
       return;
    }

    /* Make sure we got a good attribute */
    if (!ap) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       return;
    }
    /* Use it if we can access it, otherwise return an error. */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (!*atext) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (!check_read_perms2(player, thing, ap, aowner, aflags) &&
        !could_doit(Owner(player), thing, A_LZONEWIZ, 0, 0) ) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    /* Evaluate it using the rest of the passed function args */

    tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
    if ( tval == -2 ) {
       result = alloc_lbuf("edefault_buff");
       sprintf(result ,"#-1 PERMISSION DENIED");
    } else {
       result = exec(thing, cause, player, EV_FCHECK | EV_EVAL, atext,
                     &(fargs[1]), nfargs - 1);
    }
    safer_unufun(tval);
    free_lbuf(atext);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
}

FUNCTION(fun_zfunlocal)
{
    dbref aowner, thing;
    int aflags, anum, goodzone, x, tval;
    ATTR *ap;
    ZLISTNODE *ptr;
    char *atext, *result, *pt, *savereg[MAX_GLOBAL_REGS];

    /* We need at least one argument */

    if (nfargs < 1) {
       safe_str("#-1 FUNCTION (ZFUNLOCAL) EXPECTS 1 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */
    /* If <attr> assume the first zone in zone list */

    if (parse_attrib_zone(player, fargs[0], &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
        ptr = db[player].zonelist;
        if ( ptr == NULL ) {
           safe_str("#-1 INVALID ZONE", buff, bufcx);
           return;
        }
        thing = ptr->object;
        ap = atr_str(fargs[0]);
    }

    /* Make sure it's a valid zone on the target */
    goodzone = 0;
    for( ptr = db[player].zonelist; ptr; ptr = ptr->next ) {
       if ( ptr->object == thing && thing != NOTHING ) {
          goodzone = 1;
          break;
       }
    }
    if ( !goodzone ) {
       safe_str("#-1 INVALID ZONE", buff, bufcx);
       return;
    }

    /* Make sure we got a good attribute */
    if (!ap) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       return;
    }
    /* Use it if we can access it, otherwise return an error. */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (!*atext) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (!check_read_perms2(player, thing, ap, aowner, aflags) &&
        !could_doit(Owner(player), thing, A_LZONEWIZ, 0, 0) ) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    /* Evaluate it using the rest of the passed function args */

    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      savereg[x] = alloc_lbuf("ulocal_reg");
      pt = savereg[x];
      safe_str(mudstate.global_regs[x],savereg[x],&pt);
      if ( mudstate.global_regs_wipe == 1 ) {
         *mudstate.global_regs[x] = '\0';
      }
    }
    tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
    if ( tval == -2 ) {
       result = alloc_lbuf("edefault_buff");
       sprintf(result ,"#-1 PERMISSION DENIED");
    } else {
       result = exec(player, cause, player, EV_FCHECK | EV_EVAL, atext,
                     &(fargs[1]), nfargs - 1);
    }
    safer_unufun(tval);
    free_lbuf(atext);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      pt = mudstate.global_regs[x];
      safe_str(savereg[x],mudstate.global_regs[x],&pt);
      free_lbuf(savereg[x]);
    }
}

FUNCTION(fun_zfunldefault)
{
    dbref aowner, thing;
    int aflags, anum, goodzone, x, chkpass, i, tval;
    ATTR *ap;
    ZLISTNODE *ptr;
    char *atext, *result, *pass, *pt, *savereg[MAX_GLOBAL_REGS], *s_fargs0, *s_xargs[LBUF_SIZE/2];

    if (nfargs < 2) {
       safe_str("#-1 FUNCTION (ZFUNLOCAL) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    s_fargs0 = exec(player, cause, caller,  EV_FCHECK | EV_EVAL | EV_STRIP, fargs[0], cargs, ncargs);
    if (parse_attrib_zone(player, s_fargs0, &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
        ptr = db[player].zonelist;
        if ( ptr == NULL ) {
           safe_str("#-1 INVALID ZONE", buff, bufcx);
           return;
        }
        thing = ptr->object;
        ap = atr_str(s_fargs0);
    }
    free_lbuf(s_fargs0);

    /* Make sure it's a valid zone on the target */
    goodzone = 0;
    for( ptr = db[player].zonelist; ptr; ptr = ptr->next ) {
       if ( ptr->object == thing && thing != NOTHING ) {
          goodzone = 1;
          break;
       }
    }
    if ( !goodzone ) {
       safe_str("#-1 INVALID ZONE", buff, bufcx);
       return;
    }
    /* Make sure we got a good attribute */

    atext = NULL;
    chkpass = 1;
    if (!ap) {
       pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
    }
    else {
      atext = atr_pget(thing, ap->number, &aowner, &aflags);
      if (!atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      } else if (Cloak(thing) && !Wizard(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      } else if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      } else if (!*atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      } else if (!check_read_perms2(player, thing, ap, aowner, aflags) &&
                 !could_doit(Owner(player), thing, A_LZONEWIZ, 0, 0) ) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      } else {
         pass = atext;
         chkpass = 0;
      }
    }
    /* Evaluate it using the rest of the passed function args */

    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      savereg[x] = alloc_lbuf("ulocal_reg");
      pt = savereg[x];
      safe_str(mudstate.global_regs[x],savereg[x],&pt);
      if ( mudstate.global_regs_wipe == 1 ) {
         *mudstate.global_regs[x] = '\0';
      }
    }
    if ( nfargs > 2 ) {
       /* initialize it */
       for ( i = 0; i < (LBUF_SIZE/2); i++ )
          s_xargs[i] = NULL;
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          s_xargs[i] = exec(player, cause, player, EV_FCHECK | EV_EVAL, fargs[2+i], cargs, ncargs);
       }
       tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(player, cause, player, EV_FCHECK | EV_EVAL, pass, s_xargs, nfargs - 2);
       }
       safer_unufun(tval);
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          free_lbuf(s_xargs[i]);
       }
    } else {
       tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(player, cause, player, EV_FCHECK | EV_EVAL, pass, &(fargs[2]), nfargs - 2);
       }
       safer_unufun(tval);
    }
    if (atext)
       free_lbuf(atext);
    if ( chkpass )
       free_lbuf(pass);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      pt = mudstate.global_regs[x];
      safe_str(savereg[x],mudstate.global_regs[x],&pt);
      free_lbuf(savereg[x]);
    }
}

FUNCTION(fun_zfun2ldefault)
{
    dbref aowner, thing;
    int aflags, anum, goodzone, x, chkpass, i, tval;
    ATTR *ap;
    ZLISTNODE *ptr;
    char *atext, *result, *pass, *pt, *savereg[MAX_GLOBAL_REGS], *s_fargs0, *s_xargs[LBUF_SIZE/2];

    if (nfargs < 2) {
       safe_str("#-1 FUNCTION (ZFUN2LDEFAULT) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    s_fargs0 = exec(player, cause, caller,  EV_FCHECK | EV_EVAL | EV_STRIP, fargs[0], cargs, ncargs);
    if (parse_attrib_zone(player, s_fargs0, &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
        ptr = db[player].zonelist;
        if ( ptr == NULL ) {
           safe_str("#-1 INVALID ZONE", buff, bufcx);
           return;
        }
        thing = ptr->object;
        ap = atr_str(s_fargs0);
    }
    free_lbuf(s_fargs0);

    /* Make sure it's a valid zone on the target */
    goodzone = 0;
    for( ptr = db[player].zonelist; ptr; ptr = ptr->next ) {
       if ( ptr->object == thing && thing != NOTHING ) {
          goodzone = 1;
          break;
       }
    }
    if ( !goodzone ) {
       safe_str("#-1 INVALID ZONE", buff, bufcx);
       return;
    }
    /* Make sure we got a good attribute */

    atext = NULL;
    chkpass = 1;
    if (!ap) {
       pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
    }
    else {
      atext = atr_pget(thing, ap->number, &aowner, &aflags);
      if (!atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (Cloak(thing) && !Wizard(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (!*atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (!check_read_perms2(player, thing, ap, aowner, aflags) &&
               !could_doit(Owner(player), thing, A_LZONEWIZ, 0, 0) ) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else {
         pass = atext;
         chkpass = 0;
      }
    }
    /* Evaluate it using the rest of the passed function args */

    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      savereg[x] = alloc_lbuf("ulocal_reg");
      pt = savereg[x];
      safe_str(mudstate.global_regs[x],savereg[x],&pt);
      if ( mudstate.global_regs_wipe == 1 ) {
         *mudstate.global_regs[x] = '\0';
      }
    }
    if ( nfargs > 2 ) {
       /* initialize it */
       for ( i = 0; i < (LBUF_SIZE/2); i++ )
          s_xargs[i] = NULL;
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          s_xargs[i] = exec(player, cause, player, EV_FCHECK | EV_EVAL, fargs[2+i], cargs, ncargs);
       }
       tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(thing, cause, player, EV_FCHECK | EV_EVAL, pass, s_xargs, nfargs - 2);
       }
       safer_unufun(tval);
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          free_lbuf(s_xargs[i]);
       }
    } else {
       tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(thing, cause, player, EV_FCHECK | EV_EVAL, pass, &(fargs[2]), nfargs - 2);
       }
       safer_unufun(tval);
    }
    if (atext)
       free_lbuf(atext);
    if ( chkpass )
       free_lbuf(pass);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      pt = mudstate.global_regs[x];
      safe_str(savereg[x],mudstate.global_regs[x],&pt);
      free_lbuf(savereg[x]);
    }
}
FUNCTION(fun_zfundefault)
{
    dbref aowner, thing;
    int aflags, anum, goodzone, chkpass, i, tval;
    ATTR *ap;
    ZLISTNODE *ptr;
    char *atext, *result, *pass, *s_fargs0, *s_xargs[LBUF_SIZE/2];

    if (nfargs < 2) {
       safe_str("#-1 FUNCTION (ZFUNDEFAULT) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    s_fargs0 = exec(player, cause, caller,  EV_FCHECK | EV_EVAL | EV_STRIP, fargs[0], cargs, ncargs);
    if (parse_attrib_zone(player, s_fargs0, &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
        ptr = db[player].zonelist;
        if ( ptr == NULL ) {
           safe_str("#-1 INVALID ZONE", buff, bufcx);
           return;
        }
        thing = ptr->object;
        ap = atr_str(s_fargs0);
    }
    free_lbuf(s_fargs0);

    /* Make sure it's a valid zone on the target */
    goodzone = 0;
    for( ptr = db[player].zonelist; ptr; ptr = ptr->next ) {
       if ( ptr->object == thing && thing != NOTHING ) {
          goodzone = 1;
          break;
       }
    }
    if ( !goodzone ) {
       safe_str("#-1 INVALID ZONE", buff, bufcx);
       return;
    }
    /* Make sure we got a good attribute */

    atext = NULL;
    chkpass = 1;
    if (!ap) {
       pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
    }
    else {
      atext = atr_pget(thing, ap->number, &aowner, &aflags);
      if (!atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (Cloak(thing) && !Wizard(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (!*atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (!check_read_perms2(player, thing, ap, aowner, aflags) &&
               !could_doit(Owner(player), thing, A_LZONEWIZ, 0, 0) ) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else {
         pass = atext;
         chkpass = 0;
      }
    }
    /* Evaluate it using the rest of the passed function args */

    if ( nfargs > 2 ) {
       /* initialize it */
       for ( i = 0; i < (LBUF_SIZE/2); i++ )
          s_xargs[i] = NULL;
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          s_xargs[i] = exec(player, cause, player, EV_FCHECK | EV_EVAL, fargs[2+i], cargs, ncargs);
       }
       tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(player, cause, player, EV_FCHECK | EV_EVAL, pass, s_xargs, nfargs - 2);
       }
       safer_unufun(tval);
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          free_lbuf(s_xargs[i]);
       }
    } else {
       tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(player, cause, player, EV_FCHECK | EV_EVAL, pass, &(fargs[2]), nfargs - 2);
       }
       safer_unufun(tval);
    }
    if (atext)
       free_lbuf(atext);
    if ( chkpass )
       free_lbuf(pass);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
}

FUNCTION(fun_udefault)
{
    dbref aowner, thing;
    int aflags, anum, chkpass, i, tval;
    ATTR *ap;
    char *atext, *result, *pass, *s_fargs0, *s_xargs[LBUF_SIZE/2];

    if (nfargs < 2) {
       safe_str("#-1 FUNCTION (UDEFAULT) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    s_fargs0 = exec(player, cause, caller,  EV_FCHECK | EV_EVAL | EV_STRIP, fargs[0], cargs, ncargs);
    if (parse_attrib(player, s_fargs0, &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing))) {
          ap = NULL;
       } else {
         ap = atr_num(anum);
       }
    } else {
       thing = player;
       ap = atr_str(s_fargs0);
    }
    free_lbuf(s_fargs0);

    /* Make sure we got a good attribute */

    atext = NULL;
    pass = NULL;
    chkpass = 1;
    if (!ap) {
       pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
    } else {
      atext = atr_pget(thing, ap->number, &aowner, &aflags);
      if (!atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      } else if (Cloak(thing) && !Wizard(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      } else if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      } else if (!*atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      } else if (!check_read_perms(player, thing, ap, aowner, aflags, buff, bufcx)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      } else {
         pass = atext;
         chkpass = 0;
      }
    }
    /* Evaluate it using the rest of the passed function args */

    /* Evaluate the arguments to the functions */
    if ( nfargs > 2 ) {
       /* initialize it */
       for ( i = 0; i < (LBUF_SIZE/2); i++ ) 
          s_xargs[i] = NULL;
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          s_xargs[i] = exec(player, cause, player, EV_FCHECK | EV_EVAL, fargs[2+i], cargs, ncargs);
       }
       tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(player, cause, player, EV_FCHECK | EV_EVAL, pass, s_xargs, nfargs - 2);
       }
       safer_unufun(tval);
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          free_lbuf(s_xargs[i]);
       }
    } else {
       tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(player, cause, player, EV_FCHECK | EV_EVAL, pass, &(fargs[2]), nfargs - 2);
       }
       safer_unufun(tval);
    }
    if (atext)
       free_lbuf(atext);
    if ( chkpass )
       free_lbuf(pass);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
}

FUNCTION(fun_u2)
{
    dbref aowner, thing;
    int aflags, anum, tval;
    ATTR *ap;
    char *atext, *result;

    /* We need at least one argument */

    if (nfargs < 1) {
       safe_str("#-1 FUNCTION (U2) EXPECTS 1 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
        if ((anum == NOTHING) || (!Good_obj(thing)))
            ap = NULL;
        else
            ap = atr_num(anum);
    } else {
        thing = player;
        ap = atr_str(fargs[0]);
    }

    /* Make sure we got a good attribute */

    if (!ap) {
        return;
    }
    /* Use it if we can access it, otherwise return an error. */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
        return;
    }
    if (Cloak(thing) && !Wizard(player)) {
        safe_str("#-1 PERMISSION DENIED", buff, bufcx);
        free_lbuf(atext);
        return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
        safe_str("#-1 PERMISSION DENIED", buff, bufcx);
        free_lbuf(atext);
        return;
    }
    if (!*atext) {
        free_lbuf(atext);
        return;
    }
    if (!check_read_perms(player, thing, ap, aowner, aflags, buff, bufcx)) {
        free_lbuf(atext);
        return;
    }
    /* Evaluate it using the rest of the passed function args */

    tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
    if ( tval == -2 ) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
    } else {
       result = exec(thing, cause, player, EV_FCHECK | EV_EVAL, atext,
                     &(fargs[1]), nfargs - 1);
    }
    free_lbuf(atext);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
    safer_unufun(tval);
}

FUNCTION(fun_zfun2default)
{
    dbref aowner, thing;
    int aflags, anum, goodzone, i, tval;
    ATTR *ap;
    ZLISTNODE *ptr;
    char *atext, *result, *pass, *s_fargs0, *s_xargs[LBUF_SIZE/2];

    if (nfargs < 2) {
        safe_str("#-1 FUNCTION (ZFUN2DEFAULT) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
        ival(buff, bufcx, nfargs);
        safe_chr(']', buff, bufcx);
        return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    s_fargs0 = exec(player, cause, caller,  EV_FCHECK | EV_EVAL | EV_STRIP, fargs[0], cargs, ncargs);
    if (parse_attrib_zone(player, s_fargs0, &thing, &anum)) {
        if ((anum == NOTHING) || (!Good_obj(thing)))
            ap = NULL;
        else
            ap = atr_num(anum);
    } else {
        ptr = db[player].zonelist;
        if ( ptr == NULL ) {
           safe_str("#-1 INVALID ZONE", buff, bufcx);
           return;
        }
        thing = ptr->object;
        ap = atr_str(s_fargs0);
    }
    free_lbuf(s_fargs0);

    /* Make sure it's a valid zone on the target */
    goodzone = 0;
    for( ptr = db[player].zonelist; ptr; ptr = ptr->next ) {
       if ( ptr->object == thing && thing != NOTHING ) {
          goodzone = 1;
          break;
       }
    }
    if ( !goodzone ) {
       safe_str("#-1 INVALID ZONE", buff, bufcx);
       return;
    }
    /* Make sure we got a good attribute */

    atext = NULL;
    if (!ap) {
       pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
    }
    else {
      atext = atr_pget(thing, ap->number, &aowner, &aflags);
      if (!atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (Cloak(thing) && !Wizard(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (!*atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (!check_read_perms2(player, thing, ap, aowner, aflags) &&
               !could_doit(Owner(player), thing, A_LZONEWIZ, 0, 0) ) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else {
         pass = atext;
      }
    }
    /* Evaluate it using the rest of the passed function args */

    if ( nfargs > 2 ) {
       /* initialize it */
       for ( i = 0; i < (LBUF_SIZE/2); i++ )
          s_xargs[i] = NULL;
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          s_xargs[i] = exec(player, cause, player, EV_FCHECK | EV_EVAL, fargs[2+i], cargs, ncargs);
       }
       tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(thing, cause, player, EV_FCHECK | EV_EVAL, pass, s_xargs, nfargs - 2);
       }
       safer_unufun(tval);
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          free_lbuf(s_xargs[i]);
       }
    } else {
       tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(thing, cause, player, EV_FCHECK | EV_EVAL, pass, &(fargs[2]), nfargs - 2);
       }
       safer_unufun(tval);
    }
    if (atext)
      free_lbuf(atext);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
}

FUNCTION(fun_u2default)
{
    dbref aowner, thing;
    int aflags, anum, chkpass, i, tval;
    ATTR *ap;
    char *atext, *result, *pass, *s_fargs0, *s_xargs[LBUF_SIZE/2];

    if (nfargs < 2) {
       safe_str("#-1 FUNCTION (U2DEFAULT) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    s_fargs0 = exec(player, cause, caller,  EV_FCHECK | EV_EVAL | EV_STRIP, fargs[0], cargs, ncargs);
    if (parse_attrib(player, s_fargs0, &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
       thing = player;
       ap = atr_str(s_fargs0);
    }
    free_lbuf(s_fargs0);

    /* Make sure we got a good attribute */

    atext = NULL;
    chkpass = 1;
    if (!ap) {
       pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
    }
    else {
      atext = atr_pget(thing, ap->number, &aowner, &aflags);
      if (!atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (Cloak(thing) && !Wizard(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (!*atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (!check_read_perms(player, thing, ap, aowner, aflags, buff, bufcx)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else {
         pass = atext;
         chkpass = 0;
      }
    }
    /* Evaluate it using the rest of the passed function args */

    if ( nfargs > 2 ) {
       /* initialize it */
       for ( i = 0; i < (LBUF_SIZE/2); i++ )
          s_xargs[i] = NULL;
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          s_xargs[i] = exec(player, cause, player, EV_FCHECK | EV_EVAL, fargs[2+i], cargs, ncargs);
       }
       tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(thing, cause, player, EV_FCHECK | EV_EVAL, pass, s_xargs, nfargs - 2);
       }
       safer_unufun(tval);
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          free_lbuf(s_xargs[i]);
       }
    } else {
       tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(thing, cause, player, EV_FCHECK | EV_EVAL, pass, &(fargs[2]), nfargs - 2);
       }
       safer_unufun(tval);
    }
    if (atext)
       free_lbuf(atext);
    if ( chkpass )
       free_lbuf(pass);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
}

FUNCTION(fun_ulocal)
{
    dbref aowner, thing;
    int aflags, anum, x, tval;
    ATTR *ap;
    char *atext, *result, *pt, *savereg[MAX_GLOBAL_REGS];

    /* We need at least one argument */

    if (nfargs < 1) {
        safe_str("#-1 FUNCTION (ULOCAL) EXPECTS 1 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
        ival(buff, bufcx, nfargs);
        safe_chr(']', buff, bufcx);
        return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
        if ((anum == NOTHING) || (!Good_obj(thing)))
            ap = NULL;
        else
            ap = atr_num(anum);
    } else {
        thing = player;
        ap = atr_str(fargs[0]);
    }

    /* Make sure we got a good attribute */

    if (!ap) {
        return;
    }
    /* Use it if we can access it, otherwise return an error. */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
        return;
    }
    if (Cloak(thing) && !Wizard(player)) {
        safe_str("#-1 PERMISSION DENIED", buff, bufcx);
        free_lbuf(atext);
        return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
        safe_str("#-1 PERMISSION DENIED", buff, bufcx);
        free_lbuf(atext);
        return;
    }
    if (!*atext) {
        free_lbuf(atext);
        return;
    }
    if (!check_read_perms(player, thing, ap, aowner, aflags, buff, bufcx)) {
        free_lbuf(atext);
        return;
    }
    /* Evaluate it using the rest of the passed function args */

    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      savereg[x] = alloc_lbuf("ulocal_reg");
      pt = savereg[x];
      safe_str(mudstate.global_regs[x],savereg[x],&pt);
      if ( mudstate.global_regs_wipe == 1 ) {
         *mudstate.global_regs[x] = '\0';
      }
    }
    tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
    if ( tval == -2 ) {
       result = alloc_lbuf("edefault_buff");
       sprintf(result ,"#-1 PERMISSION DENIED");
    } else {
       result = exec(player, cause, player, EV_FCHECK | EV_EVAL, atext,
                     &(fargs[1]), nfargs - 1);
    }
    safer_unufun(tval);
    free_lbuf(atext);
    safe_str(result, buff, bufcx);
    free_lbuf(result);

    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      pt = mudstate.global_regs[x];
      safe_str(savereg[x],mudstate.global_regs[x],&pt);
      free_lbuf(savereg[x]);
    }
}

FUNCTION(fun_uldefault)
{
    dbref aowner, thing;
    int aflags, anum, x, chkpass, i, tval;
    ATTR *ap;
    char *atext, *result, *pass, *pt, *savereg[MAX_GLOBAL_REGS], *s_fargs0, *s_xargs[LBUF_SIZE/2];

    if (nfargs < 2) {
        safe_str("#-1 FUNCTION (ULDEFAULT) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
        ival(buff, bufcx, nfargs);
        safe_chr(']', buff, bufcx);
        return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    s_fargs0 = exec(player, cause, caller,  EV_FCHECK | EV_EVAL | EV_STRIP, fargs[0], cargs, ncargs);
    if (parse_attrib(player, s_fargs0, &thing, &anum)) {
        if ((anum == NOTHING) || (!Good_obj(thing)))
            ap = NULL;
        else
            ap = atr_num(anum);
    } else {
        thing = player;
        ap = atr_str(s_fargs0);
    }
    free_lbuf(s_fargs0);

    /* Make sure we got a good attribute */

    atext = NULL;
    chkpass = 1;
    if (!ap) {
       pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
    }
    else {
      atext = atr_pget(thing, ap->number, &aowner, &aflags);
      if (!atext) {
          pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (Cloak(thing) && !Wizard(player)) {
          pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
          pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (!*atext) {
          pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (!check_read_perms(player, thing, ap, aowner, aflags, buff, bufcx)) {
          pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else {
          pass = atext;
          chkpass = 0;
      }
    }
    /* Evaluate it using the rest of the passed function args */

    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      savereg[x] = alloc_lbuf("ulocal_reg");
      pt = savereg[x];
      safe_str(mudstate.global_regs[x],savereg[x],&pt);
      if ( mudstate.global_regs_wipe == 1 ) {
         *mudstate.global_regs[x] = '\0';
      }
    }

    if ( nfargs > 2 ) {
       /* initialize it */
       for ( i = 0; i < (LBUF_SIZE/2); i++ )
          s_xargs[i] = NULL;
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          s_xargs[i] = exec(player, cause, player, EV_FCHECK | EV_EVAL, fargs[2+i], cargs, ncargs);
       }
       tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(player, cause, player, EV_FCHECK | EV_EVAL, pass, s_xargs, nfargs - 2);
       }
       safer_unufun(tval);
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          free_lbuf(s_xargs[i]);
       }
    } else {
       tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(player, cause, player, EV_FCHECK | EV_EVAL, pass, &(fargs[2]), nfargs - 2);
       }
       safer_unufun(tval);
    }
    if (atext)
       free_lbuf(atext);
    if ( chkpass )
       free_lbuf(pass);
    safe_str(result, buff, bufcx);
    free_lbuf(result);

    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      pt = mudstate.global_regs[x];
      safe_str(savereg[x],mudstate.global_regs[x],&pt);
      free_lbuf(savereg[x]);
    }
}

FUNCTION(fun_u2local)
{
    dbref aowner, thing;
    int aflags, anum, x, tval;
    ATTR *ap;
    char *atext, *result, *pt, *savereg[MAX_GLOBAL_REGS];

    /* We need at least one argument */

    if (nfargs < 1) {
       safe_str("#-1 FUNCTION (U2LOCAL) EXPECTS 1 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
       thing = player;
       ap = atr_str(fargs[0]);
    }

    /* Make sure we got a good attribute */

    if (!ap) {
       return;
    }
    /* Use it if we can access it, otherwise return an error. */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (!*atext) {
       free_lbuf(atext);
       return;
    }
    if (!check_read_perms(player, thing, ap, aowner, aflags, buff, bufcx)) {
       free_lbuf(atext);
       return;
    }
    /* Evaluate it using the rest of the passed function args */

    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      savereg[x] = alloc_lbuf("ulocal_reg");
      pt = savereg[x];
      safe_str(mudstate.global_regs[x],savereg[x],&pt);
      if ( mudstate.global_regs_wipe == 1 ) {
         *mudstate.global_regs[x] = '\0';
      }
    }
    tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
    if ( tval == -2 ) {
       result = alloc_lbuf("edefault_buff");
       sprintf(result ,"#-1 PERMISSION DENIED");
    } else {
       result = exec(thing, cause, player, EV_FCHECK | EV_EVAL, atext,
                     &(fargs[1]), nfargs - 1);
    }
    safer_unufun(tval);
    free_lbuf(atext);
    safe_str(result, buff, bufcx);
    free_lbuf(result);

    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      pt = mudstate.global_regs[x];
      safe_str(savereg[x],mudstate.global_regs[x],&pt);
      free_lbuf(savereg[x]);
    }
}

FUNCTION(fun_zfun2local)
{
    dbref aowner, thing;
    int aflags, anum, goodzone, x, tval;
    ATTR *ap;
    ZLISTNODE *ptr;
    char *atext, *result, *pt, *savereg[MAX_GLOBAL_REGS];

    /* We need at least one argument */

    if (nfargs < 1) {
       safe_str("#-1 FUNCTION (ZFUN2LOCAL) EXPECTS 1 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */
    /* If <attr> assume the first zone in zone list */

    if (parse_attrib_zone(player, fargs[0], &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
        ptr = db[player].zonelist;
        if ( ptr == NULL ) {
           safe_str("#-1 INVALID ZONE", buff, bufcx);
           return;
        }
        thing = ptr->object;
        ap = atr_str(fargs[0]);
    }

    /* Make sure it's a valid zone on the target */
    goodzone = 0;
    for( ptr = db[player].zonelist; ptr; ptr = ptr->next ) {
       if ( ptr->object == thing && thing != NOTHING ) {
          goodzone = 1;
          break;
       }
    }
    if ( !goodzone ) {
       safe_str("#-1 INVALID ZONE", buff, bufcx);
       return;
    }

    /* Make sure we got a good attribute */
    if (!ap) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       return;
    }
    /* Use it if we can access it, otherwise return an error. */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (!*atext) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    if (!check_read_perms2(player, thing, ap, aowner, aflags) &&
        !could_doit(Owner(player), thing, A_LZONEWIZ, 0, 0) ) {
       safe_str("#-1 NO SUCH USER FUNCTION", buff, bufcx);
       free_lbuf(atext);
       return;
    }
    /* Evaluate it using the rest of the passed function args */

    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      savereg[x] = alloc_lbuf("ulocal_reg");
      pt = savereg[x];
      safe_str(mudstate.global_regs[x],savereg[x],&pt);
      if ( mudstate.global_regs_wipe == 1 ) {
         *mudstate.global_regs[x] = '\0';
      }
    }
    tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
    if ( tval == -2 ) {
       result = alloc_lbuf("edefault_buff");
       sprintf(result ,"#-1 PERMISSION DENIED");
    } else {
       result = exec(thing, cause, player, EV_FCHECK | EV_EVAL, atext,
                     &(fargs[1]), nfargs - 1);
    }
    safer_unufun(tval);
    free_lbuf(atext);
    safe_str(result, buff, bufcx);
    free_lbuf(result);
    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      pt = mudstate.global_regs[x];
      safe_str(savereg[x],mudstate.global_regs[x],&pt);
      free_lbuf(savereg[x]);
    }
}

FUNCTION(fun_u2ldefault)
{
    dbref aowner, thing;
    int aflags, anum, x, chkpass, i, tval;
    ATTR *ap;
    char *atext, *result, *pass, *pt, *savereg[MAX_GLOBAL_REGS], *s_fargs0, *s_xargs[LBUF_SIZE/2];

    if (nfargs < 2) {
       safe_str("#-1 FUNCTION (U2LDEFAULT) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    s_fargs0 = exec(player, cause, caller,  EV_FCHECK | EV_EVAL | EV_STRIP, fargs[0], cargs, ncargs);
    if (parse_attrib(player, s_fargs0, &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
       thing = player;
       ap = atr_str(s_fargs0);
    }
    free_lbuf(s_fargs0);

    /* Make sure we got a good attribute */

    atext = NULL;
    chkpass = 1;
    if (!ap) {
       pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
    }
    else {
      atext = atr_pget(thing, ap->number, &aowner, &aflags);
      if (!atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (Cloak(thing) && !Wizard(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (!*atext) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else if (!check_read_perms(player, thing, ap, aowner, aflags, buff, bufcx)) {
         pass = exec(player, cause, caller, EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
      }
      else {
         pass = atext;
         chkpass = 0;
      }
    }
    /* Evaluate it using the rest of the passed function args */

    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      savereg[x] = alloc_lbuf("ulocal_reg");
      pt = savereg[x];
      safe_str(mudstate.global_regs[x],savereg[x],&pt);
      if ( mudstate.global_regs_wipe == 1 ) {
         *mudstate.global_regs[x] = '\0';
      }
    }

    if ( nfargs > 2 ) {
       /* initialize it */
       for ( i = 0; i < (LBUF_SIZE/2); i++ )
          s_xargs[i] = NULL;
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          s_xargs[i] = exec(player, cause, player, EV_FCHECK | EV_EVAL, fargs[2+i], cargs, ncargs);
       }
       tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(thing, cause, player, EV_FCHECK | EV_EVAL, pass, s_xargs, nfargs - 2);
       }
       safer_unufun(tval);
       for ( i = 0; ( (i < (nfargs - 2)) && (i < MAX_ARGS) ); i++) {
          free_lbuf(s_xargs[i]);
       }
    } else {
       tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
       if ( tval == -2 ) {
          result = alloc_lbuf("edefault_buff");
          sprintf(result ,"#-1 PERMISSION DENIED");
       } else {
          result = exec(thing, cause, player, EV_FCHECK | EV_EVAL, pass, &(fargs[2]), nfargs - 2);
       }
       safer_unufun(tval);
    }

    if (atext)
       free_lbuf(atext);
    if ( chkpass )
       free_lbuf(pass);
    safe_str(result, buff, bufcx);
    free_lbuf(result);

    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      pt = mudstate.global_regs[x];
      safe_str(savereg[x],mudstate.global_regs[x],&pt);
      free_lbuf(savereg[x]);
    }
}

FUNCTION(fun_elements)
{
  char delim, *pt1, *pt2, *pos1, *pos2, *mybuff, filler;
  int x, place, got, end;

  if (!fn_range_check("ELEMENTS", nfargs, 2, 4, buff, bufcx)) {
    return;
  }
  if (!*fargs[0] || !*fargs[1])
    return;
  if ((nfargs > 2) && (*fargs[2]))
    delim = *fargs[2];
  else
    delim = ' ';
  if ((nfargs > 3) && (*fargs[3]))
    filler = *fargs[3];
  else
    filler = delim;
  pos1 = fargs[1];
  while (*pos1 && isspace((int)*pos1))
    pos1++;
  got = 0;
  mybuff = alloc_lbuf("fun_element");
  if (mybuff == NULL)
    return;
  memset(mybuff, 0, LBUF_SIZE);
  while (*pos1) {
    pos2 = pos1 + 1;
    while (*pos2 && !isspace((int)*pos2))
      pos2++;
    if (*pos2) {
      end = 0;
      *pos2 = '\0';
    }
    else
      end = 1;
    place = atoi(pos1);
    if ((place > 0) && (place < LBUF_SIZE)) {
      pt1 = fargs[0];
      while (*pt1 && (*pt1 == delim)) {
  pt1++;
      }
      if (*pt1) {
  for (x = 1; x < place; x++) {
    while (*pt1 && (*pt1 != delim))
      pt1++;
    if (!*pt1)
      break;
    while (*pt1 && (*pt1 == delim)) {
      pt1++;
          }
  }
  if (*pt1) {
    pt2 = pt1+1;
    while (*pt2 && (*pt2 != delim))
      pt2++;
    x = pt2 - pt1;
    if (x >= LBUF_SIZE)
      x = LBUF_SIZE - 1;
    strncpy(mybuff,pt1,x);
    *(mybuff+x) = '\0';
    if (got)
      safe_chr(filler, buff, bufcx);
    safe_str(mybuff, buff, bufcx);
    got = 1;
  }
      }
    }
    if (end)
      pos1 = pos2;
    else {
      pos1 = pos2 + 1;
      while (*pos1 && isspace((int)*pos1))
  pos1++;
    }
  }
  free_lbuf(mybuff);
}

/******************************************************************
 * Elements - taken from MUX
 *
 ******************************************************************/
FUNCTION(fun_elementsmux)
{
  int nwords, cur, bFirst;
  char *ptrs[LBUF_SIZE / 2];
  char *wordlist, *s, *r, sep, osep;
  svarargs_preamble("ELEMENTSMUX", 4);
  bFirst = 1;

  /* Turn the first list into an array. */
  wordlist = alloc_lbuf("fun_elements.wordlist");

  /* Same size - no worries */
  strcpy(wordlist, fargs[0]);
  nwords = list2arr(ptrs, LBUF_SIZE / 2, wordlist, sep);
  s = trim_space_sep(fargs[1], ' ');

  /* Go through the second list, grabbing the numbers and finding the
   * corresponding elements.
   */

  do {
    r = split_token(&s, ' ');
    cur = atoi(r) - 1;
    if (  (cur >= 0) && (cur < nwords) && ptrs[cur]) {
      if (!bFirst) {
  safe_chr(osep, buff, bufcx);
      }
      bFirst = 0;
      safe_str(ptrs[cur], buff, bufcx);
    }
  } while (s);
  free_lbuf(wordlist);
}

/* ---------------------------------------------------------------------------
 * fun_parent: Get parent of object.
 */

FUNCTION(fun_parents)
{
    dbref it;
    int max_lev, gotone = 0;

    it = match_thing(player, fargs[0]);
    max_lev = 0;
    while (Good_obj(it) && (it != NOTHING) && (it != AMBIGUOUS) && (Examinable(player, it) || (it == cause))) {
  if (Good_obj(Parent(it))) {
      if (gotone) {
    safe_chr(' ', buff, bufcx);
            }
            dbval(buff, bufcx, Parent(it));
      gotone = 1;
  }
  max_lev++;
  if ( max_lev > mudconf.parent_nest_lim )
     break;
  it = Parent(it);
    }
    if (!gotone)
  safe_str("#-1", buff, bufcx);
}

FUNCTION(fun_parent)
{
    dbref it;
#ifdef USE_SIDEEFFECT
    CMDENT *cmdp;

#endif
    it = match_thing(player, fargs[0]);
#ifdef USE_SIDEEFFECT
    if (!fn_range_check("PARENT", nfargs, 1, 2, buff, bufcx))
        return;
    if (nfargs > 1) {
        mudstate.sidefx_currcalls++;
        if ( !(mudconf.sideeffects & SIDE_PARENT) ) {
           notify(player, "#-1 SIDE-EFFECT PORTION OF PARENT DISABLED");
           return;
        }
        if ( !SideFX(player) || Fubar(player) || Slave(player) || return_bit(player) < mudconf.restrict_sidefx ) {
           notify(player, "Permission denied.");
           return;
        }
        cmdp = (CMDENT *) hashfind((char *)"@parent", &mudstate.command_htab);
        if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@parent") ||
             cmdtest(Owner(player), "@parent") || zonecmdtest(player, "@parent") ) {
           notify(player, "Permission denied.");
           return;
        }
        mudstate.store_lastcr = -1;
        do_parent(player, cause, SIDEEFFECT, fargs[0], fargs[1]);
        if ( mudstate.store_lastcr == NOTHING ) {
           if ((it != NOTHING) && (it != AMBIGUOUS) && Good_obj(it) &&
               (Examinable(player, it) || (it == cause))) {
               dbval(buff, bufcx, Parent(it));
           } else {
               safe_str("#-1", buff, bufcx);
           }
        } else
           dbval(buff, bufcx, mudstate.store_lastcr);
    } else {
        if ((it != NOTHING) && (it != AMBIGUOUS) && Good_obj(it) && (Examinable(player, it) || (it == cause))) {
           dbval(buff, bufcx, Parent(it));
        } else {
           safe_str("#-1", buff, bufcx);
        }
    }
#else
    if ((it != NOTHING) && (it != AMBIGUOUS) && Good_obj(it) && (Examinable(player, it) || (it == cause))) {
        dbval(buff, bufcx, Parent(it));
    } else {
        safe_str("#-1", buff, bufcx);
    }
#endif
    return;
}

/* ---------------------------------------------------------------------------
 * fun_parse: Make list from evaluating arg3 with each member of arg2.
 * arg1 specifies a delimiter character to use in the parsing of arg2.
 * NOTE: This function expects that its arguments have not been evaluated.
 */

FUNCTION(fun_parse)
{
    char *sop_buf, *sep_buf, *curr, *objstring, *buff3, *result, *cp, sep, sop[LBUF_SIZE + 1];
    char *st_buff, *tpr_buff, *tprp_buff;
    int first, cntr, size;

    if (!fn_range_check("PARSE", nfargs, 2, 4, buff, bufcx))
       return;

    memset(sop, 0, sizeof(sop));

    if (nfargs > 3 ) {
       sop_buf=exec(player, cause, caller,
          EV_STRIP | EV_FCHECK | EV_EVAL, fargs[3], cargs, ncargs);
       strncpy(sop, sop_buf, sizeof(sop)-1);
       free_lbuf(sop_buf);
    } else {
       strcpy(sop, " ");
    }
    if (nfargs > 2 && *fargs[2] ) {
       sep_buf=exec(player, cause, caller,
          EV_STRIP | EV_FCHECK | EV_EVAL, fargs[2], cargs, ncargs);
       sep = *sep_buf;
       free_lbuf(sep_buf);
    } else {
       sep = ' ';
    }

    st_buff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0],
         cargs, ncargs);
    cp = curr = alloc_lbuf("fun_parse");
    safe_str(strip_ansi(st_buff), curr, &cp);
    free_lbuf(st_buff);
    cp = curr;
    cp = trim_space_sep(cp, sep);
    if (!*cp) {
       free_lbuf(curr);
       return;
    }
    first = 1;
    cntr = 1;
    size = 0;
    tprp_buff = tpr_buff = alloc_lbuf("fun_parse");
    while (cp) {
        objstring = split_token(&cp, sep);
        tprp_buff = tpr_buff;
        buff3 = replace_tokens(fargs[1], objstring, safe_tprintf(tpr_buff, &tprp_buff, "%d",cntr), NULL);
        result = exec(player, cause, caller,
                      EV_STRIP | EV_FCHECK | EV_EVAL, buff3, cargs, ncargs);
        free_lbuf(buff3);
        if ( (strlen(result) + strlen(buff) ) > LBUF_SIZE )
           size = 1;
        if (!first)
           safe_str(sop, buff, bufcx);
        first = 0;
        safe_str(result, buff, bufcx);
        free_lbuf(result);
        cntr++;
        if (size)
           break;
    }
    free_lbuf(tpr_buff);
    free_lbuf(curr);
}

/* ---------------------------------------------------------------------------
 * fun_left: Returns first n characters in a string
 */

FUNCTION(fun_left)
{
    int len = atoi(fargs[1]);

    if (len < 1) {
        safe_chr('\0', buff, bufcx);
    } else {
        if (len < (LBUF_SIZE - 1)) {
            fargs[0][len] = '\0';
        }
        safe_str(fargs[0], buff, bufcx);
    }
}


/* ---------------------------------------------------------------------------
 * fun_right: Returns last n characters in a string
 */

FUNCTION(fun_right)
{
    int len = atoi(fargs[1]);

    if (len < 1) {
        safe_chr('\0', buff, bufcx);
    } else {
        len = strlen(fargs[0]) - len;
        if (len < 1 || len > (LBUF_SIZE - 1) )
            safe_str(fargs[0], buff, bufcx);
        else
            safe_str(fargs[0] + len, buff, bufcx);
    }
}


/* ---------------------------------------------------------------------------
 * fun_mid: mid(foobar,2,3) returns oba
 */

FUNCTION(fun_mid)
{
    int l, len;
    char *outbuff, *s_output;
    ANSISPLIT outsplit[LBUF_SIZE];


    l = atoi(fargs[1]);
    len = atoi(fargs[2]);
    if ((l < 0) || (len < 0) || (len > LBUF_SIZE-1) || (l > LBUF_SIZE-1)) {
       safe_str( "#-1 OUT OF RANGE", buff, bufcx);
       return;
    }


    initialize_ansisplitter(outsplit, LBUF_SIZE);
    outbuff = alloc_lbuf("fun_mid");
    memset(outbuff, '\0', LBUF_SIZE);
    split_ansi(strip_ansi(fargs[0]), outbuff, outsplit);

    if ( (l + len) < LBUF_SIZE )
       *(outbuff + l + len) = '\0';

    s_output = rebuild_ansi(outbuff+l, outsplit+l);
    safe_str(s_output, buff, bufcx);
    free_lbuf(outbuff);
    free_lbuf(s_output);
}

/* ---------------------------------------------------------------------------
 * fun_first: Returns first word in a string
 */

FUNCTION(fun_first)
{
    char *s, *first, sep;

    /* If we are passed an empty arglist return a null string */

    if (nfargs == 0) {
        return;
    }
    varargs_preamble("FIRST", 2);
    s = trim_space_sep(fargs[0], sep);  /* leading spaces ... */
    first = split_token(&s, sep);
    if (first) {
        safe_str(first, buff, bufcx);
    }
}

/* ---------------------------------------------------------------------------
 * fun_rest: Returns all but the first word in a string
 */


FUNCTION(fun_rest)
{
    char *s, sep;

    /* If we are passed an empty arglist return a null string */

    if (nfargs == 0) {
        return;
    }
    varargs_preamble("REST", 2);
    s = trim_space_sep(fargs[0], sep);  /* leading spaces ... */
    split_token(&s, sep);
    if (s) {
        safe_str(s, buff, bufcx);
    }
}

/* ---------------------------------------------------------------------------
 * fun_v: Function form of %-substitution
 */

FUNCTION(fun_v)
{
    dbref aowner;
    int aflags, i_shifted, i_oldshift;
    char *sbuf, *sbufc, *tbuf, s_field[20];
    ATTR *ap;

    tbuf = fargs[0];

#ifdef ATTR_HACK
    if ((isalpha((int)(tbuf[0])) || (tbuf[0] == '_') || (tbuf[0] == '~')) && tbuf[1]) {
#else
    if (isalpha((int)(tbuf[0])) && tbuf[1]) {
#endif

  /* Fetch an attribute from me.  First see if it exists,
   * returning a null string if it does not. */

        ap = atr_str(fargs[0]);
        if (!ap) {
            return;
        }
  /* If we can access it, return it, otherwise return a
   * null string */

        atr_pget_info(player, ap->number, &aowner, &aflags);
        if (See_attr(player, player, ap, aowner, aflags, 0)) {
            tbuf = atr_pget(player, ap->number, &aowner, &aflags);
            safe_str(tbuf, buff, bufcx);
            free_lbuf(tbuf);
        }
        return;
    }
    /* Not an attribute, process as %<arg> */

    sbuf = alloc_sbuf("fun_v");
    sbufc = sbuf;
    safe_sb_chr('%', sbuf, &sbufc);
    if ( isdigit(*fargs[0]) ) {
       i_shifted = atoi(fargs[0]) / 10;
       if ( i_shifted < 0 )
          i_shifted = 0;
       if ( i_shifted > (MAX_ARGS/10) )
          i_shifted = (MAX_ARGS/10);
       sprintf(s_field, "%d", atoi(fargs[0]) % 10);
       safe_sb_str(s_field, sbuf, &sbufc);
       *sbufc = '\0';
       if ( i_shifted ) {
          i_oldshift = mudstate.shifted;
          mudstate.shifted = i_shifted;
       }
    } else {
       safe_sb_str(fargs[0], sbuf, &sbufc);
    }
    tbuf = exec(player, cause, caller, EV_FIGNORE, sbuf, cargs, ncargs);
    if ( i_shifted ) {
       mudstate.shifted = i_oldshift;
    }
    safe_str(tbuf, buff, bufcx);
    free_lbuf(tbuf);
    free_sbuf(sbuf);
}

/* Bypass function with ignore case if bypass() called */
FUNCTION(fun_bypass)
{
   if ( *fargs[0] && mudstate.allowbypass )
      mudstate.func_bypass = (atoi(fargs[0]) ? 1 : 0);
   else
      mudstate.func_bypass = 0;
}

/* ---------------------------------------------------------------------------
 * shift the 0-9 registers to be able to access them
 */
FUNCTION(fun_shift)
{
   int i_shift;

   i_shift = 0;
   if ( *fargs[0] ) {
      i_shift = atoi(fargs[0]);
      if ( i_shift < 0 )
         i_shift = 0;
      if ( i_shift > (MAX_ARGS/10) )
         i_shift = (MAX_ARGS/10);
      mudstate.shifted = i_shift;
   } else {
      ival(buff, bufcx, mudstate.shifted);
   }
}

/* ---------------------------------------------------------------------------
 * fun_s: Force substitution to occur.
 */

FUNCTION(fun_s)
{
    char *tbuf;

    tbuf = exec(player, cause, caller, EV_FIGNORE | EV_EVAL, fargs[0],
    cargs, ncargs);
    safe_str(tbuf, buff, bufcx);
    free_lbuf(tbuf);
}

/* ---------------------------------------------------------------------------
 * localfunc() calls a user-defined function from local.c
 */
FUNCTION(fun_localfunc)
{
  if (nfargs < 2 ) {
     safe_str("#-1 FUNCTION (LOCALFUNC) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
     ival(buff, bufcx, nfargs);
     safe_chr(']', buff, bufcx);
     return;
  }
  local_function(fargs[0], buff, bufcx, player, cause, fargs+1, nfargs-1, cargs, ncargs);
}

FUNCTION(fun_privatize)
{
    if ( !fargs[0] || !*fargs[0] ) {
       safe_str("#-1 FUNCTION (PRIVATIZE) EXPECTS 1 ARGUMENT [RECEIVED 0]", buff, bufcx);
       return;
    }
    if ( atoi(fargs[0]) == 0 ) {
       mudstate.global_regs_wipe = 0;
    } else {
       mudstate.global_regs_wipe = 1;
    }
}

/* ---------------------------------------------------------------------------
 * fun_localize: Keep function's inside of it localized with memory registers
 * Idea borrowed from TinyMUSH 3.0
 */
FUNCTION(fun_localize)
{
    char *result, *pt, *savereg[MAX_GLOBAL_REGS], *resbuff;
    int x, i_flagreg[MAX_GLOBAL_REGS], i_reverse;

    if (!fn_range_check("LOCALIZE", nfargs, 1, 3, buff, bufcx)) {
       return;
    }

    i_reverse = 0;
    if ( (nfargs > 2) && *fargs[2] ) {
       resbuff = exec(player, cause, caller, EV_FCHECK | EV_STRIP | EV_EVAL, fargs[2], cargs, ncargs);
       i_reverse = atoi(resbuff);
       free_lbuf(resbuff);
    }
    if ( (nfargs > 1) && *fargs[1] ) {
       resbuff = exec(player, cause, caller, EV_FCHECK | EV_STRIP | EV_EVAL, fargs[1], cargs, ncargs);
       if ( *resbuff ) {
          pt = resbuff;
          while ( *pt ) {
             *pt = ToLower(*pt);
             pt++;
          }
          for (x = 0; x < MAX_GLOBAL_REGS; x++) {
             if ( strchr(resbuff, mudstate.nameofqreg[x]) != NULL )
                i_flagreg[x] = (i_reverse ? 0 : 1);
             else
                i_flagreg[x] = (i_reverse ? 1 : 0);
          }
       } else {
          for (x = 0; x < MAX_GLOBAL_REGS; x++) {
             i_flagreg[x] = 1;
          }
       }
       free_lbuf(resbuff);
    } else {
       for (x = 0; x < MAX_GLOBAL_REGS; x++) {
          i_flagreg[x] = 1;
       }
    }
    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      if ( !i_flagreg[x] )
         continue;
      savereg[x] = alloc_lbuf("ulocal_reg");
      pt = savereg[x];
      safe_str(mudstate.global_regs[x],savereg[x],&pt);
      if ( mudstate.global_regs_wipe == 1 ) {
         *mudstate.global_regs[x] = '\0';
      }
    }
    result = exec(player, cause, caller, EV_FCHECK | EV_STRIP | EV_EVAL, fargs[0],
    cargs, ncargs);
    for (x = 0; x < MAX_GLOBAL_REGS; x++) {
      if ( !i_flagreg[x] )
         continue;
      pt = mudstate.global_regs[x];
      safe_str(savereg[x],mudstate.global_regs[x],&pt);
      free_lbuf(savereg[x]);
    }
    safe_str(result, buff, bufcx);
    free_lbuf(result);
}

/* ---------------------------------------------------------------------------
 * fun_null: eat output
 * Function borrowed from TinyMUSH
 * Function '@@' borrowed from PENN
 */
FUNCTION(fun_null)
{
   return;
}

/* This is the function '@@' */
FUNCTION(fun_nsnull)
{
   return;
}


/* ---------------------------------------------------------------------------
 * fun_con: Returns first item in contents list of object/room
 */

FUNCTION(fun_con)
{
    dbref it, it2;

    it = match_thing(player, fargs[0]);

    if ((it != NOTHING) && (it != AMBIGUOUS) &&
        (Has_contents(it)) &&
        (Examinable(player, it) ||
         (where_is(player) == it) ||
         (it == cause))) {
        it2 = Contents(it);
#ifdef REALITY_LEVELS
        while ( (it2 != NOTHING) && (!IsReal(player, it2) || ((SCloak(it2) && !Immortal(player)) || ((Wizard(it2) && !Wizard(player))))) ) {
            it2 = Next(it2);
        }
#else
        while ((it2 != NOTHING) && ((SCloak(it2) && !Immortal(player)) || ((Wizard(it2) && !Wizard(player))))) {
            it2 = Next(it2);
        }
#endif /* REALITY_LEVELS */
        dbval(buff, bufcx, it2);
    } else
        safe_str("#-1", buff, bufcx);
    return;
}

FUNCTION(fun_strfunc)
{
   FUN *fp;
   char *ptrs[LBUF_SIZE / 2], *list, *p, *q, sep, *tpr_buff, *tprp_buff, *strtok, *strtokptr;
   int nitems, tst, i;

   varargs_preamble("STRFUNC", 3);

   tst = strlen(fargs[0]);
   if ( !tst ) {
      safe_str("#-1 INVALID FUNCTION", buff, bufcx);
      return;
   }
   p = list = alloc_lbuf("fun_strfunc2");
   memset(list, '\0', LBUF_SIZE);
   q = fargs[0];
   while ( q && *q ) {
      *p = tolower(*q);
      q++;
      p++;
   }

   fp = (FUN *) hashfind(list, &mudstate.func_htab);
   if ( !fp ) {
      safe_str("#-1 INVALID FUNCTION", buff, bufcx);
      free_lbuf(list);
      return;
   }

   free_lbuf(list);
   if (!check_access(player, fp->perms, fp->perms2, 0)) {
      if ( mudstate.func_ignore && !mudstate.func_bypass ) {
         safe_str("#-1 INVALID FUNCTION", buff, bufcx);
         return;
      } else if ( !mudstate.func_ignore ) {
         safe_str("#-1 PERMISSION DENIED", buff, bufcx);
         return;
      }
   }

   if ( !stricmp((char *)fp->name, "strfunc") ) {
      safe_str("#-1 CAN NOT STRFUNC ITSELF", buff, bufcx);
      return;
   }

   list = alloc_lbuf("fun_strfunc");

   /* These will always be the same list and null terminated */
   strcpy(list, fargs[1]);

   nitems = 0;
   if ( strchr(fargs[1], sep) != NULL ) {
      strtokptr = trim_space_sep(list, sep);
      strtok = split_token(&strtokptr, sep);
      while ( strtok ) {
         ptrs[nitems] = alloc_lbuf("strfunc_lbuf_alloc");
         strcpy(ptrs[nitems], strtok);
         if ( nitems >= (LBUF_SIZE / 2) ) {
            nitems++;
            break;
         }
         strtok = split_token(&strtokptr, sep);
         nitems++;
      }
   } else {
      ptrs[nitems] = alloc_lbuf("strfunc_lbuf_alloc");
      strcpy(ptrs[nitems], fargs[1]);
      nitems++;
   }

   if ( (nitems == fp->nargs) || (nitems == -fp->nargs) ||
        (fp->flags & FN_VARARGS) ) {
      fp->fun(buff, bufcx, player, cause, caller, ptrs, nitems, cargs, ncargs);
   } else {
      tprp_buff = tpr_buff = alloc_lbuf("strfunc_tprbuff");
      safe_str(safe_tprintf(tpr_buff, &tprp_buff, "#-1 FUNCTION (%s) EXPECTS %d ARGUMENTS [RECEIVED %d]",
                            fp->name, fp->nargs, nitems), buff, bufcx);
      free_lbuf(tpr_buff);
   }
   for ( i = 0; i < nitems; i++ ) {
      free_lbuf(ptrs[i]);
   }
   free_lbuf(list);
}

/* ---------------------------------------------------------------------------
 * fun_eval: evaluate string *or* attribute based if second arg exists
 */

FUNCTION(fun_eval)
{
   dbref thing, aowner;
   int attrib, aflags, free_buffer, eval_it, tval;
   char *tbuf, *atr_gotten, *atr_gotten2, *tpr_buff, *tprp_buff;
   struct boolexp *bool;
   ATTR *attr;

   if (!fn_range_check("EVAL", nfargs, 1, 2, buff, bufcx)) {
      return;
   }
   /* Evaluate string */
   if ( nfargs == 1 ) {
       if ( !*fargs[0] )
          return;
       tbuf = exec(player, cause, caller, EV_FIGNORE | EV_EVAL, fargs[0],
                   cargs, ncargs);
       safe_str(tbuf, buff, bufcx);
       free_lbuf(tbuf);
       return;
   } else if ( nfargs == 2 ) {
      if ( !*fargs[0] || !*fargs[1] )
         return;
      tprp_buff = tpr_buff = alloc_lbuf("fun_eval");
      if (!parse_attrib(player, safe_tprintf(tpr_buff, &tprp_buff, "%s/%s",fargs[0], fargs[1]), &thing, &attrib)) {
         safe_str("#-1 NO MATCH", buff, bufcx);
         free_lbuf(tpr_buff);
         return;
      }
      free_lbuf(tpr_buff);
      if (attrib == NOTHING) {
         return;
      }
      if (Cloak(thing) && !Wizard(player)) {
         safe_str("#-1 PERMISSION DENIED", buff, bufcx);
         return;
      }
      if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
         safe_str("#-1 PERMISSION DENIED", buff, bufcx);
         return;
      }
      free_buffer = 1;
      eval_it = 1;
      attr = atr_num(attrib); /* We need the attr's flags for this: */
      if (!attr) {
         return;
      }
      if (attr->flags & AF_IS_LOCK) {
         atr_gotten = atr_get(thing, attrib, &aowner, &aflags);
         if (Read_attr(player, thing, attr, aowner, aflags, 0)) {
            bool = parse_boolexp(player, atr_gotten, 1);
            free_lbuf(atr_gotten);
            atr_gotten = unparse_boolexp(player, bool);
            free_boolexp(bool);
         } else {
            free_lbuf(atr_gotten);
            atr_gotten = (char *) "#-1 PERMISSION DENIED";
         }
         free_buffer = 0;
         eval_it = 0;
      } else {
         atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);
      }
      if (!check_read_perms(player, thing, attr, aowner, aflags, buff, bufcx)) {
         if (free_buffer)
            free_lbuf(atr_gotten);
         return;
      }
      if (eval_it) {
         tval = safer_ufun(player, thing, thing, (attr ? attr->flags : 0), aflags);
         if ( tval == -2 ) {
            atr_gotten2 = alloc_lbuf("edefault_buff");
            sprintf(atr_gotten2 ,"#-1 PERMISSION DENIED");
         } else {
            atr_gotten2 = exec(thing, player, player, EV_FIGNORE | EV_EVAL, atr_gotten,
                               (char **) NULL, 0);
         }
         safe_str(atr_gotten2, buff, bufcx);
         free_lbuf(atr_gotten2);
         safer_unufun(tval);
      } else {
         safe_str(atr_gotten, buff, bufcx);
      }
      if (free_buffer)
         free_lbuf(atr_gotten);
      return;
   }
}

/* ---------------------------------------------------------------------------
 * fun_exit: Returns first exit in exits list of room.
 */

FUNCTION(fun_exit)
{
    dbref it, exit;
    int key;

    it = match_thing(player, fargs[0]);
    if ((it != NOTHING) && (it != AMBIGUOUS) && Good_obj(it) && Has_exits(it) && Good_obj(Exits(it))) {
        key = 0;
        if (Examinable(player, it))
            key |= VE_LOC_XAM;
        if (Dark(it) && !(!SCloak(it) && could_doit(player, it, A_LDARK, 0, 0)))
            key |= VE_LOC_DARK;
        DOLIST(exit, Exits(it)) {
            if (exit_visible(exit, player, key)) {
                dbval(buff, bufcx, exit);
                return;
            }
        }
    }
    safe_str("#-1", buff, bufcx);
    return;
}

/* ---------------------------------------------------------------------------
 * fun_next: return next thing in contents or exits chain
 */

FUNCTION(fun_next)
{
    dbref it, loc, exit, ex_here;
    int key;

    it = match_thing(player, fargs[0]);
    if (Good_obj(it) && Has_siblings(it)) {
        loc = where_is(it);
        ex_here = Good_obj(loc) ? Examinable(player, loc) : 0;
        if (ex_here || (loc == player) || (loc == where_is(player))) {
            if (!isExit(it)) {
                if (Cloak(it) && ((!Immortal(player) && Immortal(it) && SCloak(it)) || !Wizard(player))) {
                    safe_str("#-1", buff, bufcx);
                    return;
                }
#ifdef REALITY_LEVELS
                while ( (Next(it) != NOTHING) && (!IsReal(player, Next(it)) || (Cloak(Next(it)) &&
                                                  ((!Immortal(player) && Immortal(Next(it)) && SCloak(Next(it))) || !Wizard(player)))) ) {
                    it = Next(it);
                }
#else
                while ( (Next(it) != NOTHING) && (Cloak(Next(it)) && ((!Immortal(player) && Immortal(Next(it)) &&
                                                  SCloak(Next(it))) || !Wizard(player))) ) {
                    it = Next(it);
                }
#endif /* REALITY_LEVELS */
                dbval(buff, bufcx, Next(it));
                return;
             } else {
                key = 0;
             if (ex_here)
                key |= VE_LOC_XAM;
             if (Dark(loc))
                key |= VE_LOC_DARK;
             DOLIST(exit, it) {
                if ((exit != it) && exit_visible(exit, player, key)) {
                   dbval(buff, bufcx, exit);
                   return;
                }
             }
          }
       }
    }
    safe_str("#-1", buff, bufcx);
    return;
}

/* ---------------------------------------------------------------------------
 * fun_loc: Returns the location of something
 */


FUNCTION(fun_loc)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if (!Good_obj(it)) {
       safe_str("#-1", buff, bufcx);
    } else if (Location(it) == HOME) {
       switch (Typeof(it)) {
          case TYPE_ROOM:
          case TYPE_EXIT:
            dbval(buff, bufcx, HOME);
            break;
          default:
            safe_str("#-1", buff, bufcx);
       }
    } else if ( (locatable(player, it, cause)) && (!Immortal(Owner(Location(it))) || Immortal(player) || !((Dark(Location(it)) &&
                                                    Hidden(Location(it)) && SCloak(Location(it))))) &&
                (Examinable(player, Location(it)) || Wizard(player) ||
                 !Cloak(Location(it)) || Jump_ok(Location(it)) || Abode(Location(it))) ) {
       if ( (Typeof(it) == TYPE_ROOM) && ((Cloak(it) && !Wizard(player)) || (Cloak(it) && SCloak(it) && !Immortal(player))) ) {
          safe_str("#-1", buff, bufcx);
       } else {
#ifdef REALITY_LEVELS
          if (!IsReal(player, it))
             safe_str("#-1", buff, bufcx);
          else
#endif /* REALITY_LEVELS */
             dbval(buff, bufcx, Location(it));
       }
    } else {
       safe_str("#-1", buff, bufcx);
    }
    return;
}

FUNCTION(fun_findable)
{
   dbref it, thing;

   it = match_thing(player, fargs[0]);
   thing = match_thing(player, fargs[1]);

   if ( !Good_obj(it) || !Good_obj(thing) || it == NOTHING || it == AMBIGUOUS ||
        thing == NOTHING || thing == AMBIGUOUS || Recover(it) || Recover(thing) ||
        Going(it) || Going(thing) ) {
      safe_chr('0', buff, bufcx);
   } else {
      if ( !Controls(player, it) )
         it = player;
      if ((locatable(it, thing, cause)) && (!Immortal(Owner(Location(thing))) ||
           Immortal(it) || !((Dark(Location(thing)) && Hidden(Location(thing)) &&
           SCloak(Location(thing))))) && (Examinable(player, Location(thing)) ||
           Wizard(it) || !Cloak(Location(thing)) || Jump_ok(Location(thing)) ||
           Abode(Location(thing)))) {
         safe_chr('1', buff, bufcx);
      } else {
         safe_chr('0', buff, bufcx);
      }
   }
}

FUNCTION(fun_brackets)
{
  int rparen, lparen, rbrack, lbrack, rbrace, lbrace;
  char *ptr, *tpr_buff, *tprp_buff;

  rparen = lparen = rbrack = lbrack = rbrace = lbrace = 0;
  if ( !fargs[0] ) {
     safe_str("0 0 0 0 0 0", buff, bufcx);
  } else {
     ptr = fargs[0];
     while ( *ptr ) {
        switch ( *ptr ) {
           case '[' : lbrack++;
                      break;
           case ']' : rbrack++;
                      break;
           case '(' : lparen++;
                      break;
           case ')' : rparen++;
                      break;
           case '{' : lbrace++;
                      break;
           case '}' : rbrace++;
                      break;
        }
        ptr++;
     }
     tpr_buff = tprp_buff = alloc_lbuf("fun_brackets");
     safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d %d %d %d %d %d", lbrack, rbrack,
                           lparen, rparen, lbrace, rbrace), buff, bufcx);
     free_lbuf(tpr_buff);
  }

}
FUNCTION(fun_bound)
{
  int i_min, i_max, i_num;

  if (!fn_range_check("BOUND", nfargs, 2, 3, buff, bufcx))
      return;
  i_min = i_max = 0;
  i_num = atoi(fargs[0]);
  i_min = atoi(fargs[1]);
  if ( nfargs > 2 ) {
     i_max = atoi(fargs[2]);
     ival(buff, bufcx, (i_num < i_min ? i_min : (i_num > i_max ? i_max : i_num)));
  } else
     ival(buff, bufcx, (i_num < i_min ? i_min : i_num));
}

FUNCTION(fun_fbound)
{
  double d_min, d_max, d_num;

  if (!fn_range_check("FBOUND", nfargs, 2, 3, buff, bufcx))
      return;
  d_min = d_max = 0.0;
  d_num = safe_atof(fargs[0]);
  d_min = safe_atof(fargs[1]);
  if ( nfargs > 2 ) {
     d_max = safe_atof(fargs[2]);
     fval(buff, bufcx, (d_num < d_min ? d_min : (d_num > d_max ? d_max : d_num)));
  } else
     fval(buff, bufcx, (d_num < d_min ? d_min : d_num));
}

FUNCTION(fun_between)
{
  int i_before, i_after, i_num;
  char sep;

  varargs_preamble("BETWEEN", 4);
  i_before = i_after = 0;
  i_before = atoi(fargs[0]);
  i_after  = atoi(fargs[1]);
  i_num    = atoi(fargs[2]);
  if ( sep == '1' )
     ival(buff, bufcx, ((i_before <= i_num) && (i_after >= i_num)) ? 1 : 0);
  else
     ival(buff, bufcx, ((i_before < i_num) && (i_after > i_num)) ? 1 : 0);
}

FUNCTION(fun_fbetween)
{
  double d_before, d_after, d_num;
  char sep;

  varargs_preamble("FBETWEEN", 4);
  d_before = d_after = 0.0;
  d_before = safe_atof(fargs[0]);
  d_after  = safe_atof(fargs[1]);
  d_num    = safe_atof(fargs[2]);
  if ( sep == '1' )
     fval(buff, bufcx, ((d_before <= d_num) && (d_after >= d_num)) ? 1 : 0);
  else
     fval(buff, bufcx, ((d_before < d_num) && (d_after > d_num)) ? 1 : 0);
}

/* ----------------------------------------------------------------------------
 * fun_lloc: return a list of containers until room, 20 objects, or recursive
 *           item is found
 * Special thanks to Earendil@Akallabeth for this code.
 * Alterations to the original code includes recursion check and rhost-compat.
 */

FUNCTION(fun_lloc)
{
  dbref room;
  char *ptr, *chkbuff, *tpr_buff, *tprp_buff;
  int rec = 0;
  dbref it;

  chkbuff = alloc_lbuf("fun_lloc");
  *chkbuff = '\0';
  ptr = chkbuff;

  it = match_thing(player, fargs[0]);
  if ( !Good_obj(it) || it == NOTHING || it == AMBIGUOUS ) {
     safe_str("#-1", buff, bufcx);
     free_lbuf(chkbuff);
     return;
  } else if ((locatable(player, it, cause)) &&
             (!Immortal(Owner(Location(it))) || Immortal(player) || !((Dark(Location(it))
              && Hidden(Location(it)) && SCloak(Location(it))))) &&
             (Examinable(player, Location(it)) || Wizard(player) ||
    !Cloak(Location(it)) || Jump_ok(Location(it)) || Abode(Location(it)))) {
     room = Location(it);
     if (!Good_obj(room)) {
        safe_str("#-1", buff, bufcx);
        free_lbuf(chkbuff);
        return;
     }
     tprp_buff = tpr_buff = alloc_lbuf("fun_lloc");
     while (Typeof(room) != TYPE_ROOM) {
        tprp_buff = tpr_buff;
        safe_str((char *)safe_tprintf(tpr_buff, &tprp_buff, " #%d ", room), chkbuff, &ptr);
        tprp_buff = tpr_buff;
        safe_str((char *)safe_tprintf(tpr_buff, &tprp_buff, "#%d", room), buff, bufcx);
        safe_chr(' ', buff, bufcx);
        room = Location(room);
        if ( !Good_obj(room) || room == AMBIGUOUS || room == NOTHING ||
             (!Immortal(player) && Immortal(Owner(room)) && SCloak(room) && Cloak(room)) ||
             (!Wizard(player) && Wizard(Owner(room)) && Cloak(room)) ) {
           safe_str("#-1", buff, bufcx);
           free_lbuf(chkbuff);
           free_lbuf(tpr_buff);
           return;
        }
        tprp_buff = tpr_buff;
        if ( strstr(chkbuff, safe_tprintf(tpr_buff, &tprp_buff, " #%d ", room)) != NULL ) {
           safe_str("#-1", buff, bufcx);
           notify(player, "Recursion detected.");
           free_lbuf(chkbuff);
           free_lbuf(tpr_buff);
           return;
        }
        rec++;
        if (rec > 20) {
            notify(player, "Too many containers.");
            safe_str("#-1", buff, bufcx);
            free_lbuf(chkbuff);
            free_lbuf(tpr_buff);
            return;
        }
    }
    tprp_buff = tpr_buff;
    safe_str((char *)safe_tprintf(tpr_buff, &tprp_buff, "#%d",room), buff, bufcx);
    free_lbuf(tpr_buff);
    free_lbuf(chkbuff);
  } else {
    notify(player, "Permission denied.");
    safe_str("#-1", buff, bufcx);
    free_lbuf(chkbuff);
  }
}

/* ---------------------------------------------------------------------------
 * fun_where: Returns the "true" location of something
 */

FUNCTION(fun_where)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if ( (it == NOTHING) || (it == AMBIGUOUS) || !Good_obj(it))
        safe_str("#-1", buff, bufcx);
    else if ((locatable(player, it, cause)) && (!Immortal(Owner(where_is(it))) || Immortal(player) || !((Dark(where_is(it))
                                                && Hidden(where_is(it)) && SCloak(where_is(it))))) &&
             (Examinable(player, where_is(it)) || !Cloak(where_is(it)) || Wizard(player) ||
              Jump_ok(where_is(it)) || Abode(where_is(it)))) {
       dbval(buff, bufcx, where_is(it));
    } else
       safe_str("#-1", buff, bufcx);
    return;
}

/* ---------------------------------------------------------------------------
 * fun_rloc: Returns the recursed location of something (specifying #levels)
 */

FUNCTION(fun_rloc)
{
    int i, levels;
    dbref it;

    levels = atoi(fargs[1]);
    if (levels > mudconf.ntfy_nest_lim)
       levels = mudconf.ntfy_nest_lim;

    it = match_thing(player, fargs[0]);
    if (locatable(player, it, cause)) {
       for (i = 0; i < levels; i++) {
           if (!Good_obj(it) || !Has_location(it) || !Good_obj(Location(it)))
              break;
           it = Location(it);
           if (!(Examinable(player, it) || Wizard(player) || (!Cloak(Owner(it)) && !Cloak(it)) || Jump_ok(it) || Abode(it))) {
              if (!Wizard(player) || (Wizard(player) && !Immortal(player) && Unfindable(it) && Hidden(it) && SCloak(it))) {
                 it = -1;
                 break;
              }
           }
       }
       dbval(buff, bufcx, it);
       return;
    }
    safe_str("#-1", buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * fun_room: Find the room an object is ultimately in.
 */

FUNCTION(fun_room)
{
    dbref it;
    int count, t1;

    t1 = 0;
    it = match_thing(player, fargs[0]);
    if (!locatable(player, it, cause))
       t1 = (Controls(player,it) || !Dark(it) || !Unfindable(it));
    if (locatable(player, it, cause)) {
       for (count = mudconf.ntfy_nest_lim; count > 0; count--) {
           it = Location(it);
           if (!Good_obj(it))
              break;
           t1 = (Controls(player,it) || !Dark(it) || !Unfindable(it));
           if (!(Examinable(player, it) || Wizard(player) || (!Cloak(Owner(it)) && !Cloak(it)) || Jump_ok(it) || Abode(it))) {
              if (!Wizard(player) || (Wizard(player) && !Immortal(player) && Unfindable(it) && Hidden(it) && SCloak(it))) {
                 it = -1;
                 break;
              }
           }
           if (!t1) {
              if (!Wizard(player) || (Wizard(player) && !Immortal(player) && Unfindable(it) && Hidden(it) && SCloak(it))) {
                 it = -1;
                 break;
              }
           }
           if (isRoom(it)) {
              dbval(buff, bufcx, it);
              return;
           }
       }
       safe_str("#-1", buff, bufcx);
    } else if (isRoom(it) && t1) {
       dbval(buff, bufcx, it);
    } else {
       safe_str("#-1", buff, bufcx);
    }
    return;
}

/* ---------------------------------------------------------------------------
 * fun_owner: Return the owner of an object.
 */

FUNCTION(fun_owner)
{
    dbref it, aowner;
    int atr, aflags;

    if (parse_attrib(player, fargs[0], &it, &atr)) {
       if (atr == NOTHING) {
           it = NOTHING;
       } else {
           atr_pget_info(it, atr, &aowner, &aflags);
           it = aowner;
       }
    } else {
       it = match_thing(player, fargs[0]);
       if (it != NOTHING)
           it = Owner(it);
    }
    dbval(buff, bufcx, it);
}

/* ---------------------------------------------------------------------------
 * fun_controls: Does x control y?
 */

FUNCTION(fun_controls)
{
    dbref x, y;

    x = match_thing(player, fargs[0]);
    if (x == NOTHING) {
       safe_str("#-1 ARG1 NOT FOUND", buff, bufcx);
       return;
    }
    y = match_thing(player, fargs[1]);
    if (y == NOTHING) {
       safe_str("#-1 ARG2 NOT FOUND", buff, bufcx);
       return;
    }
    ival(buff, bufcx, Controls(x, y));
}

/* ---------------------------------------------------------------------------
 * fun_sees: Can X see Y in the normal Contents list of the room. If X
 *           or Y do not exist, 0 is returned.
 */

FUNCTION(fun_sees)
{
    dbref it, thing, thingexit;
    int can_see_loc, i_chkexit, key, i_loc, i_match;

    if (!fn_range_check("SEES", nfargs, 2, 3, buff, bufcx))
       return;

    i_loc = thingexit = NOTHING;
    i_match = i_chkexit = key = 0;

    if ( (nfargs > 2) && *fargs[2] )
       i_chkexit = atoi(fargs[2]);

    it = match_thing(player, fargs[0]);
    if ( !Good_obj(it) ) {
        ival(buff, bufcx, 0);
        return;
    }
    if (!Controls(player, it)) {
        ival(buff, bufcx, 0);
        return;
    }
    thing = match_thing_quiet(player, fargs[1]);
    if ( i_chkexit != 0 ) {
       i_match = 1;
       i_loc = Location(it);
       if ( !Good_chk(i_loc) ) {
          i_match = 0;
       } else {
          init_match_check_keys(player, fargs[1], TYPE_EXIT);
          match_exit_with_parents();
          thingexit = last_match_result();
          if ( !Good_chk(thingexit) || !Good_chk(where_is(thingexit)) ) {
             i_match = 0;
          } else {
             if (Examinable(it, i_loc))
                 key |= VE_LOC_XAM;
             if (Dark(i_loc) && !(!Cloak(i_loc) && could_doit( it, i_loc, A_LDARK, 0, 0)))
                 key |= VE_LOC_DARK;
             if (exit_visible(thingexit, it, key)) {
                if ( !(Flags3(thingexit) & PRIVATE) || (where_is(thingexit) == it) ) {
                   ival(buff, bufcx, 1);
                   return;
                }
             }
          }
       }
       i_match = 0;
    } 
    if ( i_match == 0 ) {
       if ( !Good_obj(thing) ) {
           ival(buff, bufcx, 0);
           return;
       }
       if (!Good_obj(Location(thing)) ) {
           ival(buff, bufcx, 0);
           return;
       }
       can_see_loc = (!Dark(Location(thing)) || (Dark(Location(thing)) &&
                       could_doit(it, Location(thing), A_LDARK, 0, 0)) ||
                      (mudconf.see_own_dark && MyopicExam(player, Location(thing))));
       if ((can_see(it, thing, can_see_loc) && mudconf.player_dark) ||
           (can_see2(it, thing, can_see_loc) && !mudconf.player_dark)) {
           ival(buff, bufcx, 1);
       }
       else {
           ival(buff, bufcx, 0);
       }
    }
}


/* ---------------------------------------------------------------------------
 * fun_fullname: Return the fullname of an object (good for exits)
 */

FUNCTION(fun_fullname)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if (it == NOTHING) {
       return;
    }
    if (!mudconf.read_rem_name) {
       if (!nearby_exam_or_control(player, it) &&
           (!isPlayer(it))) {
           safe_str("#-1 TOO FAR AWAY TO SEE", buff, bufcx);
           return;
       }
    }
    if (Cloak(it) && !Wizard(player)) {
       safe_str("#-1", buff, bufcx);
       return;
    }
    if (SCloak(it) && Cloak(it) && !Immortal(player)) {
       safe_str("#-1", buff, bufcx);
       return;
    }
    safe_str(Name(it), buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * fun_guild: Return an players's guild
 */

FUNCTION(fun_guild)
{
    dbref it, aowner;
    int aflags;
    char *tbuf;

    it = match_thing(player, fargs[0]);
    if ((it == NOTHING) || (Typeof(it) != TYPE_PLAYER))
       safe_str("#-1", buff, bufcx);
    else {
       tbuf = atr_get(it, A_GUILD, &aowner, &aflags);
       safe_str(tbuf, buff, bufcx);
       free_lbuf(tbuf);
    }
    if (it != NOTHING) {
       if (Cloak(it) && !Wizard(player)) {
           safe_str("#-1", buff, bufcx);
           return;
       }
       if (SCloak(it) && Cloak(it) && !Immortal(player)) {
           safe_str("#-1", buff, bufcx);
           return;
       }
    }
}

/* ---------------------------------------------------------------------------
 * fun_race: Return an players's race
 */

FUNCTION(fun_race)
{
    dbref it, aowner;
    int aflags;
    char *tbuf;

    it = match_thing(player, fargs[0]);
    if ((it == NOTHING) || (Typeof(it) != TYPE_PLAYER))
       safe_str("#-1", buff, bufcx);
    else {
       tbuf = atr_get(it, A_RACE, &aowner, &aflags);
       safe_str(tbuf, buff, bufcx);
       free_lbuf(tbuf);
    }
    if (it != NOTHING) {
       if (Cloak(it) && !Wizard(player)) {
           safe_str("#-1", buff, bufcx);
           return;
       }
       if (SCloak(it) && Cloak(it) && !Immortal(player)) {
           safe_str("#-1", buff, bufcx);
           return;
       }
    }
}

/* ---------------------------------------------------------------------------
 * fun_name: Return the name of an object
 */

FUNCTION(fun_name)
{
    dbref it;
    char *s;
    char *namebuff,
         *namebufcx;
#ifdef USE_SIDEEFFECT
    CMDENT *cmdp;
#endif

#ifdef USE_SIDEEFFECT
    if (!fn_range_check("NAME", nfargs, 1, 2, buff, bufcx))
        return;
    if ( nfargs > 1 ) {
        mudstate.sidefx_currcalls++;
        if ( !(mudconf.sideeffects & SIDE_NAME) ) {
           notify(player, "#-1 SIDE-EFFECT PORTION OF LOCK DISABLED");
           return;
        }
        if ( !SideFX(player) || Fubar(player) || Slave(player) || return_bit(player) < mudconf.restrict_sidefx ) {
           notify(player, "Permission denied.");
           return;
        }
        cmdp = (CMDENT *)hashfind((char *)"@name", &mudstate.command_htab);
        if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@name") ||
              cmdtest(Owner(player), "@name") || zonecmdtest(player, "@name") ) {
           notify(player, "Permission denied.");
           return;
        }
        do_name(player, cause, (SIDEEFFECT), fargs[0], fargs[1]);
    } else {
       it = match_thing(player, fargs[0]);
       if (it == NOTHING) {
          return;
       }
       if (!mudconf.read_rem_name) {
          if (!nearby_exam_or_control(player, it) && !isPlayer(it)) {
              safe_str("#-1 TOO FAR AWAY TO SEE", buff, bufcx);
              return;
          }
       }
       namebuff  = namebufcx = alloc_lbuf("fun_name.namebuff");
       safe_str(Name(it), namebuff, &namebufcx);
       *namebufcx = '\0';
       if (isExit(it)) {
          for (s = namebuff; *s && (*s != ';'); s++);
          *s = '\0';
       }
       if (Cloak(it) && !Wizard(player)) {
          safe_str("#-1", buff, bufcx);
          free_lbuf(namebuff);
          return;
       }
       if (SCloak(it) && Cloak(it) && !Immortal(player)) {
          safe_str("#-1", buff, bufcx);
          free_lbuf(namebuff);
          return;
       }
       safe_str(namebuff, buff, bufcx);
       free_lbuf(namebuff);
    }
#else
    it = match_thing(player, fargs[0]);
    if (it == NOTHING) {
       return;
    }
    if (!mudconf.read_rem_name) {
       if (!nearby_exam_or_control(player, it) && !isPlayer(it)) {
           safe_str("#-1 TOO FAR AWAY TO SEE", buff, bufcx);
           return;
       }
    }
    namebuff  = namebufcx = alloc_lbuf("fun_name.namebuff");
    safe_str(Name(it), namebuff, &namebufcx);
    *namebufcx = '\0';
    if (isExit(it)) {
       for (s = namebuff; *s && (*s != ';'); s++);
       *s = '\0';
    }
    if (Cloak(it) && !Wizard(player)) {
       safe_str("#-1", buff, bufcx);
       free_lbuf(namebuff);
       return;
    }
    if (SCloak(it) && Cloak(it) && !Immortal(player)) {
       safe_str("#-1", buff, bufcx);
       free_lbuf(namebuff);
       return;
    }
    safe_str(namebuff, buff, bufcx);
    free_lbuf(namebuff);
#endif
}

FUNCTION(fun_cname)
{
    dbref it, aname;
    int i_extansi, aflags;
    char *s;
    char *namebuff, *namebufcx, *ansibuf, *ansiparse;

    it = match_thing(player, fargs[0]);
    if (it == NOTHING) {
       return;
    }
    if (!mudconf.read_rem_name) {
       if (!nearby_exam_or_control(player, it) && !isPlayer(it)) {
           safe_str("#-1 TOO FAR AWAY TO SEE", buff, bufcx);
           return;
       }
    }
    ansibuf = atr_pget(it, A_ANSINAME, &aname, &aflags);
    i_extansi = 0;

    namebuff  = namebufcx = alloc_lbuf("fun_name.namebuff");
    safe_str(Name(it), namebuff, &namebufcx);
    *namebufcx = '\0';
    if (isExit(it)) {
       for (s = namebuff; *s && (*s != ';'); s++);
       *s = '\0';
    }
    if ( ExtAnsi(it) && (strcmp(namebuff, strip_all_special(ansibuf)) == 0) )
       i_extansi = 1;

    if (Cloak(it) && !Wizard(player)) {
       safe_str("#-1", buff, bufcx);
       free_lbuf(namebuff);
       free_lbuf(ansibuf);
       return;
    }
    if (SCloak(it) && Cloak(it) && !Immortal(player)) {
       safe_str("#-1", buff, bufcx);
       free_lbuf(namebuff);
       free_lbuf(ansibuf);
       return;
    }
    if ( i_extansi ) {
       safe_str(ansibuf, buff, bufcx);
    } else if ( !ExtAnsi(it) ) {
       ansiparse =  parse_ansi_name(cause, ansibuf);
       safe_str(ansiparse, buff, bufcx);
       free_lbuf(ansiparse);
       safe_str(namebuff, buff, bufcx);
#ifdef ZENTY_ANSI
       safe_str(SAFE_ANSI_NORMAL, buff, bufcx);
#else
       safe_str(ANSI_NORMAL, buff, bufcx);
#endif
    } else {
       safe_str(namebuff, buff, bufcx);
    }

    free_lbuf(namebuff);
    free_lbuf(ansibuf);
}

/* ---------------------------------------------------------------------------
 * fun_listmatch
 * returns values in argument of what matched.  1-10 args.
 */
FUNCTION(fun_listmatch)
{
   char *t_args[10], sep;
   int i, first;

   varargs_preamble("LISTMATCH", 3);
   if (wild_match(fargs[1], fargs[0], t_args, 10, 0)) {
      first = 0;
      for (i = 0; i < 10; i++) {
         if ( t_args[i] ) {
            if ( first )
               safe_chr(sep, buff, bufcx);
            safe_str(t_args[i], buff, bufcx);
            first = 1;
         }
      }
   }
   for (i = 0; i < 10; i++) {
      if (t_args[i])
          free_lbuf(t_args[i]);
   }
}
/* ---------------------------------------------------------------------------
 * fun_setqmatch
 * stores the values of what it matches (reg 1-10)
 */
FUNCTION(fun_setqmatch)
{
   char *t_args[10], sep, *pt;
   int i, first;

   varargs_preamble("SETQMATCH", 3);
   first = 0;
   if (wild_match(fargs[1], fargs[0], t_args, 10, 0)) {
      for (i = 0; i < 10; i++) {
         if ( t_args[i] ) {
            if (!mudstate.global_regs[i])
               mudstate.global_regs[i] = alloc_lbuf("fun_setq");
            pt = mudstate.global_regs[i];
            safe_str(t_args[i],mudstate.global_regs[i],&pt);
            first = 1;
         }
      }
   }
   for (i = 0; i < 10; i++) {
      if (t_args[i])
          free_lbuf(t_args[i]);
   }
   if ( sep != 'n' )
      ival(buff, bufcx, first);
}

/* ---------------------------------------------------------------------------
 * fun_match, fun_strmatch: Match arg2 against each word of arg1 returning
 * index of first match, or against the whole string.
 */

FUNCTION(fun_nummatch)
{
    int pcount, mcount;
    char *r, *s, sep, *working;

    varargs_preamble("NUMMATCH", 3);

    if (mudstate.last_cmd_timestamp == mudstate.now) {
       mudstate.heavy_cpu_recurse += 1;
    }
    if ( mudstate.heavy_cpu_recurse > mudconf.heavy_cpu_max ) {
       safe_str("#-1 HEAVY CPU RECURSION LIMIT EXCEEDED", buff, bufcx);
       return;
    }

    pcount = mcount = 0;
    working = alloc_lbuf("tot_match2");
    strcpy(working, fargs[0]);
    while (*(working + pcount) == ' ')
       pcount++;
    while (*(working + pcount)) {
       s = trim_space_sep(working + pcount, sep);
       do {
           r = split_token(&s, sep);
           while (*(working + pcount) == ' ')
              pcount++;
           pcount += strlen(r) + 1;
           if (quick_wild(fargs[1], r)) {
              mcount++;
              break;
           }
       } while (s);
       if (!s)
           break;
       strcpy(working, fargs[0]);
    }
    free_lbuf(working);
    ival(buff, bufcx, mcount);
}

FUNCTION(fun_totmatch)
{
    int wcount, pcount, gotone = 0;
    char *r, *s, sep, *working, *tbuf;

    varargs_preamble("TOTMATCH", 3);

    if (mudstate.last_cmd_timestamp == mudstate.now) {
       mudstate.heavy_cpu_recurse += 1;
    }
    if ( mudstate.heavy_cpu_recurse > mudconf.heavy_cpu_max ) {
       safe_str("#-1 HEAVY CPU RECURSION LIMIT EXCEEDED", buff, bufcx);
       return;
    }

    pcount = 0;
    working = alloc_lbuf("tot_match2");
    strcpy(working, fargs[0]);
    while (*(working + pcount) == ' ')
       pcount++;
    tbuf = alloc_mbuf("tot_match");
    wcount = 1;
    while (*(working + pcount)) {
       s = trim_space_sep(working + pcount, sep);
       do {
           r = split_token(&s, sep);
           while (*(working + pcount) == ' ')
              pcount++;
           pcount += strlen(r) + 1;
           if (quick_wild(fargs[1], r)) {
              sprintf(tbuf, "%d", wcount);
              if (gotone)
                safe_str(" ", buff, bufcx);
              safe_str(tbuf, buff, bufcx);
                gotone = 1;
              wcount++;
              break;
           }
           wcount++;
       } while (s);
       if (!s)
           break;
       strcpy(working, fargs[0]);
    }
    free_lbuf(working);
    free_mbuf(tbuf);
    if (!gotone)
       safe_str("0", buff, bufcx);
}

FUNCTION(fun_randmatch)
{
    int wcount, pcount, gotone = 0, *tlist, i, j, k;
    char *r, *s, sep, *working, *tbuf;

    if ( !fargs[0] || !*fargs[0] ) {
      safe_str("0", buff, bufcx);
       return;
    }

    tlist = (int *)malloc(sizeof(int) * strlen(fargs[0]));
    if (tlist == NULL) {
      safe_str("0", buff, bufcx);
      return;
    }
    varargs_preamble("RANDMATCH", 3);
    pcount = 0;
    working = alloc_lbuf("tot_match2");
    strcpy(working, fargs[0]);
    while (*(working + pcount) == ' ')
       pcount++;
    tbuf = alloc_mbuf("tot_match");
    wcount = 1;
    while (*(working + pcount)) {
       s = trim_space_sep(working + pcount, sep);
       do {
           r = split_token(&s, sep);
           while (*(working + pcount) == ' ')
              pcount++;
           pcount += strlen(r) + 1;
           if (quick_wild(fargs[1], r)) {
              *(tlist + gotone++) = wcount;
              wcount++;
              break;
           }
           wcount++;
       } while (s);
       if (!s)
           break;
       strcpy(working, fargs[0]);
    }
    if (gotone > 1) {
      for (i = gotone; i > 1; i--) {
          j = random() % i;
          if (j < (i - 1)) {
             k = *(tlist + j);
             *(tlist + j) = *(tlist + i - 1);
             *(tlist + i - 1) = k;
          }
      }
    }
    if (gotone > 0) {
      for (i = 0; i < gotone; i++) {
         if (i > 0)
           safe_chr(' ', buff, bufcx);
         ival(buff, bufcx, *(tlist + i));
      }
    }
    else
      safe_str("0", buff, bufcx);
    free(tlist);
    free_lbuf(working);
    free_mbuf(tbuf);
}
/* -------------------------------------------------------
 * This works like match() except it checks wildcard
 * matches on the LEFT side instead of the RIGHT side.
 */
FUNCTION(fun_wildmatch)
{
    int wcount;
    char *r, *s, sep;

    varargs_preamble("WILDMATCH", 3);

    wcount = 1;
    s = trim_space_sep(fargs[0], sep);
    do {
       r = split_token(&s, sep);
       if (quick_wild(r, fargs[1])) {
           ival(buff, bufcx, wcount);
           return;
       }
       wcount++;
    } while (s);
    safe_str("0", buff, bufcx);
}

/* -------------------------------------------------------
 * This works like totmatch() except it checks wildcard
 * matches on the LEFT side instead of the RIGHT side.
 */
FUNCTION(fun_totwildmatch)
{
    int wcount, pcount, gotone = 0;
    char *r, *s, sep, *working, *tbuf;

    varargs_preamble("TOTWILDMATCH", 3);
    pcount = 0;
    working = alloc_lbuf("tot_wildmatch2");
    strcpy(working, fargs[0]);
    while (*(working + pcount) == ' ')
       pcount++;
    tbuf = alloc_mbuf("tot_wildmatch");
    wcount = 1;
    while (*(working + pcount)) {
       s = trim_space_sep(working + pcount, sep);
       do {
           r = split_token(&s, sep);
           while (*(working + pcount) == ' ')
              pcount++;
           pcount += strlen(r) + 1;
           if (quick_wild(r, fargs[1])) {
              sprintf(tbuf, "%d", wcount);
              if (gotone)
                safe_str(" ", buff, bufcx);
              safe_str(tbuf, buff, bufcx);
                gotone = 1;
              wcount++;
              break;
           }
           wcount++;
       } while (s);
       if (!s)
           break;
       strcpy(working, fargs[0]);
    }
    free_lbuf(working);
    free_mbuf(tbuf);
    if (!gotone)
       safe_str("0", buff, bufcx);
}

/* -------------------------------------------------------
 * This works like nummatch() except it checks wildcard
 * matches on the LEFT side instead of the RIGHT side.
 */
FUNCTION(fun_numwildmatch)
{
    int pcount, mcount;
    char *r, *s, sep, *working;

    varargs_preamble("NUMWILDMATCH", 3);
    pcount = mcount = 0;
    working = alloc_lbuf("tot_match2");
    strcpy(working, fargs[0]);
    while (*(working + pcount) == ' ')
       pcount++;
    while (*(working + pcount)) {
       s = trim_space_sep(working + pcount, sep);
       do {
           r = split_token(&s, sep);
           while (*(working + pcount) == ' ')
              pcount++;
           pcount += strlen(r) + 1;
           if (quick_wild(r, fargs[1])) {
              mcount++;
              break;
           }
       } while (s);
       if (!s)
           break;
       strcpy(working, fargs[0]);
    }
    free_lbuf(working);
    ival(buff, bufcx, mcount);
}

FUNCTION(fun_match)
{
    int wcount;
    char *r, *s, sep;

    varargs_preamble("MATCH", 3);

    /* Check each word individually, returning the word number of the
     * first one that matches.  If none match, return 0.
     */

    wcount = 1;
    s = trim_space_sep(fargs[0], sep);
    do {
       r = split_token(&s, sep);
       if (quick_wild(fargs[1], r)) {
           ival(buff, bufcx, wcount);
           return;
       }
       wcount++;
    } while (s);
    safe_str("0", buff, bufcx);
}

FUNCTION(fun_strmatch)
{
    /* Check if we match the whole string.  If so, return 1 */

    if (quick_wild(fargs[1], fargs[0]))
        safe_str("1", buff, bufcx);
    else
        safe_str("0", buff, bufcx);
    return;
}

FUNCTION(fun_streq)
{
   if ( !fargs[0] || !*fargs[0] || !fargs[1] || !*fargs[1] )
      safe_chr('0', buff, bufcx);
   else if ( !string_compare(fargs[0], fargs[1]) )
      safe_chr('1', buff, bufcx);
   else
      safe_chr('0', buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * fun_extract: extract words from string:
 * extract(foo bar baz,1,2) returns 'foo bar'
 * extract(foo bar baz,2,1) returns 'bar'
 * extract(foo bar baz,2,2) returns 'bar baz'
 *
 * Now takes optional separator extract(foo-bar-baz,1,2,-) returns 'foo-bar'
 */

FUNCTION(fun_randextract)
{
  int *used, nword, numext, got, pos, x, y, z, end;
  char *p1, *p2, sep, te, *b2, osep;

  if (!fn_range_check("RANDEXTRACT", nfargs, 1, 5, buff, bufcx)) {
    return;
  }
  if (nfargs > 3)
    te = toupper(*fargs[3]);
  else
    te = 'R';
  if (nfargs == 1)
     numext = 1;
  else
     numext = atoi(fargs[1]);
  if ((!*fargs[0]) || (numext < 1) || (numext > LBUF_SIZE) || ((te != 'L') && (te != 'R') && (te != 'D')))
    return;
  if ((nfargs > 2) && (*fargs[2]))
    sep = *fargs[2];
  else
    sep = ' ';
  if (nfargs > 4 && (*fargs[4]))
    osep = *fargs[4];
  else
    osep = sep;
  nword = 0;
  p1 = fargs[0];
  while (*p1 && (*p1 == sep))
    p1++;
  while (*p1) {     /* count number of words */
    p2 = p1 + 1;
    while (*p2 && (*p2 != sep))
      p2++;
    nword++;
    if (!*p2)
      break;
    p1 = p2 + 1;
    while (*p1 && (*p1 == sep))
      p1++;
  }
  if (!nword)
    return;
  got = 0;
  used = NULL;
  switch (te) {
    case 'L':
      pos = random() % nword;
      p1 = fargs[0];
      while (*p1 == sep)
         p1++;
      for (x = 0; x < pos; x++) { /* find start of extract */
         p2 = p1 + 1;
         while (*p2 != sep)
           p2++;
         p1 = p2 + 1;
         while (*p1 == sep)
           p1++;
      }
      end = 0;
      for (x = 0; x < numext && *p1; x++) { /* do extract */
         p2 = p1 + 1;
         while (*p2 && (*p2 != sep))
           p2++;
         if (*p2)
           *p2 = '\0';
         else
           end = 1;
         if (got)
           safe_chr(osep,buff,bufcx);
         safe_str(p1, buff, bufcx);
         got = 1;
         if (end)
           break;
         p1 = p2 + 1;
         while (*p1 && (*p1 == sep))
           p1++;
      }
      break;
    case 'R':
      if (numext > nword)
         numext = nword;
      used = (int *)malloc(sizeof(int) * nword);
      if (used == NULL)
         break;
      for (x = 0; x < nword; x++)
         *(used + x) = x;
      for (x = nword; x > 1; x--) {
         y = random() % x;
         if (y < (x - 1)) {
           z = *(used + y);
           *(used + y) = *(used + x - 1);
           *(used + x - 1) = z;
         }
      }       /* fall trough on purpose */
    case 'D':
      b2 = alloc_lbuf("randextract");
      if (b2 == NULL)
         break;
      for (x = 0; x < numext; x++) {
         if (te == 'R')
           pos = *(used+x);
         else
           pos = random() % nword;
         p1 = fargs[0];
         while (*p1 == sep)
           p1++;
         for (y = 0; y < pos; y++) { /* find start of word */
           p2 = p1+1;
           while (*p2 != sep)
             p2++;
           p1 = p2 + 1;
           while (*p1 == sep)
             p1++;
         }
         p2 = p1 + 1;
         while (*p2 && (*p2 != sep)) /* find end of word */
           p2++;
         z = p2 - p1;
         if (z >= LBUF_SIZE)
           z = LBUF_SIZE - 1;
         strncpy(b2, p1, z);
         *(b2 + z) = '\0';
         if (got)
           safe_chr(osep,buff,bufcx);
         safe_str(b2, buff, bufcx);
         got = 1;
      }
      free_lbuf(b2);
      break;
  }
  if (used)
    free(used);
}

FUNCTION(fun_extractword)
{
   int start, len, i_cntr, i_cntr2, first, i_del;
   char *sep, *osep, *pos, *prevpos, *fargbuff, *outbuff;
   ANSISPLIT outsplit[LBUF_SIZE], *optr, *prevoptr;

   if (!fn_range_check("WORDEXTRACT", nfargs, 3, 6, buff, bufcx))
      return;

   if ( !*fargs[0] )
      return;

   initialize_ansisplitter(outsplit, LBUF_SIZE);
   fargbuff = alloc_lbuf("fun_extractword_fargbuff");
   memset(fargbuff, '\0', LBUF_SIZE);
   split_ansi(strip_ansi(fargs[0]), fargbuff, outsplit);

   start = atoi(fargs[1]);
   len = atoi(fargs[2]);
   if ((start < 1) || (len < 1)) {
      return;
   }

   sep  = alloc_lbuf("fun_extractword_sep");
   osep = alloc_lbuf("fun_extractword_osep");
   if ( (nfargs > 3) && *fargs[3] ) {
      strcpy(sep, strip_all_ansi(fargs[3]));
   } else {
      strcpy(sep, (char *)" ");
   }
   if ( (nfargs > 4) && *fargs[4] ) {
      strcpy(osep, fargs[4]);
   } else {
      strcpy(osep, sep);
   }
   
   if ( (nfargs > 5) && *fargs[5] ) {
      i_del = atoi(fargs[5]);
   } else {
      i_del = 0;
   }
   pos = fargbuff;
   optr = outsplit;
   i_cntr = 1;
   first = i_cntr2 = 0;
   while ( pos && *pos ) {
      prevpos = strstr(pos, sep);
      prevoptr = optr + (prevpos - pos);
      if ( !prevpos || !*prevpos )
         break;
      if ( ((i_cntr < start) && !i_del) ||
           ( ((i_cntr >= start) && (i_cntr < (start + len))) && i_del) ) {
         i_cntr++;
         *(prevpos)='\0';
         pos = prevpos + strlen(sep);
         optr = prevoptr + strlen(sep);
         if ( !pos || !*pos ) 
            break;
         *(pos-1) = '\0';
         continue;
      }
      *(prevpos)='\0';
      if ( first )
         safe_str(osep, buff, bufcx);
      first = 1;
      outbuff = rebuild_ansi(pos, optr);
      safe_str(outbuff, buff, bufcx);
      free_lbuf(outbuff);
      pos = prevpos + strlen(sep);
      optr = prevoptr + strlen(sep);
      if ( !pos || !*pos ) 
         break;
      *(pos-1) = '\0';
      i_cntr++;
      i_cntr2++;
      if ( (i_cntr2 >= len) && !i_del )
         break;
   }
   if ( pos && *pos && ( 
        ((i_cntr2 < len) && (start <= i_cntr) && !i_del) || 
        (((i_cntr < start) || (i_cntr >= (start + len))) && i_del) )) {
      if ( first )
         safe_str(osep, buff, bufcx);
      outbuff = rebuild_ansi(pos, optr);
      safe_str(outbuff, buff, bufcx);
      free_lbuf(outbuff);
   }
   free_lbuf(sep);
   free_lbuf(osep);
   free_lbuf(fargbuff);
}

FUNCTION(fun_extract)
{
    int start, len;
    char *r, *s, sep;

    varargs_preamble("EXTRACT", 4);

    s = fargs[0];
    start = atoi(fargs[1]);
    len = atoi(fargs[2]);

    if ((start < 1) || (len < 1)) {
         return;
    }
    /* Skip to the start of the string to save */

    start--;
    s = trim_space_sep(s, sep);
    while (start && s) {
         s = next_token(s, sep);
         start--;
    }

    /* If we ran of the end of the string, return nothing */

    if (!s || !*s) {
         return;
    }
    /* Count off the words in the string to save */

    r = s;
    len--;
    while (len && s) {
         s = next_token(s, sep);
         len--;
    }

    /* Chop off the rest of the string, if needed */

    if (s && *s)
         split_token(&s, sep);
    safe_str(r, buff, bufcx);
}

/* --------------------------------------------------------------
 * delextract works opposite of extract().  It returns what it
 * does not match instead of what it does.
 */
FUNCTION(fun_delextract)
{
    char *curr, *objstring, *cp, *sep_buf, sep;
    int cntr, first, second, flag;

    if (!fn_range_check("DELEXTRACT", nfargs, 3, 4, buff, bufcx))
       return;

    if ( !*fargs[0] )
       return;

    first = atoi(fargs[1]);
    second = atoi(fargs[2]);

    if ( first < 1 )
       first = 1;
    if ( second < 0 )
       second = 0;
    else
       second = second + first - 1;

    if (nfargs > 3 ) {
       sep_buf = exec(player, cause, caller,
          EV_STRIP | EV_FCHECK | EV_EVAL, fargs[3], cargs, ncargs);
       sep = *sep_buf;
       free_lbuf(sep_buf);
    } else {
       sep = ' ';
    }

    curr = alloc_lbuf("fun_delextract");
    cp = curr;
    safe_str(strip_ansi(fargs[0]),curr,&cp);
    cp = curr;
    cp = trim_space_sep(cp, sep);
    if (!*cp) {
         free_lbuf(curr);
         return;
    }

    cntr = 1;
    flag = 1;
    while (cp) {
        objstring = split_token(&cp, sep);
        if ( cntr < first || cntr > second ) {
           if (!flag)
             safe_chr(sep, buff, bufcx);
           flag = 0;
           safe_str(objstring, buff, bufcx);
        }
        cntr++;
    }
    free_lbuf(curr);
}

int
xlate(char *arg)
{
    int temp;
    char *temp2;

    if (arg[0] == '#') {
       arg++;
       if (is_integer(arg)) {
          temp = atoi(arg);
          if (temp == -1) {
             if ( !mudconf.notonerr_return )
                temp = 0;
             else
                temp = 1;
          }
          return temp;
       }
       if ( (mudconf.notonerr_return) && (atoi(arg) == -1) )
          return 1;
       return 0;
    }
    temp2 = trim_space_sep(arg, ' ');
    if (!*temp2)
       return 0;
    if (is_integer(temp2))
       return atoi(temp2);
    return 1;
}

/* ---------------------------------------------------------------------------
 * fun_index:  like extract(), but it works with an arbitrary separator.
 * index(a b | c d e | f gh | ij k, |, 2, 1) => c d e
 * index(a b | c d e | f gh | ij k, |, 2, 2) => c d e | f g h
 */

FUNCTION(fun_index)
{
    int start, end;
    char c, *s, *p;

    s = fargs[0];
    c = *fargs[1];
    start = atoi(fargs[2]);
    end = atoi(fargs[3]);

    if ((start < 1) || (end < 1) || (*s == '\0'))
       return;
    if (c == '\0')
       c = ' ';

    /* move s to point to the start of the item we want */

    start--;
    while (start && s && *s) {
       if ((s = (char *) index(s, c)) != NULL)
           s++;
       start--;
    }

    /* skip over just spaces, not tabs or newlines, since people may
     * MUSHcode strings like "%r%tAmberyl %r%tMoonchilde %r%tEvinar"
     */

    while (s && (*s == ' '))
       s++;
    if (!s || !*s)
       return;

    /* figure out where to end the string */

    p = s;
    while (end && p && *p) {
       if ((p = (char *) index(p, c)) != NULL) {
           if (--end == 0) {
              do {
                  p--;
              } while ((*p == ' ') && (p > s));
              *(++p) = '\0';
              safe_str(s, buff, bufcx);
              return;
           } else {
              p++;
           }
       }
    }

    /* if we've gotten this far, we've run off the end of the string */

    safe_str(s, buff, bufcx);
}


FUNCTION(fun_cat)
{
    int i;

    safe_str(fargs[0], buff, bufcx);
    for (i = 1; i < nfargs; i++) {
       safe_chr(' ', buff, bufcx);
       safe_str(fargs[i], buff, bufcx);
    }
}

FUNCTION(fun_version)
{
    char *tpr_buff, *tprp_buff;

    if(nfargs == 0) {
      safe_str(mudstate.version, buff, bufcx);
    } else {
      tprp_buff = tpr_buff = alloc_lbuf("fun_version");
      safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%.150s",
           MUSH_BUILD_DATE), buff, bufcx);
      free_lbuf(tpr_buff);
  }
}

FUNCTION(fun_strlenvis)
{
   int i_count;
   
   i_count = count_extended(fargs[0]);
   ival(buff, bufcx, (int) strlen(strip_all_special(fargs[0])) - i_count);
}

FUNCTION(fun_strlenraw)
{
    ival(buff, bufcx, (int) strlen(fargs[0]));
}

FUNCTION(fun_strlen)
{
    ival(buff, bufcx, (int) strlen(strip_all_special(fargs[0])));
}

FUNCTION(fun_num)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if (it != NOTHING) {
       if (Cloak(it) && !Wizard(player)) {
           safe_str("#-1", buff, bufcx);
           return;
       }
       if (SCloak(it) && Cloak(it) && !Immortal(player)) {
           safe_str("#-1", buff, bufcx);
           return;
       }
    }
    dbval(buff, bufcx, it);
}

FUNCTION(fun_rnum)
{
    dbref it, loc;

    loc = match_thing(player, fargs[0]);
    if (loc != NOTHING) {
       if (Cloak(loc) && !Wizard(player)) {
           safe_str("#-1", buff, bufcx);
           return;
       }
       if (SCloak(loc) && Cloak(loc) && !Immortal(player)) {
           safe_str("#-1", buff, bufcx);
           return;
       }
       if ( !Examinable(player, loc) ) {
          safe_str("#-1", buff, bufcx);
          return;
       }
       if ( !Controls(loc, player) ) {
          safe_str("#-1", buff, bufcx);
          return;
       }
    }
    it = match_thing(loc, fargs[1]);
    dbval(buff, bufcx, it);
}

FUNCTION(fun_gt)
{
    ival(buff, bufcx, (safe_atof(fargs[0]) > safe_atof(fargs[1])));
}

FUNCTION(fun_gte)
{
    ival(buff, bufcx, (safe_atof(fargs[0]) >= safe_atof(fargs[1])));
}

FUNCTION(fun_lt)
{
    ival(buff, bufcx, (safe_atof(fargs[0]) < safe_atof(fargs[1])));
}

FUNCTION(fun_lte)
{
    ival(buff, bufcx, (safe_atof(fargs[0]) <= safe_atof(fargs[1])));
}

FUNCTION(fun_eq)
{
    ival(buff, bufcx, (safe_atof(fargs[0]) == safe_atof(fargs[1])));
}

FUNCTION(fun_neq)
{
    ival(buff, bufcx, (safe_atof(fargs[0]) != safe_atof(fargs[1])));
}

FUNCTION(fun_mask)
{
  char t;
  int x, y, i, i_maxargs;

  if (nfargs < 2) {
       safe_str("#-1 FUNCTION (MASK) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
  }
  if ( nfargs == 2 ) {
     i_maxargs = 2;
     t = '&';
  } else {
     i_maxargs = nfargs - 1;
     t = '&';
     if ( *fargs[nfargs-1] ) {
        t = *fargs[nfargs-1];
     }
     switch (t) {
        case '|' : break;
        case '^' : break;
        case '~' : break;
        case '1' : break;
        case '2' : break;
        default  : t = '&'; break;
     }
  }

  for ( i = 0; i < i_maxargs; i++ ) {
     if ( !is_number(fargs[i]) ) {
       i_maxargs = -1;
       break;
     }
  }
  if ( i_maxargs == -1 ) {
     safe_str("#-1",buff,bufcx);
     return;
  }

  x = atoi(fargs[0]);
  if ( x < 0 ) {
     safe_str("#-1",buff,bufcx);
     return;
  }
  if ( (t == '1') || (t == '2') ) {
     switch (t) {
        case '1': /* 1's Compliment */
           x = ~x;
           break;
        case '2': /* 2's Compliment */
           x = ~x+1;
           break;
      }
  } else {
     for ( i = 1; i < i_maxargs; i++ ) {
        y = atoi(fargs[i]);
        if ( y < 0 ) {
          i_maxargs = -1;
          break;
        }
        switch (t) {
           case '&': /* Bitwise And */
              x &= y;
              break;
           case '|': /* Bitwise Or */
              x |= y;
              break;
           case '^': /* Bitwise Xor */
              x ^= y;
              break;
           case '~': /* And 1's Comp (nand) */
              x &= ~y;
              break;
        }
     }
  }

  if ( i_maxargs == -1 ) {
     safe_str("#-1",buff,bufcx);
     return;
  }
  uival(buff,bufcx,x);
}

FUNCTION(fun_andchr)
{
  char *pt1, *pt2;
  int norig, nfound;

  norig = strlen(fargs[1]);
  nfound = 0;
  pt1 = fargs[0];
  while (pt1 && *pt1 && *fargs[1]) {
    pt1 = strpbrk(pt1,fargs[1]);
    if (pt1) {
      nfound++;
      pt2 = strchr(fargs[1],*pt1);
      while (*pt2) {
        *pt2 = *(pt2+1);
        pt2++;
      }
      pt1++;
    }
  }
  if (nfound == norig)
    safe_str("1",buff,bufcx);
  else
    safe_str("0",buff,bufcx);
}

FUNCTION(fun_xorchr)
{
  char *pt1, *pt2;
  int nfound;

  nfound = 0;
  pt1 = fargs[0];
  while (pt1 && *pt1 && *fargs[1]) {
    pt1 = strpbrk(pt1,fargs[1]);
    if (pt1) {
      nfound++;
      pt2 = strchr(fargs[1],*pt1);
      while (*pt2) {
        *pt2 = *(pt2+1);
        pt2++;
      }
      pt1++;
    }
  }
  if (nfound == 1)
    safe_str("1",buff,bufcx);
  else
    safe_str("0",buff,bufcx);
}

FUNCTION(fun_orchr)
{
  if (strpbrk(fargs[0],fargs[1]))
    safe_str("1",buff,bufcx);
  else
    safe_str("0",buff,bufcx);
}

FUNCTION(fun_notchr)
{
  if (!strpbrk(fargs[0],fargs[1]))
    safe_str("1",buff,bufcx);
  else
    safe_str("0",buff,bufcx);
}

FUNCTION(fun_and)
{
    int i, val, tval, got_one;

    val = 0;
    for (i = 0, got_one = 0; i < nfargs; i++) {
        tval = atoi(fargs[i]);
        if (i > 0) {
            got_one = 1;
            val = val && atoi(fargs[i]);
        } else {
            val = tval;
        }
    }
    if (!got_one) {
       safe_str("#-1 FUNCTION (AND) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
    } else {
        ival(buff, bufcx, val);
    }
    return;
}

FUNCTION(fun_cand)
{
    int i, val, tval;
    char *retbuff;

    val = 0;
    if (nfargs < 2) {
       safe_str("#-1 FUNCTION (CAND) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    for (i = 0; i < nfargs; i++) {
        retbuff = exec(player, cause, caller, EV_STRIP|EV_FCHECK|EV_EVAL, (char *) fargs[i], cargs, ncargs);
        tval = atoi(retbuff);
        free_lbuf(retbuff);
        if (i > 0) {
            val = val && tval;
        } else {
            val = tval && 1;
        }
        if ( val == 0 )
           break;
    }
    ival(buff, bufcx, val);
    return;
}

FUNCTION(fun_nand)
{
    int i, val, tval, got_one;
    val = 0;
    for (i = 0, got_one = 0; i < nfargs; i++) {
      tval = atoi(fargs[i]);
      if (i > 0) {
        got_one = 1;
        if (mudconf.nand_compat) {
          val = !(val && atoi(fargs[i]));
        } else {
          val = val && atoi(fargs[i]);
        }
      } else {
        val = tval;
      }
    }

    if (!got_one) {
      safe_str("#-1 FUNCTION (NAND) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
      ival(buff, bufcx, nfargs);
      safe_chr(']', buff, bufcx);
    } else {
      ival(buff, bufcx, (mudconf.nand_compat) ? val : !val);
    }
    return;
}

FUNCTION(fun_or)
{
    int i, val, tval, got_one;

    val = 0;
    for (i = 0, got_one = 0; i < nfargs; i++) {
        tval = atoi(fargs[i]);
        if (i > 0) {
            got_one = 1;
            val = val || atoi(fargs[i]);
        } else {
            val = tval;
        }
    }
    if (!got_one) {
        safe_str("#-1 FUNCTION (OR) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
        ival(buff, bufcx, nfargs);
        safe_chr(']', buff, bufcx);
    } else {
        ival(buff, bufcx, val);
    }
    return;
}
FUNCTION(fun_cor)
{
    int i, val, tval;
    char *retbuff;

    val = 0;
    if ( nfargs < 2 ) {
        safe_str("#-1 FUNCTION (COR) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
        ival(buff, bufcx, nfargs);
        safe_chr(']', buff, bufcx);
        return;
    }
    for (i = 0; i < nfargs; i++) {
        retbuff = exec(player, cause, caller, EV_STRIP|EV_FCHECK|EV_EVAL, (char *) fargs[i], cargs, ncargs);
        tval = atoi(retbuff);
        free_lbuf(retbuff);
        if (i > 0) {
            val = val || tval;
        } else {
            val = tval || 0;
        }
        if ( val == 1 )
           break;
    }
    ival(buff, bufcx, val);
    return;
}

FUNCTION(fun_nor)
{
    int i, val, tval, got_one;

    val = 0;
    for (i = 0, got_one = 0; i < nfargs; i++) {
        tval = atoi(fargs[i]);
        if (i > 0)
          got_one = 1;
        if(val || !got_one)
          val = !tval;
          //else
          //{
          //  got_one = 1;
          //  val = !(val || tval);
          //}
    }
    if (!got_one) {
        safe_str("#-1 FUNCTION (NOR) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
        ival(buff, bufcx, nfargs);
        safe_chr(']', buff, bufcx);
    } else {
        ival(buff, bufcx, val);
    }
    return;
}

FUNCTION(fun_xor)
{
    int i, val, tval, got_one;

    val = 0;
    for (i = 0, got_one = 0; i < nfargs; i++) {
        tval = atoi(fargs[i]);
        if (i > 0) {
            got_one = 1;
            val = (val && !tval) || (!val && tval);
        } else {
            val = tval;
        }
    }
    if (!got_one) {
        safe_str("#-1 FUNCTION (XOR) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
        ival(buff, bufcx, nfargs);
        safe_chr(']', buff, bufcx);
    } else {
        ival(buff, bufcx, val);
    }
    return;
}

FUNCTION(fun_xnor)
{
    int i, val, tval, got_one;

    val = 0;
    for (i = 0, got_one = 0; i < nfargs; i++) {
        tval = atoi(fargs[i]);
        if (i > 0) {
            got_one = 1;
            val = !((val && !tval) || (!val && tval));
        } else {
            val = tval;
        }
    }
    if (!got_one) {
        safe_str("#-1 FUNCTION (XNOR) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
        ival(buff, bufcx, nfargs);
        safe_chr(']', buff, bufcx);
    } else {
        ival(buff, bufcx, val);
    }
    return;
}

FUNCTION(fun_not)
{
    ival(buff, bufcx, !xlate(fargs[0]));
}

FUNCTION(fun_sqrt)
{
    double val;

    val = safe_atof(fargs[0]);
    if (val < 0) {
        safe_str("#-1 SQUARE ROOT OF NEGATIVE", buff, bufcx);
    } else if (val == 0) {
        safe_str("0", buff, bufcx);
    } else {
        fval(buff, bufcx, sqrt(val));
    }
}

FUNCTION(fun_avg)
{
    double sum;
    int i, got_one;

    sum = 0;
    for (i = 0, got_one = 0; i < nfargs; i++) {
        sum = sum + safe_atof(fargs[i]);
        if (i > 0)
            got_one = 1;
    }
    if (!got_one) {
        safe_str("#-1 FUNCTION (AVG) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
        ival(buff, bufcx, nfargs);
        safe_chr(']', buff, bufcx);
    } else
        fval(buff, bufcx, sum/(long)nfargs);
    return;
}

FUNCTION(fun_add)
{
    double sum;
    int i, got_one;

    sum = 0;
    for (i = 0, got_one = 0; i < nfargs; i++) {
        sum = sum + safe_atof(fargs[i]);
        if (i > 0)
            got_one = 1;
    }
    if (!got_one) {
       safe_str("#-1 FUNCTION (ADD) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
    } else
       fval(buff, bufcx, sum);
    return;
}

FUNCTION(fun_sub)
{
    fval(buff, bufcx, safe_atof(fargs[0]) - safe_atof(fargs[1]));
}

FUNCTION(fun_mul)
{
    int i, got_one;
    double prod;

    prod = 1;
    for (i = 0, got_one = 0; i < nfargs; i++) {
       prod = prod * safe_atof(fargs[i]);
       if (i > 0)
           got_one = 1;
    }
    if (!got_one) {
       safe_str("#-1 FUNCTION (MUL) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
    } else
       fval(buff, bufcx, prod);
    return;
}

FUNCTION(fun_floor)
{
    char tempbuff[LBUF_SIZE/2];

    sprintf(tempbuff, "%.0f", floor(safe_atof(fargs[0])));
    safe_str(tempbuff, buff, bufcx);
}
FUNCTION(fun_ceil)
{
    char tempbuff[LBUF_SIZE/2];
    sprintf(tempbuff, "%.0f", ceil(safe_atof(fargs[0])));
    safe_str(tempbuff, buff, bufcx);
}
FUNCTION(fun_round)
{
    static char fstr[6];
    char tempbuff[LBUF_SIZE/2];
    int i_val;

    i_val = atoi(fargs[1]);
    memset(fstr, 0, sizeof(fstr));
    if ( (i_val > 0) && (i_val < 60))
        sprintf(fstr, "%%.%df", i_val);
    else
        strcpy(fstr, "%.0f");
    if ( (i_val < 0) && (i_val > -100) ) {
       i_val = abs(i_val);
       sprintf(tempbuff, fstr, (safe_atof(fargs[0]) / (pow(10,i_val))));
       sprintf(tempbuff, fstr, (safe_atof(tempbuff) * (pow(10,i_val))));
    } else {
       sprintf(tempbuff, fstr, safe_atof(fargs[0]) + ((mudconf.round_kludge) ? 0.00000001 : 0.0));
    }

    /* Handle bogus result of "-0" from sprintf.  Yay, cclib. */

    if (!strcmp(tempbuff, "-0")) {
        safe_str("0", buff, bufcx);
    }
    else
        safe_str(tempbuff, buff, bufcx);
}

FUNCTION(fun_trunc)
{
    int num;

    num = atoi(fargs[0]);
    ival(buff, bufcx, num);
}

FUNCTION(fun_div)
{
    int bot, top;

    bot = atoi(fargs[1]);
    top = 0;
    if (bot == 0) {
       safe_str("#-1 DIVIDE BY ZERO", buff, bufcx);
    } else {
       top = atoi(fargs[0]);
       if ( top < -(INT_MAX) )
          top = -(INT_MAX);
       ival(buff, bufcx, top / bot);
    }
}

/* Functionality borrowed from MUX2 */
FUNCTION(fun_floordiv)
{
    int bot, top;

    bot = atoi(fargs[1]);
    if (bot == 0) {
       safe_str("#-1 DIVIDE BY ZERO", buff, bufcx);
    } else {
       top = atoi(fargs[0]);
       if ( top < -(INT_MAX) )
          top = -(INT_MAX);
       if (bot < 0) {
          if (top <= 0) {
             ival(buff, bufcx, (top / bot));
          } else {
             ival(buff, bufcx, ((top - bot - 1) / bot));
          }
       } else {
          if (top < 0) {
             ival(buff, bufcx, ((top - bot + 1) / bot));
          } else {
             ival(buff, bufcx, (top / bot));
          }
       }
    }
}

FUNCTION(fun_fdiv)
{
    double bot;

    bot = safe_atof(fargs[1]);
    if (bot == 0) {
       safe_str("#-1 DIVIDE BY ZERO", buff, bufcx);
    } else {
       fval(buff, bufcx, (safe_atof(fargs[0]) / bot));
    }
}

FUNCTION(fun_mod)
{
    int bot, top;

    bot = atoi(fargs[1]);
    top = atoi(fargs[0]);
    if (bot == 0)
       bot = 1;
    if ( top < -(INT_MAX) )
       top = -(INT_MAX);
    ival(buff, bufcx, top % bot);
}

FUNCTION(fun_fmod)
{
    double bot;

    bot = safe_atof(fargs[1]);
    if (bot == 0)
      bot = 1;
    fval(buff, bufcx, fmod(safe_atof(fargs[0]),bot));
}

/*******************************************************************************/
/* This returns the MODULUS of the number.  This logic taken from PENN 1.7.3.x */
/*******************************************************************************/
FUNCTION(fun_modulo)
{
    int bot, i_top;
    bot = atoi(fargs[1]);
    i_top = atoi(fargs[0]);

    if ( bot == 0 )
       bot = 1;
    if (i_top < 0) {
      if (bot < 0) {
        if ( i_top < -(INT_MAX) )
           i_top = -(INT_MAX);
        i_top = -(-i_top % -bot);
      } else
        i_top = (bot - (-i_top % bot)) % bot;
    } else {
      if (bot < 0)
        i_top = -((-bot - (i_top % -bot)) % -bot);
      else
        i_top = i_top % bot;
    }
    ival(buff, bufcx, i_top);
}

FUNCTION(fun_pi)
{
    safe_str("3.141592654", buff, bufcx);
}
FUNCTION(fun_ee)
{
   double d_ee;
   int i_ee;
   char *s_ee;

   if (!fn_range_check("EE", nfargs, 1, 2, buff, bufcx))
      return;

   i_ee = 2;
   d_ee = safe_atof(fargs[0]); 
   if ( (nfargs > 1) && *fargs[1] ) {
      i_ee = atoi(fargs[1]);
      if ( i_ee < 0 )
         i_ee = 0;
      if ( i_ee > 10 )
         i_ee = 10;
   }
   s_ee = alloc_lbuf("fun_ee");
   sprintf(s_ee, "%.*E", i_ee, d_ee);
   safe_str(s_ee, buff, bufcx);
   free_lbuf(s_ee);
}

FUNCTION(fun_e)
{
    safe_str("2.718281828", buff, bufcx);
}
/**********************************************
 * Degree, Radian, Grad converter - From MUX2
 * Used with permission
 **********************************************/
static double ConvertRDG2R(double d, const char *szUnits)
{
    switch (tolower(szUnits[0]))
    {
    case 'd':
        // Degrees to Radians.
        //
        d *= 0.017453292519943295;
        break;

    case 'g':
        // Gradians to Radians.
        //
        d *= 0.011570796326794896;
        break;
    }
    return d;
}

static double ConvertR2RDG(double d, const char *szUnits)
{
    switch (tolower(szUnits[0]))
    {
    case 'd':
        // Radians to Degrees.
        //
        d *= 57.29577951308232;
        break;

    case 'g':
        // Radians to Gradians.
        //
        d *= 63.66197723675813;
        break;
    }
    return d;
}

FUNCTION(fun_sin)
{
    double d;

    if (!fn_range_check("SIN", nfargs, 1, 2, buff, bufcx)) {
       return;
    }
    d = safe_atof(fargs[0]);
    if (nfargs == 2) {
        d = ConvertRDG2R(d, fargs[1]);
    }
    fval(buff, bufcx, sin(d));
}

FUNCTION(fun_cos)
{
    double d;

    if (!fn_range_check("COS", nfargs, 1, 2, buff, bufcx)) {
       return;
    }
    d = safe_atof(fargs[0]);
    if (nfargs == 2) {
        d = ConvertRDG2R(d, fargs[1]);
    }
    fval(buff, bufcx, cos(d));
}

FUNCTION(fun_tan)
{
    double d;

    if (!fn_range_check("TAN", nfargs, 1, 2, buff, bufcx)) {
       return;
    }
    d = safe_atof(fargs[0]);
    if (nfargs == 2) {
        d = ConvertRDG2R(d, fargs[1]);
    }
    fval(buff, bufcx, tan(d));
}

FUNCTION(fun_sinh)
{
    double d;

    if (!fn_range_check("SINH", nfargs, 1, 2, buff, bufcx)) {
       return;
    }
    d = safe_atof(fargs[0]);
    if (nfargs == 2) {
        d = ConvertRDG2R(d, fargs[1]);
    }
    fval(buff, bufcx, sinh(d));
}

FUNCTION(fun_cosh)
{
    double d;

    if (!fn_range_check("COSH", nfargs, 1, 2, buff, bufcx)) {
       return;
    }
    d = safe_atof(fargs[0]);
    if (nfargs == 2) {
        d = ConvertRDG2R(d, fargs[1]);
    }
    fval(buff, bufcx, cosh(d));
}

FUNCTION(fun_tanh)
{
    double d;

    if (!fn_range_check("TANH", nfargs, 1, 2, buff, bufcx)) {
       return;
    }
    d = safe_atof(fargs[0]);
    if (nfargs == 2) {
        d = ConvertRDG2R(d, fargs[1]);
    }
    fval(buff, bufcx, tanh(d));
}


FUNCTION(fun_exp)
{
    fval(buff, bufcx, exp(safe_atof(fargs[0])));
}

FUNCTION(fun_power)
{
    double val1, val2;

    val1 = safe_atof(fargs[0]);
    val2 = safe_atof(fargs[1]);
    if (val1 < 0) {
       safe_str("#-1 POWER OF NEGATIVE", buff, bufcx);
    } else if ((val1 > 1461990.0) || (val2 > 50.0)) {
       safe_str("#-1 OPERAND OUT OF RANGE", buff, bufcx);
    } else {
       fval(buff, bufcx, pow(val1, val2));
    }
}

FUNCTION(fun_power10)
{
    double val1;

    val1 = safe_atof(fargs[0]);
    if ((val1 > 3000) || (val1 < -2000000000)) {
       safe_str( "#-1 OPERAND OUT OF RANGE", buff, bufcx);
    } else {
       fval(buff, bufcx, pow(10,val1));
    }
}

FUNCTION(fun_ln)
{
    double val;

    val = safe_atof(fargs[0]);
    if (val > 0)
       fval(buff, bufcx, log(val));
    else
       safe_str("#-1 LN OF NEGATIVE OR ZERO", buff, bufcx);
}

FUNCTION(fun_log)
{
    double val, val2, lval2;

    lval2 = val2 = val = 0L;
    if (!fn_range_check("LOG", nfargs, 1, 2, buff, bufcx))
       return;
    val = safe_atof(fargs[0]);
    if ( nfargs > 1 )
      val2 = safe_atof(fargs[1]);
    if ((val > 0) && (nfargs == 1)) {
       fval(buff, bufcx, log10(val));
    } else if ((val > 0) && (val2 > 0)) {
        lval2 = log10(val2);
        if ( lval2 == 0 )
          safe_str("#-1 DIVISION BY ZERO", buff, bufcx);
        else
           fval(buff, bufcx, (log10(val) / lval2));
    } else {
       safe_str("#-1 LOG OF NEGATIVE OR ZERO", buff, bufcx);
    }
}

FUNCTION(fun_asin)
{
    double val;

    if (!fn_range_check("ASIN", nfargs, 1, 2, buff, bufcx)) {
       return;
    }
    val = safe_atof(fargs[0]);
    if ((val < -1) || (val > 1)) {
       safe_str("#-1 ASIN ARGUMENT OUT OF RANGE", buff, bufcx);
    } else {
       val = asin(val);
       if (nfargs == 2) {
           val = ConvertR2RDG(val, fargs[1]);
       }
       fval(buff, bufcx, val);
    }
}

FUNCTION(fun_acos)
{
    double val;

    if (!fn_range_check("ACOS", nfargs, 1, 2, buff, bufcx)) {
       return;
    }
    val = safe_atof(fargs[0]);
    if ((val < -1) || (val > 1)) {
        safe_str("#-1 ACOS ARGUMENT OUT OF RANGE", buff, bufcx);
    } else {
        val = acos(val);
        if (nfargs == 2) {
            val = ConvertR2RDG(val, fargs[1]);
        }
        fval(buff, bufcx, val);
    }
}

FUNCTION(fun_atan)
{
    double val;

    if (!fn_range_check("ATAN", nfargs, 1, 2, buff, bufcx)) {
       return;
    }
    val = atan(safe_atof(fargs[0]));
    if ( nfargs == 2 ) {
            val = ConvertR2RDG(val, fargs[1]);
    }
    fval(buff, bufcx, val);
}
FUNCTION(fun_ctu)
{
    double val;

    val = safe_atof(fargs[0]);
    val = ConvertRDG2R(val, fargs[1]);
    val = ConvertR2RDG(val, fargs[2]);
    fval(buff, bufcx, val);
}

FUNCTION(fun_dist2d)
{
    double d, r;

    d = safe_atof(fargs[0]) - safe_atof(fargs[2]);
    r = (double) (d * d);
    d = safe_atof(fargs[1]) - safe_atof(fargs[3]);
    r += (double) (d * d);
    d = sqrt(r);
    fval(buff, bufcx, d);
}

FUNCTION(fun_dist3d)
{
    double d, r;

    d = safe_atof(fargs[0]) - safe_atof(fargs[3]);
    r = (double) (d * d);
    d = safe_atof(fargs[1]) - safe_atof(fargs[4]);
    r += (double) (d * d);
    d = safe_atof(fargs[2]) - safe_atof(fargs[5]);
    r += (double) (d * d);
    d = sqrt(r);
    fval(buff, bufcx, d);
}



/* ---------------------------------------------------------------------------
 * fun_comp: string compare.
 */

FUNCTION(fun_comp)
{
    int x;

    x = strcmp(fargs[0], fargs[1]);
    if (x > 0)
       safe_str("1", buff, bufcx);
    else if (x < 0)
       safe_str("-1", buff, bufcx);
    else
       safe_str("0", buff, bufcx);
}

FUNCTION(fun_ncomp)
{
   int x, y;

   x = atoi(fargs[0]);
   y = atoi(fargs[1]);
   ival(buff, bufcx, (x == y ? 0 : (x < y ? -1 : 1)) );
}

/* --------------------------------------------------------------------------
 * fun_lzone: Return a list of objects in zone list
 */
FUNCTION(fun_lzone)
{
  dbref it;
  int i_cntr, i_type, i_max, i_ceil, i_domin, i_domax;
  char *tbuf, c_type;
  ZLISTNODE *ptr;

  if (!fn_range_check("LZONE", nfargs, 1, 2, buff, bufcx)) {
    return;
  }
  it = match_thing(player, fargs[0]);

  i_type = 0;
  c_type = '\0';
  i_domin = i_domax = 0;
  if ( (nfargs > 1) && *fargs[1] ) {
     i_type = atoi(fargs[1]);
     c_type = *fargs[1];
     if ( (c_type == '~') && (strchr(fargs[1], '-') != NULL) ) {
        i_domin = atoi(fargs[1]+1);
        i_domax = i_domin + atoi(strchr(fargs[1], '-')+1) - 1;
     }
  }
  if ( i_type < 0 )
     i_type = 0;

  i_cntr = 0;
  i_max = (i_type - 1) * 400;
  i_ceil = i_max + 400;
  if( (it != NOTHING) &&
      Examinable(player, it) ) {
    tbuf = alloc_sbuf("fun_lzone");
    for( ptr = db[it].zonelist; ptr; ptr = ptr->next ) {
       i_cntr++;
       if ( (i_type && (i_cntr < i_max)) || (c_type == 'l'))
          continue;
       if ( (i_domin > 0) && ((i_cntr < i_domin) || (i_cntr > i_domax)) )
          continue;
       if ( i_type && (i_cntr >= i_ceil) )
          break;
       /* NOTE: These sprintfs will not overflow an SBUF */
       if( ptr->next ) {
         sprintf(tbuf, "#%d ", ptr->object);
       }
       else {
         sprintf(tbuf, "#%d", ptr->object);
       }
       safe_str(tbuf, buff, bufcx);
    }
    if ( c_type == 'l' ) {
       sprintf(tbuf, "%d %d", ((i_cntr/400)+1), i_cntr);
       safe_str(tbuf, buff, bufcx);
    }
    free_sbuf(tbuf);
  }
  else {
    safe_str("#-1", buff, bufcx);
  }
}

/* ---------------------------------------------------------------------------
 * fun_zwho: Return a list of players in a specified zone.
 * It pulls from a linked list -> Is *NOT* computationally expensive.
 */
FUNCTION(fun_zwho)
{
  dbref it;
  int gotone;
  char *tbuf;
  ZLISTNODE *ptr;

  it = match_thing(player, fargs[0]);

  gotone = 0;
  if ( (it != NOTHING) && Examinable(player, it) && ZoneMaster(it) ) {
     tbuf = alloc_sbuf("fun_zwho");
     for ( ptr = db[it].zonelist; ptr; ptr = ptr->next ) {
        if ( Good_obj(ptr->object) && !Recover(ptr->object) &&
             isPlayer(ptr->object) ) {
           if ( gotone )
              safe_chr(' ', buff, bufcx);
           gotone = 1;
           sprintf(tbuf, "#%d", ptr->object);
           safe_str(tbuf, buff, bufcx);
        }
     }
     free_sbuf(tbuf);
  }
}

/* ---------------------------------------------------------------------------
 * fun_inzone: Return a list of rooms in a specified zone.
 * It pulls from a linked list -> Is *NOT* computationally expensive.
 */
FUNCTION(fun_inzone)
{
  dbref it;
  int gotone;
  char *tbuf;
  ZLISTNODE *ptr;

  it = match_thing(player, fargs[0]);

  gotone = 0;
  if ( (it != NOTHING) && Examinable(player, it) && ZoneMaster(it) ) {
     tbuf = alloc_sbuf("fun_zwho");
     for ( ptr = db[it].zonelist; ptr; ptr = ptr->next ) {
        if ( Good_obj(ptr->object) && !Recover(ptr->object) &&
             isRoom(ptr->object) ) {
           if ( gotone )
              safe_chr(' ', buff, bufcx);
           gotone = 1;
           sprintf(tbuf, "#%d", ptr->object);
           safe_str(tbuf, buff, bufcx);
        }
     }
     free_sbuf(tbuf);
  }
}

/* ---------------------------------------------------------------------------
 * fun_xcon: Return a list of contents starting at X for X count
 */
FUNCTION(fun_xcon)
{
    dbref thing, it, parent, aowner;
    char *tbuf, *pt1, *buff2, *as, *s, *pt2;
    int i, j, t, loop, can_prnt, did_prnt;
    int gotone = 0;
    int canhear, cancom, isplayer, ispuppet;
    int attr, aflags;
    int first, how_many;
    ATTR *ap;

    if (!fn_range_check("XCON", nfargs, 3, 4, buff, bufcx))
      return;

    t = 0;
    canhear = 0;
    cancom = 0;
    isplayer = 0;
    ispuppet = 0;
    first = atoi(fargs[1]);
    how_many = atoi(fargs[2]);
    if ( first <= 0 || how_many <= 0 ) {
       if ( !mudconf.mux_lcon_compat ) {
          safe_str("#-1", buff, bufcx);
       }
       return;
    }

    pt2 = NULL;
    pt1 = NULL;
    if ( (nfargs > 3) && *fargs[3] ) {
       pt2 = fargs[3];
    } else
       pt1 = strchr(fargs[0],'/');
    if (pt2) {
      if (!stricmp(pt2,"PLAYER"))
        t = 1;
      else if (!stricmp(pt2,"OBJECT"))
        t = 2;
      else if (!stricmp(pt2,"PUPPET"))
        t = 3;
      else if (!stricmp(pt2,"LISTEN"))
        t = 4;
      else if (!stricmp(pt2,"CONNECT"))
        t = 5;
    } else if (pt1) {
      if (!stricmp(pt1+1,"PLAYER")) {
        t = 1;
        *pt1 = '\0';
      } else if (!stricmp(pt1+1,"OBJECT")) {
        t = 2;
        *pt1 = '\0';
      } else if (!stricmp(pt1+1,"PUPPET")) {
        t = 3;
        *pt1 = '\0';
      } else if (!stricmp(pt1+1,"LISTEN")) {
        t = 4;
        *pt1 = '\0';
      } else if (!stricmp(pt1+1,"CONNECT")) {
        t = 5;
        *pt1 = '\0';
      }
    }
    it = match_thing(player, fargs[0]);
    j = 0;
    i = 0;
    can_prnt = 0;
    did_prnt = 0;
    if (Good_obj(it) &&
        (Has_contents(it)) &&
        (Examinable(player, it) ||
         (Location(player) == it) ||
         (it == cause))) {
       tbuf = alloc_sbuf("fun_xcon");
       DOLIST(thing, Contents(it)) {
          i++;
          if ( did_prnt >= how_many )
             break;
          switch (t) {
            case 1:
              if (Typeof(thing) != TYPE_PLAYER) {
                j++;
                continue;
              }
              break;
            case 2:
              if (Typeof(thing) != TYPE_THING) {
                j++;
                continue;
              }
              break;
            case 3:
              if (!Puppet(thing)) {
                j++;
                continue;
              }
              break;
            case 4:
              if ((Typeof(thing) == TYPE_EXIT) && Audible(thing))
                canhear = 1;
              else {
                if (Monitor(thing))
                  buff2 = alloc_lbuf("lcon");
                else
                  buff2 = NULL;
                if ( (mudconf.listen_parents == 0) || !Monitor(thing) ) {
                   for (attr = atr_head(thing, &as); attr; attr = atr_next(&as)) {
                     if (attr == A_LISTEN) {
                       canhear = 1;
                       break;
                     }
                     if (Monitor(thing)) {
                       ap = atr_num(attr);
                       if (!ap || (ap->flags & AF_NOPROG))
                         continue;
                       atr_get_str(buff2, thing, attr, &aowner, &aflags);
                       if ((buff2[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
                         continue;
                       for (s = buff2 + 1; *s && (*s != ':'); s++);
                       if (s) {
                         canhear = 1;
                         break;
                       }
                     }
                   }
                } else {
                   ITER_PARENTS(thing, parent, loop) {
                      for (attr = atr_head(parent, &as); attr; attr = atr_next(&as)) {
                        if ((thing == parent) && (attr == A_LISTEN)) {
                          canhear = 1;
                          break;
                        }
                        if (Monitor(thing)) {
                          ap = atr_num(attr);
                          if (!ap || (ap->flags & AF_NOPROG))
                            continue;
                          atr_get_str(buff2, parent, attr, &aowner, &aflags);
                          if ( (thing != parent) && ((ap->flags & AF_PRIVATE) || (aflags & AF_PRIVATE)) )
                            continue;
                          if ((buff2[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
                            continue;
                          for (s = buff2 + 1; *s && (*s != ':'); s++);
                          if (s) {
                            canhear = 1;
                            break;
                          }
                        }
                      }
                   }
                }
                if (buff2)
                  free_lbuf(buff2);
              }
              if (Typeof(thing) == TYPE_PLAYER)
                isplayer = 1;
              if (Puppet(thing))
                ispuppet = 1;
              if (!canhear && !isplayer && !ispuppet) {
                ITER_PARENTS(thing, parent, loop) {
                  if (Commer(parent))
                    cancom = 1;
                }
              }
              if (!canhear && !cancom && !isplayer && !ispuppet) {
                j++;
                continue;
              }
              break;
            case 5:
              if ((Typeof(thing) != TYPE_PLAYER) || !Connected(thing)) {
                j++;
                continue;
              }
              break;
          }
          if (( !Cloak(thing) || (Immortal(player) || (Wizard(player) && !( Immortal(thing) &&
                 SCloak(thing))))) || MyopicExam(player,thing) ) {
#ifdef REALITY_LEVELS
             if (!IsReal(player, thing)) {
                j++;
             } else if ( mudconf.lcon_checks_dark &&
                      ((mudconf.who_unfindable && Unfindable(thing) && !Admin(player)) ||
                       (!mudconf.player_dark && mudconf.allow_whodark &&
                        !mudconf.who_unfindable && Dark(thing) && !Admin(player))) ) {
                j++;
             } else {
#endif /* REALITY_LEVELS */
               can_prnt++;
               if ( first > can_prnt )
                  ;
               else {
                  did_prnt++;
                  if (gotone)
                    sprintf(tbuf, " #%d", thing);
                  else
                    sprintf(tbuf, "#%d", thing);
                  gotone = 1;
                  safe_str(tbuf, buff, bufcx);
               }
#ifdef REALITY_LEVELS
             }
#endif /* REALITY_LEVELS */
           } else {
             j++;
           }
         }
         free_sbuf(tbuf);
         if (i == j) {
           if ( !mudconf.mux_lcon_compat ) {
              safe_str("#-1", buff, bufcx);
           }
         }
     } else if (Good_obj(it) && !Has_contents(it)) {
        return;
     } else {
        if ( !mudconf.mux_lcon_compat ) {
           safe_str("#-1", buff, bufcx);
        }
     }
}

/* ---------------------------------------------------------------------------
 * fun_lcon: Return a list of contents.
 */
FUNCTION(fun_lcon)
{
    dbref thing, it, parent, aowner;
    char *tbuf, *pt1, *buff2, *as, *s, *pt2, *namebuff, *namebufcx;
    int i, j, t, loop;
    int gotone = 0;
    int canhear, cancom, isplayer, ispuppet;
    int attr, aflags;
    ATTR *ap;

    if (!fn_range_check("LCON", nfargs, 1, 4, buff, bufcx))
      return;
    if ( (nfargs > 3) && *fargs[3] )
      if(stricmp(fargs[3],"0") && stricmp(fargs[3],"1") && stricmp(fargs[3],"")) {
        safe_str( "#-1 LAST ARGUMENT MUST BE 0 OR 1", buff, bufcx );
        return;
      }
    t = 0;
    canhear = 0;
    cancom = 0;
    isplayer = 0;
    ispuppet = 0;
    pt2 = NULL;
    pt1 = NULL;
    if ( (nfargs > 1) && *fargs[1] ) {
       pt2 = fargs[1];
    } else
       pt1 = strchr(fargs[0],'/');
    if (pt2) {
      if (!stricmp(pt2,"PLAYER"))
        t = 1;
      else if (!stricmp(pt2,"OBJECT"))
        t = 2;
      else if (!stricmp(pt2,"PUPPET"))
        t = 3;
      else if (!stricmp(pt2,"LISTEN"))
        t = 4;
      else if (!stricmp(pt2,"CONNECT"))
        t = 5;
    } else if (pt1) {
      if (!stricmp(pt1+1,"PLAYER")) {
        t = 1;
        *pt1 = '\0';
      } else if (!stricmp(pt1+1,"OBJECT")) {
        t = 2;
        *pt1 = '\0';
      } else if (!stricmp(pt1+1,"PUPPET")) {
        t = 3;
        *pt1 = '\0';
      } else if (!stricmp(pt1+1,"LISTEN")) {
        t = 4;
        *pt1 = '\0';
      } else if (!stricmp(pt1+1,"CONNECT")) {
        t = 5;
        *pt1 = '\0';
      }
    }
    it = match_thing(player, fargs[0]);
    j = 0;
    i = 0;
    if (Good_obj(it) &&
        (Has_contents(it)) &&
        (Examinable(player, it) ||
         (Location(player) == it) ||
         (it == cause))) {
        tbuf = alloc_sbuf("fun_lcon");
        DOLIST(thing, Contents(it)) {
            i++;
            switch (t) {
              case 1:
                 if (Typeof(thing) != TYPE_PLAYER) {
                   j++;
                   continue;
                 }
                 break;
              case 2:
                 if (Typeof(thing) != TYPE_THING) {
                   j++;
                   continue;
                 }
                 break;
              case 3:
                 if (!Puppet(thing)) {
                   j++;
                   continue;
                 }
                 break;
             case 4:
                 if ((Typeof(thing) == TYPE_EXIT) && Audible(thing))
                    canhear = 1;
                 else {
                   if (Monitor(thing))
                      buff2 = alloc_lbuf("lcon");
                   else
                      buff2 = NULL;
                   if ( (mudconf.listen_parents == 0) || !Monitor(thing) ) {
                      for (attr = atr_head(thing, &as); attr; attr = atr_next(&as)) {
                         if (attr == A_LISTEN) {
                            canhear = 1;
                            break;
                         }
                         if (Monitor(thing)) {
                            ap = atr_num(attr);
                            if (!ap || (ap->flags & AF_NOPROG))
                               continue;
                            atr_get_str(buff2, thing, attr, &aowner, &aflags);
                            if ((buff2[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
                               continue;
                            for (s = buff2 + 1; *s && (*s != ':'); s++);
                            if (s) {
                               canhear = 1;
                               break;
                            }
                         }
                      }
                   } else {
                      ITER_PARENTS(thing, parent, loop) {
                         for (attr = atr_head(parent, &as); attr; attr = atr_next(&as)) {
                            if ((parent == thing) && (attr == A_LISTEN)) {
                               canhear = 1;
                               break;
                            }
                            if (Monitor(thing)) {
                               ap = atr_num(attr);
                               if (!ap || (ap->flags & AF_NOPROG))
                                  continue;
                               atr_get_str(buff2, parent, attr, &aowner, &aflags);
                               if ( (thing != parent) && ((ap->flags & AF_PRIVATE) || (aflags & AF_PRIVATE)) )
                                  continue;
                               if ((buff2[0] != AMATCH_LISTEN) || (aflags & AF_NOPROG))
                                  continue;
                               for (s = buff2 + 1; *s && (*s != ':'); s++);
                               if (s) {
                                  canhear = 1;
                                  break;
                               }
                            }
                         }
                      }
                   }
                   if (buff2)
                      free_lbuf(buff2);
                 }
                 if (Typeof(thing) == TYPE_PLAYER)
                   isplayer = 1;
                 if (Puppet(thing))
                   ispuppet = 1;
                 if (!canhear && !isplayer && !ispuppet) {
                   ITER_PARENTS(thing, parent, loop) {
                     if (Commer(parent))
                       cancom = 1;
                   }
                 }
                 if (!canhear && !cancom && !isplayer && !ispuppet) {
                   j++;
                   continue;
                 }
                 break;
              case 5:
                 if ((Typeof(thing) != TYPE_PLAYER) || !Connected(thing)) {
                   j++;
                   continue;
                 }
                 break;
            }
            if (( !Cloak(thing) || (Immortal(player) || (Wizard(player) && !( Immortal(thing) &&
                  SCloak(thing))))) || (MyopicExam(player,thing) && !(Cloak(thing))) ) {
#ifdef REALITY_LEVELS
                if (!IsReal(player, thing)) {
                   j++;
                } else if ( mudconf.lcon_checks_dark &&
                         ((mudconf.who_unfindable && Unfindable(thing) && !Admin(player)) ||
                          (!mudconf.player_dark && mudconf.allow_whodark &&
                           !mudconf.who_unfindable && Dark(thing) && !Admin(player))) ) {
                   j++;
                } else {
#endif /* REALITY_LEVELS */
                    if ( (nfargs > 3) && *fargs[3] && (atoi(fargs[3]) == 1)) {
                       namebuff  = namebufcx = alloc_lbuf("fun_lcon.namebuff");
                       /* Safe_str() automatically null terminates */
                       if ( !mudconf.read_rem_name &&
                            ( !nearby_exam_or_control(player, thing) ||
                              (Cloak(thing) && !Wizard(player)) ||
                              (SCloak(thing) && Cloak(thing) && !Immortal(player)) ) )  {
                          safe_str("#-1", namebuff, &namebufcx);
                       } else  {
                          safe_str(Name(thing), namebuff, &namebufcx);
                          if (gotone) {
                              /* tbuf is an SBUF so we can't just add this to it */
                              if ( (nfargs > 2) && *fargs[2] ) {
                                 safe_str(fargs[2], buff, bufcx);
                                 safe_str(namebuff, buff, bufcx);
                              } else {
                                 safe_chr(' ', buff, bufcx);
                                 safe_str(namebuff, buff, bufcx);
                              }
                          } else {
                              safe_str(namebuff, buff, bufcx);
                          }
                       }
                       gotone = 1;
                       free_lbuf(namebuff);
                    } else {
                       if (gotone) {
                          /* tbuf is an SBUF so we can't just add this to it */
                          if ( (nfargs > 2) && *fargs[2] ) {
                             safe_str(fargs[2], buff, bufcx);
                             sprintf(tbuf, "#%d", thing);
                          } else {
                             sprintf(tbuf, " #%d", thing);
                          }
                       } else {
                          sprintf(tbuf, "#%d", thing);
                       }
                       gotone = 1;
                       safe_str(tbuf, buff, bufcx);
                    }
#ifdef REALITY_LEVELS
                }
#endif /* REALITY_LEVELS */
            } else {
                j++;
            }
        }
        free_sbuf(tbuf);
        if (i == j) {  
            if ( !mudconf.mux_lcon_compat ) {
               safe_str("#-1", buff, bufcx);
            }
        }
    } else if ( Good_obj(it) && !Has_contents(it) ) {
        return;
    } else {
       if ( !mudconf.mux_lcon_compat ) {
          safe_str("#-1", buff, bufcx);
       }
    }
}

/* ---------------------------------------------------------------------------
 * fun_lexits: Return a list of exits.
 */

FUNCTION(fun_lexits)
{
    dbref thing, it, parent;
    char *tbuf, *s, *namebuff, *namebufcx;
    int exam, lev, key;
    int gotone = 0, i_type = 0, i_pageval = 0, i_counter = 0, i_start = 0, i_max = 0, i_min = -1;

    if (!fn_range_check("LEXITS", nfargs, 1, 4, buff, bufcx))
      return;
    if ( (nfargs > 2) && *fargs[2] ) {
       if(stricmp(fargs[2],"0") && stricmp(fargs[2],"1") && stricmp(fargs[2],"")) {
          safe_str( "#-1 THIRD ARGUMENT MUST BE 0 OR 1", buff, bufcx );
          return;
       }
    }

    if ( (nfargs > 3) && *fargs[3] ) {
       if ( *fargs[3] == 'l' ) {
          i_type = -1;   
       } else if ( (*fargs[3] == '~') && (strchr(fargs[3], '-') != NULL) ) {
          i_min = atoi(fargs[3]+1) - 1;
          i_max = i_min + atoi(strchr(fargs[3], '-')+1) - 1;
       } else {
          i_type = atoi(fargs[3]);
          if ( i_type < 0 )
             i_type = 0;
       }
       if ( *fargs[2] == '1' ) {
          i_pageval = 20;
       } else {
          i_pageval = 400;
       }
       i_start = i_pageval * (i_type - 1);
    }
    it = match_thing(player, fargs[0]);

    if (!Good_obj(it) || !Has_exits(it)) {
       safe_str("#-1", buff, bufcx);
       return;
    }
    exam = Examinable(player, it);
    if (!exam && (where_is(player) != it) && (it != cause)) {
       safe_str("#-1", buff, bufcx);
       return;
    }
    tbuf = alloc_sbuf("fun_lexits");

    /* Return info for all parent levels */

    ITER_PARENTS(it, parent, lev) {

  /* Look for exits at each level */

       if (!Has_exits(parent))
           continue;
       key = 0;
       if (Examinable(player, parent))
           key |= VE_LOC_XAM;
       if (Dark(parent) && !(!Cloak(parent) && could_doit( player, parent, A_LDARK, 0, 0)))
           key |= VE_LOC_DARK;
       if (Dark(it) && !(!Cloak(it) && could_doit( player, it, A_LDARK, 0 ,0)))
           key |= VE_BASE_DARK;
       DOLIST(thing, Exits(parent)) {
           if (exit_visible(thing, player, key)) {
              if ( !(Flags3(thing) & PRIVATE) || (where_is(thing) == it) ) {
                 if ( i_type == -1 ) {
                    i_counter++;
                 } else if ( (i_type > 0) && ((i_counter < i_start) || (i_counter >= (i_start + i_pageval))) ) {
                    i_counter++;
                 } else if ( (i_min >= 0) && ((i_counter < i_min) || (i_counter > i_max)) ) {
                    i_counter++;
                 } else if ( (nfargs > 2) && *fargs[2] && (atoi(fargs[2]) == 1)) {
                    namebuff  = namebufcx = alloc_lbuf("fun_lexits.namebuff");
                    if ( !mudconf.read_rem_name &&
                         (!nearby_exam_or_control(player, thing) ||
                           (Cloak(thing) && !Wizard(player)) ||
                           (SCloak(thing) && Cloak(thing) && !Immortal(player))) ) {
                        safe_str("#-1", namebuff, &namebufcx);
                    } else {
                        safe_str(Name(thing), namebuff, &namebufcx);
                        for (s = namebuff; *s && (*s != ';'); s++);
                           *s = '\0';
                    }
                    if (gotone) {
                       if ( (nfargs > 1) && *fargs[1] ) {
                       /* tbuf is an SBUF so we can't just add this to it */
                          safe_str(fargs[1], buff, bufcx);
                       } else {
                          safe_chr(' ', buff, bufcx);
                       }
                    }
                    safe_str(namebuff, buff, bufcx);
                    gotone = 1;
                    free_lbuf(namebuff);
                    i_counter++;
                 } else {
                    if (gotone) {
                       if ( (nfargs > 1) && *fargs[1] ) {
                          safe_str(fargs[1], buff, bufcx);
                          sprintf(tbuf, "#%d", thing);
                       } else {
                          sprintf(tbuf, " #%d", thing);
                       }
                    } else {
                       sprintf(tbuf, "#%d", thing);
                    }
                    gotone = 1;
                    safe_str(tbuf, buff, bufcx);
                    i_counter++;
                 }
              }
           }
       }
    }
    if ( i_type == -1 ) {
       sprintf(tbuf, "%d %d", ((i_counter / i_pageval) + 1), i_counter);
       safe_str(tbuf, buff, bufcx);
    }
    free_sbuf(tbuf);
    return;
}

/* --------------------------------------------------------------------------
 * fun_home: Return an object's home
 */

FUNCTION(fun_home)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if (!Good_obj(it))
       safe_str("#-1", buff, bufcx);
    else if ( !Examinable(player, it)) {
        if ( (Wizard(player) && !SCloak(it)) || Immortal(player) || (Guildmaster(player) && !Cloak(it))) {
           if (Has_home(it))
              dbval(buff, bufcx, Home(it));
           else if (Has_dropto(it))
              dbval(buff, bufcx, Dropto(it));
           else if (isExit(it))
              dbval(buff, bufcx, where_is(it));
        } else
           safe_str("#-1", buff, bufcx);
    } else if (Has_home(it)) {
       dbval(buff, bufcx, Home(it));
    } else if (Has_dropto(it)) {
       dbval(buff, bufcx, Dropto(it));
    } else if (isExit(it)) {
       dbval(buff, bufcx, where_is(it));
    } else
       safe_str("#-1", buff, bufcx);
    return;
}

/* ---------------------------------------------------------------------------
 * fun_money: Return an object's value
 */

FUNCTION(fun_money)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if ((it == NOTHING) || !Examinable(player, it)) {
       safe_str("#-1", buff, bufcx);
    } else {
       ival(buff, bufcx, Pennies(it));
    }
}

FUNCTION(fun_moneyname)
{
   int i_val, i_good;

   i_good = 1;
   if (!fn_range_check("MONEYNAME", nfargs, 0, 1, buff, bufcx))
      return;
   if ( (nfargs == 1) && *fargs[0] )
      i_val = atoi(fargs[0]);
   else {
      i_val = i_good =  0;
   }
   if ( !i_good )
      safe_str(mudconf.one_coin, buff, bufcx);
   else if ( (i_val == 0) || (i_val < -1) || (i_val > 1) )
      safe_str(mudconf.many_coins, buff, bufcx);
   else
      safe_str(mudconf.one_coin, buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * fun_pos: Find a word in a string */

FUNCTION(fun_pos)
{
    int i = 1;
    char *s, *t, *u;

    i = 1;
    s = strip_all_special(fargs[1]);
    while (*s) {
       u = s;
       t = fargs[0];
       while (*t && *t == *u)
           ++t, ++u;
       if (*t == '\0') {
           ival(buff, bufcx, i);
           return;
       }
       ++i, ++s;
    }
    safe_str("#-1", buff, bufcx);
    return;
}

FUNCTION(fun_numpos)
{
    int i = 1, count;
    char *s, *t, *u;

    i = 1;
    count = 0;
    s = strip_all_special(fargs[1]);
    while (*s) {
       u = s;
       t = fargs[0];
       while (*t && *t == *u)
           ++t, ++u;
       if (*t == '\0') {
           count++;
       }
       ++i, ++s;
    }
    ival(buff, bufcx, count);
    return;
}

FUNCTION(fun_totpos)
{
    int i = 1;
    char *s, *t, *u;
    int gotone = 0;

    i = 1;
    s = strip_all_special(fargs[1]);
    while (*s) {
       u = s;
       t = fargs[0];
       while (*t && *t == *u)
           ++t, ++u;
       if (*t == '\0') {
           if( gotone )
              safe_chr(' ', buff, bufcx);
           gotone = 1;
           ival(buff, bufcx, i);
       }
       ++i, ++s;
    }
    return;
}

FUNCTION(fun_randpos)
{
    int i, j, k, *tlist;
    char *s, *t, *u;
    int gotone = 0;

    if (!*fargs[1])
      return;
    tlist = (int *)malloc(sizeof(int) * strlen(strip_all_special(fargs[1])));
    if (tlist == NULL)
      return;
    i = 1;
    s = strip_all_special(fargs[1]);
    while (*s) {
      u = s;
      t = fargs[0];
      while (*t && *t == *u)
          ++t, ++u;
      if (*t == '\0') {
          *(tlist + gotone++) = i;
      }
      ++i, ++s;
    }
    if (gotone > 1) {
      for (i = gotone; i > 1; i--) {
          j = random() % i;
          if (j < (i - 1)) {
            k = *(tlist + j);
            *(tlist + j) = *(tlist + i - 1);
            *(tlist + i - 1) = k;
          }
      }
    }
    if (gotone > 0) {
      for (i = 0; i < gotone; i++) {
          if (i > 0)
            safe_chr(' ', buff, bufcx);
          ival(buff, bufcx, *(tlist + i));
      }
    }
    free(tlist);
    return;
}

/* ---------------------------------------------------------------------------
 * ldelete: Remove a word from a string by place
 *  ldelete(<list>,<position>[,<separator>])
 *
 * insert: insert a word into a string by place
 *  insert(<list>,<position>,<new item> [,<separator>])
 *
 * replace: replace a word into a string by place
 *  replace(<list>,<position>,<new item>[,<separator>])
 */

#define IF_DELETE       0
#define IF_REPLACE      1
#define IF_INSERT       2

static void
do_itemfuns(char *buff, char **bufcx, char *str, int el, char *word,
      char sep, int flag)
{
    int ct, overrun;
    char *sptr, *iptr, *eptr;

    /* If passed a null string return an empty string, except that we
     * are allowed to append to a null string.
     */

/*
    if ((!str || !*str)) {
       return;
    }
*/
    if ((!str || !*str) && ((flag != IF_INSERT) || (abs(el) != 1))) {
        return;
    }

    if ( el < 0 ) {
        if ( !str || !*str ) {
           el = 1;
        } else {
           el = countwords(str,sep) + ((flag == IF_INSERT) ? 2 : 1) + el;
        }
    }

    if ((!str || !*str) && ((flag != IF_INSERT) || (el != 1))) {
        return;
    }
    /* we can't fiddle with anything before the first position */

    if (el < 1) {
        safe_str(str, buff, bufcx);
        return;
    }
    /* Split the list up into 'before', 'target', and 'after' chunks
     * pointed to by sptr, iptr, and eptr respectively.
     */

    /* No 'before' portion, just split off element 1 */
    if (el == 1) {
        sptr = NULL;
        if (!str || !*str) {
            eptr = NULL;
            iptr = NULL;
        } else {
            eptr = trim_space_sep(str, sep);
            iptr = split_token(&eptr, sep);
        }
    } else {
    /* Break off 'before' portion */

        sptr = eptr = trim_space_sep(str, sep);
        overrun = 1;
        for (ct = el; ct > 2 && eptr; eptr = next_token(eptr, sep), ct--);
            if (eptr) {
                overrun = 0;
                iptr = split_token(&eptr, sep);
            }
        /* If we didn't make it to the target element, just return
         * the string.  Insert is allowed to continue if we are
         * exactly at the end of the string, but replace and delete
         * are not.
         */

        if (!(eptr || ((flag == IF_INSERT) && !overrun))) {
            safe_str(str, buff, bufcx);
            return;
        }
        /* Split the 'target' word from the 'after' portion. */

        if (eptr)
            iptr = split_token(&eptr, sep);
        else
            iptr = NULL;
    }

    switch (flag) {
        case IF_DELETE:   /* deletion */
            if (sptr) {
                safe_str(sptr, buff, bufcx);
                if (eptr)
                    safe_chr(sep, buff, bufcx);
            }
            if (eptr) {
                safe_str(eptr, buff, bufcx);
            }
            break;
        case IF_REPLACE:    /* replacing */
            if (sptr) {
                safe_str(sptr, buff, bufcx);
                safe_chr(sep, buff, bufcx);
            }
            safe_str(word, buff, bufcx);
            if (eptr) {
                safe_chr(sep, buff, bufcx);
                safe_str(eptr, buff, bufcx);
            }
            break;
        case IF_INSERT:   /* insertion */
            if (sptr) {
                safe_str(sptr, buff, bufcx);
                safe_chr(sep, buff, bufcx);
            }
            safe_str(word, buff, bufcx);
            if (iptr) {
                safe_chr(sep, buff, bufcx);
                safe_str(iptr, buff, bufcx);
            }
            if (eptr) {
                safe_chr(sep, buff, bufcx);
                safe_str(eptr, buff, bufcx);
            }
            break;
     }
}

int
sanitize_input_cnt(char *s_base_str, char *s_in_str, char sep, int  *i_len, int i_pos[LBUF_SIZE], int i_key) 
{
    char *st_tok, *st_tokptr;
    int i_tmp, i_found;

    *i_len = countwords(s_base_str, sep);
    if ( (*i_len == 0) && (i_key != IF_INSERT) ) {
       return(0);
    }

    if ( *i_len > (LBUF_SIZE - 10) ) {
       *i_len = LBUF_SIZE - 10;
    }
 
    for (i_tmp = 0; i_tmp < LBUF_SIZE; i_tmp++) {
       i_pos[i_tmp]=0;
    }
    
    i_found=0;
    st_tok = strtok_r(s_in_str, " \t", &st_tokptr);
    while ( st_tok ) {
       i_tmp=atoi(st_tok);
       if ( i_tmp < 0 ) {
          if ( i_key == IF_INSERT ) {
             i_tmp += *i_len + 2;
          } else {
             i_tmp += *i_len + 1;
          }
       } 
       if ( ((i_tmp <= *i_len) || ((i_tmp <= (*i_len + 1)) && (i_key == IF_INSERT))) && (i_tmp >= 0) ) {
          i_found = i_pos[i_tmp] = 1;
       }          
       st_tok = strtok_r(NULL, " \t", &st_tokptr);
    }

    if ( !i_found ) {
       return(0);
    }
   
    return(1);
}

FUNCTION(fun_replace)
{       /* replace a word at position X of a list */
    char sep, *st_tmp, *st_tmpptr, *st_mash;
    int i_pos[LBUF_SIZE], i_tmp, i_len, i_found;

    varargs_preamble("REPLACE", 4);

    i_len=0;
    i_found = sanitize_input_cnt((char *)fargs[0], (char *)fargs[1], sep, &i_len, (int *)&i_pos, IF_REPLACE);
    if ( !i_found ) {
       safe_str(fargs[0], buff, bufcx);
       return;
    }

    st_tmpptr = st_tmp = alloc_lbuf("fun_replace");
    st_mash = alloc_lbuf("fun_replace2");
    memcpy(st_mash, fargs[0], LBUF_SIZE);

    for (i_tmp = (i_len + 2); i_tmp>=0; i_tmp--) {
       if ( i_pos[i_tmp] == 1 ) {
          do_itemfuns(st_tmp, &st_tmpptr, st_mash, i_tmp, fargs[2], sep, IF_REPLACE);
          st_tmpptr=st_tmp;
          memcpy(st_mash, st_tmp, LBUF_SIZE);
       }
    }

    safe_str(st_tmp, buff, bufcx);
    free_lbuf(st_tmp);
    free_lbuf(st_mash);
}

FUNCTION(fun_ldelete)
{       /* delete a word at position X of a list */
    char sep, *st_tmp, *st_tmpptr, *st_mash;
    int i_pos[LBUF_SIZE], i_tmp, i_len, i_found;

    varargs_preamble("LDELETE", 3);

    i_len=0;
    i_found = sanitize_input_cnt((char *)fargs[0], (char *)fargs[1], sep, &i_len, (int *)&i_pos, IF_DELETE);
    if ( !i_found ) {
       safe_str(fargs[0], buff, bufcx);
       return;
    }

    st_tmpptr = st_tmp = alloc_lbuf("fun_replace");
    st_mash = alloc_lbuf("fun_replace2");
    memcpy(st_mash, fargs[0], LBUF_SIZE);

    for (i_tmp = (i_len + 2); i_tmp>=0; i_tmp--) {
       if ( i_pos[i_tmp] == 1 ) {
          do_itemfuns(st_tmp, &st_tmpptr, st_mash, i_tmp, NULL, sep, IF_DELETE);
          st_tmpptr=st_tmp;
          memcpy(st_mash, st_tmp, LBUF_SIZE);
       }
    }

    safe_str(st_tmp, buff, bufcx);
    free_lbuf(st_tmp);
    free_lbuf(st_mash);
}

FUNCTION(fun_insert)
{       /* insert a word at position X of a list */
    char sep, *st_tmp, *st_tmpptr, *st_mash;
    int i_pos[LBUF_SIZE], i_tmp, i_len, i_found;

    varargs_preamble("INSERT", 4);

    i_len=0;
    i_found = sanitize_input_cnt((char *)fargs[0], (char *)fargs[1], sep, &i_len, (int *)&i_pos, IF_INSERT);
    if ( !i_found ) {
       safe_str(fargs[0], buff, bufcx);
       return;
    }

    st_tmpptr = st_tmp = alloc_lbuf("fun_replace");
    st_mash = alloc_lbuf("fun_replace2");
    memcpy(st_mash, fargs[0], LBUF_SIZE);

    for (i_tmp = (i_len + 2); i_tmp>=0; i_tmp--) {
       if ( i_pos[i_tmp] == 1 ) {
          do_itemfuns(st_tmp, &st_tmpptr, st_mash, i_tmp, fargs[2], sep, IF_INSERT);
          st_tmpptr=st_tmp;
          memcpy(st_mash, st_tmp, LBUF_SIZE);
       }
    }

    safe_str(st_tmp, buff, bufcx);
    free_lbuf(st_tmp);
    free_lbuf(st_mash);
}

/* ---------------------------------------------------------------------------
 * fun_remove: Remove a word from a string
 */

FUNCTION(fun_remove)
{
    char *s, *sp, *word;
    char sep;
    int first, found;

    varargs_preamble("REMOVE", 3);
    if (index(fargs[1], sep)) {
        safe_str("#-1 CAN ONLY DELETE ONE ELEMENT", buff, bufcx);
        return;
    }
    s = fargs[0];
    word = fargs[1];

    /* Walk through the string copying words until (if ever) we get to
     * one that matches the target word.
     */

    sp = s;
    found = 0;
    first = 1;
    while (s) {
        sp = split_token(&s, sep);
        if (found || strcmp(sp, word)) {
            if (!first)
                safe_chr(sep, buff, bufcx);
            safe_str(sp, buff, bufcx);
            first = 0;
        } else {
            found = 1;
        }
    }
}

FUNCTION(fun_median)
{
}

FUNCTION(fun_stddev)
{
}

/* ---------------------------------------------------------------------------
 * fun_member: Is a word in a string
 */

FUNCTION(fun_nummember)
{
    int pcount, mcount;
    char *r, *s, sep, *working;

    varargs_preamble("NUMMEMBER", 3);
    pcount = mcount = 0;
    working = alloc_lbuf("tot_match2");
    strcpy(working, fargs[0]);
    while (*(working + pcount) == ' ')
      pcount++;
    while (*(working + pcount)) {
      s = trim_space_sep(working + pcount, sep);
      do {
          r = split_token(&s, sep);
          while (*(working + pcount) == ' ')
            pcount++;
          pcount += strlen(r) + 1;
          if (!strcmp(fargs[1], r)) {
            mcount++;
            break;
          }
      } while (s);
      if (!s)
          break;
      strcpy(working, fargs[0]);
    }
    free_lbuf(working);
    ival(buff, bufcx, mcount);
}

FUNCTION(fun_totmember)
{
    int wcount, pcount, gotone = 0;
    char *r, *s, sep, *working, *tbuf;

    varargs_preamble("TOTMEMBER", 3);
    pcount = 0;
    working = alloc_lbuf("tot_match2");
    strcpy(working, fargs[0]);
    while (*(working + pcount) == ' ')
      pcount++;
    tbuf = alloc_mbuf("tot_match");
    wcount = 1;
    while (*(working + pcount)) {
      s = trim_space_sep(working + pcount, sep);
      do {
          r = split_token(&s, sep);
          while (*(working + pcount) == ' ')
            pcount++;
          pcount += strlen(r) + 1;
          if (!strcmp(fargs[1], r)) {
            sprintf(tbuf, "%d", wcount);
            if (!gotone)
                safe_str(tbuf, buff, bufcx);
            else {
                safe_chr(' ', buff, bufcx);
                safe_str(tbuf, buff, bufcx);
            }
            gotone = 1;
            wcount++;
            break;
          }
          wcount++;
      } while (s);
      if (!s)
         break;
      strcpy(working, fargs[0]);
    }
    free_lbuf(working);
    free_mbuf(tbuf);
    if( !gotone )
       safe_str("0", buff, bufcx);
}

FUNCTION(fun_member)
{
    int wcount;
    char *r, *s, sep;

    varargs_preamble("MEMBER", 3);
    wcount = 1;
    s = trim_space_sep(fargs[0], sep);
    do {
       r = split_token(&s, sep);
       if (!strcmp(fargs[1], r)) {
           ival( buff, bufcx, wcount);
           return;
       }
       wcount++;
    } while (s);
    safe_str("0", buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * fun_secure, fun_escape: escape [, ], %, \, and the beginning of the string.
 */

FUNCTION(fun_secure)
{
    char *s, *s2;

    if (!fn_range_check("SECUREX", nfargs, 1, 2, buff, bufcx))
       return;
    s = fargs[0];
    if ( nfargs > 1 )
       s2 = fargs[1];
    else
       s2 = NULL;
    while (*s) {
       switch (*s) {
            case '%':
            case '$':
            case '^':   /* Added 6/97 Thorin */
            case '\\':
            case '[':
            case ']':
            case '(':
            case ')':
            case '{':
            case '}':
            case ',':
            case ';':
                 if ( !(s2 && strchr(s2, *s)) ) {
                   safe_chr(' ', buff, bufcx);
                   break;
                 }
            default:
                safe_chr(*s, buff, bufcx);
       }
       s++;
    }
}

FUNCTION(fun_escape)
{
    char *s, *d, *s2;

    if (!fn_range_check("ESCAPEX", nfargs, 1, 2, buff, bufcx))
       return;
    s = fargs[0];
    if ( nfargs > 1 )
       s2 = fargs[1];
    else
       s2 = NULL;

    d=*bufcx;
    while (*s) {
       switch (*s) {
          case '%':
          case '^':   /* Added 6/97 Thorin */
          case '\\':
          case '[':
          case ']':
          case '{':
          case '}':
          case '(':   /* Added 7/00 Ash */
          case ')':   /* Added 7/00 Ash */
          case ';':
          case ',':   /* Added 7/00 Ash */
              if ( !(s2 && strchr(s2, *s)) )
                 safe_chr('\\', buff, bufcx);
          default:
              if ( (d == *bufcx) && !(s2 && strchr(s2, 'f')) ) {
                 safe_chr('\\', buff, bufcx);
              }
              safe_chr(*s, buff, bufcx);
       }
       s++;
    }
}

/* str and nostr which are based off of the C strstr */
FUNCTION(fun_str)
{
    char *pt1;

    pt1 = strstr(fargs[0], fargs[1]);
    if (pt1)
       safe_str(pt1, buff, bufcx);
    else
       safe_str("#-1", buff, bufcx);
}

FUNCTION(fun_nostr)
{
    char *pt1, *pt2;

    pt1 = strstr(fargs[0], fargs[1]);
    if (pt1) {
       pt2 = fargs[0];
       while (pt2 < pt1) {
          safe_chr(*pt2, buff, bufcx);
          pt2++;
       }
    } else
       safe_str("#-1", buff, bufcx);
}

/* Take a character position and return which word that char is in.
 * wordpos(<string>, <charpos>)
 */
FUNCTION(fun_wordpos)
{
    int charpos, i;
    char *cp, *tp, *xp, sep;

    varargs_preamble("WORDPOS", 3);

    charpos = atoi(fargs[1]);
    cp = fargs[0];
    if ((charpos > 0) && (charpos <= strlen(cp))) {
       tp = &(cp[charpos - 1]);
       cp = trim_space_sep(cp, sep);
       xp = split_token(&cp, sep);
       for (i = 1; xp; i++) {
           if (tp < (xp + strlen(xp)))
              break;
           xp = split_token(&cp, sep);
       }
       ival(buff, bufcx, i);
       return;
    }
    safe_str("#-1", buff, bufcx);
    return;
}

FUNCTION(fun_type)
{
    dbref it;

    it = match_thing(player, fargs[0]);
    if (!Good_obj(it)) {
       safe_str("#-1 NOT FOUND", buff, bufcx);
       return;
    }
    if ( (Cloak(it) && !Wizard(player)) || (Cloak(it) && SCloak(it) && !Immortal(player)) ) {
       safe_str("#-1 NOT FOUND", buff, bufcx);
       return;
    }
    switch (Typeof(it)) {
         case TYPE_ROOM:
            safe_str("ROOM", buff, bufcx);
            break;
         case TYPE_EXIT:
            safe_str("EXIT", buff, bufcx);
            break;
         case TYPE_PLAYER:
            safe_str("PLAYER", buff, bufcx);
            break;
         case TYPE_THING:
            safe_str("THING", buff, bufcx);
            break;
         default:
            safe_str("#-1 ILLEGAL TYPE", buff, bufcx);
    }
    return;
}

FUNCTION(fun_hasflag)
{
    dbref it;
    OBLOCKMASTER master;

    olist_init(&master);
    if (parse_attrib_wild(player, fargs[0], &it, 0, 1, 0, &master, 0, 0, 0)) {
       if ((it != NOTHING) && (it != AMBIGUOUS) && (!Cloak(it) || (Cloak(it) && (Examinable(player, it) || Wizard(player)))) &&
            (!(SCloak(it) && Cloak(it)) || (SCloak(it) && Cloak(it) && Immortal(player))) &&
            (mudconf.pub_flags || Examinable(player, it) || (it == cause)))  {
          if (has_aflag(player, it, olist_first(&master), fargs[1]))
              safe_str("1",buff,bufcx);
          else
              safe_str("0",buff,bufcx);
       }
       olist_cleanup(&master);
       return;
    }
    olist_cleanup(&master);

    it = match_thing(player, fargs[0]);
    if (!Good_obj(it)) {
       safe_str("#-1 NOT FOUND", buff, bufcx);
       return;
    }
    if ((!Cloak(it) || (Cloak(it) && (Examinable(player, it) || Wizard(player)))) &&
        (!(SCloak(it) && Cloak(it)) || (SCloak(it) && Cloak(it) && Immortal(player))) &&
        (mudconf.pub_flags || Examinable(player, it) || (it == cause)))  {
       if (has_flag(player, it, fargs[1]))
          safe_str("1", buff, bufcx);
       else
          safe_str("0", buff, bufcx);
    } else {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
    }
}

FUNCTION(fun_xorflag)
{
  dbref it;
  int x, track;

  if (nfargs < 2) {
    safe_str("#-1 FUNCTION (XORFLAG) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
    ival(buff, bufcx, nfargs);
    safe_chr(']', buff, bufcx);
  } else {
    it = match_thing(player, fargs[0]);
    if (!Good_obj(it)) {
       safe_str("#-1 NOT FOUND", buff, bufcx);
       return;
    }
    if ((mudconf.pub_flags || Examinable(player, it) || (it == cause)) && (!Cloak(it) || Examinable(player, it))) {
       track = 0;
       for (x = 0; x < nfargs-1; x++) {
         if (track && has_flag(player, it, fargs[1+x])) {
           track = 0;
           break;
         } else
           track = track || has_flag(player, it, fargs[1+x]);
       }
       if (track)
           safe_str("1", buff, bufcx);
       else
           safe_str("0", buff, bufcx);
    } else {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
    }
  }
}

FUNCTION(fun_orflag)
{
  dbref it;
  int x, track;

  if (nfargs < 2) {
    safe_str("#-1 FUNCTION (ORFLAG) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
    ival(buff, bufcx, nfargs);
    safe_chr(']', buff, bufcx);
  } else {
    it = match_thing(player, fargs[0]);
    if (!Good_obj(it)) {
       safe_str("#-1 NOT FOUND", buff, bufcx);
       return;
    }
    if ((mudconf.pub_flags || Examinable(player, it) || (it == cause)) && (!Cloak(it) || Examinable(player, it))) {
       track = 0;
       for (x = 0; x < nfargs-1; x++)
         track = track || has_flag(player, it, fargs[1+x]);
       if (track)
           safe_str("1", buff, bufcx);
       else
           safe_str("0", buff, bufcx);
    } else {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
    }
  }
}

FUNCTION(fun_andflag)
{
  dbref it;
  int x, track;

  if (nfargs < 2)  {
    safe_str("#-1 FUNCTION (ANDFLAG) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
    ival(buff, bufcx, nfargs);
    safe_chr(']', buff, bufcx);
  } else {
    it = match_thing(player, fargs[0]);
    if (!Good_obj(it)) {
       safe_str("#-1 NOT FOUND", buff, bufcx);
       return;
    }
    if ((mudconf.pub_flags || Examinable(player, it) || (it == cause)) && (!Cloak(it) || Examinable(player, it))) {
       track = 1;
       for (x = 0; x < nfargs-1; x++) {
         if (*fargs[1+x] == '!')
           track = track && !has_flag(player, it, fargs[1+x]+1);
         else
           track = track && has_flag(player, it, fargs[1+x]);
       }
       if (track)
           safe_str("1", buff, bufcx);
       else
           safe_str("0", buff, bufcx);
    } else {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
    }
  }
}

FUNCTION(fun_delete)
{
    char *s;
    int i, start, nchars, len;

    s = fargs[0];
    start = atoi(fargs[1]);
    nchars = atoi(fargs[2]);
    len = strlen(s);
    if ((start >= len) || (nchars <= 0) || (start < 0)) {
       safe_str(s, buff, bufcx);
       return;
    }
    if (nchars > LBUF_SIZE)
      nchars = LBUF_SIZE;
    for (i = 0; i < start; i++) {
       if (safe_chr(*s++, buff, bufcx))
         break;
    }
    if ((i + nchars) < len) {
       s += nchars;
       while (*s)
            safe_chr(*s++, buff, bufcx);
    }
}

FUNCTION(fun_lflags)
{
    dbref target;
    char *pt1;
    OBLOCKMASTER master;

    olist_init(&master);
    if ( parse_attrib_wild(player, fargs[0], &target, 0, 1, 0, &master, 0, 0, 0) ) {
       if ( (target != NOTHING) && (target != AMBIGUOUS) && (!Cloak(target) || (Cloak(target) &&
            (Examinable(player, target) || Wizard(player)))) &&
            (!(SCloak(target) && Cloak(target)) || (SCloak(target) && Cloak(target) && Immortal(player))) &&
            (mudconf.pub_flags || Examinable(player, target) || (target == cause)) )
          parse_aflags(player, target, olist_first(&master), buff, bufcx, 1);
          olist_cleanup(&master);
          return;
    }
    olist_cleanup(&master);

    init_match(player, fargs[0], NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    target = noisy_match_result();
    if ( (target != NOTHING) && (target != AMBIGUOUS) && (!Cloak(target) || (Cloak(target) &&
         (Examinable(player, target) || Wizard(player)))) &&
         (!(SCloak(target) && Cloak(target)) || (SCloak(target) && Cloak(target) && Immortal(player))) &&
         (mudconf.pub_flags || Examinable(player, target) || (target == cause)) ) {
       pt1 = flag_description(player, target, 0, (int *)NULL, 0);
       safe_str(pt1, buff, bufcx);
       free_lbuf(pt1);
    } else {
       safe_str("#-1", buff, bufcx);
    }
}

FUNCTION(fun_ltoggles)
{
    dbref target;
    char *pt1;

    target = match_thing(player, fargs[0]);
    if (!Good_obj(target) || (!Controls(player, target))) {
       safe_str("#-1", buff, bufcx);
    } else {
       pt1 = toggle_description(player, target, 0, 0, (int *)NULL);
       safe_str(pt1, buff, bufcx);
       free_lbuf(pt1);
    }
}

FUNCTION(fun_lpowers)
{
    dbref target;
    char *pt1;

    target = match_thing(player, fargs[0]);
    if (!Good_obj(target) || (!Controls(player, target))) {
       safe_str("#-1", buff, bufcx);
    } else {
       pt1 = power_description(player, target, 0, 0);
       safe_str(pt1, buff, bufcx);
       free_lbuf(pt1);
    }
}

FUNCTION(fun_ldepowers)
{
    dbref target;
    char *pt1;

    target = match_thing(player, fargs[0]);
    if (!Good_obj(target) || (!Controls(player, target))) {
       safe_str("#-1", buff, bufcx);
    } else {
       pt1 = depower_description(player, target, 0, 0);
       safe_str(pt1, buff, bufcx);
       free_lbuf(pt1);
    }
}

FUNCTION(fun_error)
{
    dbref target;

    if (!fn_range_check("ERROR", nfargs, 0, 1, buff, bufcx))
       return;

    target = NOTHING;

    if ( fargs[0] && *fargs[0] )
       target = match_thing(player, fargs[0]);

    if (!Good_obj(target) || (!Controls(player, target))) 
       target = player;

    safe_str(errmsg(target), buff, bufcx);
}

FUNCTION(fun_size)
{
    dbref target;
    int i_type;

    if (!fn_range_check("SIZE", nfargs, 1, 2, buff, bufcx)) {
       return;
    } else if (nfargs == 2 )
       i_type=atoi(fargs[1]);
    else
       i_type = 0;
    if ( i_type < 0 || i_type > 3 )
       i_type = 0;
    if ( i_type > 2 )
       target = match_thing(player, fargs[0]);
    else
       target = lookup_player(player, fargs[0], 0);
    if ((target == NOTHING) || (target == AMBIGUOUS) || !Controls(player, target)) {
       safe_str("#-1", buff, bufcx);
    } else {
        ival(buff, bufcx, count_player(target, i_type));
    }
}

FUNCTION(fun_mailsize)
{
    dbref target;

    if ( mudstate.mail_state != 1 ) {
      safe_str("#-1 MAIL SYSTEM IS CURRENTLY OFF", buff, bufcx);
      return;
    }
    target = lookup_player(player, fargs[0], 0);
    if ((target == NOTHING) || !Controls(player, target)) {
       safe_str("#-1", buff, bufcx);
    } else {
       ival(buff, bufcx, mcount_size(target, fargs[1]));
    }
}

FUNCTION(fun_msizetot)
{
    if ( mudstate.mail_state != 1 ) {
      safe_str("#-1 MAIL SYSTEM IS CURRENTLY OFF", buff, bufcx);
      return;
    }
    ival(buff,bufcx, mcount_size_tot());
}

FUNCTION(fun_lock)
{
    dbref it, aowner;
    int aflags;
    char *tbuf, *t_lbuf;
    ATTR *attr;
    struct boolexp *bool;
#ifdef USE_SIDEEFFECT
    CMDENT *cmdp;
    char   *str;
    int     anum;
#endif

    /* Parse the argument into obj + lock */

#ifdef USE_SIDEEFFECT
    if (!fn_range_check("LOCK", nfargs, 1, 3, buff, bufcx))
        return;
    if ( nfargs > 1 ) {
        mudstate.sidefx_currcalls++;
        if ( !(mudconf.sideeffects & SIDE_LOCK) ) {
           notify(player, "#-1 SIDE-EFFECT PORTION OF LOCK DISABLED");
           return;
        }
        if ( mudstate.inside_locks ) {
           notify(player, "#-1 LOCK ATTEMPTED INSIDE LOCK");
           return;
        }
        if ( !SideFX(player) || Fubar(player) || Slave(player) || return_bit(player) < mudconf.restrict_sidefx ) {
           notify(player, "Permission denied.");
           return;
        }
        cmdp = (CMDENT *)hashfind((char *)"@lock", &mudstate.command_htab);
        if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@lock") ||
              cmdtest(Owner(player), "@lock") || zonecmdtest(player, "@lock") ) {
           notify(player, "Permission denied.");
           return;
        }
        if (parse_thing_slash(player, fargs[0], &str, &it)) {
           anum = search_nametab(player, lock_sw, str);
        } else if ( str == NULL && !Good_obj(it) ) {
           anum = A_LOCK;
           it = lookup_player(player, fargs[0], 0);
        } else {
           notify(player, "Unrecognized switch.");
           return;
        }
        if ( anum < 0 ) {
           notify(player, "Unrecognized switch.");
           return;
        }
        if ( *fargs[1] ) {
           if ( (nfargs > 2) && *fargs[2] ) {
              t_lbuf=alloc_lbuf("fun_lock");
              sprintf(t_lbuf, "%.1000s/%.1000s", fargs[0], fargs[2]);
              do_lock(player, cause, anum, t_lbuf, fargs[1]);
              free_lbuf(t_lbuf);
           } else {
              do_lock(player, cause, anum, fargs[0], fargs[1]);
           }
        } else {
           if ( (nfargs > 2) && *fargs[2] ) {
              t_lbuf=alloc_lbuf("fun_lock");
              sprintf(t_lbuf, "%.1000s/%.1000s", fargs[0], fargs[2]);
              do_unlock(player, cause, anum, t_lbuf);
              free_lbuf(t_lbuf);
           } else {
              do_unlock(player, cause, anum, fargs[0]);
           }
        }
    } else {
       if (!get_obj_and_lock(player, fargs[0], &it, &attr, buff, bufcx))
           return;

       /* Get the attribute and decode it if we can read it */

       tbuf = atr_get(it, attr->number, &aowner, &aflags);
       if (!Read_attr(player, it, attr, aowner, aflags, 0)) {
           free_lbuf(tbuf);
       } else {
           bool = parse_boolexp(player, tbuf, 1);
           free_lbuf(tbuf);
           tbuf = (char *) unparse_boolexp_function(player, bool);
           free_boolexp(bool);
           safe_str(tbuf, buff, bufcx);
       }
    }
#else
    if (!get_obj_and_lock(player, fargs[0], &it, &attr, buff, bufcx))
        return;

    /* Get the attribute and decode it if we can read it */

    tbuf = atr_get(it, attr->number, &aowner, &aflags);
    if (!Read_attr(player, it, attr, aowner, aflags, 0)) {
        free_lbuf(tbuf);
    } else {
        bool = parse_boolexp(player, tbuf, 1);
        free_lbuf(tbuf);
        tbuf = (char *) unparse_boolexp_function(player, bool);
        free_boolexp(bool);
        safe_str(tbuf, buff, bufcx);
    }
#endif
}

FUNCTION(fun_elock)
{
    dbref it, victim, aowner, i_locktype;
    int aflags;
    char *tbuf;
    ATTR *attr;
    struct boolexp *bool;

    /* Parse lock supplier into obj + lock */

    if (!fn_range_check("ELOCK", nfargs, 2, 3, buff, bufcx))
        return;

    if (!get_obj_and_lock(player, fargs[0], &it, &attr, buff, bufcx))
        return;

    if ((Recover(it) || Going(it)) && !(Immortal(player))) {
        notify(player, "I don't see that here.");
        safe_str("#-1 NOT FOUND", buff, bufcx);
        return;
    }
    /* Get the victim and ensure we can do it */
    i_locktype = 0;
    if ( (nfargs > 2) && *fargs[2] ) {
       i_locktype = atoi(fargs[2]);
       if ( (i_locktype < 0) || (i_locktype > 2) )
          i_locktype = 0;
    }

    victim = match_thing(player, fargs[1]);
    if (!Good_obj(victim)) {
        safe_str("#-1 NOT FOUND", buff, bufcx);
    } else if (!nearby_or_control(player, victim) &&
               !nearby_or_control(player, it)) {
        safe_str("#-1 TOO FAR AWAY", buff, bufcx);
    } else {
        tbuf = atr_get(it, attr->number, &aowner, &aflags);
        if ((attr->number == A_LOCK) ||
            Read_attr(player, it, attr, aowner, aflags, 0)) {
            bool = parse_boolexp(player, tbuf, 1);
            ival(buff, bufcx, eval_boolexp(victim, it, it, bool, i_locktype));
            free_boolexp(bool);
        } else {
            safe_str("0", buff, bufcx);
        }
        free_lbuf(tbuf);
    }
}

/* ---------------------------------------------------------------------------
 * fun_lwho: Return list of connected users.
 */

FUNCTION(fun_lwho)
{
    int i_type;
    dbref victim;

    if (!fn_range_check("LWHO", nfargs, 0, 2, buff, bufcx))
       return;
    if ( nfargs >= 1 ) {
      i_type = atoi(fargs[0]);
    } else {
      i_type = 0;
    }

    if ( (nfargs == 2) && Wizard(player) ) {
      victim = match_thing_quiet(player, fargs[1]);
    } else {
      victim = NOTHING;
    }

    make_ulist(player, buff, bufcx, i_type, victim);
}

/* ---------------------------------------------------------------------------
 * fun_nearby: Return whether or not obj1 is near obj2.
 */

FUNCTION(fun_nearby)
{
    dbref obj1, obj2;

    obj1 = match_thing(player, fargs[0]);
    obj2 = match_thing(player, fargs[1]);
    if (!(nearby_or_control(player, obj1) ||
          nearby_or_control(player, obj2)))
        safe_str("0", buff, bufcx);
    else if (nearby(obj1, obj2)) {
      if ((Cloak(obj2) && !Controls(obj1,obj2)) || (Cloak(obj1) && !(Controls(obj2,obj1))))
        if ((!SCloak(obj2) && Wizard(player)) || (!SCloak(obj1) && Wizard(player)))
           safe_str("1", buff, bufcx);
        else if ( Immortal(player) )
           safe_str("1", buff, bufcx);
        else
           safe_str("0", buff, bufcx);
      else
        safe_str("1", buff, bufcx);
    }
    else
        safe_str("0", buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * fun_obj, fun_poss, and fun_subj: perform pronoun sub for object.
 */

static void
process_sex(dbref player, char *what, const char *token,
            char *buff, char **bufcx)
{
    dbref it;
    char *tbuff;

    it = match_thing(player, what);
    if (!Good_obj(it) ||
        (!isPlayer(it) && !nearby_or_control(player, it))) {
       safe_str("#-1 NO MATCH", buff, bufcx);
    } else {
       tbuff = exec(it, it, it, 0, (char *) token, (char **) NULL, 0);
       safe_str(tbuff, buff, bufcx);
       free_lbuf(tbuff);
    }
}

FUNCTION(fun_obj)
{
    mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_O;
    process_sex(player, fargs[0], "%o", buff, bufcx);
    mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_O;
}

FUNCTION(fun_poss)
{
    mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_P;
    process_sex(player, fargs[0], "%p", buff, bufcx);
    mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_P;
}

FUNCTION(fun_subj)
{
    mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_S;
    process_sex(player, fargs[0], "%s", buff, bufcx);
    mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_S;
}

FUNCTION(fun_aposs)
{
    mudstate.sub_overridestate = mudstate.sub_overridestate | SUB_A;
    process_sex(player, fargs[0], "%a", buff, bufcx);
    mudstate.sub_overridestate = mudstate.sub_overridestate & ~SUB_A;
}

/* ---------------------------------------------------------------------------
 * fun_mudname: Return the name of the mud.
 */

FUNCTION(fun_mudname)
{
    safe_str(mudconf.mud_name, buff, bufcx);
    return;
}

/* ---------------------------------------------------------------------------
 * fun_lcstr, fun_ucstr, fun_capstr: Lowercase, uppercase, or capitalize str.
 */

FUNCTION(fun_lcstr)
{
    char *ap;

    ap = fargs[0];
    while (*ap) {
#ifdef ZENTY_ANSI
        if ( (*ap == '%') && ((*(ap+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                          || (*(ap+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                          || (*(ap+1) == SAFE_CHR3)
#endif
)) {
           if ( isAnsi[(int) *(ap+2)] ) {
              safe_chr(*ap, buff, bufcx);
              safe_chr(*(ap+1), buff, bufcx);
              safe_chr(*(ap+2), buff, bufcx);
              ap+=3;
              continue;
           }
           if ( (*(ap+2) == '0') && ((*(ap+3) == 'x') || (*(ap+3) == 'X')) &&
                *(ap+4) && *(ap+5) && isxdigit(*(ap+4)) && isxdigit(*(ap+5)) ) {
              safe_chr(*ap, buff, bufcx);
              safe_chr(*(ap+1), buff, bufcx);
              safe_chr(*(ap+2), buff, bufcx);
              safe_chr(*(ap+3), buff, bufcx);
              safe_chr(*(ap+4), buff, bufcx);
              safe_chr(*(ap+5), buff, bufcx);
              ap+=6;
              continue;
           }
        }
        if ( (*ap == '%') && (*(ap+1) == 'f') ) {
           if ( isprint(*(ap+2)) ) {
              safe_chr(*ap, buff, bufcx);
              safe_chr(*(ap+1), buff, bufcx);
              safe_chr(*(ap+2), buff, bufcx);
              ap+=3;
              continue;
           }
        }
#endif
       safe_chr(ToLower((int)*ap), buff, bufcx);
       ap++;
    }
}

FUNCTION(fun_ucstr)
{
    char *ap;

    ap = fargs[0];
    while (*ap) {
#ifdef ZENTY_ANSI
        if ( (*ap == '%') && ((*(ap+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                          || (*(ap+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                          || (*(ap+1) == SAFE_CHR3)
#endif
)) {
           if ( isAnsi[(int) *(ap+2)] ) {
              safe_chr(*ap, buff, bufcx);
              safe_chr(*(ap+1), buff, bufcx);
              safe_chr(*(ap+2), buff, bufcx);
              ap+=3;
              continue;
           }
           if ( (*(ap+2) == '0') && ((*(ap+3) == 'x') || (*(ap+3) == 'X')) &&
                *(ap+4) && *(ap+5) && isxdigit(*(ap+4)) && isxdigit(*(ap+5)) ) {
              safe_chr(*ap, buff, bufcx);
              safe_chr(*(ap+1), buff, bufcx);
              safe_chr(*(ap+2), buff, bufcx);
              safe_chr(*(ap+3), buff, bufcx);
              safe_chr(*(ap+4), buff, bufcx);
              safe_chr(*(ap+5), buff, bufcx);
              ap+=6;
              continue;
           }
        }
        if ( (*ap == '%') && (*(ap+1) == 'f') ) {
           if ( isprint(*(ap+2)) ) {
              safe_chr(*ap, buff, bufcx);
              safe_chr(*(ap+1), buff, bufcx);
              safe_chr(*(ap+2), buff, bufcx);
              ap+=3;
              continue;
           }
        }
#endif
       safe_chr(ToUpper((int)*ap), buff, bufcx);
       ap++;
    }
}

FUNCTION(fun_capstr)
{
   char *ap = fargs[0];

   if( *fargs[0]  ) {
#ifdef ZENTY_ANSI
       while ( *ap ) {
          if ( (*ap == '%') && ((*(ap+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                            || (*(ap+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                            || (*(ap+1) == SAFE_CHR3)
#endif
)) {
             if ( isAnsi[(int) *(ap+2)] ) {
                safe_chr(*ap, buff, bufcx);
                safe_chr(*(ap+1), buff, bufcx);
                safe_chr(*(ap+2), buff, bufcx);
                ap+=3;
                continue;
             }
             if ( (*(ap+2) == '0') && ((*(ap+3) == 'x') || (*(ap+3) == 'X')) &&
                  *(ap+4) && *(ap+5) && isxdigit(*(ap+4)) && isxdigit(*(ap+5)) ) {
                safe_chr(*ap, buff, bufcx);
                safe_chr(*(ap+1), buff, bufcx);
                safe_chr(*(ap+2), buff, bufcx);
                safe_chr(*(ap+3), buff, bufcx);
                safe_chr(*(ap+4), buff, bufcx);
                safe_chr(*(ap+5), buff, bufcx);
                ap+=6;
                continue;
             }
          }
          if ( (*ap == '%') && (*(ap+1) == 'f') ) {
             if ( isprint(*(ap+2)) ) {
                safe_chr(*ap, buff, bufcx);
                safe_chr(*(ap+1), buff, bufcx);
                safe_chr(*(ap+2), buff, bufcx);
                ap+=3;
                continue;
             }
          }
          break;
      }
#endif
      if ( *ap ) {
         safe_chr( ToUpper((int)*ap), buff, bufcx);
         safe_str( ap + 1, buff, bufcx);
      }
    }
}

/**
 * Performs list-based capitalization.
 *
 * @author Loki
 * @since 3.9.1p1
 * @var char *curr	The current word being affected.
 * @var char *osep	The output separator.
 * @var char *sep	The input separator/delimiter.
 * @var char style	The style of capitalization to perform.
 * @var char *wordlist	The list of words to be affected.
 * @var int bFirst	Boolean determiner of first word in list.
 * @var int len		The length of the current word.
 * @var int pos		The position in the current word.
 **/
FUNCTION(fun_caplist)
{
    char *curr, *osep, *sep, style, *wordlist, *t_str, *s_tok, *s_tokr, *ap;
    int bFirst, i_found, i_last, i_first, i_lastchk, i_cap;
    
    // We aren't allowed more than 4 args here.
    if( !fn_range_check( "CAPLIST", nfargs, 1, 4, buff, bufcx ) )
      return;

    // If there's no list, then just return nothing.
    if( !fargs[0] )
      return;

    // Set up our arguments.
    sep = ( fargs[1] && *fargs[1] ) ? fargs[1] : (char *)" ";
    osep = ( fargs[2] && *fargs[2] ) ? fargs[2] : sep;
    style = ( fargs[3] && *fargs[3] ) ? ToUpper(*fargs[3]) : 'N';
    wordlist = trim_space_sep( fargs[0], *sep );
    i_last = countwords( fargs[0], *sep );
    i_first = i_found = i_cap = 0;

    if ( !*wordlist ) {
       return;
    }
    
    t_str = alloc_lbuf("caplist");

    bFirst = i_lastchk = 1;
    do {
       // Set the current word.
       curr = split_token( &wordlist, *sep );

       // If we're not on the first word, put our osep in.
       if( !bFirst )
          safe_chr( *osep, buff, bufcx );
       bFirst = 0;

       if ( !*curr )
          continue;

       switch( style ) {
          // Capitalize the first letter of every word
  	  // Lowercase the rest of each word.
          case 'L': /* Enforce Lower Case */
             i_cap = 0;
             ap = curr;
             while ( *ap ) {
#ifdef ZENTY_ANSI
                if ( (*ap == '%') && ((*(ap+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                                   || (*(ap+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                                   || (*(ap+1) == SAFE_CHR3)
#endif
)) {
                   if ( isAnsi[(int) *(ap+2)] ) {
                      safe_chr(*ap, buff, bufcx);
                      safe_chr(*(ap+1), buff, bufcx);
                      safe_chr(*(ap+2), buff, bufcx);
                      ap+=3;
                      continue;
                   }
                   if ( (*(ap+2) == '0') && ((*(ap+3) == 'x') || (*(ap+3) == 'X')) &&
                        *(ap+4) && *(ap+5) && isxdigit(*(ap+4)) && isxdigit(*(ap+5)) ) {
                      safe_chr(*ap, buff, bufcx);
                      safe_chr(*(ap+1), buff, bufcx);
                      safe_chr(*(ap+2), buff, bufcx);
                      safe_chr(*(ap+3), buff, bufcx);
                      safe_chr(*(ap+4), buff, bufcx);
                      safe_chr(*(ap+5), buff, bufcx);
                      ap+=6;
                      continue;
                   }
                }
                if ( (*ap == '%') && (*(ap+1) == 'f') ) {
                   if ( isprint(*(ap+2)) ) {
                      safe_chr(*ap, buff, bufcx);
                      safe_chr(*(ap+1), buff, bufcx);
                      safe_chr(*(ap+2), buff, bufcx);
                      ap+=3;
                      continue;
                   }
                }
#endif
                if ( !i_cap ) {
  	           safe_chr( ToUpper( *ap ), buff, bufcx );
                   i_cap = 1;
                } else {
  	           safe_chr( ToLower( *ap ), buff, bufcx );
                }
                ap++;
	    }
	    break;

        // True capitilization -- Title NIVA standards
        case 'T': /* True capitalization */
           i_found = 0;
           if ( i_first && (i_last != i_lastchk) && *(mudconf.cap_conjunctions) ) {
              memcpy(t_str, mudconf.cap_conjunctions, LBUF_SIZE-2);
              s_tok = strtok_r(t_str, " ", &s_tokr);
              while ( s_tok ) {
                 if ( stricmp(s_tok, strip_all_special(curr)) == 0 ) {
                    i_found = 1;
                    break;
                 }
                 s_tok = strtok_r(NULL, " ", &s_tokr);
              }
           }
           if ( i_first && (i_last != i_lastchk) && !i_found && *(mudconf.cap_articles) ) {
              memcpy(t_str, mudconf.cap_articles, LBUF_SIZE-2);
              s_tok = strtok_r(t_str, " ", &s_tokr);
              while ( s_tok ) {
                 if ( stricmp(s_tok, strip_all_special(curr)) == 0 ) {
                    i_found = 1;
                    break;
                 }
                 s_tok = strtok_r(NULL, " ", &s_tokr);
              }
           }
           if ( i_first && (i_last != i_lastchk) && !i_found && *(mudconf.cap_preposition) ) {
              memcpy(t_str, mudconf.cap_preposition, LBUF_SIZE-2);
              s_tok = strtok_r(t_str, " ", &s_tokr);
              while ( s_tok ) {
                 if ( stricmp(s_tok, strip_all_special(curr)) == 0 ) {
                    i_found = 1;
                    break;
                 }
                 s_tok = strtok_r(NULL, " ", &s_tokr);
              }
          }
          i_cap = 0;
          ap = curr;
	  while( *ap ) {
#ifdef ZENTY_ANSI
             if ( (*ap == '%') && ((*(ap+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                               || (*(ap+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                               || (*(ap+1) == SAFE_CHR3)
#endif
)) {
                if ( isAnsi[(int) *(ap+2)] ) {
                   safe_chr(*ap, buff, bufcx);
                   safe_chr(*(ap+1), buff, bufcx);
                   safe_chr(*(ap+2), buff, bufcx);
                   ap+=3;
                   continue;
                }
                if ( (*(ap+2) == '0') && ((*(ap+3) == 'x') || (*(ap+3) == 'X')) &&
                      *(ap+4) && *(ap+5) && isxdigit(*(ap+4)) && isxdigit(*(ap+5)) ) {
                   safe_chr(*ap, buff, bufcx);
                   safe_chr(*(ap+1), buff, bufcx);
                   safe_chr(*(ap+2), buff, bufcx);
                   safe_chr(*(ap+3), buff, bufcx);
                   safe_chr(*(ap+4), buff, bufcx);
                   safe_chr(*(ap+5), buff, bufcx);
                   ap+=6;
                   continue;
                }
             }
             if ( (*ap == '%') && (*(ap+1) == 'f') ) {
                if ( isprint(*(ap+2)) ) {
                   safe_chr(*ap, buff, bufcx);
                   safe_chr(*(ap+1), buff, bufcx);
                   safe_chr(*(ap+2), buff, bufcx);
                   ap+=3;
                   continue;
                }
             }
#endif
             if ( !i_cap ) {
                if ( !i_found || !i_first || (i_lastchk == i_last) ) {
                   safe_chr( ToUpper( *ap ), buff, bufcx );
                   i_first = 1;
                } else {
                   safe_chr( ToLower( *ap ), buff, bufcx );
                   i_first = 1;
                }
                i_cap = 1;
             } else {
	        if( *ap ) {
	           safe_chr( ToLower( *ap ), buff, bufcx );
                }
             }
             ap++;
	  }
          i_lastchk++;
          break;

	// Capitalize the first letter of every word
	// Leave the rest of each word as natural.
        case 'N': /* Normalized caplist */
        default:
           ap = curr;
           i_cap = 0;
           while ( *ap ) {
#ifdef ZENTY_ANSI
              if ( (*ap == '%') && ((*(ap+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                               || (*(ap+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                               || (*(ap+1) == SAFE_CHR3)
#endif
)) {
                 if ( isAnsi[(int) *(ap+2)] ) {
                    safe_chr(*ap, buff, bufcx);
                    safe_chr(*(ap+1), buff, bufcx);
                    safe_chr(*(ap+2), buff, bufcx);
                    ap+=3;
                    continue;
                 }
                 if ( (*(ap+2) == '0') && ((*(ap+3) == 'x') || (*(ap+3) == 'X')) &&
                       *(ap+4) && *(ap+5) && isxdigit(*(ap+4)) && isxdigit(*(ap+5)) ) {
                    safe_chr(*ap, buff, bufcx);
                    safe_chr(*(ap+1), buff, bufcx);
                    safe_chr(*(ap+2), buff, bufcx);
                    safe_chr(*(ap+3), buff, bufcx);
                    safe_chr(*(ap+4), buff, bufcx);
                    safe_chr(*(ap+5), buff, bufcx);
                    ap+=6;
                    continue;
                 }
              }
              if ( (*ap == '%') && (*(ap+1) == 'f') ) {
                 if ( isprint(*(ap+2)) ) {
                    safe_chr(*ap, buff, bufcx);
                    safe_chr(*(ap+1), buff, bufcx);
                    safe_chr(*(ap+2), buff, bufcx);
                    ap+=3;
                    continue;
                 }
              }
#endif
              if ( !i_cap ) {
                 safe_chr( ToUpper( *ap ), buff, bufcx );
                 i_cap = 1;
              } else {
                 safe_chr( *ap, buff, bufcx );
              }
              ap++;
           }
	   break;
       }
    } while( wordlist );
    free_lbuf(t_str);
    return;
}

/* ---------------------------------------------------------------------------
 * fun_creplace: Replace/Overwrite/Overwrite & Cut, at position.
 */
FUNCTION(fun_creplace)
{
   char *curr, *cp, sep, *sop, *sp, *curr_temp, *sop_temp;
   int  i_val, i_cntr, exit_val, i_range, i_rangecnt;

   if (!fn_range_check("CREPLACE", nfargs, 3, 5, buff, bufcx))
       return;

   curr_temp = exec(player, cause, caller,
                    EV_STRIP | EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
   i_val = atoi(curr_temp);
   free_lbuf(curr_temp);
   if ( i_val < 1 || i_val > (LBUF_SIZE-1) ) {
      curr_temp = alloc_mbuf("creplace");
      sprintf(curr_temp, "#-1 VALUE MUST BE > 0 < %d", LBUF_SIZE);
      safe_str(curr_temp, buff, bufcx);
      free_mbuf(curr_temp);
      return;
   }

   curr_temp = exec(player, cause, caller,
                    EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0], cargs, ncargs);
   sop_temp = exec(player, cause, caller,
                   EV_STRIP | EV_FCHECK | EV_EVAL, fargs[2], cargs, ncargs);
   curr = alloc_lbuf("fun_creplace");
   cp = curr;
   safe_str(strip_ansi(curr_temp),curr,&cp);
   free_lbuf(curr_temp);
   cp = curr;
   sop = alloc_lbuf("fun_creplace");
   sp = sop;
   safe_str(strip_ansi(sop_temp),sop,&sp);
   free_lbuf(sop_temp);
   sp = sop;
   if ( nfargs > 3 ) {
      sep = ToLower((int)*fargs[3]);
   } else {
      sep = 'o';
   }
   i_rangecnt = i_range = 0;
   if ( nfargs > 4 ) {
      i_range = atoi(fargs[4]);
      if ( i_range < 0 ) {
         i_range = 0;
      }
   }
   i_cntr = 1;
   exit_val = 0;
   if ( sep == 'c' ) {    /* Overwrite and Cut */
      while (( *cp || *sp ) && !exit_val ) {
         if ( *sp && *cp && i_cntr >= i_val ) {
            i_rangecnt++;
            safe_chr(*sp, buff, bufcx);
            sp++;
            if ( (i_rangecnt <= i_range) || (i_range == 0) )
               cp++;
         } else if ( *cp ) {
            if ( (i_cntr >= i_val) && (i_rangecnt < i_range) && (i_range != 0) ) {
               i_rangecnt++;
            } else {
               safe_chr(*cp, buff, bufcx);
            }
            cp++;
         } else {
            exit_val=1;
         }
         i_cntr++;
      }
   } else if ( sep == 'i' ) { /* Insert at location */
      while (( *cp || *sp )) {
         if ( *sp && i_cntr >= i_val ) {
            safe_chr(*sp, buff, bufcx);
            sp++;
         } else if ( *cp ) {
            safe_chr(*cp, buff, bufcx);
            cp++;
         }
         i_cntr++;
      }
   } else {     /* Just overwrite */
      while (( *cp || *sp )) {
         if ( *sp && i_cntr >= i_val ) {
            if ( *sp ) {
               i_rangecnt++;
               safe_chr(*sp, buff, bufcx);
               sp++;
            }
            if ( *cp && ((i_rangecnt <= i_range) || (i_range == 0)) )
               cp++;
         } else {
            if ( *cp ) {
               if ( (i_cntr >= i_val) && (i_rangecnt < i_range) && (i_range !=0) ) {
                  i_rangecnt++;
               } else {
                  safe_chr(*cp, buff, bufcx);
               }
               cp++;
            }
         }
         i_cntr++;
      }
   }
   free_lbuf(curr);
   free_lbuf(sop);
}

/* ---------------------------------------------------------------------------
 * fun_lnum: Return a list of numbers.
 */
FUNCTION(fun_lnum)
{
   int ctr, x, y, over, i_step;
   char sep;

   ctr = x = y = over = i_step = 0;
   if ((nfargs < 1) || (nfargs > 4)) {
      safe_str("#-1 INCORRECT NUMBER OF ARGUMENTS", buff, bufcx);
   } else {
      over = 0;
      x = atoi(fargs[0]);
      if ( (nfargs == 1) && (mudconf.lnum_compat == 1) && (x < 1) ) {
         return;
      }
      sep = ' ';
      if (nfargs == 1) {
          y = x-1;
          x = 0;
      }
      else if (nfargs >= 2) {
          if (*fargs[1]);
              y = atoi(fargs[1]);
          if ((nfargs >= 3) && (*fargs[2]))
              sep = *fargs[2];
          if ((nfargs == 4) && (*fargs[3])) {
              i_step = atoi(fargs[3]);
              if ( (i_step >= 2000000000) || (i_step <= 1) )
                 i_step = 1;
              i_step--;
          }
       }
       if ((x >= 0) || (mudconf.lnum_compat == 1) ) {
          ival(buff, bufcx, x);
          if (y >= x) {
              if ( i_step > 0 )
                 x += (i_step + 1);
              else
                 x++;
              for (ctr = x; ctr <= y && !over; ((i_step > 0) ? ctr += (i_step + 1) : ctr++) ) {
                 over = safe_chr(sep, buff, bufcx);
                 ival(buff, bufcx, ctr);
              }
          } else {
              if ( i_step > 0 )
                 x -= (i_step + 1);
              else
                 x--;
              for (ctr = x; (ctr >= y && (ctr >= 0 || mudconf.lnum_compat == 1)) && !over; ((i_step > 0) ? ctr -= (i_step + 1) : ctr--) ) {
                over = safe_chr(sep, buff, bufcx);
                ival(buff, bufcx, ctr);
              }
          }
       }
   }
}

FUNCTION(fun_lnum2)
{
    int ctr, x, y, t, over, i_step;

    ctr = x = y = t = over = i_step = 0;
    if ((nfargs < 1) || (nfargs > 4)) {
      safe_str("#-1 INCORRECT NUMBER OF ARGUMENTS", buff, bufcx);
    } else {
      over = 0;
      x = 0;
      t = atoi(fargs[0]);
      if ( (nfargs == 1) && (mudconf.lnum_compat == 1) && (x < 1) ) {
         return;
      }
      if (nfargs == 1)
         y = x + t - 1;
      else if (nfargs >= 2) {
         if (*fargs[1])
            x = atoi(fargs[1]);
         if ( (nfargs >= 4) && *fargs[3] ) {
            i_step = atoi(fargs[3]);
            if ( (i_step >= 2000000000) || (i_step <= 1) )
               i_step = 1;
            i_step--;
         }
         if ((nfargs >= 3) && *fargs[2]) {
            if (!strcmp(fargs[2],"+")) {
               if ( i_step > 0 )
                  y = x + ((i_step + 1) * t) - 1;
               else
                  y = x + t - 1;
            } else if (!strcmp(fargs[2],"-")) {
               if ( i_step > 0 )
                  y = x - ((i_step + 1) * t) + 1;
               else
                  y = x - t + 1;
            }
         } else
            y = x + t -1;
      }
      if (((x >= 0) && (t >= 0)) || (mudconf.lnum_compat == 1)) {
         ival(buff, bufcx, x);
         if (y >= x) {
            if ( i_step > 0 )
               x += (i_step + 1);
            else
               x++;
            for (ctr = x; ctr <= y && !over; ((i_step > 0) ? ctr += (i_step + 1) : ctr++) ) {
               over = safe_chr(' ', buff, bufcx);
               ival(buff, bufcx, ctr);
            }
         } else {
            if ( i_step > 0 )
               x -= (i_step + 1);
            else
               x--;
            for (ctr = x; (ctr >= y && (ctr >= 0 || mudconf.lnum_compat == 1)) && !over;
                 ((i_step > 0) ? ctr -= (i_step + 1) : ctr--) ) {
               over = safe_chr(' ', buff, bufcx);
               ival(buff, bufcx, ctr);
            }
         }
      }
   }
}


/* ---------------------------------------------------------------------------
 * fun_lattr: Return list of attributes I can see on the object.
 */

void
parse_lattr(char *buff, char **bufcx, dbref player, dbref cause, dbref caller,
            char *fargs[], int nfargs, char *cargs[], int ncargs, char *s_name,
            int i_is_parent, int i_is_cluster)
{
    dbref thing, target, aowner;
    int ca, first, chk_cmd, aflags, i_pageval, i_currcnt, i_dispcnt, i_retpage, 
        i_allattrs, i_ntfnd, i_min, i_max, i_regexp, i_tree;
    char *s_shoveattr, c_lookup, *s_tmpbuff, *s_tmpbuffptr, *s_ptr, *tpr_buff, 
        *tprp_buff, *s_storname[3];
    ATTR *attr;
    OBLOCKMASTER master;

    /* Check for wildcard matching.  parse_attrib_wild checks for read
     * permission, so we don't have to.  Have p_a_w assume the slash-star
     * if it is missing.
     */

    if (!fn_range_check(s_name, nfargs, 1, 6, buff, bufcx))
       return;

    if (nfargs >= 2) {
      if ( *fargs[1] == '*' )
         target = lookup_player(player, fargs[1]+1, 0);
      else
         target = lookup_player(player, fargs[1], 0);
      if (target == NOTHING)
         target = player;
      else if ( !Controls(player,target) )
         target = player;
    } else
      target = player;
    chk_cmd = i_currcnt = i_dispcnt = i_retpage = 0;
    c_lookup = ' ';
    if ((nfargs >= 3) && (*fargs[2])) {
       c_lookup = *fargs[2];
    }
    if ((nfargs > 4) && (*fargs[4])) {
       i_regexp = atoi(fargs[4]);
    } else {
       i_regexp = 0;
    }
    if ((nfargs > 5) && (*fargs[5])) {
       i_tree = atoi(fargs[5]);
       if ( i_tree != 0 )
          i_tree = 1;
    } else {
       i_tree = 0;
    }
    
    if ( (c_lookup == '$') || (c_lookup == '^') ) {
       chk_cmd = 1;
    } else {
       chk_cmd = 0;
    }
    if ( c_lookup == 'l' )
       i_retpage = 1;
    if ( isdigit((int)c_lookup) ) {
       i_pageval = atoi(fargs[2]);
    } else {
       i_pageval = 0;
    }
    i_allattrs = i_ntfnd = 0;
    if ( c_lookup == '+' )
       i_allattrs = 1;
    if ( (c_lookup == '|') || (c_lookup == '&') ) {
       /* And or or with no flags specified */
       if ( (nfargs < 4) || !*fargs[3] )
          return;
       /* internal flags should never be above 32 chars long */
       if ( strlen(fargs[3]) > 32 )
          *(fargs[3]+31) = '\0';
    }
    if ( (c_lookup == '~') && (strchr(fargs[2], '-') != NULL) ) {
       i_min = atoi(fargs[2]+1);
       i_max = i_min + atoi(strchr(fargs[2], '-')+1) - 1;
    } else {
       i_min = i_max = 0;
    }
    first = 1;
    olist_init(&master);
    if (parse_attrib_wild(target, fargs[0], &thing, i_is_parent, i_allattrs, 1, &master, i_is_cluster, i_regexp, i_tree)) {
        if ( (SCloak(thing) && Cloak(thing) && Immortal(thing) && !(Immortal(target))) ||
             (Cloak(thing) && Wizard(thing) && !(Wizard(target))) ||
             (!(Immortal(target)) && (Going(thing) || Recover(thing))) ) {

           if ( !(mudconf.lattr_default_oldstyle) )
              safe_str("#-1 NO MATCH", buff, bufcx);
        } else {
            if ( chk_cmd > 0 )
               s_shoveattr = alloc_lbuf("fun_lattr.cmd");
            s_tmpbuff = alloc_lbuf("lattr_andor");
            if ( (c_lookup == '>') || (c_lookup == '<') ) {
               s_storname[0] = alloc_sbuf("lattr_storname");
               s_storname[1] = alloc_sbuf("lattr_storname");
               s_storname[2] = alloc_sbuf("lattr_storname");
            }
            for (ca = olist_first(&master); ca != NOTHING; ca = olist_next(&master)) {
                attr = atr_num(ca);
                if (attr) {
                    if ( (c_lookup == '>') || (c_lookup == '<') ) {
                       if ( *s_storname[0] ) {
                          memcpy(s_storname[1], attr->name, SBUF_SIZE);
                          for ( s_ptr = s_storname[1]; *s_ptr; s_ptr++ )
                             *s_ptr = ToUpper(*s_ptr);
                          qsort((void *)s_storname, 2, sizeof(char *), s_comp);
                          if ( c_lookup == '>' ) {
                             memcpy(s_storname[0], s_storname[1], SBUF_SIZE);
                             for ( s_ptr = s_storname[0]; *s_ptr; s_ptr++ )
                                *s_ptr = ToUpper(*s_ptr);
                             memcpy(s_storname[2], s_storname[1], SBUF_SIZE);
                          }
                       } else {
                          memcpy(s_storname[0], attr->name, SBUF_SIZE);
                          for ( s_ptr = s_storname[0]; *s_ptr; s_ptr++ )
                             *s_ptr = ToUpper(*s_ptr);
                          memcpy(s_storname[2], attr->name, SBUF_SIZE);
                       }
                       continue;
                    }
                    if ( (c_lookup == '&') || (c_lookup == '|') ) {
                       memset(s_tmpbuff, '\0', LBUF_SIZE);
                       s_tmpbuffptr = s_tmpbuff;
                       parse_aflags(player, thing, ca, s_tmpbuff, &s_tmpbuffptr, 0);
                       s_ptr = fargs[3];
                       if (c_lookup == '&') {
                          i_ntfnd = 0;
                          while ( *s_ptr ) {
                             if ( strchr(s_tmpbuff, *s_ptr) == NULL ) {
                                i_ntfnd = 1;
                                break;
                             }
                             s_ptr++;
                          }
                       } else {
                          i_ntfnd = 1;
                          while ( *s_ptr ) {
                             if ( strchr(s_tmpbuff, *s_ptr) != NULL ) {
                                i_ntfnd = 0;
                                break;
                             }
                             s_ptr++;
                          }
                       }
                       if ( i_ntfnd )
                          continue;
                    }
                    if ( (c_lookup == '+') && !(attr->flags & AF_IS_LOCK) )
                       continue;
                    i_currcnt++;
                    if ( i_retpage )
                       continue;
                    if (i_pageval > 0) {
                       if ( (i_currcnt <= (PAGE_VAL * (i_pageval - 1))) || (i_dispcnt >= PAGE_VAL) )
                          continue;
                       i_dispcnt++;
                    }
                    if ( (i_max != 0) && ((i_currcnt > i_max) || (i_currcnt < i_min)) ) {
                       continue;
                    }
                    if (chk_cmd > 0) {
                       atr_get_str(s_shoveattr, thing, attr->number, &aowner, &aflags);
                       if ( s_shoveattr[0] == c_lookup ) {
                          if (!first)
                              safe_chr(' ', buff, bufcx);
                          first = 0;
                          safe_str((char *) attr->name, buff, bufcx);
                       }
                    } else {
                       if (!first)
                          safe_chr(' ', buff, bufcx);
                       first = 0;
                       safe_str((char *) attr->name, buff, bufcx);
                    }
                 }
            }
            if ( (c_lookup == '>') || (c_lookup == '<') ) {
               safe_str(s_storname[0], buff, bufcx);
            }
            if ( (c_lookup == '>') || (c_lookup == '<') ) {
               free_sbuf(s_storname[0]);
               free_sbuf(s_storname[1]);
               free_sbuf(s_storname[2]);
            }
            free_lbuf(s_tmpbuff);
            if ( chk_cmd > 0 )
               free_lbuf(s_shoveattr);
        }
    } else {
        if ( !(mudconf.lattr_default_oldstyle) )
           safe_str("#-1 NO MATCH", buff, bufcx);
    }
    olist_cleanup(&master);
    if ( i_retpage ) {
       tprp_buff = tpr_buff = alloc_lbuf("fun_lattr");
       safe_str(safe_tprintf(tpr_buff, &tprp_buff, "%d %d", i_currcnt,
                             ((i_currcnt / PAGE_VAL) + 1)), buff, bufcx);
       free_lbuf(tpr_buff);
    }
    return;
}

FUNCTION(fun_lattr)
{
   parse_lattr(buff, bufcx, player, cause, caller, fargs, nfargs,
               cargs, ncargs, (char *)"LATTR", 0, 0);
   return;
}

FUNCTION(fun_lattrp)
{
   parse_lattr(buff, bufcx, player, cause, caller, fargs, nfargs,
               cargs, ncargs, (char *)"LATTRP", 1, 0);
   return;
}

FUNCTION(fun_cluster_lattr)
{
    dbref target;
    char *s_shoveattr, *s_ptr, *s_ptr2;

    /* Check for wildcard matching.  parse_attrib_wild checks for read
     * permission, so we don't have to.  Have p_a_w assume the slash-star
     * if it is missing.
     */

    /* Verify object is a valid cluster */
    if ( nfargs < 5 ) {
       if ( !*fargs[0] ) {
          safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
          return;
       } else {
          if ( strchr(fargs[0], '/') != NULL ) {
             s_ptr2 = s_shoveattr = alloc_lbuf("fun_cluster_lattr.lookup");
             s_ptr = fargs[0];
             while ( *s_ptr && (*s_ptr != '/') ) {
                safe_chr(*s_ptr, s_shoveattr, &s_ptr2);
                s_ptr++;
             }
             target = match_thing(player, s_shoveattr);
             free_lbuf(s_shoveattr);
          } else {
             target = match_thing(player, fargs[0]);
          }
          if ( !Good_chk(target) || !Cluster(target) ) {
             safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
             return;
          }
       }
    }
    parse_lattr(buff, bufcx, player, cause, caller, fargs, nfargs,
                cargs, ncargs, (char *)"CLUSTER_LATTR", 0, 1);
    return;
}

FUNCTION(fun_lcmds)
{
    dbref thing, aowner;
    int ca, first, aflags, found, in_break;
    char *s_shoveattr, c_lookup, sep, *c_ptr;
    ATTR *attr;
    OBLOCKMASTER master;

    /* Check for wildcard matching.  parse_attrib_wild checks for read
     * permission, so we don't have to.  Have p_a_w assume the slash-star
     * if it is missing.
     */

    if (!fn_range_check("LCMDS", nfargs, 1, 3, buff, bufcx))
       return;

    if (nfargs >= 2 && *fargs[1]) {
      sep = *fargs[1];
    } else {
      sep = ' ';
    }
    c_lookup = '\0';
    if ((nfargs == 3) && (*fargs[2])) {
       c_lookup = *fargs[2];
    }
    if ( (c_lookup != '$') && (c_lookup != '^') ) {
       c_lookup = '$';
    }
    first = 1;
    olist_init(&master);
    if (parse_attrib_wild(player, fargs[0], &thing, 0, 0, 1, &master, 0, 0, 0)) {
        if ( (SCloak(thing) && Cloak(thing) && Immortal(thing) && !(Immortal(player))) ||
             (Cloak(thing) && Wizard(thing) && !(Wizard(player))) ||
             (!(Immortal(player)) && (Going(thing) || Recover(thing))) ) {

        if ( !(mudconf.lattr_default_oldstyle) )
           safe_str("#-1 NO MATCH", buff, bufcx);
        } else {
            s_shoveattr = alloc_lbuf("fun_lcmds.cmd");
            for (ca = olist_first(&master); ca != NOTHING; ca = olist_next(&master)) {
                attr = atr_num(ca);
                if (attr) {
                    atr_get_str(s_shoveattr, thing, attr->number, &aowner, &aflags);
                    if ( s_shoveattr[0] == c_lookup ) {
                       found = in_break = 0;
                       c_ptr = s_shoveattr+1;
                       if ( *c_ptr != ':' ) {
                          while ( *c_ptr && !found ) {
                             if ( *c_ptr == '\\' )
                                in_break = !in_break;
                             if ( (*c_ptr == ':') && !in_break ) {
                                *c_ptr = '\0';
                                found = 1;
                             }
                             if ( in_break && (*c_ptr != '\\') )
                                in_break = 0;
                             c_ptr++;
                          }
                       }
                       if (found) {
                          c_ptr = s_shoveattr+1;
                          while (*c_ptr) {
                             *c_ptr = ToLower((int)*c_ptr);
                             c_ptr++;
                          }
                          if (!first)
                             safe_chr(sep, buff, bufcx);
                          first = 0;
                          safe_str(s_shoveattr+1, buff, bufcx);
                       }
                    }
                }
            }
            free_lbuf(s_shoveattr);
        }
    } else {
        if ( !(mudconf.lattr_default_oldstyle) )
           safe_str("#-1 NO MATCH", buff, bufcx);
    }
    olist_cleanup(&master);
    return;
}

/* array(string, regcount, length [[,delim][,type]]) */
FUNCTION(fun_array)
{
   char *s_inptr, *s_outptr, *s_ptr2, *s_input, *s_output, *s_tptr, *s_tmpbuff, sep;
   int i_width, i_regs, i_regcurr, i_type, i, i_kill, i_counter[MAX_GLOBAL_REGS];
   time_t it_now;
   ANSISPLIT insplit[LBUF_SIZE], outsplit[LBUF_SIZE], *p_in, *p_out;

   if (!fn_range_check("ARRAY", nfargs, 3, 5, buff, bufcx))
      return;

   /* insanely dangerous function -- only allow 10 per command */
   it_now = time(NULL);
   if (it_now > (mudstate.now + 5)) {
      mudstate.chkcpu_toggle = 1;
      safe_str("#-1 HEAVY CPU RECURSION LIMIT EXCEEDED", buff, bufcx);
      return;
   }

   i_width = -1;
   i_regs = i_regcurr = i_type = 0;
   memset(i_counter, '\0', sizeof(i_counter));

   if ( *fargs[1] ) {
      i_regs = atoi(fargs[1]);
   }
   if ( (i_regs < 1) || (i_regs > MAX_GLOBAL_REGS) ) {
      safe_str("#-1 ARRAY REQUIRES BETWEEN 1 AND ", buff, bufcx);
      ival(buff, bufcx, MAX_GLOBAL_REGS);
      safe_str(" FOR REGISTER COUNT", buff, bufcx);
      return;
   }
   if ( *fargs[2] ) {
      i_width = atoi(fargs[2]);
   }
   sep = '\0';
   if ( (nfargs > 3) && *fargs[3] ) {
      sep = *fargs[3];
   }
   if ( !sep && ((i_width < 0 ) || (i_width > LBUF_SIZE)) ) {
      s_input = alloc_mbuf("fun_array");
      sprintf(s_input, "#-1 ARRAY REQUIRES WIDTH >= 0 AND WIDTH < %d", LBUF_SIZE);
      safe_str(s_input, buff, bufcx);
      free_mbuf(s_input);
      return;
   }
   if ( (nfargs > 4) && *fargs[4] ) {
      i_type = atoi(fargs[4]);
      if ( i_type != 0 )
         i_type = 1;
   }
   if ( (i_width == 0) && !sep )
      sep = ' ';

   initialize_ansisplitter(insplit, LBUF_SIZE);
   initialize_ansisplitter(outsplit, LBUF_SIZE);

   s_input = alloc_lbuf("fun_array");
   s_output = alloc_lbuf("fun_array2");
   split_ansi(strip_ansi(fargs[0]), s_input, insplit);


   for ( i = 0; i < i_regs; i++ ) {
      if ( !mudstate.global_regs[i] )
         mudstate.global_regs[i] = alloc_lbuf("fun_setq");
      *mudstate.global_regs[i] = '\0';
   }
   s_inptr = s_input;
   p_in = insplit;
   p_out = outsplit;

   /* Over then down */
   if ( !i_type ) {
      i = 0;
      memset(s_output, '\0', LBUF_SIZE);
      s_outptr = s_output;
      i_kill = 0;
      while ( s_inptr && *s_inptr) {
         *s_outptr = *s_inptr;
         clone_ansisplitter(p_out, p_in);
         i_kill++;
         if ( i_kill > LBUF_SIZE ) {
            notify(player, unsafe_tprintf("Artificially aborted: %d / :%s: / %s", i, s_output, s_inptr));
            break;
         }
         s_outptr++;
         p_out++;
         i++;
         if ( *s_inptr && ( (sep && (*s_inptr == sep)) || ((i == i_width) && (i_width != 0)) ) ) {
            i = 0;
            if ( !sep ) {
               s_ptr2 = s_inptr;
               i = strlen(s_output);
               while ( (i > 0) && *s_ptr2 && !isspace(*s_ptr2) ) {
                  s_ptr2--;
                  i--;
               }
               if ( i > 0 ) {
                  p_in = p_in - (s_inptr - s_ptr2);
                  *(s_output + i) = '\0';
                  s_inptr = s_ptr2;
               }
               i = 0;
            } else {
               *(s_outptr-1) = '\0';
            }
            s_tmpbuff = rebuild_ansi(s_output, outsplit);
            s_tptr = mudstate.global_regs[i_regcurr] + strlen(mudstate.global_regs[i_regcurr]);
            if ( *mudstate.global_regs[i_regcurr] )
               safe_str("\r\n", mudstate.global_regs[i_regcurr], &s_tptr);
            safe_str(s_tmpbuff, mudstate.global_regs[i_regcurr], &s_tptr);
            free_lbuf(s_tmpbuff);
            initialize_ansisplitter(outsplit, LBUF_SIZE);
            memset(s_output, '\0', LBUF_SIZE);
            s_outptr = s_output;
            p_out = outsplit;
            i_regcurr = ((i_regcurr + 1) % i_regs);
            *s_outptr = *s_inptr;
            clone_ansisplitter(p_out, p_in);
         }
         p_in++;
         s_inptr++;
      }   
      if ( *s_output && i ) {
         s_tmpbuff = rebuild_ansi(s_output, outsplit);
         s_tptr = mudstate.global_regs[i_regcurr] + strlen(mudstate.global_regs[i_regcurr]);
         if ( *mudstate.global_regs[i_regcurr] )
            safe_str("\r\n", mudstate.global_regs[i_regcurr], &s_tptr);
         safe_str(s_tmpbuff, mudstate.global_regs[i_regcurr], &s_tptr);
         free_lbuf(s_tmpbuff);
      }
   /* Down then over */
   } else {
      i = i_regcurr = 0;
      s_inptr = s_input;
      while ( s_inptr && *s_inptr) {
         i++;
         if ( *s_inptr && ((sep && (*s_inptr == sep)) || ((i == i_width) && (i_width != 0))) ) {
            if ( !sep ) {
               s_ptr2 = s_inptr;
               i = i_width;
               while ( (i > 0) && *s_ptr2 && !isspace(*s_ptr2) ) {
                  s_ptr2--;
                  i--;
               }
               if ( i > 0 ) {
                  s_inptr = s_ptr2;
               }
               i = 0;
            } 
            i_counter[i_regcurr]++;
            i_regcurr = ((i_regcurr + 1) % i_regs);
            i = 0;
         }
         s_inptr++;
      }
      if ( i > 0 ) 
         i_counter[i_regcurr]++;
    
      s_inptr = s_input;
      i_regcurr = i = 0;
      memset(s_output, '\0', LBUF_SIZE);
      s_outptr = s_output;
      i_kill = 0;
      while ( s_inptr && *s_inptr) {
         *s_outptr = *s_inptr;
         clone_ansisplitter(p_out, p_in);
         i_kill++;
         if ( i_kill > LBUF_SIZE ) {
            notify(player, unsafe_tprintf("Artificially aborted: %d / :%s: / %s", i, s_output, s_inptr));
            break;
         }
         s_outptr++;
         p_out++;
         i++;
         if ( *s_inptr && ((sep && (*s_inptr == sep)) || ((i == i_width) && (i_width != 0))) ) {
            i = 0;
            if ( !sep ) {
               s_ptr2 = s_inptr;
               i = strlen(s_output);
               while ( (i > 0) && *s_ptr2 && !isspace(*s_ptr2) ) {
                  s_ptr2--;
                  i--;
               }
               if ( i > 0 ) {
                  p_in = p_in - (s_inptr - s_ptr2);
                  *(s_output + i) = '\0';
                  s_inptr = s_ptr2;
               }
               i = 0;
            } else {
               *(s_outptr-1) = '\0';
            }
            s_tmpbuff = rebuild_ansi(s_output, outsplit);


            if ( !i_counter[i_regcurr] ) {
               i_regcurr++;
            }
            i_counter[i_regcurr]--;

            if ( i_regcurr > i_regs ) { /* Something bad happened, break out here */
               i_regcurr = 0;
               break;
            }

            s_tptr = mudstate.global_regs[i_regcurr] + strlen(mudstate.global_regs[i_regcurr]);
            if ( *mudstate.global_regs[i_regcurr] )
               safe_str("\r\n", mudstate.global_regs[i_regcurr], &s_tptr);
            safe_str(s_tmpbuff, mudstate.global_regs[i_regcurr], &s_tptr);
            free_lbuf(s_tmpbuff);
            initialize_ansisplitter(outsplit, LBUF_SIZE);
            memset(s_output, '\0', LBUF_SIZE);
            s_outptr = s_output;
            p_out = outsplit;
            *s_outptr = *s_inptr;
            clone_ansisplitter(p_out, p_in);
         }
         p_in++;
         s_inptr++;
      }   
      if ( *s_output && i ) {
         s_tmpbuff = rebuild_ansi(s_output, outsplit);
         s_tptr = mudstate.global_regs[i_regcurr] + strlen(mudstate.global_regs[i_regcurr]);
         if ( *mudstate.global_regs[i_regcurr] )
            safe_str("\r\n", mudstate.global_regs[i_regcurr], &s_tptr);
         safe_str(s_tmpbuff, mudstate.global_regs[i_regcurr], &s_tptr);
         free_lbuf(s_tmpbuff);
      }
   }
   free_lbuf(s_input);
   free_lbuf(s_output);
}

FUNCTION(fun_reverse)
{
    char *s_output, *mybuff, *pmybuff;
    ANSISPLIT outsplit[LBUF_SIZE], outsplit2[LBUF_SIZE];
    int i_icntr, i_max;

    initialize_ansisplitter(outsplit, LBUF_SIZE);
    initialize_ansisplitter(outsplit2, LBUF_SIZE);

    s_output = alloc_lbuf("fun_reverse");
    split_ansi(strip_ansi(fargs[0]), s_output, outsplit);
    i_icntr = 0;
    i_max = strlen(s_output);
    while ( i_icntr < i_max ) {
       clone_ansisplitter(outsplit2+i_icntr, outsplit+(i_max - 1 - i_icntr));
       i_icntr++;
    }
    pmybuff = mybuff = alloc_lbuf("fun_reverse");
    do_reverse(s_output, mybuff, &pmybuff);
    free_lbuf(s_output);
    s_output = rebuild_ansi(mybuff, outsplit2);
    free_lbuf(mybuff);
    safe_str(s_output, buff, bufcx);
    free_lbuf(s_output);
}

FUNCTION(fun_revwords)
{
    char *temp, *tempcx, *tp, *t1, sep;
    int first;

    /* If we are passed an empty arglist return a null string */

    if (nfargs == 0) {
        return;
    }
    varargs_preamble("REVWORDS", 2);
    temp = tempcx = alloc_lbuf("fun_revwords");

    /* Reverse the whole string */

    do_reverse(fargs[0], temp, &tempcx);
    *tempcx = '\0';


    /* Now individually reverse each word in the string.  This will
     * undo the reversing of the words (so the words themselves are
     * forwards again.
     */

    tp = temp;
    first = 1;
    while (tp) {
        if (!first)
            safe_chr(sep, buff, bufcx);
        t1 = split_token(&tp, sep);
        do_reverse(t1, buff, bufcx);
        first = 0;
    }
    free_lbuf(temp);
}

/* ---------------------------------------------------------------------------
 * fun_after, fun_before: Return substring after or before a specified string.
 */

FUNCTION(fun_after)
{
    char *bp, *cp, *mp, *string, *s_output;
    int mlen;
    ANSISPLIT outsplit[LBUF_SIZE], *p_sp;

    if (nfargs == 0) {
        return;
    }
    if (!fn_range_check("AFTER", nfargs, 1, 2, buff, bufcx))
        return;
    bp = fargs[0];
    mp = fargs[1];

    /* Sanity-check arg1 and arg2 */

    if (bp == NULL)
        bp = "";
    if (mp == NULL)
        mp = " ";
    if (!mp || !*mp)
        mp = (char *) " ";
    mlen = strlen(mp);
    if ((mlen == 1) && (*mp == ' '))
        bp = trim_space_sep(bp, ' ');

    initialize_ansisplitter(outsplit, LBUF_SIZE);
    string = alloc_lbuf("fun_after");
    split_ansi(strip_ansi(bp), string, outsplit);
    bp = string;
    p_sp = outsplit;

    /* Look for the target string */

    /* Search for the first character in the target string */
    while (*bp) {

        cp = (char *) index(bp, *mp);
        if (cp == NULL) {
        /* Not found, return empty string */
           free_lbuf(string);
           return;
        }

        /* See if what follows is what we are looking for */
         if (!strncmp(cp, mp, mlen)) {
            /* Yup, return what follows */
            p_sp = p_sp + (cp - bp) + mlen;
            bp = cp + mlen;
            s_output = rebuild_ansi(bp, p_sp);
            safe_str(s_output, buff, bufcx);
            free_lbuf(string);
            free_lbuf(s_output);
            return;
         }
         /* Continue search after found first character */
         p_sp = p_sp + (cp - bp) + 1;
         bp = cp + 1;
    }

    /* Ran off the end without finding it */

    free_lbuf(string);
    return;
}

FUNCTION(fun_before)
{
    char *bp, *cp, *mp, *ip, *string, *s_output;
    int mlen;
    ANSISPLIT outsplit[LBUF_SIZE];

    if (nfargs == 0) {
       return;
    }
    if (!fn_range_check("BEFORE", nfargs, 1, 2, buff, bufcx))
       return;

    bp = fargs[0];
    mp = fargs[1];

    /* Sanity-check arg1 and arg2 */

    if (bp == NULL)
       bp = "";
    if (mp == NULL)
       mp = " ";
    if (!mp || !*mp)
       mp = (char *) " ";
    mlen = strlen(mp);
    if ((mlen == 1) && (*mp == ' '))
       bp = trim_space_sep(bp, ' ');

    initialize_ansisplitter(outsplit, LBUF_SIZE);
    string = alloc_lbuf("fun_before");
    split_ansi(strip_ansi(bp), string, outsplit);

    bp = ip = string;

    /* Look for the target string */

    while (*bp) {

       /* Search for the first character in the target string */

       cp = (char *) index(bp, *mp);
       if (cp == NULL) {

           /* Not found, return entire string */

           s_output = rebuild_ansi(ip, outsplit);
           safe_str(s_output, buff, bufcx);
           free_lbuf(string);
           free_lbuf(s_output);
           return;
       }
       /* See if what follows is what we are looking for */

       if (!strncmp(cp, mp, mlen)) {

           /* Yup, return what follows */

           *cp = '\0';
           s_output = rebuild_ansi(ip, outsplit);
           safe_str(s_output, buff, bufcx);
           free_lbuf(string);
           free_lbuf(s_output);
           return;
       }
       /* Continue search after found first character */

       bp = cp + 1;
    }

    /* Ran off the end without finding it */

    s_output = rebuild_ansi(ip, outsplit);
    safe_str(s_output, buff, bufcx);
    free_lbuf(string);
    free_lbuf(s_output);
    return;
}

/* ---------------------------------------------------------------------------
 * fun_max, fun_min: Return maximum (minimum) value.
 */

FUNCTION(fun_max)
{
    int i, got_one;
    double max, j;

    max = 0;
    for (i = 0, got_one = 0; i < nfargs; i++) {
       if (fargs[i]) {
           j = safe_atof(fargs[i]);
           if (!got_one || (j > max)) {
              got_one = 1;
              max = j;
           }
       }
    }

    if (!got_one) {
       safe_str("#-1 FUNCTION (MAX) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
    } else
       fval(buff, bufcx, max);
    return;
}

FUNCTION(fun_min)
{
    int i, got_one;
    double min, j;

    min = 0;
    for (i = 0, got_one = 0; i < nfargs; i++) {
       if (fargs[i]) {
           j = safe_atof(fargs[i]);
           if (!got_one || (j < min)) {
              got_one = 1;
              min = j;
           }
       }
    }

    if (!got_one) {
       safe_str("#-1 FUNCTION (MIN) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
    } else {
       fval(buff, bufcx, min);
    }
    return;
}

FUNCTION(fun_alphamax)
{
    int i, got_one;
    char *max, *mxp;

    max = alloc_lbuf("fun_alphamax");
    *max = '\0';
    for (i = 0, got_one = 0; i < nfargs; i++) {
       if (fargs[i]) {
           if (!got_one || (strcmp(fargs[i],max) > 0)) {
              got_one = 1;
              mxp = max;
              safe_str(fargs[i],max,&mxp);
           }
       }
    }

    if (!got_one) {
       safe_str("#-1 FUNCTION (ALPHAMAX) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
    } else
       safe_str(max, buff, bufcx);
    free_lbuf(max);
    return;
}

FUNCTION(fun_alphamin)
{
    int i, got_one;
    char *min, *mxp;

    min = alloc_lbuf("fun_alphamin");
    *min = '\0';
    for (i = 0, got_one = 0; i < nfargs; i++) {
       if (fargs[i]) {
           if (!got_one || (strcmp(fargs[i],min) < 0)) {
              got_one = 1;
              mxp = min;
              safe_str(fargs[i],min,&mxp);
           }
       }
    }

    if (!got_one) {
       safe_str("#-1 FUNCTION (ALPHAMIN) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
    } else {
       safe_str(min, buff, bufcx);
    }
    free_lbuf(min);
    return;
}

/* ---------------------------------------------------------------------------
 * fun_search: Search the db for things, returning a list of what matches
 */

void
search_guts(dbref player, dbref cause, dbref caller, char *fargs[], char *buff, char **bufcx, int key)
{
    dbref thing;
    char *nbuf;
    SEARCH searchparm;
    FILE *master;
    int gotone = 0, evc;

    /* Set up for the search.  If any errors, abort. */

    if (mudstate.last_cmd_timestamp == mudstate.now) {
       mudstate.heavy_cpu_recurse += 1;
    }
    if ( mudstate.heavy_cpu_recurse > mudconf.heavy_cpu_max ) {
       safe_str("#-1 HEAVY CPU RECURSION LIMIT EXCEEDED", buff, bufcx);
       return;
    }

    evc = 0;
    if (mudstate.evalnum < MAXEVLEVEL) {
       if (DePriv(player,player,DP_SEARCH_ANY,POWER7,NOTHING)) {
          evc = DePriv(player,NOTHING,DP_SEARCH_ANY,POWER7,POWER_LEVEL_NA);
       if (!DPShift(player))
          mudstate.evalstate[mudstate.evalnum] = evc - 1;
       else
          mudstate.evalstate[mudstate.evalnum] = evc;
       mudstate.evaldb[mudstate.evalnum++] = player;
       } else if (HasPriv(player,player,POWER_SEARCH_ANY,POWER4,NOTHING)) {
          evc = HasPriv(player,NOTHING,POWER_SEARCH_ANY,POWER4,POWER_LEVEL_NA);
          mudstate.evalstate[mudstate.evalnum] = evc;
          mudstate.evaldb[mudstate.evalnum++] = player;
       }
    }
    if (evc) {
       if (!search_setup(player, fargs[0], &searchparm, 1)) {
          safe_str("#-1 ERROR DURING SEARCH", buff, bufcx);
          mudstate.evalnum--;
          return;
       }
    } else {
       if (!search_setup(player, fargs[0], &searchparm, 0)) {
          safe_str("#-1 ERROR DURING SEARCH", buff, bufcx);
          return;
       }
    }
    /* Do the search and report the results */

    file_olist_init(&master,"fsearch.tmp");
    search_perform(player, cause, &searchparm, &master);
    nbuf = alloc_sbuf("fun_search");
    for (thing = file_olist_first(&master); thing != NOTHING; thing = file_olist_next(&master)) {
        if ( key && (Going(thing) || Recover(thing)) ) continue;
           if (gotone)
              safe_chr(' ', buff, bufcx);
        dbval(buff, bufcx, thing);
        gotone = 1;
    }
    free_sbuf(nbuf);
    file_olist_cleanup(&master);
    if (evc)
       mudstate.evalnum--;
}

int mush_minimum(int a,int b, int c)
{
        if (a < b) b = a;
        if (b < c) c = b;
        return(c);
}

FUNCTION(fun_strdistance)
{
   char *s_word1, *s_word2, *p_word1, *p_word2, *p_wrd, c_cur, *s_noansibuf, c_lst, c_lst2;
   int i_dist[LBUF_SIZE], i_dmin, i_lccheck, i_lenword1, i_lenword2, i, j, i_disttmp1, i_disttmp2, k, i_damareu;
   
   if (!fn_range_check("STRDISTANCE", nfargs, 2, 4, buff, bufcx)) 
      return;

   if ( !fargs[0] || !*fargs[0] || !fargs[1] || !*fargs[1] ) {
      ival(buff, bufcx, -1);
      return;
   }

   p_word1 = s_word1 = alloc_lbuf("fun_strdistance_word1");
   p_word2 = s_word2 = alloc_lbuf("fun_strdistance_word2");
   i_lccheck = i_damareu = 0;
   if ( (nfargs > 2) && *fargs[2] ) {
      i_lccheck = atoi(fargs[2]);
   }
   if ( (nfargs > 3) && *fargs[3] ) {
      i_damareu = atoi(fargs[3]);
   }
   if ( i_lccheck != 0 ) {
      s_noansibuf = alloc_lbuf("fun_strdistance_noansibuf");
      strcpy(s_noansibuf, strip_ansi(fargs[0]));
      p_wrd = s_noansibuf;
      while ( *p_wrd ) {
         safe_chr(ToLower(*p_wrd), s_word1, &p_word1);
         p_wrd++;
      }
      strcpy(s_noansibuf, strip_ansi(fargs[1]));
      p_wrd = s_noansibuf;
      while ( *p_wrd ) {
         safe_chr(ToLower(*p_wrd), s_word2, &p_word2);
         p_wrd++;
      }
      free_lbuf(s_noansibuf);
   } else {
      strcpy(s_word1, strip_ansi(fargs[0]));
      strcpy(s_word2, strip_ansi(fargs[1]));
   }
   i_lenword1 = strlen(s_word1);
   i_lenword2 = strlen(s_word2);

   i_dist[0] = i = 1;
   for ( j = 0; j < i_lenword1; j++ ) {
      if ( *s_word2 == *(s_word1 + j) ) {
         i = 0;
      }
      i_dist[j + 1] = j + i;
   }
   i = 1;
   i_dmin = i_dist[1];
   c_cur = c_lst2 = '\0';
   c_lst = *s_word2;
   k = i_disttmp1 = i_disttmp2 = 0;
   while ( i < i_lenword2 ) {
      c_cur = *( s_word2 + i );
      i_disttmp2 = i_dist[0];
      i_dmin = i_disttmp2 + 1;
      i_dist[0] = i_dmin;
      i++;
      for ( j = 1; j <= i_lenword1; j++) {
         i_disttmp1 = i_disttmp2;
         i_disttmp2 = i_dist[j];
         k = 1;
         if ( i_damareu && (c_lst == *(s_word1 + j - 1)) && (c_cur == c_lst2) )
            k = 0;
         if ( c_cur == ( c_lst2 = *(s_word1 + j - 1)) )
            k = 0;
         i_dist[j] = mush_minimum( (i_disttmp1 + k), (i_disttmp2 + 1), (i_dist[j - 1] + 1) );
         if ( i_dist[j] < i_dmin) {
            i_dmin = i_dist[j];
         }
      }
      c_lst = c_cur;
   }
   ival(buff, bufcx, i_dist[i_lenword1]); 
   free_lbuf(s_word1);
   free_lbuf(s_word2);
}

FUNCTION(fun_search)
{
   if ( mudconf.switch_search == 0 )
      (void)search_guts(player, cause, caller, fargs, buff, bufcx, 0);
   else
      (void)search_guts(player, cause, caller, fargs, buff, bufcx, 1);
}

FUNCTION(fun_searchng)
{
   if ( mudconf.switch_search == 0 )
      (void)search_guts(player, cause, caller, fargs, buff, bufcx, 1);
   else
      (void)search_guts(player, cause, caller, fargs, buff, bufcx, 0);
}


/* ---------------------------------------------------------------------------
 * fun_stats: Get database size statistics.
 */

FUNCTION(fun_stats)
{
    dbref who;
    STATS statinfo;

    if (mudstate.last_cmd_timestamp == mudstate.now) {
       mudstate.heavy_cpu_recurse += 1;
    }
    if ( mudstate.heavy_cpu_recurse > mudconf.heavy_cpu_max ) {
       safe_str("#-1 HEAVY CPU RECURSION LIMIT EXCEEDED", buff, bufcx);
       return;
    }
    if ((!fargs[0]) || !*fargs[0] || !string_compare(fargs[0], "all")) {
       who = NOTHING;
    } else {
       who = lookup_player(player, fargs[0], 1);
       if (who == NOTHING) {
           safe_str("#-1 NOT FOUND", buff, bufcx);
           return;
       }
    }
    if (!get_stats(player, who, &statinfo)) {
       safe_str("#-1 ERROR GETTING STATS", buff, bufcx);
       return;
    }
    ival(buff, bufcx, statinfo.s_total);
    safe_chr(' ', buff, bufcx);
    ival(buff, bufcx, statinfo.s_rooms);
    safe_chr(' ', buff, bufcx);
    ival(buff, bufcx, statinfo.s_exits);
    safe_chr(' ', buff, bufcx);
    ival(buff, bufcx, statinfo.s_things);
    safe_chr(' ', buff, bufcx);
    ival(buff, bufcx, statinfo.s_players);
    safe_chr(' ', buff, bufcx);
    ival(buff, bufcx, statinfo.s_garbage);
}

/* ---------------------------------------------------------------------------
 * fun_merge:  given two strings and a character, merge the two strings
 *   by replacing characters in string1 that are the same as the given
 *   character by the corresponding character in string2 (by position).
 *   The strings must be of the same length.
 */

FUNCTION(fun_merge)
{
    char *str, *rep;
    char c;

    /* do length checks first */

    if (strlen(fargs[0]) != strlen(fargs[1])) {
       safe_str("#-1 STRING LENGTHS MUST BE EQUAL", buff, bufcx);
       return;
    }
    if (strlen(fargs[2]) > 1) {
       safe_str("#-1 CHARACTER ARGUMENT MUST BE A SINGLE CHARACTER", buff, bufcx);
       return;
    }
    /* find the character to look for. null character is considered
     * a space
     */

    if (!*fargs[2])
       c = ' ';
    else
       c = *fargs[2];

    /* walk strings, copy from the appropriate string */

    for (str = fargs[0], rep = fargs[1]; *str && *rep; str++, rep++) {
       if (*str == c)
          safe_chr(*rep, buff, bufcx);
       else
           safe_chr(*str, buff, bufcx);
    }

    return;
}

/* ---------------------------------------------------------------------------
 * fun_splice: similar to MERGE(), eplaces by word instead of by character.
 */

FUNCTION(fun_splice)
{
    char *p1, *p2, *q1, *q2, sep, osep;
    int words, i, first;

    svarargs_preamble("SPLICE", 5);

    /* length checks */

    if (countwords(fargs[2], sep) > 1) {
       safe_str("#-1 TOO MANY WORDS", buff, bufcx);
       return;
    }
    words = countwords(fargs[0], sep);
    if (words != countwords(fargs[1], sep)) {
       safe_str("#-1 NUMBER OF WORDS MUST BE EQUAL", buff, bufcx);
       return;
    }
    /* loop through the two lists */

    p1 = fargs[0];
    q1 = fargs[1];
    first = 1;
    for (i = 0; i < words; i++) {
       p2 = split_token(&p1, sep);
       q2 = split_token(&q1, sep);
       if (!first)
           safe_chr(osep, buff, bufcx);
       if (!strcmp(p2, fargs[2]))
           safe_str(q2, buff, bufcx);  /* replace */
       else
           safe_str(p2, buff, bufcx);  /* copy */
       first = 0;
    }
}

/* ---------------------------------------------------------------------------
 * fun_repeat: repeats a string
 */

FUNCTION(fun_repeat)
{
    int times, i, over = 0;

    times = atoi(fargs[1]);
    if ((times < 1) || (times > LBUF_SIZE - 1)) {
      /* nothing */
    } else if (times == 1) {
       safe_str(fargs[0], buff, bufcx);
    } else if (*fargs[0]) {
       for (i = 0; i < times && !over; i++)
           over = safe_str(fargs[0], buff, bufcx);
    } else
       safe_chr('\0',buff,bufcx);
}

/* ---------------------------------------------------------------------------
 * fun_nsiter: Make list from evaluating arg2 with each member of arg1.
 * NOTE: This function expects that its arguments have not been evaluated.
 */

FUNCTION(fun_nsiter)
{
    char *curr, *objstring, *buff3, *result, *cp, sep, *st_buff, *tpr_buff, *tprp_buff;
    int cntr;

    evarargs_preamble("NSITER", 3);
    st_buff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0],
                   cargs, ncargs);
    cp = curr = alloc_lbuf("fun_nsiter");
    safe_str(strip_ansi(st_buff), curr, &cp);
    free_lbuf(st_buff);
    cp = curr;
    cp = trim_space_sep(cp, sep);
    if (!*cp) {
       free_lbuf(curr);
       return;
    }
    cntr=1;
    tprp_buff = tpr_buff = alloc_lbuf("fun_nsiter");
    while (cp) {
       objstring = split_token(&cp, sep);
       tprp_buff = tpr_buff;
       buff3 = replace_tokens(fargs[1], objstring, safe_tprintf(tpr_buff, &tprp_buff, "%d",cntr), NULL);
       result = exec(player, cause, caller,
                     EV_STRIP | EV_FCHECK | EV_EVAL, buff3, cargs, ncargs);
       free_lbuf(buff3);
       safe_str(result, buff, bufcx);
       free_lbuf(result);
       cntr++;
    }
    free_lbuf(tpr_buff);
    free_lbuf(curr);
}

/* ---------------------------------------------------------------------------
 * fun_strmath: Apply math functions to numeric values in given string
 * NOTE: Any string (numeric or otherwise) that couldn't be modified is
 *       returned as-is.  This includes division-by-zero errors.
 */
FUNCTION(fun_strmath)
{
   char *curr, *cp, *objstring, sep, osep, *tcurr, *tmp,
        *s_str, *s_strtok, sep2, osep2, osep_str[3];
   static char mybuff[LBUF_SIZE];
   int  first, first2, i_start, i_cnt, i_currcnt, i_applycnt;
   double f_val, f_base, f_number;

   if (!fn_range_check("STRMATH", nfargs, 3, 9, buff, bufcx))
      return;
   if ((nfargs > 3) && *fargs[3]) {
      tmp = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[3],
                 cargs, ncargs);
      if ( *tmp ) {
         sep = *tmp;
      } else {
         sep = ' ';
      }
      free_lbuf(tmp);
   } else {
      sep = ' ';
   }
   if ((nfargs > 4) && *fargs[4]) {
      tmp = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[4],
                 cargs, ncargs);
      if ( *tmp ) {
         osep = *tmp;
      } else {
         osep = sep;
      }
      free_lbuf(tmp);
   } else {
      osep = sep;
   }
   if ((nfargs > 5) && *fargs[5]) {
      tmp = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[5],
                 cargs, ncargs);
      i_start = atoi(tmp);
      free_lbuf(tmp);
   } else {
      i_start = 1;
   }
   if (i_start < 1)
      i_start = 0;
   if ((nfargs > 6) && *fargs[6]) {
      tmp = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[6],
                 cargs, ncargs);
      i_cnt = atoi(tmp);
      free_lbuf(tmp);
   } else {
      i_cnt = LBUF_SIZE;
   }
   if ( !i_start ) {
      tmp = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0],
                 cargs, ncargs);
      safe_str(tmp, buff, bufcx);
      free_lbuf(tmp);
      return;
   }
   memset(osep_str, '\0', sizeof(osep_str));
   if ((nfargs > 7) && *fargs[7]) {
      tmp = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[7],
                 cargs, ncargs);
      if ( *tmp ) {
         sep2 = *fargs[7];
         sprintf(osep_str, "%c", sep2);
      } else {
         sep2 = ' ';
         sprintf(osep_str, "%c\t", sep2);
      }
      free_lbuf(tmp);
   } else {
      sep2 = ' ';
      sprintf(osep_str, "%c\t", sep2);
   }
   if ((nfargs > 8) && *fargs[8] ) {
      tmp = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[8],
                 cargs, ncargs);
      if ( *tmp ) {
         osep2 = *tmp;
      } else {
         osep2 = sep2;
      }
      free_lbuf(tmp);
   } else {
      osep2 = sep2;
   }

   tmp = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0],
                    cargs, ncargs);
   curr = alloc_lbuf("fun_strmath");
   cp = curr;
   safe_str(strip_ansi(tmp),curr,&cp);
   free_lbuf(tmp);
   cp = curr;
   cp = trim_space_sep(cp, sep);
   if (!*cp) {
      free_lbuf(curr);
      return;
   }
   tcurr = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[1],
                    cargs, ncargs);
   if ( !*tcurr || !tcurr )
      f_base = 0.0;
   else
      f_base = safe_atof(tcurr);
   free_lbuf(tcurr);
   first = 1;
   i_currcnt = 1;
   i_applycnt = 0;
   while (cp) {
      objstring = split_token(&cp, sep);
      memset(mybuff, '\0', sizeof(mybuff));
      memcpy(mybuff, objstring, LBUF_SIZE - 1);
      s_str = strtok_r(mybuff, osep_str, &s_strtok);
      if ( !first )
         safe_chr(osep, buff, bufcx);
      first2 = 1;
      while ( s_str ) {   
         if ( !first2 ) {
            if ( (i_currcnt >= i_start) && (i_applycnt <= i_cnt) ) {
               safe_chr(osep2, buff, bufcx);
            } else {
               safe_chr(sep2, buff, bufcx);
            }
         }
         if ( is_float(s_str) ) {
            f_number = safe_atof(s_str);
            if ( (i_currcnt >= i_start) && (i_applycnt <= i_cnt) ) {
               switch( *fargs[2] ) {
                  case '+': f_val = f_number + f_base;
                            break;
                  case '-': f_val = f_number - f_base;
                            break;
                  case '/': if ( f_base == 0 )
                                  f_val = f_number;
                            else {
                               f_val = f_number / f_base;
                            }
                            break;
                  case '*': f_val = f_number * f_base;
                            break;
                  case '%': if ( f_base == 0)
                               f_base = 1;
                            f_val = fmod(f_number, f_base);
                            break;
                  default : f_val = f_number + f_base;
                            break;
               }
            } else {
               f_val = f_number;
            }
            fval(buff, bufcx, f_val);
         } else {
            safe_str(s_str, buff, bufcx);
         }
         s_str = strtok_r(NULL, osep_str, &s_strtok);
         first2 = 0;
      }
      if ( i_currcnt >= i_start )
         i_applycnt++;
      i_currcnt++;
      first = 0;
   }
   free_lbuf(curr);
}

FUNCTION(fun_objeval)
{
    dbref obj;
    int prev_nocode;
    char *result, *cp;

    if (!fn_range_check("OBJEVAL", nfargs, 2, 3, buff, bufcx))
       return;
    mudstate.objevalst = 0;
    prev_nocode = mudstate.nocodeoverride;

    if ( (nfargs >= 3) && *fargs[2] && Wizard(player) ) {
       cp = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[2],
                 cargs, ncargs);
       if ( cp && *cp && (atoi(cp) > 0) ) {
          mudstate.nocodeoverride = 1;
       }
       free_lbuf(cp);
    }
    cp = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0],
              cargs, ncargs);
    init_match(player, cp, NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    obj = noisy_match_result();
    if ((obj != NOTHING) && !Controls(player,Owner(obj)))
        obj = NOTHING;
    free_lbuf(cp);
    if (obj != NOTHING) {
        if ( obj != NOTHING && !Wizard(obj) && obj != player )
           mudstate.objevalst = 1;
        result = exec(obj, player, player, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
    } else {
        if ( !Wizard(cause) && cause != player )
           mudstate.objevalst = 1;
        result = exec(player, cause, player, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[1], cargs, ncargs);
    }
    safe_str(result, buff, bufcx);
    free_lbuf(result);
    mudstate.nocodeoverride = prev_nocode;
    mudstate.objevalst = 0;
}

/* ---------------------
 * Function iter information
 */
FUNCTION(fun_iter)
{
    char *sop_buf, *sep_buf, *curr, *objstring, *buff3, *result, *cp, sep, sop[LBUF_SIZE+1];
    char *bptr, *st_buff, *tpr_buff, *tprp_buff;
    int first, cntr;

    if (!fn_range_check("ITER", nfargs, 2, 4, buff, bufcx))
       return;

    if ( mudstate.iter_inum >= 49 ) {
       safe_str("#-1 FUNCTION RECURSION LIMIT EXCEEDED", buff, bufcx);
       return;
    }
    memset(sop, 0, sizeof(sop));

    if (nfargs > 3 ) {
       sop_buf=exec(player, cause, caller,
          EV_STRIP | EV_FCHECK | EV_EVAL, fargs[3], cargs, ncargs);
       strncpy(sop, sop_buf, sizeof(sop)-1);
       free_lbuf(sop_buf);
    } else {
       strcpy(sop, " ");
    }
    if (nfargs > 2 && *fargs[2] ) {
       sep_buf=exec(player, cause, caller,
          EV_STRIP | EV_FCHECK | EV_EVAL, fargs[2], cargs, ncargs);
       sep = *sep_buf;
       free_lbuf(sep_buf);
    } else {
       sep = ' ';
    }

    st_buff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0],
         cargs, ncargs);
    cp = curr = alloc_lbuf("fun_iter");
    safe_str(strip_ansi(st_buff), curr, &cp);
    free_lbuf(st_buff);
    cp = curr;
    cp = trim_space_sep(cp, sep);
    if (!*cp) {
       free_lbuf(curr);
       return;
    }
    first = 1;
    cntr=1;
    mudstate.iter_inum++;
    mudstate.iter_arr[mudstate.iter_inum] = alloc_lbuf("recursive_iter");
    mudstate.iter_inumbrk[mudstate.iter_inum] = 0;
    tprp_buff = tpr_buff = alloc_lbuf("fun_iter");
    while (cp) {
        objstring = split_token(&cp, sep);
        bptr = mudstate.iter_arr[mudstate.iter_inum];
        safe_str(objstring, mudstate.iter_arr[mudstate.iter_inum], &bptr);
        mudstate.iter_inumarr[mudstate.iter_inum] = cntr;
        tprp_buff = tpr_buff;
        buff3 = replace_tokens(fargs[1], objstring, safe_tprintf(tpr_buff, &tprp_buff, "%d",cntr), NULL);
        result = exec(player, cause, caller,
                      EV_STRIP | EV_FCHECK | EV_EVAL, buff3, cargs, ncargs);
        free_lbuf(buff3);
        if (!first)
           safe_str(sop, buff, bufcx);
        first = 0;
        safe_str(result, buff, bufcx);
        free_lbuf(result);
        if ( mudstate.iter_inumbrk[mudstate.iter_inum] )
           break;
        cntr++;
    }
    mudstate.iter_inumbrk[mudstate.iter_inum] = 0;
    free_lbuf(tpr_buff);
    free_lbuf(mudstate.iter_arr[mudstate.iter_inum]);
    mudstate.iter_inum--;
    free_lbuf(curr);
}

FUNCTION(fun_itext)
{
   int inum_val, i_key;

   if (!fn_range_check("ITEXT", nfargs, 1, 2, buff, bufcx))
       return;

   i_key = 0;
   if ( (nfargs > 1) && *fargs[1] )
      i_key = atoi(fargs[1]);

   inum_val = atoi(fargs[0]);

   if ( i_key ) {
      if ( (inum_val < 0) || (inum_val > (mudstate.dolistnest - 1)) ) {
         safe_str("#-1 ARGUMENT OUT OF RANGE", buff, bufcx);
      } else {
         if ( (*fargs[0] == 'l') || (*fargs[0] == 'L') ) {
            safe_str(mudstate.dol_arr[0], buff, bufcx);
         } else {
            safe_str(mudstate.dol_arr[(mudstate.dolistnest - 1) - inum_val], buff, bufcx);
         }
      }
   } else {
      if ( (inum_val < 0) || (inum_val > mudstate.iter_inum) ) {
         safe_str("#-1 ARGUMENT OUT OF RANGE", buff, bufcx);
      } else {
         if ( (*fargs[0] == 'l') || (*fargs[0] == 'L') ) {
            safe_str(mudstate.iter_arr[0], buff, bufcx);
         } else {
            safe_str(mudstate.iter_arr[mudstate.iter_inum - inum_val], buff, bufcx);
         }
      }
   }
}

FUNCTION(fun_inum)
{
   int inum_val, i_key;

   if (!fn_range_check("INUM", nfargs, 1, 2, buff, bufcx))
       return;

   i_key = 0;
   if ( (nfargs > 1) && *fargs[1] )
      i_key = atoi(fargs[1]);

   inum_val = atoi(fargs[0]);
 
   if ( i_key ) {
      if ( (inum_val < 0) || (inum_val > (mudstate.dolistnest - 1)) ) {
         safe_str("#-1 ARGUMENT OUT OF RANGE", buff, bufcx);
      } else {
         if ( (*fargs[0] == 'l') || (*fargs[0] == 'L') ) {
            ival(buff, bufcx, mudstate.dol_inumarr[mudstate.dolistnest - 1]);
         } else {
            ival(buff, bufcx, mudstate.dol_inumarr[(mudstate.dolistnest - 1) - inum_val]);
         }
      }
   } else {
      if ( (inum_val < 0) || (inum_val > mudstate.iter_inum) ) {
         safe_str("#-1 ARGUMENT OUT OF RANGE", buff, bufcx);
      } else {
         if ( (*fargs[0] == 'l') || (*fargs[0] == 'L') ) {
            ival(buff, bufcx, mudstate.iter_inumarr[mudstate.iter_inum]);
         } else {
            ival(buff, bufcx, mudstate.iter_inumarr[mudstate.iter_inum - inum_val]);
         }
      }
   }
}

FUNCTION(fun_ibreak)
{
   int inum_brk;

   inum_brk = atoi(fargs[0]);
   if ( (inum_brk < 0) || (inum_brk > mudstate.iter_inum) ) {
      safe_str("#-1 ARGUMENT OUT OF RANGE", buff, bufcx);
   } else {
      mudstate.iter_inumbrk[mudstate.iter_inum - inum_brk] = 1;
   }
}

FUNCTION(fun_ilev)
{
   ival(buff, bufcx, mudstate.iter_inum);
}

FUNCTION(fun_citer)
{
    char *curr, objstring[SBUF_SIZE], *buff3, *result, *cp, sep, *tpr_buff, *tprp_buff, *outbuff, *s_output;
    int first, cntr;
    ANSISPLIT outsplit[LBUF_SIZE], *p_sp;


    evarargs_preamble("CITER", 3);
    if ( (nfargs >= 3) && !*fargs[2] )
       sep = *fargs[2];

    outbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0], cargs, ncargs);

    initialize_ansisplitter(outsplit, LBUF_SIZE);
    curr = alloc_lbuf("fun_citer2");

    split_ansi(strip_ansi(outbuff), curr, outsplit);
    free_lbuf(outbuff);

    cp = curr;
    p_sp = outsplit;
    if (!*cp) {
       free_lbuf(curr);
       return;
    }
    first = 1;
    cntr=1;
    tprp_buff  = tpr_buff = alloc_lbuf("fun_citer");
    while (*cp) {
       if (*cp == '\r') {
         if (*(cp + 1) == '\n')
           cp++;
           p_sp++;
       }
       *objstring = *cp;
       *(objstring+1) = '\0';
       s_output = rebuild_ansi(objstring, p_sp);
       cp++;
       p_sp++;
       tprp_buff = tpr_buff;
       buff3 = replace_tokens(fargs[1], s_output, safe_tprintf(tpr_buff, &tprp_buff, "%d",cntr), NULL);
       result = exec(player, cause, caller,
                     EV_STRIP | EV_FCHECK | EV_EVAL, buff3, cargs, ncargs);
       free_lbuf(buff3);
       free_lbuf(s_output);
       if (!first && sep)
          safe_chr(sep, buff, bufcx);
       first = 0;
       if ( result && *result != '\0') {
          safe_str(result, buff, bufcx);
       }
       else if ( !sep )
          safe_chr(' ', buff, bufcx);
       free_lbuf(result);
       cntr++;
    }
    free_lbuf(tpr_buff);
    free_lbuf(curr);
}

/* ----------------------------------------------------------------
 * fun_list:
 * This works just like iter() except it uses notify() to bypass
 * the buffer limitations.
 * Eventhough this is considered a side-effect it's harmless so
 * will be seperate from the normal list of 'side-effects'
 */

FUNCTION(fun_list)
{
    char *sep_buf, *curr, *objstring, *buff3, *result, *cp, sep, *st_buff, *bptr,
         *tpr_buff, *tprp_buff, *hdr_buff;
    dbref target_cause;
    int cntr, i_maxchars;

    if (!fn_range_check("LIST", nfargs, 2, 5, buff, bufcx))
       return;

    if ( !(mudconf.sideeffects & SIDE_LIST) ) {
       notify(player, "#-1 FUNCTION DISABLED");
       return;
    }

    if ( mudstate.iter_inum >= 49 ) {
       safe_str("#-1 FUNCTION RECURSION LIMIT EXCEEDED", buff, bufcx);
       return;
    }

    if (nfargs > 2 && *fargs[2] ) {
       sep_buf=exec(player, cause, caller,
          EV_STRIP | EV_FCHECK | EV_EVAL, fargs[2], cargs, ncargs);
       sep = *sep_buf;
       free_lbuf(sep_buf);
    } else {
       sep = ' ';
    }

    st_buff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[0],
                   cargs, ncargs);
    cp = curr = alloc_lbuf("fun_list");
    safe_str(strip_ansi(st_buff), curr, &cp);
    free_lbuf(st_buff);
    cp = curr;
    cp = trim_space_sep(cp, sep);
    if (!*cp) {
       free_lbuf(curr);
       return;
    }
    cntr=1;
    mudstate.iter_inum++;
    mudstate.iter_arr[mudstate.iter_inum] = alloc_lbuf("recursive_list");
    mudstate.iter_inumbrk[mudstate.iter_inum] = 0;
    tprp_buff = tpr_buff = alloc_lbuf("fun_list");
    target_cause = cause;
    if ( (nfargs > 4) && *fargs[4] ) {
       hdr_buff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[4],
                       cargs, ncargs);
/*     target_cause = lookup_player(player, hdr_buff, 1); */
       init_match(player, hdr_buff, NOTYPE);
       match_everything(0);
       target_cause = match_result();
       if ( !(Good_chk(target_cause) && Controls(cause, target_cause)) )
          target_cause = cause;
       free_lbuf(hdr_buff);
    }
    if ( (nfargs > 3) && *fargs[3] ) {
       hdr_buff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL, fargs[3],
                       cargs, ncargs);
       if ( *hdr_buff ) {
          notify(target_cause, hdr_buff);
       }
       free_lbuf(hdr_buff);
    }
    i_maxchars = 0;
    while (cp) {
        if ( mudstate.iter_inumbrk[mudstate.iter_inum] == 2 )
           break;
        objstring = split_token(&cp, sep);
        bptr = mudstate.iter_arr[mudstate.iter_inum];
        safe_str(objstring, mudstate.iter_arr[mudstate.iter_inum], &bptr);
        mudstate.iter_inumarr[mudstate.iter_inum] = cntr;
        tprp_buff = tpr_buff;
        buff3 = replace_tokens(fargs[1], objstring, safe_tprintf(tpr_buff, &tprp_buff, "%d",cntr), NULL);
        result = exec(player, cause, caller,
                      EV_STRIP | EV_FCHECK | EV_EVAL, buff3, cargs, ncargs);
        free_lbuf(buff3);
        i_maxchars += strlen(result);
        if ( i_maxchars > mudconf.list_max_chars ) {
           for (i_maxchars = 0; i_maxchars <= mudstate.iter_inum; i_maxchars++)
              mudstate.iter_inumbrk[i_maxchars] = 2;
           sprintf(result, "%d %s killed.  %d char ceiling.", 
                   (mudstate.iter_inum + 1), ((mudstate.iter_inum + 1) > 1 ? "stacks" : "stack"), mudconf.list_max_chars);
           broadcast_monitor(player, MF_CPUEXT, "LIST() OUTPUT OVERFLOW", result, NULL, cause, 0, 0, NULL);
           free_lbuf(result);
           break;
        }
        notify(target_cause, result);
        free_lbuf(result);
        if ( mudstate.iter_inumbrk[mudstate.iter_inum] )
           break;
        cntr++;
    }
    mudstate.iter_inumbrk[mudstate.iter_inum] = 0;
    free_lbuf(tpr_buff);
    free_lbuf(mudstate.iter_arr[mudstate.iter_inum]);
    mudstate.iter_inum--;
    free_lbuf(curr);
}

/* ---------------------------------------------------------------------------
 * fun_fold: iteratively eval an attrib with a list of arguments
 *        and an optional base case.  With no base case, the first list element
 *    is passed as %0 and the second is %1.  The attrib is then evaluated
 *    with these args, the result is then used as %0 and the next arg is
 *    %1 and so it goes as there are elements left in the list.  The
 *    optinal base case gives the user a nice starting point.
 *
 *    > &REP_NUM object=[%0][repeat(%1,%1)]
 *    > say fold(OBJECT/REP_NUM,1 2 3 4 5,->)
 *    You say "->122333444455555"
 *
 *      NOTE: To use added list separator, you must use base case!
 */

static int count_words(char *s1, char sep)
{
  int rt = 0;
  char *p1, *p2;

  p1 = s1;
  while (*p1 && (*p1 == sep))
    p1++;
  while (*p1) {
    rt++;
    p2 = p1 + 1;
    while (*p2 && (*p2 != sep))
      p2++;
    if (*p2) {
      p1 = p2 + 1;
      while (*p1 && (*p1 == sep))
  p1++;
    }
    else
      p1 = p2;
  }
  return rt;
}

static int count_words2(char *s1, char sep)
{
  int rt = 0;
  char *p1;

  p1 = s1;
  while (*p1) {
    while (*p1 && (*p1 != sep))
      p1++;
    if (p1 != s1)
      rt++;
    if (*p1)
      p1++;
  }
  return rt;
}

static int find_pos(char *source, char *word, char sep, int *pos, int at)
{
  int rt = 0, work = 0, len, x;
  char *p1, *p2;

  p1 = source;
  len = strlen(word);
  while (*p1 && (*p1 == sep))
    p1++;
  while (*p1) {
    work++;
    if (!strncmp(p1,word,len)) {
      if ((*(p1 + len) == sep) || !*(p1 + len)) {
  for (x = 0; x < at; x++) {
    if (*(pos+x) == work)
      break;
  }
  if (x == at) {
    rt = work;
    break;
  }
      }
    }
    p2 = p1 + 1;
    while (*p2 && (*p2 != sep))
      p2++;
    if (*p2) {
      p1 = p2 + 1;
      while (*p1 && (*p1 == sep))
  p1++;
    }
    else
      p1 = p2;
  }
  return rt;
}

static void get_word(char *source, int pos, char sep, char *buff)
{
  int rt = 0;
  char *p1, *p2;

  p1 = source;
  *buff = '\0';
  while (*p1 && (*p1 == sep))
    p1++;
  while (*p1) {
    p2 = p1 + 1;
    while (*p2 && (*p2 != sep))
      p2++;
    if (rt == pos) {
      strncpy(buff,p1,p2-p1);
      *(buff + (p2 - p1)) = '\0';
      break;
    }
    if (*p2) {
      p1 = p2 + 1;
      while (*p1 && (*p1 == sep))
  p1++;
    }
    else
      p1 = p2;
    rt++;
  }
}

void indmast(char *source, char *replace, char *pos, char sep, char *bf, char **bfx, int tt)
{
  int num, num2, ipos, x;
  char *p1, *p2, c1;

  if (!sep || !*replace) {
    safe_str(source, bf, bfx);
    return;
  }
  num = count_words2(source,sep);
  ipos = atoi(pos);
  if (!num || (ipos < 1) || (ipos > num)) {
    if ( !num && (ipos == 1) && (tt == 4) )
       safe_str(replace, bf, bfx);
    else
       safe_str(source, bf, bfx);
    return;
  }
  p1 = source;
  for (x = 1; x < ipos; x++) {
    p2 = p1;
    while (*p2 != sep)
      p2++;
    *p2 = '\0';
    if (*p1)
      safe_str(p1,bf,bfx);
    safe_chr(sep,bf,bfx);
    p1 = p2 + 1;
  }
  switch (tt) {
    case 1:
      safe_str(replace,bf,bfx);
      num2 = count_words2(replace,sep);
      if (num2 > (num - ipos))
        *p1 = '\0';
      else {
        safe_chr(sep,bf,bfx);
        for (x = 0; x < num2; x++) {
          while (*(p1++) != sep);
        }
      }
      break;
    case 2:
    case 4:
      safe_str(replace,bf,bfx);
      safe_chr(sep,bf,bfx);
      break;
    case 3:
      p2 = p1;
      while (*p2 && (*p2 != sep))
        p2++;
      c1 = *p2;
      *p2 = '\0';
      safe_str(p1,bf,bfx);
      safe_chr(sep,bf,bfx);
      safe_str(replace,bf,bfx);
      if (ipos < num)
        safe_chr(sep,bf,bfx);
      *p2 = c1;
      p1 = p2;
      if (c1)
        p1++;
      break;
  }
  if (*p1)
      safe_str(p1,bf,bfx);
}

FUNCTION(fun_rindex)
{
  indmast(fargs[0],fargs[1],fargs[2],*fargs[3],buff,bufcx,1);
}

FUNCTION(fun_iindex)
{
  indmast(fargs[0],fargs[1],fargs[2],*fargs[3],buff,bufcx,2);
}

FUNCTION(fun_aindex)
{
  indmast(fargs[0],fargs[1],fargs[2],*fargs[3],buff,bufcx,3);
}

FUNCTION(fun_aiindex)
{
  indmast(fargs[0],fargs[1],fargs[2],*fargs[3],buff,bufcx,4);
}

FUNCTION(fun_lreplace)
{
  char sep;

  if (!fn_range_check("LREPLACE", nfargs, 3, 4, buff, bufcx)) {
    return;
  }
  if (nfargs == 4) {
    if (*fargs[3])
      sep = *fargs[3];
    else
      sep = ' ';
  }
  else
    sep = ' ';
  indmast(fargs[0],fargs[2],fargs[1],sep,buff,bufcx,1);
}

FUNCTION(fun_munge)
{
    dbref thing, aowner;
    int attrib, aflags, numorig, numsubs, *posorig, *possub, x, min, ind, tval;
    ATTR *attr;
    char *atr_gotten, *atr_gotten2, *clist[2], *sv1, *sv2, sep, osep;

    if (!fn_range_check("MUNGE", nfargs, 3, 5, buff, bufcx)) {
      return;
    }
    if (parse_attrib(player, fargs[0], &thing, &attrib)) {
       if ((attrib == NOTHING) || (!Good_obj(thing)) || Going(thing) || Recover(thing))
           attr = NULL;
       else
           attr = atr_num(attrib);
    } else {
       thing = player;
       attr = atr_str(fargs[0]);
       if (attr != NULL)
         attrib = attr->number;
    }
    if (attr == NULL) {
       safe_str("#-1 NO MATCH", buff, bufcx);
       return;
    }
    if (Cloak(thing) && !Wizard(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    if (SCloak(thing) && Cloak(thing) && !Immortal(player)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    if ( (attr->number != A_LAMBDA) && (attr->flags & AF_NOPROG) ) {
      safe_str("#-1 PERMISSION DENIED",buff,bufcx);
      return;
    }
    if (nfargs > 3) {
      if (*fargs[3])
        sep = *fargs[3];
      else
        sep = ' ';
    } else
      sep = ' ';
    if (nfargs > 4) {
       if ( *fargs[4])
          osep = *fargs[4];
       else
          osep = sep;
    } else
       osep = sep;
    atr_gotten = atr_pget(thing, attrib, &aowner, &aflags);
    numorig = count_words(fargs[1],sep);
    numsubs = count_words(fargs[2],sep);
    if (!*atr_gotten || !numorig || !numsubs) {
      free_lbuf(atr_gotten);
      return;
    }
    if (!check_read_perms(player, thing, attr, aowner, aflags, buff, bufcx)) {
      free_lbuf(atr_gotten);
      return;
    }
    sv1 = alloc_lbuf("fun_munge");
    sv2 = alloc_lbuf("fun_munge");
    strcpy(sv1,fargs[1]);
    *sv2 = sep;
    *(sv2 + 1) = '\0';
    clist[0] = sv1;
    clist[1] = sv2;
    tval = safer_ufun(player, thing, thing, (attr ? attr->flags : 0), aflags);
    if ( tval == -2 ) {
       atr_gotten2 = alloc_lbuf("edefault_buff");
       sprintf(atr_gotten2 ,"#-1 PERMISSION DENIED");
    } else {
       atr_gotten2 = exec(thing, player, caller, EV_STRIP | EV_FCHECK | EV_EVAL, atr_gotten, clist, 2);
    }
    safer_unufun(tval);
    free_lbuf(sv1);
    free_lbuf(sv2);

    posorig = (int *)malloc(sizeof(int) * numorig);
    if (!posorig) {
      free_lbuf(atr_gotten);
      free_lbuf(atr_gotten2);
      return;
    }
    possub = (int *)malloc(sizeof(int) * numorig);
    if (!possub) {
      free(posorig);
      free_lbuf(atr_gotten);
      free_lbuf(atr_gotten2);
      return;
    }
    for (x = 0; x < numorig; x++)
      *(posorig + x) = 0;
    sv1 = fargs[1];
    while (*sv1 && (*sv1 == sep))
      sv1++;
    for (x = 0; x < numorig; x++) {
      sv2 = sv1 + 1;
      while (*sv2 && (*sv2 != sep))
          sv2++;
      *sv2 = '\0';
      *(posorig + x) = find_pos(atr_gotten2,sv1,sep,posorig,x);
      if (x < (numorig - 1)) {
          sv1 = sv2 + 1;
          while (*sv1 && (*sv1 == sep))
            sv1++;
      }
    }
    ind = 0;
    do {
      min = LBUF_SIZE;
      for (x = 0; x < numorig; x++) {
          if ((*(posorig + x) > 0) && (*(posorig + x) < min)) {
            min = *(posorig + x);
            *(possub + ind) = x;
          }
      }
      if (min < LBUF_SIZE)
          *(posorig + *(possub + ind++)) = 0;
    } while (min < LBUF_SIZE);
    min = 0;
    for (x = 0; x < ind; x++) {
      get_word(fargs[2], *(possub + x), sep, atr_gotten);
      if (*atr_gotten) {
          if (min)
            safe_chr(osep,buff,bufcx);
          safe_str(atr_gotten,buff,bufcx);
          min = 1;
      }
    }
    free(posorig);
    free(possub);
    free_lbuf(atr_gotten);
    free_lbuf(atr_gotten2);
    return;
}

FUNCTION(fun_fold)
{
    dbref aowner, thing;
    int aflags, anum, tval;
    ATTR *ap;
    char *atext, *result, *curr, *cp, *atextbuf, *clist[2], *rstore, sep;

    /* We need two to four arguements only */

    mvarargs_preamble("FOLD", 2, 4);

    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)) || Going(thing) || Recover(thing))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
       thing = player;
       ap = atr_str(fargs[0]);
    }

    /* Make sure we got a good attribute */

    if (!ap) {
       return;
    }
    /* Use it if we can access it, otherwise return an error. */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
       return;
    } else if (!*atext || !See_attr(player, thing, ap, aowner, aflags, 0)) {
       free_lbuf(atext);
       return;
    }
    /* Evaluate it using the rest of the passed function args */

    cp = curr = fargs[1];
    atextbuf = alloc_lbuf("fun_fold");
    strcpy(atextbuf, atext);

    /* may as well handle first case now */

    if ((nfargs >= 3) && (*fargs[2])) {
        clist[0] = fargs[2];
        clist[1] = split_token(&cp, sep);
        if ( (mudconf.secure_functions & 4) ) {
           tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(thing, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            atextbuf, clist, 2);
           }
        } else {
           tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            atextbuf, clist, 2);
           }
        }
        safer_unufun(tval);
    } else {
        clist[0] = split_token(&cp, sep);
        clist[1] = split_token(&cp, sep);
        if ( (mudconf.secure_functions & 4) ) {
           tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(thing, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            atextbuf, clist, 2);
           }
        } else {
           tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            atextbuf, clist, 2);
           }
        }
        safer_unufun(tval);
    }

    rstore = result;
    result = NULL;

    while (cp) {
        clist[0] = rstore;
        clist[1] = split_token(&cp, sep);
        strcpy(atextbuf, atext);
        if ( (mudconf.secure_functions & 4) ) {
           tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(thing, cause, caller,
                            EV_STRIP | EV_FCHECK | EV_EVAL, atextbuf, clist, 2);
           }
        } else {
           tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(player, cause, caller,
                            EV_STRIP | EV_FCHECK | EV_EVAL, atextbuf, clist, 2);
           }
        }
        safer_unufun(tval);
        strcpy(rstore, result);
        free_lbuf(result);
    }
    safe_str(rstore, buff, bufcx);
    free_lbuf(rstore);
    free_lbuf(atext);
    free_lbuf(atextbuf);
}

/* ---------------------------------------------------------------------------
 * Tiny 3.0 list-based numeric operators
 *
 * Modified for use with RhostMUSH
 */

FUNCTION(fun_lavg)
{
    double sum;
    int cntr;
    char *cp, *curr, sep;

    varargs_preamble("LAVG", 2);

    sum = 0;
    cp = trim_space_sep(fargs[0], sep);
    cntr = 0;
    while (cp) {
       cntr++;
       curr = split_token(&cp, sep);
       sum = sum + safe_atof(curr);
    }
    if ( cntr == 0)
       cntr = 1;
    fval(buff, bufcx, sum/cntr);
}

FUNCTION(fun_ladd)
{
    double sum;
    char *cp, *curr, sep;

    varargs_preamble("LADD", 2);

    sum = 0;
    cp = trim_space_sep(fargs[0], sep);
    while (cp) {
       curr = split_token(&cp, sep);
       sum = sum + safe_atof(curr);
    }
    fval(buff, bufcx, sum);
}

FUNCTION(fun_lsub)
{
   double sum;
   char *cp, *curr, sep;

   varargs_preamble("LSUB", 2);

   cp = trim_space_sep(fargs[0], sep);
   if ( cp ) {
      curr = split_token(&cp, sep);
      sum = safe_atof(curr);
   } else {
      sum = 0;
   }
   while (cp) {
      curr = split_token(&cp, sep);
      sum = sum - safe_atof(curr);
   }
   fval(buff, bufcx, sum);
}

FUNCTION(fun_lmul)
{
   double sum;
   char *cp, *curr, sep;

   varargs_preamble("LMUL", 2);

   cp = trim_space_sep(fargs[0], sep);
   if (cp) {
      curr = split_token(&cp, sep);
      sum = safe_atof(curr);
   } else {
      sum = 0;
   }
   while (cp) {
      curr = split_token(&cp, sep);
      sum = sum * safe_atof(curr);
   }
   fval(buff, bufcx, sum);
}

FUNCTION(fun_ldiv)
{
   double sum, chksum;
   char *cp, *curr, sep;

   varargs_preamble("LDIV", 2);

   cp = trim_space_sep(fargs[0], sep);
   if (cp) {
      curr = split_token(&cp, sep);
      sum = safe_atof(curr);
   } else {
      sum = 0;
   }
   while (cp) {
      curr = split_token(&cp, sep);
      chksum = safe_atof(curr);
      if ( chksum != 0 )
         sum = sum / chksum;
   }
   fval(buff, bufcx, sum);
}

FUNCTION(fun_lor)
{
   int i;
   char *cp, *curr, sep;

   varargs_preamble("LOR", 2);

   i = 0;
   cp = trim_space_sep(fargs[0], sep);
   while (cp && !i) {
      curr = split_token(&cp, sep);
      i = atoi(curr);
   }
   ival(buff, bufcx, (i != 0));
}

FUNCTION(fun_land)
{
   int i;
   char *cp, *curr, sep;

   varargs_preamble("LAND", 2);

   i = 1;
   cp = trim_space_sep(fargs[0], sep);
   while (cp && i) {
      curr = split_token(&cp, sep);
      i = atoi(curr);
   }
   ival(buff, bufcx, (i != 0));
}

FUNCTION(fun_lxor)
{
   int val, i_val, cntr;
   char *cp, *curr, sep;

   varargs_preamble("LXOR", 2);

   cp = trim_space_sep(fargs[0], sep);
   val = 0;
   cntr = 0;
   while (cp) {
      curr = split_token(&cp, sep);
      i_val = atoi(curr);
      if (cntr > 0) {
         val = (val && !i_val) || (!val && i_val);
      } else {
         val = i_val;
         cntr++;
      }
   }
   ival(buff, bufcx, val);
}

FUNCTION(fun_lnor)
{
   int val, i_val, cntr;
   char *cp, *curr, sep;

   varargs_preamble("LNOR", 2);

   cp = trim_space_sep(fargs[0], sep);
   val = 0;
   cntr = 0;
   while (cp) {
      curr = split_token(&cp, sep);
      i_val = atoi(curr);
      if (cntr > 0) {
         val = !(val || i_val);
      } else {
         val = i_val;
         cntr++;
      }
   }
   ival(buff, bufcx, val);
}

FUNCTION(fun_lxnor)
{
   int val, i_val, cntr;
   char *cp, *curr, sep;

   varargs_preamble("LXNOR", 2);

   cp = trim_space_sep(fargs[0], sep);
   val = 0;
   cntr = 0;
   while (cp) {
      curr = split_token(&cp, sep);
      i_val = atoi(curr);
      if (cntr > 0) {
         val = !((val && !i_val) || (!val && i_val));
      } else {
         val = i_val;
         cntr++;
      }
   }
   ival(buff, bufcx, val);
}

FUNCTION(fun_lmax)
{
   double max, val;
   char *cp, *curr, sep;

   varargs_preamble("LMAX", 2);

   cp = trim_space_sep(fargs[0], sep);
   if (cp) {
      curr = split_token(&cp, sep);
      max = safe_atof(curr);
      while (cp) {
         curr = split_token(&cp, sep);
         val = safe_atof(curr);
         if ( max < val )
            max = val;
      }
      fval(buff, bufcx, max);
   }
}

FUNCTION(fun_lmin)
{
   double min, val;
   char *cp, *curr, sep;

   varargs_preamble("LMIN", 2);

   cp = trim_space_sep(fargs[0], sep);
   if (cp) {
      curr = split_token(&cp, sep);
      min = safe_atof(curr);
      while (cp) {
         curr = split_token(&cp, sep);
         val = safe_atof(curr);
         if ( min > val)
            min = val;
      }
      fval(buff, bufcx, min);
   }
}

/* ---------------------------------------------------------------------------
 * fun_filter: iteratively perform a function with a list of arguments
 *              and return the arg, if the function evaluates to TRUE using the
 *      arg.
 *
 *      > &IS_ODD object=mod(%0,2)
 *      > say filter(object/is_odd,1 2 3 4 5)
 *      You say "1 3 5"
 *      > say filter(object/is_odd,1-2-3-4-5,-)
 *      You say "1-3-5"
 *
 *  NOTE:  If you specify a separator it is used to delimit returned list
 */

FUNCTION(fun_filter)
{
    dbref aowner, thing;
    int aflags, anum, first, dynargs, i, tval;
    ATTR *ap;
    char *atext, *result, *curr, *cp, *atextbuf, sep, osep;
    char *s_dynargs[MAX_ARGS+1];

    dynargs = 1;
    if (!fn_range_check("FILTER", nfargs, 2, MAX_ARGS, buff, bufcx))
       return;

    if ( (nfargs > 2) && *fargs[2] )
       sep = *fargs[2];
    else
       sep = ' ';

    if ( (nfargs > 3) && *fargs[3] )
       osep = *fargs[3];
    else
       osep = sep;

    for ( i = 0; i < (MAX_ARGS+1); i++ )
        s_dynargs[i] = NULL;

    if ( nfargs > 4 ) {
       for ( i = 1; i <= (nfargs - 4); i++ ) {
          s_dynargs[i] = fargs[3 + i];
          dynargs++;
       }
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)) || Going(thing) || Recover(thing))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
       thing = player;
       ap = atr_str(fargs[0]);
    }

    /* Make sure we got a good attribute */

    if (!ap) {
       return;
    }
    /* Use it if we can access it, otherwise return an error. */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
       return;
    } else if (!*atext || !See_attr(player, thing, ap, aowner, aflags, 0)) {
       free_lbuf(atext);
       return;
    }
    /* Now iteratively eval the attrib with the argument list */

    cp = curr = trim_space_sep(fargs[1], sep);
    atextbuf = alloc_lbuf("fun_filter");
    first = 1;
    if ( nfargs == 3 )
       osep = sep;
    while (cp) {
        s_dynargs[0] = split_token(&cp, sep);
        strcpy(atextbuf, atext);
        if ( (mudconf.secure_functions & 8) ) {
           tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(thing, cause, caller,
                            EV_STRIP | EV_FCHECK | EV_EVAL, atextbuf, s_dynargs, dynargs);
           }
        } else {
           tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(player, cause, caller,
                            EV_STRIP | EV_FCHECK | EV_EVAL, atextbuf, s_dynargs, dynargs);
           }
        }
        safer_unufun(tval);
        if ( !first && ((*result == '1') && (strlen(result) < 2)) )
            safe_chr(osep, buff, bufcx);
        if ( ((*result == '1') && (strlen(result) < 2)) ) {
            safe_str(s_dynargs[0], buff, bufcx);
            first = 0;
        }
        free_lbuf(result);
    }
    free_lbuf(atext);
    free_lbuf(atextbuf);
}

FUNCTION(fun_parsestr)
{
   int first, i_type, i_cntr, i_pass, aflags;
   dbref aname, target;
   char *atext, *atextptr, *result, *objstring, *cp, *atextbuf, sep, *osep, *osep_pre, *osep_post, *tbuff;
   char *savebuff[5], *ansibuf, *target_result, *c_transform, *p_transform, *tpr_buff, *tprp_buff, *tprstack[3];

   if (!fn_range_check("PARSESTR", nfargs, 2, 10, buff, bufcx))
     return;

   if ( !*fargs[0] )
     return;

   if ( !*fargs[1] ) {
     safe_str(fargs[0], buff, bufcx);
     return;
   }

   atextbuf = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                   fargs[0], cargs, ncargs);
   if ( !*atextbuf ) {
     free_lbuf(atextbuf);
     return;
   }
   atextptr = atext = alloc_lbuf("parsestr_buffer");
   safe_str(strip_ansi(atextbuf), atext, &atextptr);
   free_lbuf(atextbuf);

   if ( (nfargs >= 3) && *fargs[2] ) {
     tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                  fargs[2], cargs, ncargs);
     sep = *tbuff;
     free_lbuf(tbuff);
   } else {
     sep = ' ';
   }

   osep_pre = alloc_lbuf("osep_pre");
   osep_post = alloc_lbuf("osep_post");
   if ( (nfargs >= 4) && *fargs[3] ) {
     osep = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                 fargs[3], cargs, ncargs);
     if ( (*osep != ' ') && (strchr(osep, ' ') != NULL) )  {
        tbuff = osep;
        result = osep_pre;
        while ( *tbuff && (*tbuff != ' ')) {
           *result = *tbuff;
           tbuff++;
           result++;
        }  
        if ( *tbuff ) 
           tbuff++;
        if ( *tbuff ) 
           strcpy(osep_post, tbuff);
     } else {
        strcpy(osep_pre, osep);
        strcpy(osep_post, osep);
     }
   } else {
     osep = alloc_lbuf("osep_buff");
     sprintf(osep, "%c", sep);
     strcpy(osep_pre, osep);
     strcpy(osep_post, osep);
   }

   if ( (nfargs >= 5) && *fargs[4] ) {
     tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                  fargs[4], cargs, ncargs);
     i_type = atoi(tbuff);
     free_lbuf(tbuff);
     i_type = (i_type > 0 ? 1 : 0);
   } else
     i_type = 0;

   target = NOTHING;
   target_result = NULL;
   if ( (nfargs >= 6) && *fargs[5] ) {
      target_result = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                  fargs[5], cargs, ncargs);
      if ( *target_result != '&' ) {
         target = match_thing(player, target_result);
         free_lbuf(target_result);
         if ( !Good_chk(target) ) {
            notify(player, "Permission denied.");
            free_lbuf(atext);
            free_lbuf(osep);
            free_lbuf(osep_pre);
            free_lbuf(osep_post);
            return;
         }
         ansibuf = atr_pget(target, A_ANSINAME, &aname, &aflags);
         if ( ExtAnsi(target) ) {
            target_result = alloc_lbuf("target_result");
            memset(target_result, '\0', LBUF_SIZE);
            if ( ansibuf && *ansibuf ) {
               memcpy(target_result, ansibuf, LBUF_SIZE - 2);
            } else {
               sprintf(target_result, "%s", Name(target));
            }
         } else {
            if ( ansibuf && *ansibuf ) {
               target_result = parse_ansi_name(target, ansibuf);
            } else {
               target_result = alloc_lbuf("target_result");
               sprintf(target_result, "%s", Name(target));
            }
         }
         free_lbuf(ansibuf);
      }
   }
   
   c_transform = NULL;
   p_transform = NULL;

   if ( (nfargs >= 7) && *fargs[6] ) {
      c_transform = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                  fargs[6], cargs, ncargs);
   }
   if ( (nfargs >= 8) && *fargs[7] ) {
      p_transform = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                  fargs[7], cargs, ncargs);
   }
   
   if ( !((nfargs >=9) && *fargs[8]) ) {
      if ( c_transform && *c_transform ) {
         free_lbuf(c_transform);
         c_transform = NULL;
      }
      if ( p_transform && *p_transform ) {
         free_lbuf(p_transform);
         p_transform = NULL;
      }
   }
   savebuff[0] = alloc_lbuf("fun_parsestr_savebuff0");
   savebuff[1] = alloc_lbuf("fun_parsestr_savebuff1");
   savebuff[2] = alloc_lbuf("fun_parsestr_savebuff2");
   savebuff[3] = alloc_lbuf("fun_parsestr_savebuff3");
   savebuff[4] = alloc_lbuf("fun_parsestr_savebuff4");
   if ( target_result && *target_result ) {
      memset(savebuff[1], '\0', LBUF_SIZE);
      if ( *target_result == '&' )
         memcpy(savebuff[1], target_result+1, LBUF_SIZE-2);
      else
         memcpy(savebuff[1], target_result, LBUF_SIZE-2);
   }
   if ( target_result )
      free_lbuf(target_result);

   tbuff = atextbuf = alloc_lbuf("fun_parsestr");
   if ( *savebuff[1] ) {
     memset(savebuff[4], '\0', LBUF_SIZE);
     if ( c_transform && *c_transform && (strchr(c_transform, *atext) != NULL) ) {
        *savebuff[4]=*atext;
     } else if ( p_transform && *p_transform && (strchr(p_transform, *(atext+(strlen(atext)-1))) != NULL) ) {
        *savebuff[4]=*(atext+(strlen(atext)-1));
     }
     if ( *atext == '"' ) {
        safe_str(savebuff[1], atextbuf, &tbuff);
        safe_chr(' ', atextbuf, &tbuff);
        if ( (nfargs >=10) && *fargs[9] ) {
           safe_str(fargs[9], atextbuf, &tbuff);
           safe_chr(' ', atextbuf, &tbuff);
        } else {
           safe_str("says", atextbuf, &tbuff);
           safe_str(", ", atextbuf, &tbuff);
        }
        safe_str(atext, atextbuf, &tbuff);
        /* Let's count the '"' chars, if it's odd, add one to the end if last char is not a '"' and not escaped out */
        cp = atext;
        first = 0;
        i_pass = 0;
        while ( *cp ) {
           if ( (*cp == '\\') || (*cp == '%') ) {
              if ( !i_pass )
                 i_pass = 1;
              else
                 i_pass = 0;
           }
           if ( (*cp == '"') && !i_pass )
              first++;
           cp++;
        }     
        if ( (first % 2) == 1)
           safe_chr('"', atextbuf, &tbuff);
     } else if ( ((*atext == ':') && (*(atext+1) != ' ')) || ((*atext == ';') && (*(atext+1) == ' ')) ) {
        safe_str(savebuff[1], atextbuf, &tbuff);
        safe_chr(' ', atextbuf, &tbuff);
        safe_str(atext+1, atextbuf, &tbuff);
     } else if ( (*atext == ';') || ((*atext == ':') && (*(atext+1) == ' ')) ) {
        safe_str(savebuff[1], atextbuf, &tbuff);
        if ( *(atext) == ':')
           safe_str(atext+2, atextbuf, &tbuff);
        else
           safe_str(atext+1, atextbuf, &tbuff);
     } else if ( *atext == '|' ) {
        safe_str(atext+1, atextbuf, &tbuff);
     } else {
        safe_str(savebuff[1], atextbuf, &tbuff);
        safe_chr(' ', atextbuf, &tbuff);
        if ( (nfargs >=10) && *fargs[9] ) {
           safe_str(fargs[9], atextbuf, &tbuff);
           safe_chr(' ', atextbuf, &tbuff);
        } else {
           safe_str("says", atextbuf, &tbuff);
           safe_str(", ", atextbuf, &tbuff);
        }
        safe_chr('"', atextbuf, &tbuff);
        safe_str(atext, atextbuf, &tbuff);
        safe_chr('"', atextbuf, &tbuff);
     }
     strcpy(atext, atextbuf);  
   }
   i_cntr = i_pass = 0;
   first = 0;
   memset(atextbuf, '\0', LBUF_SIZE);
   cp = trim_space_sep(atext, sep);
   tprp_buff = tpr_buff = alloc_lbuf("parestr_escape_out");
   tprstack[0] = alloc_lbuf("parsestr_stack");
   tprstack[1] = alloc_lbuf("parsestr_stack2");
   sprintf(tprstack[1], "%s", "f");
   tprstack[2] = NULL;
   while ( cp ) {
     i_cntr++;
     objstring = split_token(&cp, sep);
     if ( (!i_type && ((i_cntr % 2) != 0)) ||
          ( i_type && ((i_cntr % 2) == 0)) ) {
       if ( first )
         safe_str(osep_post, buff, bufcx);
       if ( !first ) {
          memset(savebuff[0], '\0', LBUF_SIZE);
          sprintf(savebuff[2], "%d", i_pass - 1);
          sprintf(savebuff[3], "%d", i_cntr - 1);
/*        strncpy(atextbuf, objstring, (LBUF_SIZE-1)); */

          strncpy(tprstack[0], objstring, (LBUF_SIZE-1));
          tprp_buff = tpr_buff;
          fun_escape(tpr_buff, &tprp_buff, player, cause, cause, tprstack, 2, (char **)NULL, 0);
          result = exec(player, cause, caller,
                        EV_STRIP | EV_FCHECK | EV_EVAL, tpr_buff, savebuff, 5); 

/*        result = exec(player, cause, caller,
                        EV_STRIP | EV_FCHECK | EV_EVAL, atextbuf, savebuff, 5);  */
          safe_str(result, buff, bufcx);
          safe_chr(' ', buff, bufcx);
          free_lbuf(result);
       } else {
          safe_str(objstring, buff, bufcx);
       }
       first = 1;
       continue;
     }
     i_pass++;
     memset(savebuff[0], '\0', LBUF_SIZE);
     memset(savebuff[4], '\0', LBUF_SIZE);
     memcpy(savebuff[0], objstring, LBUF_SIZE-2);
     sprintf(savebuff[2], "%d", i_pass - 1);
     sprintf(savebuff[3], "%d", i_cntr - 1);
     if ( c_transform && *c_transform && (strchr(c_transform, *objstring) != NULL) ) {
        strncpy(atextbuf, fargs[8], (LBUF_SIZE-1));
        *savebuff[4]=*objstring;
     } else if ( p_transform && *p_transform && (strchr(p_transform, *(objstring+(strlen(objstring)-1))) != NULL) ) {
        strncpy(atextbuf, fargs[8], (LBUF_SIZE-1));
        *savebuff[4]=*(objstring+(strlen(objstring)-1));
     } else {
        strncpy(atextbuf, fargs[1], (LBUF_SIZE-1));
     }
     *(atextbuf + LBUF_SIZE - 1) = '\0';
     result = exec(player, cause, caller,
                   EV_STRIP | EV_FCHECK | EV_EVAL, atextbuf, savebuff, 5);
     if ( first )
       safe_str(osep_pre, buff, bufcx);
     first = 1;
     safe_str(result, buff, bufcx);
     free_lbuf(result);
   }
   free_lbuf(tpr_buff);
   free_lbuf(tprstack[0]);
   free_lbuf(tprstack[1]);
   if ( c_transform )
      free_lbuf(c_transform);
   if ( p_transform )
      free_lbuf(p_transform);
   free_lbuf(savebuff[0]);
   free_lbuf(savebuff[1]);
   free_lbuf(savebuff[2]);
   free_lbuf(savebuff[3]);
   free_lbuf(savebuff[4]);
   free_lbuf(atextbuf);
   free_lbuf(atext);
   free_lbuf(osep);
   free_lbuf(osep_pre);
   free_lbuf(osep_post);
}

/* ---------------------------------------------------------------------------
 * fun_map: iteratively evaluate an attribute with a list of arguments.
 *
 *  > &DIV_TWO object=fdiv(%0,2)
 *  > say map(1 2 3 4 5,object/div_two)
 *  You say "0.5 1 1.5 2 2.5"
 *  > say map(object/div_two,1-2-3-4-5,-)
 *  You say "0.5-1-1.5-2-2.5"
 *
 */

FUNCTION(fun_map)
{
    dbref aowner, thing;
    int aflags, anum, first, i, dynargs, tval;
    ATTR *ap;
    char *atext, *result, *cp, *atextbuf, sep, *osep;
    char *s_dynargs[MAX_ARGS+1];

    dynargs = 1;
    if (!fn_range_check("MAP", nfargs, 2, MAX_ARGS, buff, bufcx))
       return;

    if ( (nfargs > 2) && *fargs[2] )
       sep = *fargs[2];
    else
       sep = ' ';

    osep = alloc_lbuf("fun_map_osep");
    if ( (nfargs > 3) ) {
       if ( *fargs[3] )
          strcpy(osep, fargs[3]);
       else {
          if ( mudconf.map_delim_space ) {
             sprintf(osep, "%c", ' ');
          } else {
             memset(osep, '\0', LBUF_SIZE);
          }
       }
    } else {
       sprintf(osep, "%c", sep);
    }

    for ( i = 0; i < (MAX_ARGS+1); i++ )
        s_dynargs[i] = NULL;

    if ( nfargs > 4 ) {
       for ( i = 1; i <= (nfargs - 4); i++ ) {
          s_dynargs[i] = fargs[3 + i];
          dynargs++;
       }
    }
    /* Two possibilities for the second arg: <obj>/<attr> and <attr>. */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)) || Recover(thing) || Going(thing))
          ap = NULL;
       else
          ap = atr_num(anum);
    } else {
        thing = player;
        ap = atr_str(fargs[0]);
    }

    /* Make sure we got a good attribute */

    if (!ap) {
       free_lbuf(osep);
       return;
    }
    /* Use it if we can access it, otherwise return an error. */

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
       free_lbuf(osep);
       return;
    } else if (!*atext || !See_attr(player, thing, ap, aowner, aflags, 0)) {
       free_lbuf(atext);
       free_lbuf(osep);
       return;
    }
    /* now process the list one element at a time */

    cp = trim_space_sep(fargs[1], sep);
    atextbuf = alloc_lbuf("fun_map");
    first = 1;
    while (cp) {
        s_dynargs[0] = split_token(&cp, sep);
        strcpy(atextbuf, atext);
        if ( (mudconf.secure_functions & 16) ) {
           tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(thing, cause, caller,
                            EV_STRIP | EV_FCHECK | EV_EVAL, atextbuf, s_dynargs, dynargs);
           }
        } else {
           tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(player, cause, caller,
                            EV_STRIP | EV_FCHECK | EV_EVAL, atextbuf, s_dynargs, dynargs);
           }
        }
        safer_unufun(tval);
        if (!first && *osep)
           safe_str(osep, buff, bufcx);
        first = 0;
        safe_str(result, buff, bufcx);
        free_lbuf(result);
    }
    free_lbuf(atext);
    free_lbuf(atextbuf);
    free_lbuf(osep);
}

/* ---------------------------------------------------------------------------
 * fun_edit: Edit text.
 */

FUNCTION(fun_edit)
{
    char *tstr;
    int i_editkey, i_compat;

    if (!fn_range_check("EDIT", nfargs, 3, 5, buff, bufcx))
       return;

    i_editkey = i_compat = 0;
    if ( (nfargs > 3) && *fargs[3] )
       i_editkey = atoi(fargs[3]);
    if ( (nfargs > 4) && *fargs[4] )
       i_compat = atoi(fargs[4]);
    if ( i_editkey != 0 )
       i_editkey = 1;
    if ( (i_compat < 0) || (i_compat > 2) )
       i_compat = 1;

    /* The '1' specifies not to alloc second pointer so not needed
     * If you use '0' you need to alloc and define another char pointer
     */
    edit_string(fargs[0], &tstr, (char **)NULL, fargs[1], fargs[2], 1, i_editkey, i_compat);
    safe_str(tstr, buff, bufcx);
    free_lbuf(tstr);
}

/* ---------------------------------------------------------------------------
 * fun_pedit: Edit text - multi ala penn.
 */
FUNCTION(fun_pedit)
{
  int len;
  char *str, *f, *r;
  int i;
  char *prebuf, *prep, *postbuf, *postp;

  if ( nfargs < 3 ) {
     safe_str("#-1 FUNCTION (PEDIT) EXPECTS 3 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
     ival(buff, bufcx, nfargs);
     safe_chr(']', buff, bufcx);
     return;
  }
  postp = postbuf = alloc_lbuf("fun_pedit");
  prep = prebuf = alloc_lbuf("fun_pedit2");
  safe_str(fargs[0], postbuf, &postp);
  for (i = 1; i < nfargs - 1; i += 2) {
    /* Old postbuf is new prebuf */
    prep = prebuf;
    safe_str(postbuf, prebuf, &prep);
    str = prebuf;   /* The current string */
    f = fargs[i];   /* find this */
    r = fargs[i + 1];   /* replace it with this */
    postp = postbuf;

    /* Check for nothing to avoid infinite loop */
    if (!*f && !*r)
      continue;

    if (!strcmp(f, "$")) {
      /* append */
      safe_str(str, postbuf, &postp);
      safe_str(r, postbuf, &postp);
    } else if (!strcmp(f, "^")) {
      /* prepend */
      safe_str(r, postbuf, &postp);
      safe_str(str, postbuf, &postp);
    } else {
      len = strlen(f);
      while (*str) {
         if (!strncmp(str, f, len)) {
           safe_str(r, postbuf, &postp);
           if (len)
             str += len;
           else {
             safe_chr(*str, postbuf, &postp);
             str++;
           }
         } else {
           safe_chr(*str, postbuf, &postp);
           str++;
         }
      }
      if (!*f)
         safe_str(r, postbuf, &postp);
    }
  }
  safe_str(postbuf, buff, bufcx);
  free_lbuf(postbuf);
  free_lbuf(prebuf);
}

/* ---------------------------------------------------------------------------
 * fun_step: A little like a fusion of iter() and mix(), it takes elements
 * of a list X at a time and passes them into a single function as %0, %1,
 * etc.   step(<attribute>,<list>,<step size>,<delim>,<outdelim>)
 *
 * Function taken from TinyMUSH 3.0 for compatibility.
 * Function modified for use with RhostMUSH compatibility.
 */

FUNCTION(fun_step)
{
    ATTR *ap;
    dbref aowner, thing;
    int aflags, anum, step_size, i, tval;
    char *atext, *str, *cp, *atextbuf, *bb_p, *os[10], *result,
         sep, osep, *tpr_buff, *tprp_buff;

    svarargs_preamble("STEP", 5);

    step_size = atoi(fargs[2]);
    if ((step_size < 1) || (step_size > NUM_ENV_VARS)) {
        tprp_buff = tpr_buff = alloc_lbuf("fun_step");
        safe_str((char *)safe_tprintf(tpr_buff, &tprp_buff, "#-1 STEP SIZE MUST BE BETWEEN 1 AND %d",
                                      NUM_ENV_VARS), buff, bufcx);
        free_lbuf(tpr_buff);
        return;
    }

    /* Get attribute. Check permissions. */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
        if ((anum == NOTHING) || !Good_obj(thing) || Recover(thing) || Going(thing))
            ap = NULL;
        else
            ap = atr_num(anum);
    } else {
        thing = player;
        ap = atr_str(fargs[0]);
    }
    if (!ap)
        return;

    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext)
        return;
    if (!*atext || !See_attr(player, thing, ap, aowner, aflags, 0)) {
        free_lbuf(atext);
        return;
    }

    cp = trim_space_sep(fargs[1], sep);
    atextbuf = alloc_lbuf("fun_step");
    bb_p = *bufcx;
    while (cp) {
        if (*bufcx != bb_p) {
            safe_chr(osep, buff, bufcx);
        }
        for (i = 0; cp && (i < step_size); i++)
            os[i] = split_token(&cp, sep);
        strcpy(atextbuf, atext);
        str = atextbuf;
        if ( (mudconf.secure_functions & 32) ) {
           tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(thing, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            str, &(os[0]), i);
           }
        } else {
           tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            str, &(os[0]), i);
           }
        }
        safer_unufun(tval);
        safe_str(result, buff, bufcx);
        free_lbuf(result);
    }
    free_lbuf(atext);
    free_lbuf(atextbuf);
}


/* ---------------------------------------------------------------------------
 * fun_mix: Takes up to 10 arguments and passes the elements on like map
 */

FUNCTION(fun_mix)
{
    dbref aowner, thing;
    int aflags, anum, i, lastn, nwords, twords, wc, first, tval;
    ATTR *ap;
    char *atext, *result, *os[10], *atextbuf, sep;
    char *cp[10];

    /* Check to see if we have an appropriate number of arguments.
     * If there are more than three arguments, the last argument is
     * ALWAYS assumed to be a delimiter.
     */

    if (!fn_range_check("MIX", nfargs, 3, 10, buff, bufcx)) {
        return;
    }
    if (nfargs < 4) {
        sep = ' ';
        lastn = nfargs - 1;
    } else if (!delim_check(fargs, nfargs, nfargs, &sep, buff, bufcx, 0,
                            player, cause, caller, cargs, ncargs)) {
        return;
    } else {
        lastn = nfargs - 2;
    }

    /* Get the attribute, check the permissions. */

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
        if ((anum == NOTHING) || !Good_obj(thing) || Recover(thing) || Going(thing))
            ap = NULL;
        else
            ap = atr_num(anum);
    } else {
        thing = player;
        ap = atr_str(fargs[0]);
    }

    if (!ap) {
        safe_chr('\0', buff, bufcx);
        return;
    }
    atext = atr_pget(thing, ap->number, &aowner, &aflags);
    if (!atext) {
        safe_chr('\0', buff, bufcx);
        return;
    } else if (!*atext || !See_attr(player, thing, ap, aowner, aflags, 0)) {
        free_lbuf(atext);
        safe_chr('\0', buff, bufcx);
        return;
    }

    for (i = 0; i < 10; i++)
        cp[i] = NULL;

    /* process the lists, one element at a time. */

    for (i = 1; i <= lastn; i++) {
        cp[i] = trim_space_sep(fargs[i], sep);
    }
    nwords = countwords(cp[1], sep);
    for (i = 2; i<= lastn; i++) {
        twords = countwords(cp[i], sep);
        if ( twords > nwords )
           nwords = twords;
    }
    atextbuf = alloc_lbuf("fun_mix");

    first=0;
    for (wc = 0; wc < nwords; wc++) {
        for (i = 1; i <= lastn; i++) {
            os[i - 1] = split_token(&cp[i], sep);
        }
        strcpy(atextbuf, atext);
        if ( (mudconf.secure_functions & 64) ) {
           tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(thing, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            atextbuf, &(os[0]), lastn);
           }
        } else {
           tval = safer_ufun(player, thing, player, (ap ? ap->flags : 0), aflags);
           if ( tval == -2 ) {
              result = alloc_lbuf("edefault_buff");
              sprintf(result ,"#-1 PERMISSION DENIED");
           } else {
              result = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            atextbuf, &(os[0]), lastn);
           }
        }
        safer_unufun(tval);
        if (first)
            safe_chr(sep, buff, bufcx);
        safe_str(result, buff, bufcx);
        free_lbuf(result);
        first=1;
    }
    free_lbuf(atext);
    free_lbuf(atextbuf);
}

/* ---------------------------------------------------------------------------
 * fun_locate: Search for things with the perspective of another obj.
 */

FUNCTION(fun_locate)
{
    int pref_type, check_locks, verbose, multiple;
    dbref thing, what;
    char *cp;

    pref_type = NOTYPE;
    check_locks = verbose = multiple = 0;

    /* Find the thing to do the looking, make sure we control it. */

    if (Wizard(player))
       thing = match_thing(player, fargs[0]);
    else
       thing = match_controlled(player, fargs[0]);
    if( !Good_obj(thing) ||
        (Cloak(thing) && !Examinable(player, thing)) ||
        (Wizard(player) && !Immortal(player) &&
         Immortal(thing) && SCloak(thing))) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    /* Get pre- and post-conditions and modifiers */

    for (cp = fargs[2]; *cp; cp++) {
       switch (*cp) {
            case 'E':
                pref_type = TYPE_EXIT;
                break;
            case 'L':
                check_locks = 1;
                break;
            case 'P':
                pref_type = TYPE_PLAYER;
                break;
            case 'R':
                pref_type = TYPE_ROOM;
                break;
            case 'T':
                pref_type = TYPE_THING;
                break;
            case 'V':
                verbose = 1;
                break;
          #ifndef NO_ENH
            case 'X':
                multiple = 1;
                break;
          #endif
       }
    }

    /* Set up for the search */

    if (check_locks)
       init_match_check_keys(thing, fargs[1], pref_type);
    else
       init_match(thing, fargs[1], pref_type);

    /* Search for each requested thing */

    for (cp = fargs[2]; *cp; cp++) {
       switch (*cp) {
            case 'a':
                match_absolute();
                break;
            case 'c':
                match_carried_exit_with_parents();
                break;
            case 'e':
                match_exit_with_parents();
                break;
            case 'h':
                match_here();
                break;
            case 'i':
                match_possession();
                break;
            case 'm':
                match_me();
                break;
            case 'n':
                match_neighbor();
                break;
            case 'p':
                match_player();
                break;
            case '*':
                match_everything(MAT_EXIT_PARENTS);
                break;
       }
    }

    /* Get the result and return it to the caller */

    if (multiple)
       what = last_match_result();
    else
       what = match_result();

    if (verbose)
       (void) match_status(player, what);
    if ( Good_obj(what) && what != AMBIGUOUS && Wizard(what) && Cloak(what) && !Examinable(player, what) && !Jump_ok(what) && !Abode(what))
       what = -1;

    dbval(buff, bufcx, what);
}

/* ---------------------------------------------------------------------------
 * fun_squish: Strip excess spaces (or specified delimiter) from string
 */
FUNCTION(fun_squish)
{
   char *t_ptr, sep;

   if (nfargs == 0) {
      safe_chr('\0', buff, bufcx);
      return;
   }
   varargs_preamble("SQUISH", 2);
   t_ptr = fargs[0];
   while ( *t_ptr ) {
      while ( *t_ptr && (*t_ptr != sep) ) {
         safe_chr(*t_ptr, buff, bufcx);
         t_ptr++;
      }
      if ( !*t_ptr ) {
         safe_chr('\0', buff, bufcx);
         return;
      }
      safe_chr(*t_ptr, buff, bufcx);
      t_ptr++;
      while ( *t_ptr && (*t_ptr == sep) )
         t_ptr++;
   }
   safe_chr('\0', buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * fun_subnetmatch: Return value based on wether specified ip address is part
 * of the specified ip/CIDR or ip and netmask combination.
 */

FUNCTION(fun_subnetmatch)
{
    int range;
		struct in_addr ip_addr, ip_addr2, netmask;
    uint32_t maskval;
    char *ptr;
    int do_cidr;
    do_cidr=0;

    if((nfargs < 2) || (nfargs > 3))
    {
       safe_str("#-1 FUNCTION (SUBNETMATCH) EXPECTS 2 OR 3 ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    if(nfargs == 2) /* subnetmatch(ip,ip/CIDR) */
      do_cidr = 1;

    if(!inet_pton(AF_INET,fargs[0], &(ip_addr.s_addr))) /* First arg to IP */
    {
       safe_str("#-1 FIRST ARGUMENT NEEDS TO BE A PROPER IP ADDRESS", buff, bufcx);
       return;
    }

    if(do_cidr) /* Pick second arg apart, get IP, calculate netmask */
    {
      ptr = strtok(fargs[1],"/");
      if(!inet_pton(AF_INET,ptr, &(ip_addr2.s_addr)))
      {
        safe_str("#-1 SECOND ARGUMENT NEEDS TO BE 'IPADDRESS/BITS'", buff, bufcx);
        return;
      }
      ptr = strtok(NULL,"/");
      if(!ptr)
      {
        safe_str("#-1 SECOND ARGUMENT NEEDS TO BE 'IPADDRESS/BITS'", buff, bufcx);
        return;
      }
      range = atoi(ptr);
      if((range < 1) || (range > 32))
      {
        safe_str("#-1 CIDR BITS NEED TO BE A VALUE BETWEEN 1 AND 32", buff, bufcx); 
        return;
      }
      maskval = (0xFFFFFFFFUL << (32 - range));
      netmask.s_addr = htonl(maskval); 
    }
    else /* Just straight up get second arg IP and third arg netmask */
    {
      if(!inet_pton(AF_INET,fargs[1], &(ip_addr2.s_addr)))
      {
        safe_str("#-1 SECOND ARGUMENT NEEDS TO BE A PROPER IP ADDRESS", buff, bufcx);
        return;
      }
      if(!inet_pton(AF_INET,fargs[2], &(netmask.s_addr)))
      {
        safe_str("#-1 THIRD ARGUMENT NEEDS TO BE A PROPER IP ADDRESS", buff, bufcx);
        return;
      }
    }
    // We have two IPs, we have the netmask. Let's match.
       if((ip_addr.s_addr & netmask.s_addr) == ip_addr2.s_addr)
         safe_str("1", buff, bufcx);
       else
         safe_str("0", buff, bufcx);
       return;
}

/* ---------------------------------------------------------------------------
 * fun_switchall: Return value based on pattern matching (ala @switch/all)
 * NOTE: This function expects that its arguments have not been evaluated.
 */

FUNCTION(fun_switchall)
{
    int i, foundval, m_returnval;
    char *mbuff, *tbuff, *retbuff;

    /* If we don't have at least 2 args, return nothing */

    if (nfargs < 2) {
       return;
    }
    /* Evaluate the target in fargs[0] */

    mbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                    fargs[0], cargs, ncargs);

    /* Loop through the patterns looking for a match */

    foundval = 0;
    m_returnval = 0;
    retbuff = NULL;
    for (i = 1; (i < nfargs - 1) && fargs[i] && fargs[i + 1]; i += 2) {
        tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                     fargs[i], cargs, ncargs);
        if ( mudconf.penn_switches )
           m_returnval = wild_match(tbuff, mbuff, (char **)NULL, 0, 1);
        else
           m_returnval = quick_wild(tbuff, mbuff);
        if ( m_returnval ) {
            free_lbuf(tbuff);
            if ( mudconf.switch_substitutions ) {
               retbuff = replace_tokens(fargs[i + 1], NULL, NULL, mbuff);
               tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                retbuff, cargs, ncargs);
               free_lbuf(retbuff);
            } else {
               tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            fargs[i + 1], cargs, ncargs);
            }
            safe_str(tbuff, buff, bufcx);
            foundval = 1;
        }
        free_lbuf(tbuff);
    }
    if ( foundval ) {
       free_lbuf(mbuff);
       return;
    }

    /* Nope, return the default if there is one */

    if ((i < nfargs) && fargs[i]) {
        if ( mudconf.switch_substitutions ) {
           retbuff = replace_tokens(fargs[i], NULL, NULL, mbuff);
           tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                        retbuff, cargs, ncargs);
           free_lbuf(retbuff);
        } else {
           tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                        fargs[i], cargs, ncargs);
        }
        safe_str(tbuff, buff, bufcx);
        free_lbuf(tbuff);
    }
    free_lbuf(mbuff);
    return;
}

/* ---------------------------------------------------------------------------
 * fun_switch: Return value based on pattern matching (ala @switch)
 * NOTE: This function expects that its arguments have not been evaluated.
 */

FUNCTION(fun_switch)
{
    int i, m_returnval;
    char *mbuff, *tbuff, *retbuff;

    /* If we don't have at least 2 args, return nothing */

    if (nfargs < 2) {
        return;
    }
    /* Evaluate the target in fargs[0] */

    mbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                 fargs[0], cargs, ncargs);

    /* Loop through the patterns looking for a match */

    m_returnval = 0;
    retbuff = NULL;
    for (i = 1; (i < nfargs - 1) && fargs[i] && fargs[i + 1]; i += 2) {
        tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                     fargs[i], cargs, ncargs);
        if ( mudconf.penn_switches )
           m_returnval = wild_match(tbuff, mbuff, (char **)NULL, 0, 1);
        else
           m_returnval = quick_wild(tbuff, mbuff);
        if ( m_returnval ) {
            free_lbuf(tbuff);
            if ( mudconf.switch_substitutions ) {
               retbuff = replace_tokens(fargs[i + 1], NULL, NULL, mbuff);
               tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            retbuff, cargs, ncargs);
               free_lbuf(retbuff);
            } else {
               tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            fargs[i + 1], cargs, ncargs);
            }
            safe_str(tbuff, buff, bufcx);
            free_lbuf(mbuff);
            free_lbuf(tbuff);
            return;
        }
        free_lbuf(tbuff);
    }

    /* Nope, return the default if there is one */

    if ((i < nfargs) && fargs[i]) {
        if ( mudconf.switch_substitutions ) {
           retbuff = replace_tokens(fargs[i], NULL, NULL, mbuff);
           tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                        retbuff, cargs, ncargs);
           free_lbuf(retbuff);
        } else {
           tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                        fargs[i], cargs, ncargs);
        }
        safe_str(tbuff, buff, bufcx);
        free_lbuf(tbuff);
    }
    free_lbuf(mbuff);
    return;
}

FUNCTION(fun_case)
{
    int i;
    char *mbuff, *tbuff, *retbuff;

    /* If we don't have at least 2 args, return nothing */

    if (nfargs < 2) {
        return;
    }
    /* Evaluate the target in fargs[0] */

    mbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                 fargs[0], cargs, ncargs);

    /* Loop through the patterns looking for a match */

    for (i = 1; (i < nfargs - 1) && fargs[i] && fargs[i + 1]; i += 2) {
        tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                     fargs[i], cargs, ncargs);
        if (!strcmp(tbuff, mbuff)) {
            free_lbuf(tbuff);
            if ( mudconf.switch_substitutions ) {
               retbuff = replace_tokens(fargs[i + 1], NULL, NULL, mbuff);
               tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            retbuff, cargs, ncargs);
               free_lbuf(retbuff);
            } else {
               tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            fargs[i + 1], cargs, ncargs);
            }
            safe_str(tbuff, buff, bufcx);
            free_lbuf(mbuff);
            free_lbuf(tbuff);
            return;
        }
        free_lbuf(tbuff);
    }

    /* Nope, return the default if there is one */

    if ((i < nfargs) && fargs[i]) {
        if ( mudconf.switch_substitutions ) {
           retbuff = replace_tokens(fargs[i], NULL, NULL, mbuff);
           tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                        retbuff, cargs, ncargs);
           free_lbuf(retbuff);
        } else {
           tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                        fargs[i], cargs, ncargs);
        }
        safe_str(tbuff, buff, bufcx);
        free_lbuf(tbuff);
    }
    free_lbuf(mbuff);
    return;
}

FUNCTION(fun_caseall)
{
    int i, foundcase;
    char *mbuff, *tbuff, *retbuff;

    /* If we don't have at least 2 args, return nothing */

    if (nfargs < 2) {
        return;
    }
    /* Evaluate the target in fargs[0] */

    mbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                 fargs[0], cargs, ncargs);

    /* Loop through the patterns looking for a match */

    foundcase = 0;
    for (i = 1; (i < nfargs - 1) && fargs[i] && fargs[i + 1]; i += 2) {
        tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                     fargs[i], cargs, ncargs);
        if (!strcmp(tbuff, mbuff)) {
            free_lbuf(tbuff);
            if ( mudconf.switch_substitutions ) {
               retbuff = replace_tokens(fargs[i + 1], NULL, NULL, mbuff);
               tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            retbuff, cargs, ncargs);
               free_lbuf(retbuff);
            } else {
               tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                            fargs[i + 1], cargs, ncargs);
            }
            safe_str(tbuff, buff, bufcx);
            foundcase=1;
        }
        free_lbuf(tbuff);
    }
    if ( foundcase ) {
       free_lbuf(mbuff);
       return;
    }

    /* Nope, return the default if there is one */

    if ((i < nfargs) && fargs[i]) {
        if ( mudconf.switch_substitutions ) {
           retbuff = replace_tokens(fargs[i], NULL, NULL, mbuff);
           tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                        retbuff, cargs, ncargs);
           free_lbuf(retbuff);
        } else {
           tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                        fargs[i], cargs, ncargs);
        }
        safe_str(tbuff, buff, bufcx);
        free_lbuf(tbuff);
    }
    free_lbuf(mbuff);
    return;
}

FUNCTION(fun_ifelse)
{
    char *mbuff, *tbuff, *retbuff;

    /* If we don't have at least 2 args, return nothing */

    if (!fn_range_check("IFELSE", nfargs, 2, 3, buff, bufcx)) {
        return;
    }
    /* Evaluate the target in fargs[0] */

    mbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                 fargs[0], cargs, ncargs);
    retbuff = NULL;
    if (atoi(mbuff)) {
        if ( mudconf.ifelse_substitutions ) {
           retbuff = replace_tokens(fargs[1], NULL, NULL, mbuff);
           tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                        retbuff, cargs, ncargs);
           free_lbuf(retbuff);
        } else {
           tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                        fargs[1], cargs, ncargs);
        }
        safe_str(tbuff, buff, bufcx);
        free_lbuf(tbuff);
    } else if (nfargs == 3) {
        if ( mudconf.ifelse_substitutions ) {
           retbuff = replace_tokens(fargs[2], NULL, NULL, mbuff);
           tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                        retbuff, cargs, ncargs);
           free_lbuf(retbuff);
        } else {
           tbuff = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                        fargs[2], cargs, ncargs);
        }
        safe_str(tbuff, buff, bufcx);
        free_lbuf(tbuff);
    }
    free_lbuf(mbuff);
    return;
}

FUNCTION(fun_beep)
{
    char tbuf1[10];
    int i, k;

    /* this function prints 1 to 5 beeps. The alert character '\a' is
     * an ANSI C invention; non-ANSI-compliant implementations may ignore
     * the '\' character and just print an 'a', or do something else nasty,
     * so we define it to be something reasonable in rhost_ansi.h.
     */

    if (fargs[0] && *fargs[0])
       k = atoi(fargs[0]);
    else
       k = 1;

    if ((k <= 0) || (k > 5)) {
       safe_str("#-1 PERMISSION DENIED", buff, bufcx);
       return;
    }
    for (i = 0; i < k; i++)
       tbuf1[i] = BEEP_CHAR;
    tbuf1[i] = '\0';
    safe_str(tbuf1, buff, bufcx);
}

static const unsigned char AccentCombo2[256] =
{   
/*  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F   */

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 1
    0, 0, 9, 0, 0, 0,13, 2, 0, 0, 0, 0, 7,12, 0, 0,  // 2
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0,  // 3
    0, 0,10, 0, 0,14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 4
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0,  // 5
    1, 0, 0, 0, 0,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,  // 6
    0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0,11, 0, 4, 0,  // 7
    
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 8
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 9
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // A
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // B
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // C
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // D
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // E
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0   // F
};  


FUNCTION(fun_accent)
{
#ifdef ZENTY_ANSI
   char *p_ptr, p_oldptr, p_oldptr2, *p_ptr2;
   int i_inaccent;
#endif

#ifndef ZENTY_ANSI
   safe_str(fargs[0], buff, bufcx);
#else
   p_ptr2 = fargs[0];
   p_ptr = fargs[1];
   p_oldptr2 = p_oldptr = '\0';
   i_inaccent = 0;
   if ( strlen(fargs[0]) != strlen(fargs[1]) ) {
      safe_str("#-1 ARGUMENTS MUST BE OF SAME LENGTH", buff, bufcx);
      return;
   }
   if ( strlen(fargs[0]) > (LBUF_SIZE - 100) ) {
      safe_str(fargs[0], buff, bufcx);
   }
   while ( *p_ptr ) {
      p_oldptr = AccentCombo2[(int)*p_ptr];
      if ( (p_oldptr == '\0') || (p_oldptr2 != p_oldptr) ) {
         if ( (int)p_oldptr > 0 ) {
            safe_str("%f", buff, bufcx);
            safe_chr(*p_ptr, buff, bufcx);
            p_oldptr2 = p_oldptr;
            i_inaccent = 1;
         } else if ( p_oldptr2 != p_oldptr ) {
            safe_str("%fn", buff, bufcx);
            p_oldptr2 = p_oldptr;
         }
      }
      if ( p_ptr2 ) {
         safe_chr(*p_ptr2, buff, bufcx);
         p_ptr2++;
      }
      p_ptr++;
   }
   if ( i_inaccent )
      safe_str("%fn", buff, bufcx);
#endif
}

#ifdef ZENTY_ANSI
FUNCTION(fun_colors)
{
    PENNANSI *cm; 
    MUXANSI *cx;
    char *s_buff;
    int i_first = 0, i_count = 0, i_val = 0, i_key = 0, i_iswild = 0 ;
    int r = 0, g = 0, b = 0;
     
    if (!fn_range_check("COLORS", nfargs, 1, 2, buff, bufcx))
       return;

    if ( (nfargs > 1) && *fargs[1] ) {
       if ( *fargs[1] == 'h' )
          i_key = 1;
       if ( *fargs[1] == 'c' )
          i_key = 2;
       if ( *fargs[1] == 'x' )
          i_key = 3;
       if ( *fargs[1] == 'r' )
          i_key = 4;
    }
    if ( *fargs[0] && (strchr(fargs[0], '*') || strchr(fargs[0], '?')) ) {
       i_iswild = 1;
    }
    if ( !i_iswild && (nfargs > 0) && *fargs[0] ) {
       if ( isalpha(*fargs[0]) ) {
          cm = (PENNANSI *)hashfind(fargs[0], &mudstate.ansi_htab);
          if ( cm ) {
             s_buff = alloc_sbuf("fun_colors");
             if ( i_key == 1) {
                sprintf(s_buff, "0x%02x", cm->i_xterm);
             } else if ( i_key == 2) {
                sprintf(s_buff, "%s", ansi_translate_16[cm->i_xterm]);
             } else if ( (i_key == 3) || (i_key == 4) ) {
                for (cx = mux_namecolors; cx->s_hex; cx++) {
                   if ( cx->i_dec == cm->i_xterm ) {
                      if ( i_key == 3 ) {
                         sprintf(s_buff, "#%s", cx->s_hex);
                      } else {
                         sscanf(cx->s_hex, "%2x%2x%2x", &r, &g, &b);
                         sprintf(s_buff, "%d %d %d", r, g, b);
                      }
                      break;
                   }
                }
             } else {
                sprintf(s_buff, "%d", cm->i_xterm);
             }
             safe_str(s_buff, buff, bufcx);
             free_sbuf(s_buff);
          } else {
             safe_str("#-1 INVALID COLOR SPECIFIED", buff, bufcx);
          }
          return;
       }
       i_val = atoi(fargs[0]);
       if ( i_val < 0 ) i_val = 0;
    }
    for (cm = penn_namecolors; cm->name; cm++) {
       if ( i_iswild ) {
          if ( quick_wild(fargs[0], cm->name) ) {
             if ( i_first ) {
                safe_chr(' ', buff, bufcx);
             }
             safe_str(cm->name, buff, bufcx);
             i_first = 1;
          }
       } else {
          i_count++;
          if ( i_count < ((i_val * 350) + 1) ) {
             continue;
          }
          if ( i_first ) {
             safe_chr(' ', buff, bufcx);
          }
          i_first = 1;
          safe_str(cm->name, buff, bufcx);
          if (i_count >= ((i_val + 1) * 350) )
             break;
       }
    }
    if ( !i_first ) {
       if ( i_iswild ) {
          safe_str((char *)"#-1 NO MATCHING COLORS FOUND", buff, bufcx);
       } else {
          safe_str((char *)"#-1 NO VALUES FOR PAGE SPECIFIED", buff, bufcx);
       }
    }
}

int ansi_omitter = 0;

FUNCTION(fun_ansi)
{
    PENNANSI *cm;
    MUXANSI  *cx;
    char *q, *s, t_buff[60], *t_buff2, t_buff3[61], *ansi_special, *ansi_specialptr, *ansi_normalfg, *ansi_normalbg;
    int i, j, k, i_haveslash, i_xterm_ansi, i_fgcolor, i_bgcolor, r1, r2, g1, g2, b1, b2, rgb_diff, rgb_diff2, i_allow[5], i_trgbackground, i_tmp;

    if ( nfargs < 2 ) {
       safe_str("#-1 FUNCTION (ANSI) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }

    j = nfargs / 2;
    r1 = r2 = g1 = g2 = b1 = b2 = 0;
    rgb_diff = rgb_diff2 = 1000;
    i_fgcolor = i_bgcolor = -1;
    for ( i = 0;i < 5; i++ ) 
       i_allow[i] = 0;

    ansi_specialptr = ansi_special = alloc_mbuf("fun_ansi");
    ansi_normalfg = alloc_mbuf("fun_ansi2");
    ansi_normalbg = alloc_mbuf("fun_ansi3");

    ansi_omitter = 0; 

    for (i = 0; i < j; i++) {
        q = trim_spaces(fargs[i * 2]);
        memset(t_buff, 0, sizeof(t_buff));
        strncpy(t_buff, q, 59);
        free_lbuf(q);
        s = t_buff;
        i_haveslash = i_tmp = i_trgbackground = 0;
        i_xterm_ansi = 0;
        while (*s) {
           switch (*s) {
              case '/': 
                 i_trgbackground = 1;
                 i_haveslash = 1;
                 for ( k = 0;k < 5; k++ ) 
                    i_allow[k] = 0;
                 break;
              case '+':
                 cm = (PENNANSI *)NULL;
                 memset(t_buff3, 0, sizeof(t_buff3));
                 q = t_buff3;
                 s++;
                 while ( s && *s && !(isspace(*s) || (*s == '/')) ) {
                    *q = *s;
                    q++;
                    s++;
                 }
                 if ( *s == '/' )
                    s--;
                 if ( *t_buff3 ) {
                    cm = (PENNANSI *)hashfind(t_buff3, &mudstate.ansi_htab);
                 }
                 if ( cm ) {
                    if ( i_trgbackground )
                       i_bgcolor = cm->i_xterm;
                    else
                       i_fgcolor = cm->i_xterm;
                    i_xterm_ansi = 1;
                 }
                 break;
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                 i_tmp = atoi(s);
                 if ( (i_tmp >= 0) && (i_tmp <= 255) ) {
                    if ( i_trgbackground )
                       i_bgcolor = i_tmp;
                    else
                       i_fgcolor = i_tmp;
                    i_xterm_ansi = 1;
                 }
                 while ( s && *s && isdigit(*s) )
                    s++;
                 if ( *s == '/' )
                    s--;
                 break;
              case '0':
                 if ( ((*(s+1) == 'x') || (*(s+1) == 'X')) && isxdigit(*(s+2)) && isxdigit(*(s+3)) ) {
                    if ( i_trgbackground )
                       sscanf(s+2, "%2x", &i_bgcolor);
                    else
                       sscanf(s+2, "%2x", &i_fgcolor);
                    s+=3;
                    i_xterm_ansi = 1;
                 } else {
                    i_tmp = atoi(s);
                    if ( (i_tmp >= 0) && (i_tmp <= 255) ) {
                       if ( i_trgbackground )
                          i_bgcolor = i_tmp;
                       else
                          i_fgcolor = i_tmp;
                       i_xterm_ansi = 1;
                    }
                    while ( s && *s && isdigit(*s) )
                       s++;
                    if ( *s == '/' )
                       s--;
                 }
                 break;
              case '#':
                 if ( isxdigit(*(s+1)) && isxdigit(*(s+2)) && isxdigit(*(s+3)) &&
                      isxdigit(*(s+4)) && isxdigit(*(s+5)) && isxdigit(*(s+6)) ) {
                    sscanf(s+1, "%2x%2x%2x", &r1, &g1, &b1);
                    rgb_diff = rgb_diff2 = 1000;
                    for (cx = mux_namecolors; cx->s_hex; cx++) {
                       if(cx->i_dec < 16) 
                          continue;
                       sscanf(cx->s_hex, "%2x%2x%2x", &r2, &g2, &b2);
                       rgb_diff = abs(r2 - r1) + abs(g2 - g1) + abs(b2 - b1);
                       if ( rgb_diff < rgb_diff2 ) {
                          rgb_diff2 = rgb_diff;
                          if ( i_trgbackground )
                             i_bgcolor = cx->i_dec;
                          else
                             i_fgcolor = cx->i_dec;
                          /* Exact match -- break out */
                          if ( rgb_diff2 == 0 )
                             break;
                       }
                    }
                    s+=6;
                    i_xterm_ansi = 1;
                 }
                 break;
              case '<':
                 if ( (*(s+1) == '#') && isxdigit(*(s+2)) && isxdigit(*(s+3)) && 
                      isxdigit(*(s+4)) && isxdigit(*(s+5)) && isxdigit(*(s+6)) && 
                      isxdigit(*(s+7)) && *(s+8) == '>' ) {
                    sscanf(s+2, "%2x%2x%2x", &r1, &g1, &b1);
                    rgb_diff = rgb_diff2 = 1000;
                    for (cx = mux_namecolors; cx->s_hex; cx++) {
                       if(cx->i_dec < 16) 
                          continue;
                       sscanf(cx->s_hex, "%2x%2x%2x", &r2, &g2, &b2);
                       rgb_diff = abs(r2 - r1) + abs(g2 - g1) + abs(b2 - b1);
                       if ( rgb_diff < rgb_diff2 ) {
                          rgb_diff2 = rgb_diff;
                          if ( i_trgbackground )
                             i_bgcolor = cx->i_dec;
                          else
                             i_fgcolor = cx->i_dec;
                          /* Exact match -- break out */
                          if ( rgb_diff2 == 0 )
                             break;
                       }
                    }
                    while ( s && *s && (*s != '>') )
                       s++;
                    i_xterm_ansi = 1;
                 } else {
                    if ( (strchr(s, '>') != NULL) && ((strchr(s, '/') == NULL) || ((long)strchr(s, '>') < (long)strchr(s, '/'))) ) {
                       r1 = atoi(s+1);
                       if ( strchr(s+1, ' ') != NULL ) {
                          g1 = atoi(strchr(s+1, ' ')+1);
                          if ( strchr(strchr(s+1, ' ')+1, ' ') ) {
                             b1 = atoi(strchr(strchr(s+1, ' ')+1, ' ')+1);
                             rgb_diff = rgb_diff2 = 1000;
                             for (cx = mux_namecolors; cx->s_hex; cx++) {
                                if(cx->i_dec < 16) 
                                   continue;
                                sscanf(cx->s_hex, "%2x%2x%2x", &r2, &g2, &b2);
                                rgb_diff = abs(r2 - r1) + abs(g2 - g1) + abs(b2 - b1);
                                if ( rgb_diff < rgb_diff2 ) {
                                   rgb_diff2 = rgb_diff;
                                   if ( i_trgbackground )
                                      i_bgcolor = cx->i_dec;
                                   else
                                      i_fgcolor = cx->i_dec;
                                   /* Exact match -- break out */
                                   if ( rgb_diff2 == 0 )
                                      break;
                                }
                             }
                          }
                       }
                       while ( s && *s && (*s != '>') )
                          s++;
                       i_xterm_ansi = 1;
                    }
                 }
                 break;
              case 'u': 
                 if ( !i_allow[0] && (mudconf.global_ansimask & MASK_UNDERSCORE) )
                    safe_str(SAFE_ANSI_UNDERSCORE, ansi_special, &ansi_specialptr);
                 i_allow[0] = 1;
                 break;
              case 'U': ansi_omitter |= SPLIT_UNDERSCORE;
                 break;
              case 'h': 
                 if ( !i_allow[1] && (mudconf.global_ansimask & MASK_HILITE) )
                    safe_str(SAFE_ANSI_HILITE, ansi_special, &ansi_specialptr);
                 i_allow[1] = 1;
                 break;
              case 'H': ansi_omitter |= SPLIT_HILITE;
                 break;
              case 'i': 
                 if ( !i_allow[2] && (mudconf.global_ansimask & MASK_INVERSE) )
                    safe_str(SAFE_ANSI_INVERSE, ansi_special, &ansi_specialptr);
                 i_allow[2] = 1;
                 break;
              case 'I': ansi_omitter |= SPLIT_INVERSE;
                 break;
              case 'f': 
                 if ( !i_allow[3] && (mudconf.global_ansimask & MASK_BLINK) )
                    safe_str(SAFE_ANSI_BLINK, ansi_special, &ansi_specialptr);
                 i_allow[3] = 1;
                 break;
              case 'F': ansi_omitter |= SPLIT_FLASH;
                 break;
              case 'n': 
                 if ( !i_allow[4] )
                    safe_str(SAFE_ANSI_NORMAL, ansi_special, &ansi_specialptr);
                 i_allow[4] = 1;
                 break;
              case 'x': if ( i_haveslash ) break;  /* black fg */
                 if ( mudconf.global_ansimask & MASK_BLACK )
                    strcpy(ansi_normalfg, SAFE_ANSI_BLACK);
                 break;
              case 'r': if ( i_haveslash ) break;  /* red fg */
                 if ( mudconf.global_ansimask & MASK_RED )
                    strcpy(ansi_normalfg, SAFE_ANSI_RED);
                 break;
              case 'g': if ( i_haveslash ) break;  /* green fg */
                 if ( mudconf.global_ansimask & MASK_GREEN )
                    strcpy(ansi_normalfg, SAFE_ANSI_GREEN);
                 break;
              case 'y': if ( i_haveslash ) break;  /* yellow fg */
                 if ( mudconf.global_ansimask & MASK_YELLOW )
                    strcpy(ansi_normalfg, SAFE_ANSI_YELLOW);
                 break;
              case 'b': if ( i_haveslash ) break;  /* blue fg */
                 if ( mudconf.global_ansimask & MASK_BLUE )
                    strcpy(ansi_normalfg, SAFE_ANSI_BLUE);
                 break;
              case 'm': if ( i_haveslash ) break;  /* magenta fg */
                 if ( mudconf.global_ansimask & MASK_MAGENTA )
                    strcpy(ansi_normalfg, SAFE_ANSI_MAGENTA);
                 break;
              case 'c': if ( i_haveslash ) break;  /* cyan fg */
                 if ( mudconf.global_ansimask & MASK_CYAN )
                    strcpy(ansi_normalfg, SAFE_ANSI_CYAN);
                 break;
              case 'w': if ( i_haveslash ) break;  /* white fg */
                 if ( mudconf.global_ansimask & MASK_WHITE )
                    strcpy(ansi_normalfg, SAFE_ANSI_WHITE);
                 break;
              case 'X': if ( i_haveslash ) break;  /* black bg */
                 if ( mudconf.global_ansimask & MASK_BBLACK )
                    strcpy(ansi_normalbg, SAFE_ANSI_BBLACK);
                 break;
              case 'R': if ( i_haveslash ) break;  /* red bg */
                 if ( mudconf.global_ansimask & MASK_BRED )
                    strcpy(ansi_normalbg, SAFE_ANSI_BRED);
                 break;
              case 'G': if ( i_haveslash ) break;  /* green bg */
                 if ( mudconf.global_ansimask & MASK_BGREEN )
                    strcpy(ansi_normalbg, SAFE_ANSI_BGREEN);
                 break;
              case 'Y': if ( i_haveslash ) break;  /* yellow bg */
                 if ( mudconf.global_ansimask & MASK_BYELLOW )
                    strcpy(ansi_normalbg, SAFE_ANSI_BYELLOW);
                 break;
              case 'B': if ( i_haveslash ) break;  /* blue bg */
                 if ( mudconf.global_ansimask & MASK_BBLUE )
                    strcpy(ansi_normalbg, SAFE_ANSI_BBLUE);
                 break;
              case 'M': if ( i_haveslash ) break;  /* magenta bg */
                 if ( mudconf.global_ansimask & MASK_BMAGENTA )
                    strcpy(ansi_normalbg, SAFE_ANSI_BMAGENTA);
                 break;
              case 'C': if ( i_haveslash ) break;  /* cyan bg */
                 if ( mudconf.global_ansimask & MASK_BCYAN )
                    strcpy(ansi_normalbg, SAFE_ANSI_BCYAN);
                 break;
              case 'W': if ( i_haveslash ) break;  /* white bg */
                 if ( mudconf.global_ansimask & MASK_BWHITE )
                    strcpy(ansi_normalbg, SAFE_ANSI_BWHITE);
                 break;
         }
         s++;
      }
      if ( i_xterm_ansi ) {
         if ( i_fgcolor >= 0 && i_fgcolor <= 255 ) {
            t_buff2 = alloc_lbuf("fun_ansi");
#ifdef TINY_SUB
            sprintf(t_buff2, "%%x0x%02x", i_fgcolor);
#else
            sprintf(t_buff2, "%%c0x%02x", i_fgcolor);
#endif
            safe_str(t_buff2, buff, bufcx);
            free_lbuf(t_buff2);
         } else {
            safe_str(ansi_normalfg, buff, bufcx);
         }
         if ( i_bgcolor >= 0 && i_bgcolor <= 255) {
            t_buff2 = alloc_lbuf("fun_ansi");
#ifdef TINY_SUB
            sprintf(t_buff2, "%%x0X%02x", i_bgcolor);
#else
            sprintf(t_buff2, "%%c0X%02x", i_bgcolor);
#endif
            safe_str(t_buff2, buff, bufcx);
            free_lbuf(t_buff2);
         } else {
            safe_str(ansi_normalbg, buff, bufcx);
         }
      } else {
         safe_str(ansi_normalfg, buff, bufcx);
         safe_str(ansi_normalbg, buff, bufcx);
      }
      safe_str(ansi_special, buff, bufcx);
      safe_str(fargs[(i * 2) + 1], buff, bufcx);
      safe_str(SAFE_ANSI_NORMAL, buff, bufcx);
      memset(ansi_normalfg, '\0', MBUF_SIZE);
      memset(ansi_normalbg, '\0', MBUF_SIZE);
      memset(ansi_special, '\0', MBUF_SIZE);
      ansi_specialptr = ansi_special;
      i_fgcolor = i_bgcolor = -1;
      for ( k = 0;k < 5; k++ ) 
         i_allow[k] = 0;
   }
   free_mbuf(ansi_normalfg);
   free_mbuf(ansi_normalbg);
   free_mbuf(ansi_special);

}

#else /* Non-zenty foo */

FUNCTION(fun_colors)
{
   safe_str("#-1 FUNCTION NOT AVAILABLE WITHOUT ZENTY-ANSI ENABLED", buff, bufcx);
}

FUNCTION(fun_ansi)
{
    char *s, t_buff[33];
    int i, j;

    if ( nfargs < 2 ) {
       safe_str("#-1 FUNCTION (ANSI) EXPECTS 2 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    j = nfargs / 2;
    memset(t_buff, 0, sizeof(t_buff));
    for (i = 0; i < j; i++) {
        strncpy(t_buff, fargs[i * 2], 32);
        s = t_buff;
        while (*s) {
           switch (*s) {
              case 'u':
                 if ( mudconf.global_ansimask & MASK_UNDERSCORE )
                    safe_str(ANSI_UNDERSCORE, buff, bufcx);
                 break;
              case 'h':   /* hilite */
                 if ( mudconf.global_ansimask & MASK_HILITE )
                    safe_str(ANSI_HILITE, buff, bufcx);
                 break;
              case 'i':   /* inverse */
                 if ( mudconf.global_ansimask & MASK_INVERSE )
                    safe_str(ANSI_INVERSE, buff, bufcx);
                 break;
              case 'f':   /* flash */
                 if ( mudconf.global_ansimask & MASK_BLINK )
                    safe_str(ANSI_BLINK, buff, bufcx);
                 break;
              case 'n':   /* normal */
                 safe_str(ANSI_NORMAL, buff, bufcx);
                 break;
              case 'x':   /* black fg */
                 if ( mudconf.global_ansimask & MASK_BLACK )
                    safe_str(ANSI_BLACK, buff, bufcx);
                 break;
              case 'r':   /* red fg */
                 if ( mudconf.global_ansimask & MASK_RED )
                    safe_str(ANSI_RED, buff, bufcx);
                 break;
              case 'g':   /* green fg */
                 if ( mudconf.global_ansimask & MASK_GREEN )
                    safe_str(ANSI_GREEN, buff, bufcx);
                 break;
              case 'y':   /* yellow fg */
                 if ( mudconf.global_ansimask & MASK_YELLOW )
                    safe_str(ANSI_YELLOW, buff, bufcx);
                 break;
              case 'b':   /* blue fg */
                 if ( mudconf.global_ansimask & MASK_BLUE )
                    safe_str(ANSI_BLUE, buff, bufcx);
                 break;
              case 'm':   /* magenta fg */
                 if ( mudconf.global_ansimask & MASK_MAGENTA )
                    safe_str(ANSI_MAGENTA, buff, bufcx);
                 break;
              case 'c':   /* cyan fg */
                 if ( mudconf.global_ansimask & MASK_CYAN )
                    safe_str(ANSI_CYAN, buff, bufcx);
                 break;
              case 'w':   /* white fg */
                 if ( mudconf.global_ansimask & MASK_WHITE )
                    safe_str(ANSI_WHITE, buff, bufcx);
                 break;
              case 'X':   /* black bg */
                 if ( mudconf.global_ansimask & MASK_BBLACK )
                    safe_str(ANSI_BBLACK, buff, bufcx);
                 break;
              case 'R':   /* red bg */
                 if ( mudconf.global_ansimask & MASK_BRED )
                    safe_str(ANSI_BRED, buff, bufcx);
                 break;
              case 'G':   /* green bg */
                 if ( mudconf.global_ansimask & MASK_BGREEN )
                    safe_str(ANSI_BGREEN, buff, bufcx);
                 break;
              case 'Y':   /* yellow bg */
                 if ( mudconf.global_ansimask & MASK_BYELLOW )
                    safe_str(ANSI_BYELLOW, buff, bufcx);
                 break;
              case 'B':   /* blue bg */
                 if ( mudconf.global_ansimask & MASK_BBLUE )
                    safe_str(ANSI_BBLUE, buff, bufcx);
                 break;
              case 'M':   /* magenta bg */
                 if ( mudconf.global_ansimask & MASK_BMAGENTA )
                    safe_str(ANSI_BMAGENTA, buff, bufcx);
                 break;
              case 'C':   /* cyan bg */
                 if ( mudconf.global_ansimask & MASK_BCYAN )
                    safe_str(ANSI_BCYAN, buff, bufcx);
                 break;
              case 'W':   /* white bg */
                 if ( mudconf.global_ansimask & MASK_BWHITE )
                    safe_str(ANSI_BWHITE, buff, bufcx);
                 break;
           }
           s++;
        }

        safe_str(fargs[(i * 2) + 1], buff, bufcx);
        safe_str(ANSI_NORMAL, buff, bufcx);
    }
}
#endif

/* editansi -- edit the color/accent markup in a string */
FUNCTION(fun_editansi)
{
   ANSISPLIT search_val[LBUF_SIZE], replace_val[LBUF_SIZE], a_input[LBUF_SIZE];
   char *s_input, *s_combine, *s_buff, *s_buffptr, *s_array[3];
   int i_omit1, i_omit2;

#ifndef ZENTY_ANSI
   safe_str((char *)"#-1 ZENTY ANSI REQUIRED FOR THIS FUNCTION.", buff, bufcx);
   return;
#endif
   if ( !*fargs[0] )
      return;

   if ( !*fargs[1] || !*fargs[2] ) {
      safe_str(fargs[0], buff, bufcx);
      return;
   }

   initialize_ansisplitter(a_input, LBUF_SIZE);
   initialize_ansisplitter(search_val, LBUF_SIZE);
   initialize_ansisplitter(replace_val, LBUF_SIZE);
   s_input = alloc_lbuf("fun_editansi_orig");
   s_combine = alloc_lbuf("fun_editansi_combine");
   memset(s_input, '\0', LBUF_SIZE);
   memset(s_combine, '\0', LBUF_SIZE);
   split_ansi(strip_ansi(fargs[0]), s_input, a_input);
   s_buffptr = s_buff = alloc_lbuf("fun_editansi");
   s_array[0] = alloc_lbuf("fun_editansi1");
   s_array[1] = alloc_lbuf("fun_editansi2");
   s_array[2] = NULL;
   sprintf(s_array[1], "%c", 'X');
   sprintf(s_array[0], "%s", fargs[1]);
   fun_ansi(s_buff, &s_buffptr, player, cause, cause, s_array, 2, (char **)NULL, 0);
   i_omit1 = ansi_omitter;
   split_ansi(s_buff, s_combine, search_val);
   memset(s_combine, '\0', LBUF_SIZE);
   memset(s_buff, '\0', LBUF_SIZE);
   s_buffptr = s_buff;
   sprintf(s_array[1], "%c", 'X');
   sprintf(s_array[0], "%s", fargs[2]);
   fun_ansi(s_buff, &s_buffptr, player, cause, cause, s_array, 2, (char **)NULL, 0);
   i_omit2 = ansi_omitter;
   split_ansi(s_buff, s_combine, replace_val);
   free_lbuf(s_combine);

   search_and_replace_ansi(s_input, a_input, search_val, replace_val, i_omit1, i_omit2);
   s_combine = rebuild_ansi(s_input, a_input);
   safe_str(s_combine, buff, bufcx);

   free_lbuf(s_input);
   free_lbuf(s_combine);
   free_lbuf(s_buff);
   free_lbuf(s_array[0]);
   free_lbuf(s_array[1]);
}
FUNCTION(fun_stripansi)
{
    /* Strips ANSI codes away from a given string of text. Starts by
     * finding the '\x' character and stripping until it hits an 'm'.
     */

    char *cp = fargs[0];

    while (*cp) {
#ifdef ZENTY_ANSI
        if ( (*cp == '%') && ((*(cp+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                          || (*(cp+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                          || (*(cp+1) == SAFE_CHR3)
#endif
)) {
           if ( isAnsi[(int) *(cp+2)] ) {
              cp+=3;
              continue;
           }
           if ( (*(cp+2) == '0') && ((*(cp+3) == 'x') || (*(cp+3) == 'X')) &&
                *(cp+4) && *(cp+5) && isxdigit(*(cp+4)) && isxdigit(*(cp+5)) ) {
              cp+=6;
              continue;
           }
           safe_chr(*cp++, buff, bufcx);
        } else {
            safe_chr(*cp++, buff, bufcx);
        }

#else
       if (*cp == ESC_CHAR)
           while ( *cp && (*cp++ != 'm'));
       else
           safe_chr(*cp++, buff, bufcx);
#endif
    }
}

FUNCTION(fun_stripaccents) {
#ifdef ZENTY_ANSI
   char *cp;

   cp = fargs[0];
   while ( *cp ) {
      if ( (*cp == '%') && (*(cp + 1) == 'f') && isprint(*(cp + 2)) )
         cp+=3;
      else
         safe_chr(*cp++, buff, bufcx);
   }
#else
   safe_str(fargs[0], buff, bufcx);
#endif
}

/* ---------------------------------------------------------------------------
 * fun_space: Make spaces.
 */

FUNCTION(fun_space)
{
    int num;

    if (!fargs[0] || !(*fargs[0])) {
       num = 1;
    } else {
       num = atoi(fargs[0]);
    }

    if (num < 1) {

       /* If negative or zero spaces return a single space,
        * -except- allow 'space(0)' to return "" for calculated
        * padding
        */

       if (!is_integer(fargs[0]) || (num != 0)) {
           num = 1;
       }
    } else if (num >= LBUF_SIZE) {
       num = LBUF_SIZE - 1;
    }
    for (; num > 0; num--)
       safe_chr(' ', buff, bufcx);
    return;
}

/* ---------------------------------------------------------------------------
 * fun_globalroom: return global room # to wizards
 */

FUNCTION(fun_globalroom)
{
  if( !Wizard(player) ||
      !Good_obj(mudconf.master_room) ) {
    safe_str("#-1", buff, bufcx);
  } else {
    dbval(buff, bufcx, mudconf.master_room);
  }
}

/* ---------------------------------------------------------------------------
 * fun_idle, fun_conn: return seconds idle or connected.
 */

FUNCTION(fun_idle)
{
    dbref target;
    int i_type, d_type, first, idletime;
    char *tmp_buff;
    DESC *d;

    if (!fn_range_check("IDLE", nfargs, 1, 3, buff, bufcx)) {
       return;
    } else if (nfargs >= 2) {
       i_type = atoi(fargs[1]);
    } else {
       i_type = 0;
    }
    if ( i_type != 0 )
       i_type = 1;
    if ( (i_type == 1) && ((nfargs >= 3) && *fargs[2]) ) {
       d_type = atoi(fargs[2]);
    } else {
       d_type = -1;
    }
    target = lookup_player(player, fargs[0], 1);
    if( !Good_obj(target) ||
        (((Dark(target) && !Wizard(player) && !(mudconf.player_dark)) ||
        (Wizard(player) && !Immortal(player)
         && Immortal(target) && SCloak(target))) && (player != target)) )
      target = NOTHING;

    if (NoWho(target) && (target != player) &&
        !(Wizard(player) || HasPriv(player, target, POWER_WIZ_WHO, POWER3, NOTHING)))
      target = NOTHING;

    if ( (i_type > 0) && (target != NOTHING) ) {
       if ( (target == player) || Wizard(player) ) {
          tmp_buff = alloc_lbuf("fun_idle");
          first = 0;
          DESC_ITER_CONN(d) {
             if (d->player == target && ((i_type == 0) || (((i_type == 1) && (d_type == -1)) ||
                                        (((d_type != -1) && d->descriptor == d_type)))) ) {
                if ( first )
                   safe_chr(' ', buff, bufcx);
                idletime = (mudstate.now - d->last_time);
                if ( (d_type == -1) && (i_type == 1) )
                   sprintf(tmp_buff, "%d:%d", d->descriptor, idletime);
                else
                   sprintf(tmp_buff, "%d", idletime);
                safe_str(tmp_buff, buff, bufcx);
                first = 1;
                if ( (d_type != -1) || (i_type == 0) )
                   break;
             }
          }
          free_lbuf(tmp_buff);
          if ( first == 0 )
             ival(buff, bufcx, -1);
       } else {
          ival(buff, bufcx, fetch_idle(target));
       }
    } else {
       ival(buff, bufcx, fetch_idle(target));
    }
}

FUNCTION(fun_conn)
{
    dbref target;
    int i_type, d_type, conntime, first;
    char *tmp_buff;
    DESC *d;

    if (!fn_range_check("CONN", nfargs, 1, 3, buff, bufcx)) {
       return;
    } else if (nfargs >= 2) {
       i_type = atoi(fargs[1]);
    } else {
       i_type = 0;
    }
    if ( i_type != 0 )
       i_type = 1;
    if ( (i_type == 1) && ((nfargs >= 3) && *fargs[2]) ) {
       d_type = atoi(fargs[2]);
    } else {
       d_type = -1;
    }
    target = lookup_player(player, fargs[0], 1);
    if ( !Good_obj(target) ||
         ((Dark(target) && !Wizard(player) && !(mudconf.player_dark)) ||
         ((Wizard(player) && !Immortal(player)
          && Immortal(target) && SCloak(target)) && (player != target))) )
      target = NOTHING;

    if (NoWho(target) && (target != player) &&
        !(Wizard(player) || HasPriv(player, target, POWER_WIZ_WHO, POWER3, NOTHING)))
      target = NOTHING;

    if ( (i_type > 0) && (target != NOTHING) ) {
       if ( (target == player) || Wizard(player) ) {
          tmp_buff = alloc_lbuf("fun_conn");
          first = 0;
          DESC_ITER_CONN(d) {
             if (d->player == target && ((i_type == 0) || (((i_type == 1) && (d_type == -1)) ||
                                        (((d_type != -1) && d->descriptor == d_type)))) ) {
                if ( first )
                   safe_chr(' ', buff, bufcx);
                conntime = (mudstate.now - d->connected_at);
                if ( (d_type == -1) && (i_type == 1))
                   sprintf(tmp_buff, "%d:%d", d->descriptor, conntime);
                else
                   sprintf(tmp_buff, "%d", conntime);
                safe_str(tmp_buff, buff, bufcx);
                first = 1;
                if ( (d_type != -1) || (i_type == 0) )
                   break;
             }
          }
          free_lbuf(tmp_buff);
          if ( first == 0 )
             ival(buff, bufcx, -1);
       } else {
          ival(buff, bufcx, fetch_connect(target));
       }
    } else {
       ival(buff, bufcx, fetch_connect(target));
    }

}

/* ---------------------------------------------------------------------------
 * fun_sort: Sort lists.
 */

typedef struct f_record f_rec;
typedef struct i_record i_rec;

struct f_record {
    double data;
    char *str;
};

struct i_record {
    long data;
    char *str;
};

static int
a_comp(const void *s1, const void *s2)
{
    return strcmp(*(char **) s1, *(char **) s2);
}

static int
f_comp(const void *s1, const void *s2)
{
    if (((f_rec *) s1)->data > ((f_rec *) s2)->data)
       return 1;
    if (((f_rec *) s1)->data < ((f_rec *) s2)->data)
       return -1;
    return 0;
}

static int
i_comp(const void *s1, const void *s2)
{
    if (((i_rec *) s1)->data > ((i_rec *) s2)->data)
       return 1;
    if (((i_rec *) s1)->data < ((i_rec *) s2)->data)
       return -1;
    return 0;
}

static void
do_asort(char *s[], int n, int sort_type)
{
    int i;
    f_rec *fp;
    i_rec *ip;

    switch (sort_type) {
         case ALPHANUM_LIST:
            qsort((void *) s, n, sizeof(char *), a_comp);
            break;
         case NUMERIC_LIST:
            ip = (i_rec *) malloc(n * sizeof(i_rec));
            for (i = 0; i < n; i++) {
               ip[i].str = s[i];
               ip[i].data = atoi(s[i]);
            }
            qsort((void *) ip, n, sizeof(i_rec), i_comp);
            for (i = 0; i < n; i++) {
               s[i] = ip[i].str;
            }
            free(ip);
            break;
         case DBREF_LIST:
            ip = (i_rec *) malloc(n * sizeof(i_rec));
            for (i = 0; i < n; i++) {
               ip[i].str = s[i];
               ip[i].data = dbnum(s[i]);
            }
            qsort((void *) ip, n, sizeof(i_rec), i_comp);
            for (i = 0; i < n; i++) {
               s[i] = ip[i].str;
            }
            free(ip);
            break;
         case FLOAT_LIST:
            fp = (f_rec *) malloc(n * sizeof(f_rec));
            for (i = 0; i < n; i++) {
               fp[i].str = s[i];
               fp[i].data = safe_atof(s[i]);
            }
            qsort((void *) fp, n, sizeof(f_rec), f_comp);
            for (i = 0; i < n; i++) {
               s[i] = fp[i].str;
            }
            free(fp);
            break;
    }
}

FUNCTION(fun_sort)
{
    int nitems, sort_type;
    char *list, sep, osep;
    char *ptrs[LBUF_SIZE / 2];

    /* If we are passed an empty arglist return a null string */

    if (nfargs == 0) {
       return;
    }
/*  mvarargs_preamble("SORT", 1, 3); */
    if (!fn_range_check("SORT", nfargs, 1, 4, buff, bufcx))
        return;
    if (!delim_check(fargs, nfargs, 3, &sep, buff, bufcx, 0,
                     player, cause, caller, cargs, ncargs))
        return;
    if (nfargs < 4)
        osep = sep;
    else if (!delim_check(fargs, nfargs, 4, &osep, buff, bufcx, 0,
                          player, cause, caller, cargs, ncargs))
        return;


    /* Convert the list to an array */

    list = alloc_lbuf("fun_sort");
    strcpy(list, fargs[0]);
    nitems = list2arr(ptrs, LBUF_SIZE / 2, list, sep);
    sort_type = get_list_type(fargs, nfargs, 2, ptrs, nitems);
    do_asort(ptrs, nitems, sort_type);
    arr2list(ptrs, nitems, buff, bufcx, osep);
    free_lbuf(list);
}

/* sortby() code borrowed from MUX 2.3
 * Ash - 05/21/04
 */
static char  ucomp_buff[LBUF_SIZE];
static dbref ucomp_caller;
static dbref ucomp_enactor;
static dbref ucomp_executor;

static int u_comp(const void *s1, const void *s2)
{
    /* Note that this function is for use in conjunction with our own
     * sane_qsort routine, NOT with the standard library qsort!
     */
    char *result, *tbuf, *elems[2], *str;
    int n;

    if ((mudstate.func_invk_ctr > mudconf.func_invk_lim) ||
        (mudstate.func_nest_lev > mudconf.func_nest_lim))
        return 0;

    tbuf = alloc_lbuf("u_comp");
    elems[0] = (char *)s1;
    elems[1] = (char *)s2;
    strcpy(tbuf, ucomp_buff);
    str = tbuf;
    result = exec(ucomp_caller, ucomp_enactor, ucomp_executor,
                  EV_STRIP | EV_FCHECK | EV_EVAL, str, &(elems[0]), 2);
    if (!result)
        n = 0;
    else {
        n = atoi(result);
        free_lbuf(result);
    }
    free_lbuf(tbuf);
    return n;
}


typedef int PV(const void *, const void *);

static void sane_qsort(void *array[], int left, int right, PV compare)
{
    /* Andrew Molitor's qsort, which doesn't require transitivity between
     * comparisons (essential for preventing crashes due to boneheads
     * who write comparison functions where a > b doesn't mean b < a).
     */
    int i, last;
    void *tmp;

loop:

    if (left >= right)
        return;

    /* Pick something at random at swap it into the leftmost slot
     * This is the pivot, we'll put it back in the right spot later.
     */
    i = random() % (right - left);
    tmp = array[left + i];
    array[left + i] = array[left];
    array[left] = tmp;

    last = left;
    for (i = left + 1; i <= right; i++) {

        /* Walk the array, looking for stuff that's less than our
         * pivot. If it is, swap it with the next thing along
         */
        if ((*compare) (array[i], array[left]) < 0)
        {
            last++;
            if (last == i)
                continue;

            tmp = array[last];
            array[last] = array[i];
            array[i] = tmp;
        }
    }

    /* Now we put the pivot back, it's now in the right spot, we never
     * need to look at it again, trust me.
     */
    tmp = array[last];
    array[last] = array[left];
    array[left] = tmp;

    /* At this point everything underneath the 'last' index is < the
     * entry at 'last' and everything above it is not < it.
     */
    if ((last - left) < (right - last))
    {
        sane_qsort(array, left, last - 1, compare);
        left = last + 1;
        goto loop;
    }
    else
    {
        sane_qsort(array, last + 1, right, compare);
        right = last - 1;
        goto loop;
    }
}

FUNCTION(fun_sortby)
{
    char osep, sep, *atext, *list, *ptrs[LBUF_SIZE / 2];
    int anum, nptrs, tval;
    dbref thing, atrowner;
    ATTR *ap;

    svarargs_preamble("SORTBY", 4);

    if (parse_attrib(player, fargs[0], &thing, &anum)) {
        if ((anum == NOTHING) || (!Good_obj(thing)) || Recover(thing) || Going(thing))
            ap = NULL;
        else
            ap = atr_num(anum);
    } else {
        thing = player;
        ap = atr_str(fargs[0]);
    }

    /* Make sure we got a good attribute */

    if (!ap) {
        return;
    }
    /* Use it if we can access it, otherwise return an error. */

    atext = atr_pget(thing, ap->number, &atrowner, &anum);
    if (!atext) {
        return;
    } else if (!*atext || !See_attr(player, thing, ap, thing, anum, 0)) {
        free_lbuf(atext);
        return;
    }
    strcpy(ucomp_buff, atext);
    /* Lensy: Note: In most cases thing == player */
    ucomp_caller   = thing;
    ucomp_enactor  = player;
    ucomp_executor = caller;

    list = alloc_lbuf("fun_sortby");
    strcpy(list, fargs[1]);
    nptrs = list2arr(ptrs, LBUF_SIZE / 2, list, sep);

    if (nptrs > 1)
    {
       tval = safer_ufun(player, thing, thing, (ap ? ap->flags : 0), anum);
       if ( tval == -2 ) {
          safe_str("#-1 PERMISSION DENIED", buff, bufcx);
          free_lbuf(list);
          free_lbuf(atext);
          safer_unufun(tval);
          return;
       } else {
          sane_qsort((void **)ptrs, 0, nptrs - 1, u_comp);
       }
       safer_unufun(tval);
    }

    arr2list(ptrs, nptrs, buff, bufcx, osep);
    free_lbuf(list);
    free_lbuf(atext);
}

/*
   ---------------------------------------------------------------------------
   * fun_setunion, fun_setdiff, fun_setinter: Set management.
 */

#define SET_UNION       1
#define SET_INTERSECT   2
#define SET_DIFF        3

static void
handle_sets(char *fargs[], char *buff, char **bufcx, int oper, char sep, char osep, int sort_type)
{
    char *list1, *list2, *oldp;
    char *ptrs1[LBUF_SIZE], *ptrs2[LBUF_SIZE];
    char *startpoint = "";
    int i1, i2, n1, n2, val, first;

    list1 = alloc_lbuf("fun_setunion.1");
    strcpy(list1, fargs[0]);
    n1 = list2arr(ptrs1, LBUF_SIZE, list1, sep);
    do_asort(ptrs1, n1, sort_type);

    list2 = alloc_lbuf("fun_setunion.2");
    strcpy(list2, fargs[1]);
    n2 = list2arr(ptrs2, LBUF_SIZE, list2, sep);
    do_asort(ptrs2, n2, sort_type);

    i1 = i2 = 0;
    first = 1;
    oldp = startpoint;

    switch (oper) {
       case SET_UNION:   /* Copy elements from both lists w/o duplicate */
          /* Handle case of two identical single-element lists */
          if ((n1 == 1) && (n2 == 1) &&
             (!strcmp(ptrs1[0], ptrs2[0]))) {
             safe_str(ptrs1[0], buff, bufcx);
             break;
          }
          /* Process until one list is empty */
          while ((i1 < n1) && (i2 < n2)) {
          /* Skip over duplicates */
             if ((i1 > 0) || (i2 > 0)) {
                while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
                    i1++;
                while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
                    i2++;
             }
             /* Compare and copy */
             if ((i1 < n1) && (i2 < n2)) {
                if (!first)
                   safe_chr(osep, buff, bufcx);
                first = 0;
                if (strcmp(ptrs1[i1], ptrs2[i2]) < 0) {
                    safe_str(ptrs1[i1], buff, bufcx);
                   oldp = ptrs1[i1];
                    i1++;
                } else {
                    safe_str(ptrs2[i2], buff, bufcx);
                    oldp = ptrs2[i2];
                    i2++;
                }
                if ( !*oldp || !oldp)
                   first = 1;
             }
          }
          /* Copy rest of remaining list, stripping duplicates */
          for (; i1 < n1; i1++) {
             if (strcmp(oldp, ptrs1[i1])) {
                if (!first)
                   safe_chr(osep, buff, bufcx);
                first = 0;
                safe_str(ptrs1[i1], buff, bufcx);
                oldp = ptrs1[i1];
             }
             if ( !*oldp || !oldp )
                first = 1;
          }
          for (; i2 < n2; i2++) {
              if (strcmp(oldp, ptrs2[i2])) {
                 if (!first)
                    safe_chr(osep, buff, bufcx);
                 first = 0;
                 safe_str(ptrs2[i2], buff, bufcx);
                 oldp = ptrs2[i2];
              }
              if ( !*oldp || !oldp )
                 first = 1;
          }
          break;
       case SET_INTERSECT: /* Copy only elements in both lists */
          while ((i1 < n1) && (i2 < n2)) {
              val = strcmp(ptrs1[i1], ptrs2[i2]);
              if (!val) {
                 /* Got a match, copy it */
                 if (!first)
                    safe_chr(osep, buff, bufcx);
                 first = 0;
                 safe_str(ptrs1[i1], buff, bufcx);
                 oldp = ptrs1[i1];
                 i1++;
                 i2++;
                 while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
                     i1++;
                 while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
                     i2++;
              } else if (val < 0) {
                 i1++;
              } else {
                 i2++;
              }
           }
           break;
       case SET_DIFF:    /* Copy elements unique to list1 */
          while ((i1 < n1) && (i2 < n2)) {
             val = strcmp(ptrs1[i1], ptrs2[i2]);
             if (!val) {
                /* Got a match, increment pointers */
                oldp = ptrs1[i1];
                while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
                   i1++;
                while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
                    i2++;
             } else if (val < 0) {
               /* Item in list1 not in list2, copy */
               if (!first)
                  safe_chr(osep, buff, bufcx);
                  first = 0;
                  safe_str(ptrs1[i1], buff, bufcx);
                  oldp = ptrs1[i1];
                  i1++;
                  while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
                     i1++;
             } else {
                /* Item in list2 but not in list1, discard */
                oldp = ptrs2[i2];
                i2++;
                while ((i2 < n2) && !strcmp(ptrs2[i2], oldp))
                    i2++;
             }
          }
          /* Copy remainder of list1 */
          while (i1 < n1) {
             if (!first)
                safe_chr(osep, buff, bufcx);
             first = 0;
             safe_str(ptrs1[i1], buff, bufcx);
             oldp = ptrs1[i1];
             i1++;
             while ((i1 < n1) && !strcmp(ptrs1[i1], oldp))
                i1++;
          }
    } /* Switch */
    free_lbuf(list1);
    free_lbuf(list2);
    return;
}


void
do_listsets(int nfargs, char *fargs[], char *buff, char **bufcx, int i_type)
{
   char *s_tmpbuff1, *s_tmpptr, *s_scratch, *s_tmpscratch, *s_sc2, *s_tmpsc2,
        *s_strtokstr, *s_compare, s_sep[2], sep, osep;
   int i_first, i_key;

   if ( !*fargs[0] && !*fargs[1] )
      return;

   if ( (nfargs > 2) && *fargs[2] )
      sep = *fargs[2];
   else
      sep = ' ';

   if ( (nfargs > 3) && *fargs[3] )
      osep = *fargs[3];
   else
      osep = sep;

   if ( (nfargs > 4) && *fargs[4] )
      i_key = atoi(fargs[4]);
   else
      i_key = 0;

   s_sep[0] = sep;
   s_sep[1] = '\0';
   s_tmpscratch = s_scratch = alloc_lbuf("fun_listunion_scratch");
   s_tmpsc2 = s_sc2 = alloc_lbuf("fun_listunion_scratch2");
   safe_chr(' ', s_scratch, &s_tmpscratch);
   safe_chr(' ', s_sc2, &s_tmpsc2);
   s_tmpbuff1 = alloc_lbuf("fun_listunion_tmpbuff1");
   s_compare = alloc_lbuf("fun_listunion_compare");
   if ( (i_type == SET_INTERSECT) || (i_type == SET_DIFF) ) {
      strcpy(s_tmpbuff1, fargs[1]);
      s_tmpptr = strtok_r(s_tmpbuff1, s_sep, &s_strtokstr);
      while ( s_tmpptr ) {
         safe_str(s_tmpptr, s_sc2, &s_tmpsc2);
         safe_chr(' ', s_sc2, &s_tmpsc2);
         s_tmpptr = strtok_r(NULL, s_sep, &s_strtokstr);
      }
   }
   strcpy(s_tmpbuff1, fargs[0]);
   s_tmpptr = strtok_r(s_tmpbuff1, s_sep, &s_strtokstr);
   i_first = 0;
   while ( s_tmpptr ) {
      sprintf(s_compare," %.*s ", (LBUF_SIZE - 10), s_tmpptr);
      if ( ((i_type == SET_INTERSECT) && strstr(s_sc2, s_compare) == NULL) ||
           ((i_type == SET_DIFF) && strstr(s_sc2, s_compare) != NULL) ) {
         s_tmpptr = strtok_r(NULL, s_sep, &s_strtokstr);
         continue;
      }
      if ( !i_key ) {
         if ( strstr(s_scratch, s_compare) == NULL ) {
            if ( i_first )
               safe_chr(osep, buff, bufcx);
            safe_str(s_tmpptr, buff, bufcx);
            safe_str(s_tmpptr, s_scratch, &s_tmpscratch);
            safe_chr(' ', s_scratch, &s_tmpscratch);
            i_first = 1;
         }
      } else {
         if ( i_first )
            safe_chr(osep, buff, bufcx);
         safe_str(s_tmpptr, buff, bufcx);
         safe_str(s_tmpptr, s_scratch, &s_tmpscratch);
         safe_chr(' ', s_scratch, &s_tmpscratch);
         i_first = 1;
      }
      s_tmpptr = strtok_r(NULL, s_sep, &s_strtokstr);
   }
   if ( i_type == SET_UNION ) {
      strcpy(s_tmpbuff1, fargs[1]);
      s_tmpptr = strtok_r(s_tmpbuff1, s_sep, &s_strtokstr);
      while ( s_tmpptr ) {
         sprintf(s_compare," %.*s ", (LBUF_SIZE - 10), s_tmpptr);
         if ( strstr(s_scratch, s_compare) == NULL ) {
            if ( i_first )
               safe_chr(osep, buff, bufcx);
            safe_str(s_tmpptr, buff, bufcx);
            safe_str(s_tmpptr, s_scratch, &s_tmpscratch);
            safe_chr(' ', s_scratch, &s_tmpscratch);
            i_first = 1;
         }
         s_tmpptr = strtok_r(NULL, s_sep, &s_strtokstr);
      }
   }
   free_lbuf(s_scratch);
   free_lbuf(s_sc2);
   free_lbuf(s_tmpbuff1);
   free_lbuf(s_compare);
}

FUNCTION(fun_listunion)
{
   if (!fn_range_check("LISTUNION", nfargs, 2, 5, buff, bufcx))
      return;

   do_listsets(nfargs, fargs, buff, bufcx, SET_UNION);
}

FUNCTION(fun_listdiff)
{
   if (!fn_range_check("LISTDIFF", nfargs, 2, 5, buff, bufcx))
      return;

   do_listsets(nfargs, fargs, buff, bufcx, SET_DIFF);
}

FUNCTION(fun_listinter)
{
   if (!fn_range_check("LISTINTER", nfargs, 2, 5, buff, bufcx))
      return;

   do_listsets(nfargs, fargs, buff, bufcx, SET_INTERSECT);
}

FUNCTION(fun_setunion)
{
    char sep, osep;
    int sort_type;

    if (!fn_range_check("SETUNION", nfargs, 2, 5, buff, bufcx))
       return;

    sep = ' ';
    if ( (nfargs > 2) && *fargs[2] )
       sep = *fargs[2];

    osep = sep;
    if ( (nfargs > 3) && *fargs[3] )
       osep = *fargs[3];

    if ( (nfargs > 4) && *fargs[4] )
       sort_type = get_list_type_basic(*fargs[4]);
    else
       sort_type = ALPHANUM_LIST;

    handle_sets(fargs, buff, bufcx, SET_UNION, sep, osep, sort_type);
    return;
}

FUNCTION(fun_setdiff)
{
    char sep, osep;
    int sort_type;

    if (!fn_range_check("SETDIFF", nfargs, 2, 5, buff, bufcx))
       return;

    sep = ' ';
    if ( (nfargs > 2) && *fargs[2] )
       sep = *fargs[2];

    osep = sep;
    if ( (nfargs > 3) && *fargs[3] )
       osep = *fargs[3];

    if ( (nfargs > 4) && *fargs[4] )
       sort_type = get_list_type_basic(*fargs[4]);
    else
       sort_type = ALPHANUM_LIST;

    handle_sets(fargs, buff, bufcx, SET_DIFF, sep, osep, sort_type);
    return;
}

FUNCTION(fun_setinter)
{
    char sep, osep;
    int sort_type;

    if (!fn_range_check("SETINTER", nfargs, 2, 5, buff, bufcx))
       return;

    sep = ' ';
    if ( (nfargs > 2) && *fargs[2] )
       sep = *fargs[2];

    osep = sep;
    if ( (nfargs > 3) && *fargs[3] )
       osep = *fargs[3];

    if ( (nfargs > 4) && *fargs[4] )
       sort_type = get_list_type_basic(*fargs[4]);
    else
       sort_type = ALPHANUM_LIST;

    handle_sets(fargs, buff, bufcx, SET_INTERSECT, sep, osep, sort_type);
    return;
}

/* ---------------------------------------------------------------------------
 * rjust, ljust, center: Justify or center text, specifying fill character
 */

FUNCTION(fun_ljust)
{
    int spaces, i, i_len, filllen;
    char filler[LBUF_SIZE], *s, *t, t_buff[8];

    if ( !fn_range_check("LJUST", nfargs, 2, 3, buff, bufcx))
       return;

    memset(t_buff, '\0', sizeof(t_buff));
    memset(filler, '\0', sizeof(filler));
    spaces = atoi(fargs[1]);
    s = t = NULL;
    i_len = 0;
#ifndef ZENTY_ANSI
    if ( s );
#endif

    filllen = 1;
    if ( (nfargs > 2) && *fargs[2] ) {
       if ( strlen(strip_all_special(fargs[2])) < 1 )
          sprintf(filler, " ");
       else {
#ifdef ZENTY_ANSI
          s = strip_all_special(fargs[2]);          
          t = filler;
          while ( *s ) {
             if ( (*s == '%') && (*(s+1) == '<') && *(s+2) && *(s+3) && *(s+3) &&
                   isdigit(*(s+2)) && isdigit(*(s+3)) && isdigit(*(s+4)) &&
                   (*(s+5) == '>') ) {
                switch ( atoi(s+2) ) {
                   case 92: *t = (char) 28;
                            break;
                   case 37: *t = (char) 29;
                            break;
                   default: *t = (char) atoi(s+2);
                            break;
                }
                s+=5;
             } else {
                *t = *s;
             }
             s++;
             t++;
          }
#else
          sprintf(filler, "%.*s", (LBUF_SIZE - 1), strip_all_special(fargs[2]));
#endif
       }
    } else {
       sprintf(filler, " ");
    }
    filllen = strlen(filler);

    /* Sanitize number of spaces */
    if ( spaces > LBUF_SIZE) 
       spaces = LBUF_SIZE;
    safe_str(fargs[0], buff, bufcx);
    i_len = strlen(strip_all_special(fargs[0])) - count_extended(fargs[0]);
    for (i = i_len; i < spaces; i++) {
       if ( ((unsigned int)filler[i % filllen] >= 160) || 
            ((unsigned int)filler[i % filllen] == 28) ||
            ((unsigned int)filler[i % filllen] == 29) ) {
          switch( (unsigned int)filler[i % filllen] % 256 ) {
             case 28: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)92);
                      break;
             case 29: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)37);
                      break;
             default: sprintf(t_buff, "%c<%03u>", '%', ((unsigned int)filler[i % filllen]) % 256);
                      break;
          }
          safe_str(t_buff, buff, bufcx);
       } else {
          safe_chr(filler[i % filllen], buff, bufcx);
       }
    }
}

FUNCTION(fun_rjust)
{
    int spaces, i, filllen;
    char filler[LBUF_SIZE], t_buff[8], *s, *t;

    if ( !fn_range_check("RJUST", nfargs, 2, 3, buff, bufcx))
       return;

    filllen = 1;
    memset(t_buff, '\0', sizeof(t_buff));
    memset(filler, '\0', sizeof(filler));
    s = t = NULL;
#ifndef ZENTY_ANSI
    if ( s );
#endif

    if ( (nfargs > 2) && *fargs[2] ) {
       if ( strlen(strip_all_special(fargs[2])) < 1 ) {
          sprintf(filler, " ");
       } else {
#ifdef ZENTY_ANSI
          s = strip_all_special(fargs[2]);          
          t = filler;
          while ( *s ) {
             if ( (*s == '%') && (*(s+1) == '<') && *(s+2) && *(s+3) && *(s+3) &&
                   isdigit(*(s+2)) && isdigit(*(s+3)) && isdigit(*(s+4)) &&
                   (*(s+5) == '>') ) {
                switch ( atoi(s+2) ) {
                   case 92: *t = (char) 28;
                            break;
                   case 37: *t = (char) 29;
                            break;
                   default: *t = (char) atoi(s+2);
                            break;
                }
                s+=5;
             } else {
                *t = *s;
             }
             s++;
             t++;
          }
#else
          sprintf(filler, "%.*s", (LBUF_SIZE - 1), strip_all_special(fargs[2]));
#endif
      }
    } else {
       sprintf(filler, " ");
    }
    filllen = strlen(filler);

    spaces = atoi(fargs[1]) - (strlen(strip_all_special(fargs[0])) - count_extended(fargs[0]));

    /* Sanitize number of spaces */
    if ( spaces > LBUF_SIZE )
       spaces = LBUF_SIZE;

    for (i = 0; i < spaces; i++) {
       if ( ((unsigned int)filler[i % filllen] >= 160) || 
            ((unsigned int)filler[i % filllen] == 28) ||
            ((unsigned int)filler[i % filllen] == 29) ) {
          switch( (unsigned int)filler[i % filllen] % 256 ) {
             case 28: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)92);
                      break;
             case 29: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)37);
                      break;
             default: sprintf(t_buff, "%c<%03u>", '%', ((unsigned int)filler[i % filllen]) % 256);
                      break;
          }
          safe_str(t_buff, buff, bufcx);
       } else {
          safe_chr(filler[i % filllen], buff, bufcx);
       }
    }
    safe_str(fargs[0], buff, bufcx);
}

FUNCTION(fun_center)
{
    char filler[LBUF_SIZE], t_buff[8], *s, *t;

    int i, q, len, lead_chrs, trail_chrs, width, filllen;

    if (!fn_range_check("CENTER", nfargs, 2, 3, buff, bufcx))
      return;

    width = atoi(fargs[1]);
    len = strlen(strip_all_special(fargs[0])) - count_extended(fargs[0]);
    if ( (len >= width) || (width > (LBUF_SIZE -2)) ) {
       safe_str(fargs[0], buff, bufcx);
       return;
    }

    filllen = 1;
    memset(t_buff, '\0', sizeof(t_buff));
    memset(filler, '\0', sizeof(filler));
    s = t = NULL;

#ifndef ZENTY_ANSI
    if ( s );
#endif
   
    if ( (nfargs > 2) && *fargs[2] ) {
       if ( strlen(strip_all_special(fargs[2])) < 1 ) {
          sprintf(filler, " ");
       } else {
#ifdef ZENTY_ANSI
          s = strip_all_special(fargs[2]);          
          t = filler;
          while ( *s ) {
             if ( (*s == '%') && (*(s+1) == '<') && *(s+2) && *(s+3) && *(s+3) &&
                   isdigit(*(s+2)) && isdigit(*(s+3)) && isdigit(*(s+4)) &&
                   (*(s+5) == '>') ) {
                switch ( atoi(s+2) ) {
                   case 92: *t = (char) 28;
                            break;
                   case 37: *t = (char) 29;
                            break;
                   default: *t = (char) atoi(s+2);
                            break;
                }
                s+=5;
             } else {
                *t = *s;
             }
             s++;
             t++;
          }
#else
          sprintf(filler, "%.*s", (LBUF_SIZE - 1), strip_all_special(fargs[2]));
#endif
       }
    } else
       sprintf(filler, " ");
    filllen = strlen(filler);

    lead_chrs = (width / 2) - (len / 2) + .5;
    for (i = 0; i < lead_chrs; i++) {
       if ( ((unsigned int)filler[i % filllen] >= 160) || 
            ((unsigned int)filler[i % filllen] == 28) ||
            ((unsigned int)filler[i % filllen] == 29) ) {
          switch( (unsigned int)filler[i % filllen] % 256 ) {
             case 28: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)92);
                      break;
             case 29: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)37);
                      break;
             default: sprintf(t_buff, "%c<%03u>", '%', ((unsigned int)filler[i % filllen]) % 256);
                      break;
          }
          safe_str(t_buff, buff, bufcx);
       } else {
          safe_chr(filler[i % filllen], buff, bufcx);
       }
    }
    safe_str(fargs[0], buff, bufcx);
    trail_chrs = width - lead_chrs - len;
    q = i + (strlen(strip_all_special(fargs[0])) - count_extended(fargs[0]));
    for (i = 0; i < trail_chrs; i++) {
       if ( ((unsigned int)filler[q % filllen] >= 160) || 
            ((unsigned int)filler[q % filllen] == 28) ||
            ((unsigned int)filler[q % filllen] == 29) ) {
          switch( (unsigned int)filler[q % filllen] % 256 ) {
             case 28: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)92);
                      break;
             case 29: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)37);
                      break;
             default: sprintf(t_buff, "%c<%03u>", '%', ((unsigned int)filler[q % filllen]) % 256);
                      break;
          }
          safe_str(t_buff, buff, bufcx);
       } else {
          safe_chr(filler[q % filllen], buff, bufcx);
       }
       q++;
    }
}

/* ---------------------------------------------------------------------------
 * setq, r: set and read global registers.
 */

FUNCTION(fun_xdec)
{
   int num;

   num = atoi(fargs[0]);
   ival(buff, bufcx, --num);
}

FUNCTION(fun_dec)
{
    int regnum, val, i;
    char *pt1, *tpr_buff, *tprp_buff;

    regnum = atoi(fargs[0]);
    i = 0;
#ifdef EXPANDED_QREGS
    if ( (((regnum < 0) || (regnum > 9)) && isdigit((int)(*fargs[0]))) || !isalnum((int)*fargs[0]) ) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else {
       if ( isalpha((int)(*fargs[0])) ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( mudstate.nameofqreg[i] == tolower(*fargs[0]) )
                break;
          }
          regnum = i;
       }
       pt1 = mudstate.global_regs[regnum];
       if (*pt1 == '-')
          pt1++;
       while (isdigit((int)*pt1))
          pt1++;
       if (*pt1) {
          safe_str("#-1 GLOBAL REGISTER NOT AN INTEGER", buff, bufcx);
       } else {
          val = atoi(mudstate.global_regs[regnum]);
          val--;
          tprp_buff = tpr_buff = alloc_lbuf("fun_dec");
          strcpy(mudstate.global_regs[regnum], safe_tprintf(tpr_buff, &tprp_buff, "%d", val));
          free_lbuf(tpr_buff);
       }
    }
#else
    if ((regnum < 0) || (regnum >= MAX_GLOBAL_REGS)) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else if (mudstate.global_regs[regnum]) {
       pt1 = mudstate.global_regs[regnum];
       if (*pt1 == '-')
          pt1++;
       while (isdigit((int)*pt1))
          pt1++;
       if (*pt1) {
          safe_str("#-1 GLOBAL REGISTER NOT AN INTEGER", buff, bufcx);
       } else {
          val = atoi(mudstate.global_regs[regnum]);
          val--;
          tprp_buff = tpr_buff = alloc_lbuf("fun_dec");
          strcpy(mudstate.global_regs[regnum], safe_tprintf(tpr_buff, &tprp_buff, "%d", val));
          free_lbuf(tpr_buff);
       }
    }
#endif
}

FUNCTION(fun_xinc)
{
   int num;

   num = atoi(fargs[0]);
   ival(buff, bufcx, ++num);
}

FUNCTION(fun_inc)
{
    int regnum, val, i;
    char *pt1, *tpr_buff, *tprp_buff;

    regnum = atoi(fargs[0]);
    i = 0;
#ifdef EXPANDED_QREGS
    if ( (((regnum < 0) || (regnum > 9)) && isdigit((int)(*fargs[0]))) || !isalnum((int)(*fargs[0])) ) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else {
       if ( isalpha((int)(*fargs[0])) ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( mudstate.nameofqreg[i] == tolower(*fargs[0]) )
                break;
          }
          regnum = i;
       }
       pt1 = mudstate.global_regs[regnum];
       if (*pt1 == '-')
          pt1++;
       while (isdigit((int)*pt1))
          pt1++;
       if (*pt1) {
          safe_str("#-1 GLOBAL REGISTER NOT AN INTEGER", buff, bufcx);
       } else {
          val = atoi(mudstate.global_regs[regnum]);
          val++;
          tprp_buff = tpr_buff = alloc_lbuf("fun_inc");
          strcpy(mudstate.global_regs[regnum], safe_tprintf(tpr_buff, &tprp_buff, "%d", val));
          free_lbuf(tpr_buff);
       }
    }
#else
    if ((regnum < 0) || (regnum >= MAX_GLOBAL_REGS)) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else if (mudstate.global_regs[regnum]) {
       pt1 = mudstate.global_regs[regnum];
       if (*pt1 == '-')
           pt1++;
       while (isdigit((int)*pt1))
           pt1++;
       if (*pt1) {
           safe_str("#-1 GLOBAL REGISTER NOT AN INTEGER", buff, bufcx);
       } else {
           val = atoi(mudstate.global_regs[regnum]);
           val++;
           tprp_buff = tpr_buff = alloc_lbuf("fun_inc");
           strcpy(mudstate.global_regs[regnum], safe_tprintf(tpr_buff, &tprp_buff, "%d", val));
           free_lbuf(tpr_buff);
       }
    }
#endif
}

FUNCTION(fun_pushregs)
{
    int regnum, cntr;
    char *strtok, *strtokptr;

    if ( !fargs[0] || !*fargs[0] ) {
       safe_str("#-1 FUNCTION (PUSHREGS) EXPECTS 1 ARGUMENT [RECEIVED 0]", buff, bufcx);
       return;
    }

    if ( is_number(fargs[0]) && (strchr(fargs[0], '-') == NULL)) {
       regnum = atoi(fargs[0]);
       if ( regnum < 0 || regnum > 0 )
          regnum = 1;

       if ( regnum == 0 ) {
          for ( cntr = 0; cntr < MAX_GLOBAL_REGS; cntr++ ) {
             if ( !mudstate.global_regs_backup[cntr] )
                continue;
             if ( !mudstate.global_regs[cntr] )
                mudstate.global_regs[cntr] = alloc_lbuf("fun_pushregs");
             strcpy(mudstate.global_regs[cntr],
                    mudstate.global_regs_backup[cntr]);
             *mudstate.global_regs_backup[cntr]='\0';
          }
       } else {
          for ( cntr = 0; cntr < MAX_GLOBAL_REGS; cntr++ ) {
             if ( !mudstate.global_regs[cntr] )
                continue;
             if ( !mudstate.global_regs_backup[cntr] )
                mudstate.global_regs_backup[cntr] = alloc_lbuf("fun_pushregs");
             strcpy(mudstate.global_regs_backup[cntr],
                    mudstate.global_regs[cntr]);
          }
       }
    } else {
       strtok = strtok_r(fargs[0], " \t", &strtokptr);
       while ( strtok ) {
          regnum = -1;
          if ( (*strtok == '+') && *(strtok+1) && !*(strtok+2) ) {
             if (isdigit(*(strtok+1))) {
                regnum = atoi(strtok+1);
             } else if (isalpha(*(strtok+1))) {
                regnum = 10 + (int)ToLower(*(strtok+1)) - (int)'a';
             }
             if ( (regnum >= 0) && (regnum < MAX_GLOBAL_REGS) ) {
                if ( mudstate.global_regs[regnum] ) {
                   if ( !mudstate.global_regs_backup[regnum] )
                      mudstate.global_regs_backup[regnum] = alloc_lbuf("fun_pushregs");
                   strcpy(mudstate.global_regs_backup[regnum],
                          mudstate.global_regs[regnum]);
                }
             }
          } else if ( (*strtok == '-') && *(strtok+1) && !*(strtok+2) ) {
             if (isdigit(*(strtok+1))) {
                regnum = atoi(strtok+1);
             } else if (isalpha(*(strtok+1))) {
                regnum = 10 + (int)ToLower(*(strtok+1)) - (int)'a';
             }
             if ( (regnum >= 0) && (regnum < MAX_GLOBAL_REGS) ) {
                if ( mudstate.global_regs_backup[regnum] ) {
                   if ( !mudstate.global_regs[regnum] )
                      mudstate.global_regs[regnum] = alloc_lbuf("fun_pushregs");
                   strcpy(mudstate.global_regs[regnum],
                          mudstate.global_regs_backup[regnum]);
                }
             }
          }
          strtok = strtok_r(NULL, " \t", &strtokptr);
       }
    }
}

FUNCTION(fun_nameq)
{
    int regnum, i_namefnd, i, i_returnnum;

    if (!fn_range_check("SETQ", nfargs, 1, 3, buff, bufcx))
      return;

    if ( !*fargs[0] )
       return;

    if ( !(nfargs > 2) && (nfargs > 1) && *fargs[1] && (strlen(fargs[1]) < 2) )
       return;

    i_namefnd = i_returnnum = 0;
    if ( (nfargs > 2) && *fargs[2] )
       i_returnnum = atoi(fargs[2]);

    if ( strlen(fargs[0]) > 1 ) {
       regnum = -1;
       for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
          if ( mudstate.global_regsname[i] && *(mudstate.global_regsname[i]) &&
               (stricmp(mudstate.global_regsname[i], fargs[0]) == 0) ) {
             regnum = i;
             i_namefnd = 1;
             break;
          }
       }
    } else {
       if (!is_number(fargs[0]))
          regnum = -1;
       else
          regnum = atoi(fargs[0]);
    }
#ifdef EXPANDED_QREGS
    if ( (regnum == -1) && isalpha(*fargs[0]) ) {
       if ( !i_namefnd && isalpha((int)*fargs[0]) ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( mudstate.nameofqreg[i] == tolower(*fargs[0]) )
                break;
          }
          if ( i < MAX_GLOBAL_REGS )
             regnum = i;
       }
    }
#endif
    if ( regnum == -1 ) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
       return;
    } else {
       if ( (nfargs == 1) || (nfargs == 3) ) {
          if ( i_returnnum && i_namefnd ) {
#ifdef EXPANDED_QREGS
             safe_chr(mudstate.nameofqreg[regnum], buff, bufcx);
#else
             ival(regnum, buff, bufcx);
#endif
          } else {
             safe_str(mudstate.global_regsname[regnum], buff, bufcx);
          }
       } else {
          strncpy(mudstate.global_regsname[regnum], fargs[1], (SBUF_SIZE - 1));
          *(mudstate.global_regsname[regnum] + SBUF_SIZE - 1) = '\0';
       }
    }
}

FUNCTION(fun_setq_old)
{
    int regnum, i, i_namefnd;

    if (!fn_range_check("SETQ", nfargs, 2, 3, buff, bufcx))
      return;

    regnum = -1;
    i_namefnd = 0;
    if ( ((strcmp(fargs[0], "!") == 0) || strcmp(fargs[0], "+") == 0) && 
         (nfargs > 2) && *fargs[2] ) {
       /* First, walk the list to match the variable name */
       for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
          if ( mudstate.global_regsname[i] && *mudstate.global_regsname[i] &&
               (stricmp(mudstate.global_regsname[i], fargs[2]) == 0) ) {
             regnum = i;
             i_namefnd = 1;
             break;
          }
       }
       if ( !i_namefnd ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( !mudstate.global_regsname[i] || !*mudstate.global_regsname[i] ) {
                if ( (strcmp(fargs[0], "+") == 0) && mudstate.global_regs[i] &&
                  *mudstate.global_regs[i] ) {
                   continue;
                }
                regnum = i;
                i_namefnd = 1;
                break;
             }
          }
       }
    } else {
       if ( strlen(fargs[0]) > 1 ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( *(mudstate.global_regsname[i]) &&
                  !stricmp(mudstate.global_regsname[i], fargs[0]) ) {
                regnum = i;
                i_namefnd = 1;
                break;
             }
          }
       } else {
          if ( !is_number(fargs[0]) )
             regnum = -1;
          else
             regnum = atoi(fargs[0]);
       }
    }
    i = 0;
#ifdef EXPANDED_QREGS
    if ( !i_namefnd &&
         ((((regnum < 0) || (regnum > 9)) && isdigit((int)(*fargs[0]))) || !isalnum((int)(*fargs[0]))) ) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else {
       if ( !i_namefnd && isalpha((int)*fargs[0]) ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( mudstate.nameofqreg[i] == tolower(*fargs[0]) )
                break;
          }
          regnum = i;
       }

       if (!mudstate.global_regs[regnum])
          mudstate.global_regs[regnum] = alloc_lbuf("fun_setq");
       strcpy(mudstate.global_regs[regnum], fargs[1]);
       if (!mudstate.global_regsname[regnum])
          mudstate.global_regsname[regnum] = alloc_sbuf("fun_setq_name");
       if ( (nfargs > 2) && *fargs[2] ) {
          strncpy(mudstate.global_regsname[regnum], fargs[2], (SBUF_SIZE - 1));
          *(mudstate.global_regsname[regnum] + SBUF_SIZE - 1) = '\0';
       }
    }
#else
    if ((regnum < 0) || (regnum >= MAX_GLOBAL_REGS)) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else {
       if (!mudstate.global_regs[regnum])
           mudstate.global_regs[regnum] = alloc_lbuf("fun_setq");
       strcpy(mudstate.global_regs[regnum], fargs[1]);
       if (!mudstate.global_regsname[regnum])
          mudstate.global_regsname[regnum] = alloc_sbuf("fun_setq_name");
       if ( (nfargs > 2) && *fargs[2] ) {
          strncpy(mudstate.global_regsname[regnum], fargs[2], (SBUF_SIZE - 1));
          *(mudstate.global_regsname[regnum] + SBUF_SIZE - 1) = '\0';
       }
    }
#endif
}

FUNCTION(fun_setq)
{
    int regnum, i, i_namefnd;
    char *result, *result_orig;

    if (!fn_range_check("SETQ", nfargs, 2, 3, buff, bufcx))
      return;

    result_orig = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                       fargs[0], cargs, ncargs);

    i_namefnd = 0;
    regnum = -1;
    if ( ((strcmp(result_orig, "!") == 0) || (strcmp(result_orig, "+") == 0)) && 
          (nfargs > 2) && *fargs[2] ) {
       /* First, walk the list to match the variable name */
       for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
          if ( mudstate.global_regsname[i] && *mudstate.global_regsname[i] &&
               (stricmp(mudstate.global_regsname[i], result_orig) == 0) ) {
             regnum = i;
             i_namefnd = 1;
             break;
          }
       }
       if ( !i_namefnd ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( !mudstate.global_regsname[i] || !*mudstate.global_regsname[i] ) {
                if ( (strcmp(result_orig, "+") == 0) && mudstate.global_regs[i] &&
                  *mudstate.global_regs[i] ) {
                   continue;
                }
                regnum = i;
                i_namefnd = 1;
                break;
             }
          }
       }
    } else {
       if ( strlen(result_orig) > 1 ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( *(mudstate.global_regsname[i]) &&
                  !stricmp(mudstate.global_regsname[i], result_orig) ) {
                regnum = i;
                i_namefnd = 1;
                break;
             }
          }
       } else {
          if ( !is_number(result_orig) )
             regnum = -1;
          else
             regnum = atoi(result_orig);
       }
    }
    i = 0;
#ifdef EXPANDED_QREGS
    if ( !i_namefnd &&
         ((((regnum < 0) || (regnum > 9)) && isdigit((int)*result_orig)) || !isalnum((int)*result_orig)) ) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else {
       if ( !i_namefnd && isalpha((int)*result_orig) ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( mudstate.nameofqreg[i] == tolower(*result_orig) )
                break;
          }
          regnum = i;
       }

       if (!mudstate.global_regs[regnum])
          mudstate.global_regs[regnum] = alloc_lbuf("fun_setq");
       result = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                     fargs[1], cargs, ncargs);
       strcpy(mudstate.global_regs[regnum], result);
       if (!mudstate.global_regsname[regnum])
          mudstate.global_regsname[regnum] = alloc_sbuf("fun_setq_name");
       if ( (nfargs > 2) && *fargs[2] ) {
          strncpy(mudstate.global_regsname[regnum], fargs[2], (SBUF_SIZE - 1));
          *(mudstate.global_regsname[regnum] + SBUF_SIZE - 1) = '\0';
       }
       free_lbuf(result);
    }
#else
    if ((regnum < 0) || (regnum >= MAX_GLOBAL_REGS)) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else {
       if (!mudstate.global_regs[regnum])
           mudstate.global_regs[regnum] = alloc_lbuf("fun_setq");
       result = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                     fargs[1], cargs, ncargs);
       strcpy(mudstate.global_regs[regnum], result);
       if (!mudstate.global_regsname[regnum])
          mudstate.global_regsname[regnum] = alloc_sbuf("fun_setq_name");
       if ( (nfargs > 2) && *fargs[2] ) {
          strncpy(mudstate.global_regsname[regnum], fargs[2], (SBUF_SIZE - 1));
          *(mudstate.global_regsname[regnum] + SBUF_SIZE - 1) = '\0';
       }
       free_lbuf(result);
    }
#endif
    free_lbuf(result_orig);
}

/* fun_setr: works like setq but returns it's input as well */

FUNCTION(fun_setr_old)
{
    int regnum, i, i_namefnd;

    if (!fn_range_check("SETR", nfargs, 2, 3, buff, bufcx))
      return;

    i_namefnd = 0;
    regnum = -1;
    if ( ((strcmp(fargs[0], "!") == 0) || (strcmp(fargs[0], "+") == 0)) && 
         (nfargs > 2) && *fargs[2] ) {
       /* First, walk the list to match the variable name */
       for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
          if ( mudstate.global_regsname[i] && *mudstate.global_regsname[i] &&
               (stricmp(mudstate.global_regsname[i], fargs[2]) == 0) ) {
             regnum = i;
             i_namefnd = 1;
             break;
          }
       }
       if ( !i_namefnd ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( !mudstate.global_regsname[i] || !*mudstate.global_regsname[i] ) {
                if ( (strcmp(fargs[0], "+") == 0) && mudstate.global_regs[i] &&
                  *mudstate.global_regs[i] ) {
                   continue;
                }
                regnum = i;
                i_namefnd = 1;
                break;
             }
          }
       }
    } else {
       if ( strlen(fargs[0]) > 1 ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( *(mudstate.global_regsname[i]) &&
                  !stricmp(mudstate.global_regsname[i], fargs[0]) ) {
                regnum = i;
                i_namefnd = 1;
                break;
             }
          }
       } else {
          if (!is_number(fargs[0]))
             regnum = -1;
          else
             regnum = atoi(fargs[0]);
       }
    }
    i = 0;
#ifdef EXPANDED_QREGS
    if ( !i_namefnd &&
         ((((regnum < 0) || (regnum > 9)) && isdigit((int)*fargs[0])) || !isalnum((int)*fargs[0])) ) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else {
       if ( !i_namefnd && isalpha((int)*fargs[0]) ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( mudstate.nameofqreg[i] == tolower(*fargs[0]) )
                break;
          }
          regnum = i;
       }

       if (!mudstate.global_regs[regnum])
          mudstate.global_regs[regnum] = alloc_lbuf("fun_setr");
       strcpy(mudstate.global_regs[regnum], fargs[1]);
       if (!mudstate.global_regsname[regnum])
          mudstate.global_regsname[regnum] = alloc_sbuf("fun_setq_name");
       if ( (nfargs > 2) && *fargs[2] ) {
          strncpy(mudstate.global_regsname[regnum], fargs[2], (SBUF_SIZE - 1));
          *(mudstate.global_regsname[regnum] + SBUF_SIZE - 1) = '\0';
       }
       safe_str(fargs[1], buff, bufcx);
    }
#else
    if ((regnum < 0) || (regnum >= MAX_GLOBAL_REGS)) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else {
       if (!mudstate.global_regs[regnum])
           mudstate.global_regs[regnum] = alloc_lbuf("fun_setr");
       strcpy(mudstate.global_regs[regnum], fargs[1]);
       if (!mudstate.global_regsname[regnum])
          mudstate.global_regsname[regnum] = alloc_sbuf("fun_setq_name");
       if ( (nfargs > 2) && *fargs[2] ) {
          strncpy(mudstate.global_regsname[regnum], fargs[2], (SBUF_SIZE - 1));
          *(mudstate.global_regsname[regnum] + SBUF_SIZE - 1) = '\0';
       }
       safe_str(fargs[1], buff, bufcx);
    }
#endif
}

FUNCTION(fun_setr)
{
    int regnum, i, i_namefnd;
    char *result, *result_orig;

    if (!fn_range_check("SETR", nfargs, 2, 3, buff, bufcx))
      return;

    result_orig = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                       fargs[0], cargs, ncargs);

    i_namefnd = 0;
    regnum = -1;
    if ( ((strcmp(result_orig, "!") == 0) || (strcmp(result_orig, "+") == 0)) && 
         (nfargs > 2) && *fargs[2] ) {
       /* First, walk the list to match the variable name */
       for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
          if ( mudstate.global_regsname[i] && *mudstate.global_regsname[i] &&
               (stricmp(mudstate.global_regsname[i], result_orig) == 0) ) {
             regnum = i;
             i_namefnd = 1;
             break;
          }
       }
       if ( !i_namefnd ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( !mudstate.global_regsname[i] || !*mudstate.global_regsname[i] ) {
                if ( (strcmp(result_orig, "+") == 0) && mudstate.global_regs[i] &&
                  *mudstate.global_regs[i] ) {
                   continue;
                }
                regnum = i;
                i_namefnd = 1;
                break;
             }
          }
       }
    } else {
       if ( strlen(result_orig) > 1 ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( *(mudstate.global_regsname[i]) &&
                  !stricmp(mudstate.global_regsname[i], result_orig) ) {
                regnum = i;
                i_namefnd = 1;
                break;
             }
          }
       } else {
          if (!is_number(result_orig))
             regnum = -1;
          else
             regnum = atoi(result_orig);
       }
    }
    i = 0;
#ifdef EXPANDED_QREGS
    if ( !i_namefnd &&
         ((((regnum < 0) || (regnum > 9)) && isdigit((int)*result_orig)) || !isalnum((int)*result_orig)) ) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else {
       if ( !i_namefnd && isalpha((int)*result_orig) ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( mudstate.nameofqreg[i] == tolower(*result_orig) )
                break;
          }
          regnum = i;
       }

       if (!mudstate.global_regs[regnum])
          mudstate.global_regs[regnum] = alloc_lbuf("fun_setr");
       result = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                     fargs[1], cargs, ncargs);
       strcpy(mudstate.global_regs[regnum], result);
       if (!mudstate.global_regsname[regnum])
          mudstate.global_regsname[regnum] = alloc_sbuf("fun_setq_name");
       if ( (nfargs > 2) && *fargs[2] ) {
          strncpy(mudstate.global_regsname[regnum], fargs[2], (SBUF_SIZE - 1));
          *(mudstate.global_regsname[regnum] + SBUF_SIZE - 1) = '\0';
       }
       safe_str(result, buff, bufcx);
       free_lbuf(result);
    }
#else
    if ((regnum < 0) || (regnum >= MAX_GLOBAL_REGS)) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else {
       if (!mudstate.global_regs[regnum])
           mudstate.global_regs[regnum] = alloc_lbuf("fun_setr");
       result = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                     fargs[1], cargs, ncargs);
       strcpy(mudstate.global_regs[regnum], result);
       if (!mudstate.global_regsname[regnum])
          mudstate.global_regsname[regnum] = alloc_sbuf("fun_setq_name");
       if ( (nfargs > 2) && *fargs[2] ) {
          strncpy(mudstate.global_regsname[regnum], fargs[2], (SBUF_SIZE - 1));
          *(mudstate.global_regsname[regnum] + SBUF_SIZE - 1) = '\0';
       }
       safe_str(result, buff, bufcx);
       free_lbuf(result);
    }
#endif
    free_lbuf(result_orig);
}

FUNCTION(fun_r)
{
    int regnum, i, i_namefnd;

    i_namefnd = 0;
    regnum = -1;
    if ( strlen(fargs[0]) > 1 ) {
       for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
          if ( *(mudstate.global_regsname[i]) &&
               !stricmp(mudstate.global_regsname[i], fargs[0]) ) {
             regnum = i;
             i_namefnd = 1;
             break;
          }
       }
    } else {
       if (!is_number(fargs[0]))
          regnum = -1;
       else
          regnum = atoi(fargs[0]);
    }
    i = 0;
#ifdef EXPANDED_QREGS
    if ( !i_namefnd &&
         ((((regnum < 0) || (regnum > 9)) && isdigit((int)*fargs[0])) || !isalnum((int)*fargs[0])) ) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else {
       if ( !i_namefnd && isalpha((int)*fargs[0]) ) {
          for ( i = 0 ; i < MAX_GLOBAL_REGS; i++ ) {
             if ( mudstate.nameofqreg[i] == tolower(*fargs[0]) )
                break;
          }
          regnum = i;
       }
       if ( mudstate.global_regs[regnum])
          safe_str(mudstate.global_regs[regnum], buff, bufcx);
    }
#else
    if ((regnum < 0) || (regnum >= MAX_GLOBAL_REGS)) {
       safe_str("#-1 INVALID GLOBAL REGISTER", buff, bufcx);
    } else if (mudstate.global_regs[regnum]) {
       safe_str(mudstate.global_regs[regnum], buff, bufcx);
    }
#endif
}

/* ---------------------------------------------------------------------------
 * isnum: is the argument a number?
 */

FUNCTION(fun_isnum)
{
    safe_str((is_number(fargs[0]) ? "1" : "0"), buff, bufcx);
}

FUNCTION(fun_isalnum)
{
    safe_str((isalnum((int)*fargs[0]) ? "1" : "0"), buff, bufcx);
}

FUNCTION(fun_isalpha)
{
    safe_str((isalpha((int)*fargs[0]) ? "1" : "0"), buff, bufcx);
}

FUNCTION(fun_isint)
{
    safe_str((is_rhointeger((char *)fargs[0]) ? "1" : "0"), buff, bufcx);
}

FUNCTION(fun_isdigit)
{
    safe_str((isdigit((int)*fargs[0]) ? "1" : "0"), buff, bufcx);
}

FUNCTION(fun_islower)
{
    safe_str((islower((int)*fargs[0]) ? "1" : "0"), buff, bufcx);
}

FUNCTION(fun_ispunct)
{
    safe_str((ispunct((int)*fargs[0]) ? "1" : "0"), buff, bufcx);
}

FUNCTION(fun_isspace)
{
    safe_str((isspace((int)*fargs[0]) ? "1" : "0"), buff, bufcx);
}

FUNCTION(fun_isupper)
{
    safe_str((isupper((int)*fargs[0]) ? "1" : "0"), buff, bufcx);
}

FUNCTION(fun_isxdigit)
{
    safe_str((isxdigit((int)*fargs[0]) ? "1" : "0"), buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * isdbref: is the argument a valid dbref?
 */

FUNCTION(fun_isdbref)
{
    char *p;
    dbref dbitem;

    p = fargs[0];
    if (*p++ == NUMBER_TOKEN) {
      if ( strlen(fargs[0]) > 1 ) {
         dbitem = parse_dbref(p);
         if (Good_obj(dbitem)) {
             safe_str("1", buff, bufcx);
             return;
         }
      }
    }
    safe_str("0", buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * trim: trim off unwanted white space.
 */

FUNCTION(fun_trim)
{
    char *p, *lastchar, *q, *sep;
    int trim, trim_size;

    if (nfargs == 0) {
         return;
    }
    if (!fn_range_check("TRIM", nfargs, 1, 3, buff, bufcx))
       return;
    if (nfargs >= 2) {
         switch (ToLower((int)*fargs[1])) {
                case 'l':
                    trim = 1;
                    break;
                case 'r':
                    trim = 2;
                    break;
                default:
                    trim = 3;
                    break;
         }
    } else {
         trim = 3;
    }
    sep = alloc_lbuf("fun_trim");
    if ( nfargs >= 3 ) {
       sprintf(sep, "%s", fargs[2]);
    } else {
       sprintf(sep, " ");
    }
    trim_size = strlen(sep);
    if (trim == 2 || trim == 3) {
         p = lastchar = fargs[0];
         while (*p != '\0') {
             if ( memchr(sep, *p, trim_size) == NULL )
                  lastchar = p;
             p++;
         }
         *(lastchar + 1) = '\0';
    }
    q = fargs[0];
    if (trim == 1 || trim == 3) {
         while (*q != '\0') {
             if ( memchr(sep, *q, trim_size) != NULL )
                  q++;
             else
                  break;
         }
    }
    safe_str(q, buff, bufcx);
    free_lbuf(sep);
}

FUNCTION(fun_chomp)
{
  /* release the trailing newlines */
  char *lastchar, *p, *pStartString, sep, *tpr_buff, *tprp_buff, mylastchr;
  int i, strlength, chomp;

  if (nfargs == 0) {
    return;
  }

  lastchar = NULL;
  mvarargs_preamble("CHOMP", 1, 2);
  if (nfargs == 2) {
    switch (ToLower((int)*fargs[1])) {
    case 'l':
      chomp = 1;
      break;
    case 'r':
      chomp = 2;
      break;
    case 'b':
      chomp = 3;
      break;
    default:
      tprp_buff = tpr_buff = alloc_lbuf("fun_chomp");
      if ( *fargs[1] )
         safe_str(safe_tprintf(tpr_buff, &tprp_buff, "#-1 INVALID ARGUMENT TO CHOMP (%c)", *fargs[1]), buff, bufcx);
      else
         safe_str(safe_tprintf(tpr_buff, &tprp_buff, "#-1 INVALID ARGUMENT TO CHOMP (NULL)", *fargs[1]), buff, bufcx);
      free_lbuf(tpr_buff);
      return;
    }
  } else {
    chomp = 2;
  }

  pStartString = fargs[0];

  if (chomp >= 2) {
    strlength = strlen(fargs[0]);
    lastchar = NULL;
    /* It's -1 because we want to skip over the existing null */
    for (i = (strlength - 1) ; i > 0 ; i--) {
      if ((fargs[0])[i] == '\r' ||
          (fargs[0])[i] == '\n' ||
          ((fargs[0])[i] == '%' && (lastchar && *lastchar == 'r') &&
           i>0 && (fargs[0])[i-1] != '%')) {
        (fargs[0])[i] = '\0';
      } else if ((fargs[0])[i] != 'r') {
         break;
      }
      lastchar = &(fargs[0])[i];
    }
  }

  if ( (chomp == 1) || (chomp == 3) ) {
    p = fargs[0];
    mylastchr = '\0';
    while ( p && (*p != '\0') ) {
      if ( *p == '\n' ||
           *p == '\r' ) {
         mylastchr = *p;
         p++;
      } else if ( (*p == '%') && (mylastchr != '%') && *(p+1) && (*(p+1) == 'r') ) {
         mylastchr = *(p+1);
         p=p+2;
      } else {
         break;
      }
    }
    pStartString = p;
  }


  safe_str(pStartString, buff, bufcx);
}

FUNCTION(fun_asc)
{
    if (strlen(fargs[0]) > 1) {
         safe_str("#-1 TOO MANY CHARACTERS", buff, bufcx);
    } else {
         ival(buff, bufcx, (int)*fargs[0]);
    }
}

FUNCTION(fun_chr)
{
    int i;
    char s_buff[8];

    if (!is_number(fargs[0])) {
         safe_str("#-1 ARGUMENT NOT A NUMBER", buff, bufcx);
    } else {
         i = atoi(fargs[0]);
         if ( (i == 37) || (i == 92) || (i < 32) || (i > 126)) {
             if ( (i == 37) || (i == 92) || ((i >= 160) && (i <= 255)) ) {
                sprintf(s_buff, "%%<%03d>", i); 
                safe_str(s_buff, buff, bufcx);
             } else
                safe_str("#-1 ARGUMENT OUT OF RANGE", buff, bufcx);
         } else {
            safe_chr((char)i, buff, bufcx);
         }
    }
}

/* fun_trace: set or unset the trace flag on an object */

FUNCTION(fun_chktrace)
{
   ival(buff, bufcx, ((Flags(player) & TRACE) ? 1 : 0));
}

FUNCTION(fun_trace)
{
  if( atoi(fargs[0]) ) {
    s_Flags(player, (Flags(player) | TRACE));
  } else {
    s_Flags(player, (Flags(player) & ~TRACE));
  }
}

/* ---------------------------------------------------------------------------
 * fun_ljc: Left justify, pad, and truncate if necessary
 *
 * Thorin 11/14/95
 */

FUNCTION(fun_ljc)
{
  int len, idx, filllen;
#ifndef ZENTY_ANSI
  int inlen;
#endif
  char *tptr, filler[LBUF_SIZE], t_buff[8], *s, *t;

  if (!fn_range_check("LJC", nfargs, 2, 3, buff, bufcx)) {
     return;
  }

#ifndef ZENTY_ANSI
  inlen = strlen(strip_all_special(fargs[0])) - count_extended(fargs[0]);
  if ( s );
#endif
  len = atoi(fargs[1]);

  /* seperator */
  filllen = 1;
  memset(filler, '\0', sizeof(filler));
  memset(t_buff, '\0', sizeof(t_buff));
  s = t = NULL;
  if ( (nfargs > 2) && *fargs[2] ) {
     if ( strlen(strip_all_special(fargs[2])) < 1 )
        sprintf(filler, " ");
     else
#ifdef ZENTY_ANSI
        s = strip_all_special(fargs[2]);          
        t = filler;
        while ( *s ) {
           if ( (*s == '%') && (*(s+1) == '<') && *(s+2) && *(s+3) && *(s+3) &&
                 isdigit(*(s+2)) && isdigit(*(s+3)) && isdigit(*(s+4)) &&
                 (*(s+5) == '>') ) {
              switch ( atoi(s+2) ) {
                 case 92: *t = (char) 28;
                          break;
                 case 37: *t = (char) 29;
                          break;
                 default: *t = (char) atoi(s+2);
                          break;
              }
              s+=5;
           } else {
              *t = *s;
           }
           s++;
           t++;
        }
#else
        sprintf(filler, "%.*s", (LBUF_SIZE - 1), strip_all_special(fargs[2]));
#endif
  } else {
     sprintf(filler, " ");
  }
  filllen = strlen(filler);

  if( len <= 0 )
    return;

  if( len > LBUF_SIZE - 1 ) {  /* save us some time if possible */
    len = LBUF_SIZE - 1;
  }

#ifdef ZENTY_ANSI
  for(tptr = fargs[0], idx=0;*tptr;tptr++) {
      if( (*tptr == '%') && ((*(tptr+1) == 'f') && isprint(*(tptr+2))) ) {
         safe_chr(*tptr++, buff, bufcx);
         safe_chr(*tptr++, buff, bufcx);
         safe_chr(*tptr, buff, bufcx);
         continue;
      }
      if( (*tptr == '%') && ((*(tptr+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                         || (*(tptr+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                         || (*(tptr+1) == SAFE_CHR3)
#endif
)) {
         if ( isAnsi[(int) *(tptr+2)] ) {
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr, buff, bufcx);
            continue;
         }
         if ( (*(tptr+2) == '0') && ((*(tptr+3) == 'x') || (*(tptr+3) == 'X')) &&
              *(tptr+4) && *(tptr+5) && isxdigit(*(tptr+4)) && isxdigit(*(tptr+5)) ) {
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr, buff, bufcx);
            continue;
         }
      }
      if(idx < len) {
         if ( (*tptr == '%') && (*(tptr+1) == '<') && *(tptr+2) && *(tptr+3) && *(tptr+3) &&
              isdigit(*(tptr+2)) && isdigit(*(tptr+3)) && isdigit(*(tptr+4)) &&
              (*(tptr+5) == '>') ) {
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
         }
         safe_chr(*tptr, buff, bufcx);
      }
      idx++;
  }
/* Second, add trailing spaces.   */
  while(idx < len) {
       if ( ((unsigned int)filler[idx % filllen] >= 160) || 
            ((unsigned int)filler[idx % filllen] == 28) ||
            ((unsigned int)filler[idx % filllen] == 29) ) {
          switch( (unsigned int)filler[idx % filllen] % 256 ) {
             case 28: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)92);
                      break;
             case 29: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)37);
                      break;
             default: sprintf(t_buff, "%c<%03u>", '%', ((unsigned int)filler[idx % filllen]) % 256);
                      break;
          }
          safe_str(t_buff, buff, bufcx);
       } else {
          safe_chr(filler[idx % filllen], buff, bufcx);
       }
      idx++;
  }
#else
  for( tptr = fargs[0], idx = 0; idx < len; idx++) {
      if( idx < inlen ) {
          safe_chr(*tptr, buff, bufcx);
          tptr++;
      } else {
          safe_chr(filler[idx % filllen], buff, bufcx);
      }
  }
#endif

}


/* ---------------------------------------------------------------------------
 * fun_rjc: Right justify, pad, and truncate if necessary
 *
 * Thorin 11/14/95
 */

FUNCTION(fun_countspecial)
{
  int i_key, i_val;
  char *s;

  if (!fn_range_check("COUNTSPECIAL", nfargs, 1, 2, buff, bufcx)) {
     return;
  }

  i_val = 0;
  
  if ( !*fargs[0] ) {
     ival(buff, bufcx, i_val);
     return;
  }

  if ( (nfargs > 1) && *fargs[1] )
     i_key = atoi(fargs[1]);
  
  if ( (i_key < 0) || (i_key > 2) )
     i_key = 0;
  
  s = fargs[0];
  while ( *s ) {
     if ( (*s == '%') && (*(s+1) == '<') && *(s+2) && *(s+3) && *(s+3) && 
          isdigit(*(s+2)) && isdigit(*(s+3)) && isdigit(*(s+4)) &&
          (*(s+5) == '>') ) {
        if ( i_key > 0 ) {
           i_val += 5;
        } else {
           i_val++;
        }
        s+=5;
     }
     if ( i_key == 2 )
        i_val++;
     s++;
  }
  ival(buff, bufcx, i_val);
}

FUNCTION(fun_rjc)
{
  int len, inlen, idx, spaces, filllen;
  char *tptr, filler[LBUF_SIZE], t_buff[8], *s, *t;

  if (!fn_range_check("RJC", nfargs, 2, 3, buff, bufcx)) {
     return;
  }
  inlen = strlen(strip_all_special(fargs[0]));
  len = atoi(fargs[1]);

  memset(filler, '\0', sizeof(filler));
  memset(t_buff, '\0', sizeof(t_buff));
  s = t = NULL;
  filllen = 1;

#ifndef ZENTY_ANSI
  if ( s );
#endif

  if ( (nfargs > 2) && *fargs[2] ) {
     if ( strlen(strip_all_special(fargs[2])) < 1 ) {
        sprintf(filler, " ");
     } else {
#ifdef ZENTY_ANSI
        s = strip_all_special(fargs[2]);          
        t = filler;
        while ( *s ) {
           if ( (*s == '%') && (*(s+1) == '<') && *(s+2) && *(s+3) && *(s+3) &&
                 isdigit(*(s+2)) && isdigit(*(s+3)) && isdigit(*(s+4)) &&
                 (*(s+5) == '>') ) {
              switch ( atoi(s+2) ) {
                 case 92: *t = (char) 28;
                          break;
                 case 37: *t = (char) 29;
                          break;
                 default: *t = (char) atoi(s+2);
                          break;
              }
              s+=5;
           } else {
              *t = *s;
           }
           s++;
           t++;
        }
#else
        sprintf(filler, "%.*s", (LBUF_SIZE - 1), strip_all_special(fargs[2]));
#endif
     }
  } else {
     sprintf(filler, " ");
  }
  filllen = strlen(filler);

  if( len <= 0 )
    return;

  if( len > LBUF_SIZE - 1 ) {  /* save us some time if possible */
    len = LBUF_SIZE - 1;
  }

  inlen -= count_extended(fargs[0]);
  if( inlen > len ) {
    spaces = 0;
  }
  else {
    spaces = len - inlen;
  }

  for( idx = 0; idx < spaces; idx++ ) {
    if ( ((unsigned int)filler[idx % filllen] >= 160) || 
         ((unsigned int)filler[idx % filllen] == 28) ||
         ((unsigned int)filler[idx % filllen] == 29) ) {
       switch( (unsigned int)filler[idx % filllen] % 256 ) {
          case 28: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)92);
                   break;
          case 29: sprintf(t_buff, "%c<%03u>", '%', (unsigned int)37);
                   break;
          default: sprintf(t_buff, "%c<%03u>", '%', ((unsigned int)filler[idx % filllen]) % 256);
                   break;
       }
       safe_str(t_buff, buff, bufcx);
    } else {
       safe_chr(filler[idx % filllen], buff, bufcx);
    }
  }
#ifdef ZENTY_ANSI
  // Traverse the entire string.
  for(tptr = fargs[0];*tptr;tptr++) {
      if( (*tptr == '%') && ((*(tptr+1) == 'f') && isprint(*(tptr+2))) ) {
         safe_chr(*tptr++, buff, bufcx);
         safe_chr(*tptr++, buff, bufcx);
         safe_chr(*tptr, buff, bufcx);
         continue;
      }
      if( (*tptr == '%') && ((*(tptr+1) == SAFE_CHR)
#ifdef SAFE_CHR2
                         || (*(tptr+1) == SAFE_CHR2)
#endif
#ifdef SAFE_CHR3
                         || (*(tptr+1) == SAFE_CHR3)
#endif
)) {
         if ( isAnsi[(int) *(tptr+2)] ) {
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr, buff, bufcx);
            continue;
         }
         if ( (*(tptr+2) == '0') && ((*(tptr+3) == 'x') || (*(tptr+3) == 'X')) &&
              *(tptr+4) && *(tptr+5) && isxdigit(*(tptr+4)) && isxdigit(*(tptr+5)) ) {
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr, buff, bufcx);
            continue;
         }
      }
      if (idx < len) {
         if ( (*tptr == '%') && (*(tptr+1) == '<') && *(tptr+2) && *(tptr+3) && *(tptr+3) &&
              isdigit(*(tptr+2)) && isdigit(*(tptr+3)) && isdigit(*(tptr+4)) &&
              (*(tptr+5) == '>') ) {
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
            safe_chr(*tptr++, buff, bufcx);
         }
         safe_chr(*tptr, buff, bufcx);
      }
      idx++;
  }
#else

  for( tptr = fargs[0]; idx < len; idx++) {
      safe_chr(*tptr++, buff, bufcx);
  }
#endif
}

FUNCTION(fun_vattrcnt)
{
  dbref target;
  char *cp, *s_mbuff;
  int i_key, anum, count;

  if (!fn_range_check("VATTRCNT", nfargs, 1, 2, buff, bufcx))
     return;


  i_key = anum = 0;

  if ( (nfargs > 1) && *fargs[1] )
     i_key = atoi(fargs[1]);
  if ( i_key != 0 )
     i_key = 1;

  init_match(player,fargs[0],NOTYPE);
  match_everything(MAT_EXIT_PARENTS);
  target = noisy_match_result();
  if (!Good_obj(target) || !Examinable(player,target))
    safe_str("-1", buff, bufcx);
  else {
    if ( i_key ) {
       count = 0;
       for (anum = atr_head(target, &cp); anum; anum = atr_next(&cp)) {
          if (anum >= A_USER_START) count++;
       }
       if ( db[target].nvattr != count ) {
          s_mbuff = alloc_mbuf("fun_vattr");
          sprintf(s_mbuff, "Value missmatch on user-count for dbref#%d.  was %d now %d.", target, db[target].nvattr, count);
          db[target].nvattr = count;
          notify_quiet(player, s_mbuff);
          free_mbuf(s_mbuff);
       }
       safe_str(myitoa(db[target].nvattr), buff, bufcx);
    } else {
       safe_str(myitoa(db[target].nvattr), buff, bufcx);
    }
  }
}

FUNCTION(fun_attrcnt)
{
  dbref target;
  int key = 0;

  if (!fn_range_check("ATTRCNT", nfargs, 1, 2, buff, bufcx))
     return;
  if ( (nfargs > 1) && *fargs[1] )
     key = atoi(fargs[1]);
  if ( (key < 0) || (key > 3) )
     key = 3;
  init_match(player,fargs[0],NOTYPE);
  match_everything(MAT_EXIT_PARENTS);
  target = noisy_match_result();
  if (!Good_obj(target) || !Examinable(player,target))
    safe_str("#-1", buff, bufcx);
  else
    safe_str(myitoa(atrcint(player,target, key)), buff, bufcx);
}

FUNCTION(fun_aflags)
{
  char *abuff;

  abuff = alloc_lbuf("fun_aflags");
  flagcheck(fargs[0], abuff);
  if (*abuff == '1')
    safe_str("#-1 BAD ATTRIBUTE", buff, bufcx);
  else
    safe_str(abuff, buff, bufcx);
  free_lbuf(abuff);
}

FUNCTION(fun_quota)
{
  dbref who;

  who = lookup_player(player, fargs[0], 0);
  if ((who == NOTHING) || !Controls(player,who))
    safe_str("#-1",buff,bufcx);
  else
    quota_dis(player, who, NULL, 0, buff, bufcx);
}

FUNCTION(fun_hasquota)
{
  dbref who;
  int tt, amount;

  if (!fn_range_check("HASQUOTA", nfargs, 1, 3, buff, bufcx)) {
    return;
  }
  if ( (nfargs >= 2) && *fargs[1])
    amount = atoi(fargs[1]);
  else
    amount = 1;
  who = lookup_player(player, fargs[0], 0);
  if ((who == NOTHING) || !Controls(player,who) || (amount < 1))
    safe_str("#-1",buff,bufcx);
  else {
    if ( (nfargs > 2) && *fargs[2] ) {
      switch (toupper(*fargs[2])) {
         case 'E':
           tt = TYPE_EXIT;
           break;
         case 'R':
           tt = TYPE_ROOM;
           break;
         case 'P':
           tt = TYPE_PLAYER;
           break;
         case 'T':
         case '\0':
           tt = TYPE_THING;
           break;
         default:
           safe_str("#-1 BAD TYPE SPECIFIER",buff,bufcx);
           return;
      }
    } else
      tt = TYPE_THING;
    if (pay_quota(player,who,amount,tt,0))
      safe_chr('1',buff,bufcx);
    else
      safe_chr('0',buff,bufcx);
  }
}

/* --------------------------------------------------------------------------
 * Functions to return dbref# information and object information
 */
FUNCTION(fun_createtime)
{
    dbref it, aowner;
    int aflags;
    char *tbuf;

    it = match_thing(player, fargs[0]);
    if ((it == NOTHING) || !Good_obj(it) || Going(it) || Recover(it) )
        safe_str("#-1", buff, bufcx);
    else {
        if (Cloak(it) && !Wizard(player)) {
            safe_str("#-1", buff, bufcx);
            return;
        }
        if (SCloak(it) && Cloak(it) && !Immortal(player)) {
            safe_str("#-1", buff, bufcx);
            return;
        }
        if ( !Examinable(player, it) ) {
            safe_str("#-1", buff, bufcx);
            return;
        }
        tbuf = atr_get(it, A_CREATED_TIME, &aowner, &aflags);
        safe_str(tbuf, buff, bufcx);
        free_lbuf(tbuf);
    }
}

FUNCTION(fun_modifytime)
{
    dbref it, aowner;
    int aflags;
    char *tbuf;

    it = match_thing(player, fargs[0]);
    if ((it == NOTHING) || !Good_obj(it) || Going(it) || Recover(it) )
        safe_str("#-1", buff, bufcx);
    else {
        if (Cloak(it) && !Wizard(player)) {
            safe_str("#-1", buff, bufcx);
            return;
        }
        if (SCloak(it) && Cloak(it) && !Immortal(player)) {
            safe_str("#-1", buff, bufcx);
            return;
        }
        if ( !Examinable(player, it) ) {
            safe_str("#-1", buff, bufcx);
            return;
        }
        tbuf = atr_get(it, A_MODIFY_TIME, &aowner, &aflags);
        safe_str(tbuf, buff, bufcx);
        free_lbuf(tbuf);
    }
}

/* Borrowed from MUX2.0 (with modifications)
 * fun_last: Returns last word in a string. Borrowed from TinyMUSH 2.2.
 */

FUNCTION(fun_last)
{
    char sep, *pStart, *pEnd, *p;
    int nLen;

    if (nfargs <= 0) {
        return;
    }
    varargs_preamble("LAST", 2);

    nLen = strlen(fargs[0]);
    pStart = trim_space_sep(fargs[0], sep);
    pEnd = pStart + nLen - 1;

    if (sep == ' ') {
        while (pStart <= pEnd && *pEnd == ' ') {
            pEnd--;
        }
        pEnd[1] = '\0';
    }

    p = pEnd;
    while (pStart <= p && *p != sep) {
        p--;
    }

    safe_str(p+1, buff, bufcx);
}

/* ---------------------------------------------------------------------------
 * fun_lastcreate: Return the last object of type Y that X created.
 * Taken from TinyMUSH
 * Modified for RhostMUSH
 */

FUNCTION(fun_lastcreate)
{
    int i, aowner, aflags, obj_list[4], obj_type;
    char *obj_str, *p, *s_tok;
    dbref obj;

    obj = match_thing(player, fargs[0]);

    if ( obj == NOTHING || obj == AMBIGUOUS ) {
       safe_str("#-1", buff, bufcx);
       return;
    }
    if (!controls(player, obj)) {    /* Automatically checks for GoodObj */
       safe_str("#-1", buff, bufcx);
       return;
    }

    switch (*fargs[1]) {
      case 'R':
      case 'r':
        obj_type = 0;
        break;
      case 'E':
      case 'e':
        obj_type = 1;
        break;
      case 'T':
      case 't':
        obj_type = 2;
        break;
      case 'P':
      case 'p':
        obj_type = 3;
        break;
      default:
        notify_quiet(player, "Invalid object type.");
        safe_str("#-1", buff, bufcx);
        return;
    }

    if ((obj_str = atr_get(obj, A_LASTCREATE, &aowner, &aflags)) == NULL) {
        safe_str("#-1", buff, bufcx);
        free_lbuf(obj_str);
        return;
    }

    if (!*obj_str) {
        free_lbuf(obj_str);
        safe_str("#-1", buff, bufcx);
        return;
    }
    for ( i = 0; i < 4; i++ )
       obj_list[i] = -1;
    for (p = (char *) strtok_r(obj_str, " ", &s_tok), i = 0;
         p && (i < 4);
         p = (char *) strtok_r(NULL, " ", &s_tok), i++) {
        obj_list[i] = atoi(p);
    }
    free_lbuf(obj_str);

    dbval(buff, bufcx, obj_list[obj_type]);
}

/* --------------------------------------------------------------------------
 * These are all the nasty side-effect functions.  USE AT OWN RISK!
 * Side effects are notorious to be not very secure.
 * To use it, you MUST have the compile time option USE_SIDEEFFECT enabled
 * as WELL as having the admin param set for each one.
 * The AND mask to use set() will be '1' for the admin param sideeffects
 * Any item to use side-effects MUST be set SIDEFX (chaotic mux conformity)
 * as well.
 */
#ifdef USE_SIDEEFFECT
FUNCTION(fun_cluster_add)
{
   CMDENT *cmdp;
   char *s_buf1;

   if ( !(mudconf.sideeffects & SIDE_CLUSTER_ADD) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || Slave(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@cluster", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@cluster") ||
         cmdtest(Owner(player), "@cluster") || zonecmdtest(player, "@cluster") ) {
      notify(player, "Permission denied.");
      return;
   }
   if ( !*fargs[0] || !*fargs[1] ) {
      notify(player, "Cluster object must be a valid object.");
      return;
   }
   s_buf1 = alloc_lbuf("fun_cluster_add");
   sprintf(s_buf1, "%.1500s=%.1500s", fargs[0], fargs[1]);
   do_cluster( player, cause, (SIDEEFFECT|CLUSTER_ADD), s_buf1, cargs, ncargs);
   free_lbuf(s_buf1);
}

FUNCTION(fun_set)
{
   char *s_buf1, *s_buf2;
   CMDENT *cmdp;

   if ( !(mudconf.sideeffects & SIDE_SET) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || Slave(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@set", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@set") ||
         cmdtest(Owner(player), "@set") || zonecmdtest(player, "@set") ) {
      notify(player, "Permission denied.");
      return;
   }
   s_buf1 = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                 fargs[0], cargs, ncargs);
   s_buf2 = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                 fargs[1], cargs, ncargs);
   do_set(player, cause, (SET_QUIET|SIDEEFFECT), s_buf1, s_buf2);
   free_lbuf(s_buf1);
   free_lbuf(s_buf2);
}

FUNCTION(fun_rset)
{
   char *s_buf1, *s_buf2;
   CMDENT *cmdp;

   if ( !(mudconf.sideeffects & SIDE_RSET) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || Slave(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@set", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@set") ||
         cmdtest(Owner(player), "@set") || zonecmdtest(player, "@set") ) {
      notify(player, "Permission denied.");
      return;
   }
   s_buf1 = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                 fargs[0], cargs, ncargs);
   s_buf2 = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                 fargs[1], cargs, ncargs);
   mudstate.lbuf_buffer = alloc_lbuf("lbuf_rset");
   do_set(player, cause, (SET_QUIET|SIDEEFFECT|SET_RSET), s_buf1, s_buf2);
   safe_str(mudstate.lbuf_buffer, buff, bufcx);
   free_lbuf(mudstate.lbuf_buffer);
   mudstate.lbuf_buffer = NULL;
   free_lbuf(s_buf1);
   free_lbuf(s_buf2);
}

FUNCTION(fun_toggle)
{
   CMDENT *cmdp;

   if ( !(mudconf.sideeffects & SIDE_TOGGLE) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || Slave(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@toggle", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@toggle") ||
         cmdtest(Owner(player), "@toggle") || zonecmdtest(player, "@toggle") ) {
      notify(player, "Permission denied.");
      return;
   }
   do_toggle(player, cause, (SET_QUIET|SIDEEFFECT), fargs[0], fargs[1]);
}

FUNCTION(fun_link)
{
   CMDENT *cmdp;

   if ( !(mudconf.sideeffects & SIDE_LINK) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || Slave(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@link", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@link") ||
         cmdtest(Owner(player), "@link") || zonecmdtest(player, "@link") ) {
      notify(player, "Permission denied.");
      return;
   }
   do_link(player, cause, (SIDEEFFECT), fargs[0], fargs[1]);
}

FUNCTION(fun_create)
{
   char *ptrs[LBUF_SIZE / 2], sep, *myfargs;
   int nitems;
   CMDENT *cmdp;

   if ( !(mudconf.sideeffects & SIDE_CREATE) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }

   if (!fn_range_check("CREATE", nfargs, 1, 3, buff, bufcx))
      return;

   mudstate.store_lastcr = -1;

   if ( !SideFX(player) || Fubar(player) || Slave(player) ||
        return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      if ( mudconf.sidefx_returnval )
         dbval(buff, bufcx, mudstate.store_lastcr);
      return;
   }

   myfargs = alloc_lbuf("fun_create");

   if ( nfargs < 2 ) {
      sprintf(myfargs, "%d", mudconf.createmin);
   } else {
      strcpy(myfargs, fargs[1]);
   }

   if ( (nfargs > 2) && *fargs[2] ) {
      sep = *fargs[2];
   } else {
      sep = ' ';
   }
   mudstate.sidefx_currcalls++;

   switch (sep) {
      case 't' : cmdp = (CMDENT *)hashfind((char *)"@create", &mudstate.command_htab);
                 if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@create") ||
                       cmdtest(Owner(player), "@create") || zonecmdtest(player, "@create") ) {
                    notify(player, "Permission denied.");
                    break;
                 }
                 do_create(player, cause, (SIDEEFFECT), fargs[0], myfargs);
                 break;
      case 'r' : cmdp = (CMDENT *)hashfind((char *)"@dig", &mudstate.command_htab);
                 if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@dig") ||
                       cmdtest(Owner(player), "@dig") || zonecmdtest(player, "@dig") ) {
                    notify(player, "Permission denied.");
                    break;
                 }
                 nitems = list2arr(ptrs, LBUF_SIZE / 2, fargs[1], ',');
                 do_dig(player, cause, (SIDEEFFECT), fargs[0], ptrs, nitems);
                 break;
      case 'e' : cmdp = (CMDENT *)hashfind((char *)"@open", &mudstate.command_htab);
                 if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@open") ||
                       cmdtest(Owner(player), "@open") || zonecmdtest(player, "@open") ) {
                    notify(player, "Permission denied.");
                    break;
                 }
                 nitems = list2arr(ptrs, LBUF_SIZE / 2, fargs[1], ',');
                 do_open(player, cause, (SIDEEFFECT), fargs[0], ptrs, nitems);
                 break;
      default:   cmdp = (CMDENT *)hashfind((char *)"@create", &mudstate.command_htab);
                 if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@create") ||
                       cmdtest(Owner(player), "@create") || zonecmdtest(player, "@create") ) {
                    notify(player, "Permission denied.");
                    break;
                 }
                 do_create(player, cause, (SIDEEFFECT), fargs[0], myfargs);
                 break;
   }
   free_lbuf(myfargs);

   if ( mudconf.sidefx_returnval )
      dbval(buff, bufcx, mudstate.store_lastcr);
}

FUNCTION(fun_dig)
{
   char *ptrs[LBUF_SIZE / 2], fillbuf[LBUF_SIZE+1];
   int nitems, i_sanitizedbref;
   CMDENT *cmdp;

   if ( !(mudconf.sideeffects & SIDE_DIG) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || Slave(player) ||
         return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@dig", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@dig") ||
         cmdtest(Owner(player), "@dig") || zonecmdtest(player, "@dig") ) {
      notify(player, "Permission denied.");
      return;
   }
   memset(fillbuf, 0, sizeof(fillbuf));
   if (!fn_range_check("DIG", nfargs, 1, 5, buff, bufcx))
      return;
   if ( nfargs > 2 ) {
      sprintf(fillbuf, "%.*s,%.*s", ((LBUF_SIZE / 2) - 1), fargs[1], ((LBUF_SIZE / 2) - 1), fargs[2]);
   }
   else if (nfargs > 1) {
      sprintf(fillbuf, "%.*s", (LBUF_SIZE - 10), fargs[1]);
   }
   nitems = list2arr(ptrs, LBUF_SIZE / 2, fillbuf, ',');
   i_sanitizedbref = -1;
   mudstate.store_lastcr = -1;
   mudstate.store_lastx1 = -1;
   mudstate.store_lastx2 = -1;
   mudstate.store_loc = -1;
   if ( (nfargs > 3) && (*fargs[3] == '#') && *(fargs[3]+1) && isdigit(*(fargs[3]+1)) ) {
      i_sanitizedbref = atoi(fargs[3]+1);
      if ( Good_chk((dbref)i_sanitizedbref) &&
           (isRoom((dbref)i_sanitizedbref) || isThing((dbref)i_sanitizedbref)) &&
           controls(player, (dbref)i_sanitizedbref) )
         mudstate.store_loc = (dbref)i_sanitizedbref;
   }
   i_sanitizedbref = 0;
   if ( (nfargs > 4) && *fargs[4] ) {
      i_sanitizedbref = atoi(fargs[4]);
      if ( i_sanitizedbref != 1 )
         i_sanitizedbref = 0;
   }
   do_dig(player, cause, (SIDEEFFECT), fargs[0], ptrs, nitems);
   mudstate.store_loc = -1;
   if ( mudconf.sidefx_returnval ) {
      if ( i_sanitizedbref ) {
         sprintf(fillbuf, "#%d #%d #%d", mudstate.store_lastcr,
                 mudstate.store_lastx1, mudstate.store_lastx2);
         safe_str(fillbuf, buff, bufcx);
      } else
         dbval(buff, bufcx, mudstate.store_lastcr);
   }
}

FUNCTION(fun_open)
{
   char *ptrs[LBUF_SIZE / 2];
   int nitems, i_sanitizedbref;
   CMDENT *cmdp;

   if ( !(mudconf.sideeffects & SIDE_OPEN) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || Slave(player) || return_bit(player) < mudconf.restrict_sidefx) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@open", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@open") ||
         cmdtest(Owner(player), "@open") || zonecmdtest(player, "@open") ) {
      notify(player, "Permission denied.");
      return;
   }
   if (!fn_range_check("OPEN", nfargs, 1, 3, buff, bufcx))
      return;
   nitems = list2arr(ptrs, LBUF_SIZE / 2, fargs[1], ',');
   mudstate.store_lastcr = -1;
   mudstate.store_loc = -1;
   if ( (nfargs > 2) && (*fargs[2] == '#') && *(fargs[2]+1) && isdigit(*(fargs[2]+1)) ) {
      i_sanitizedbref = atoi(fargs[2]+1);
      if ( Good_chk((dbref)i_sanitizedbref) &&
           (isRoom((dbref)i_sanitizedbref) || isThing((dbref)i_sanitizedbref)) &&
           controls(player, (dbref)i_sanitizedbref) )
         mudstate.store_loc = (dbref)i_sanitizedbref;
   }
   do_open(player, cause, (SIDEEFFECT), fargs[0], ptrs, nitems);
   mudstate.store_loc = -1;
   if ( mudconf.sidefx_returnval )
      dbval(buff, bufcx, mudstate.store_lastcr);
}

FUNCTION(fun_lemit)
{
   CMDENT *cmdp;

   if ( !(mudconf.sideeffects & SIDE_LEMIT) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@emit", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@emit") ||
         cmdtest(Owner(player), "@emit") || zonecmdtest(player, "@emit") ) {
      notify(player, "Permission denied.");
      return;
   }

   do_say(player, cause, (SAY_EMIT|SAY_ROOM), fargs[0]);
}

FUNCTION(fun_emit)
{
   CMDENT *cmdp;

   if ( !(mudconf.sideeffects & SIDE_EMIT) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@emit", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@emit") ||
         cmdtest(Owner(player), "@emit") || zonecmdtest(player, "@emit") ) {
      notify(player, "Permission denied.");
      return;
   }

   do_say(player, cause, SAY_EMIT, fargs[0]);
}

FUNCTION(fun_mailread)
{
   dbref target;
   int i_key;
   CMDENT *cmdp;

   cmdp = (CMDENT *)hashfind((char *)"mail", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "mail") ||
         cmdtest(Owner(player), "mail") || zonecmdtest(player, "mail") ) {
      notify(player, "Permission denied.");
      return;
   }

   if (!fn_range_check("MAILREAD", nfargs, 3, 4, buff, bufcx))
      return;

   if ( !*fargs[0] || !*fargs[1] || !*fargs[2] ) {
      safe_str("#-1 INVALID ARGUMENTS TO MAILREAD", buff, bufcx);
      return;
   }
   target = lookup_player(player, fargs[0], 1);
   if ( !Good_chk(target) || !Controls(player, target) ) {
      safe_str("#-1 PERMISSION DENIED", buff, bufcx);
      return;
   }

   i_key = 0;
   if ( (nfargs > 3) && *fargs[3] )
      i_key = atoi(fargs[3]);

   if ( i_key )
      i_key = 1;

   mail_read_func(target, fargs[1], player, fargs[2], i_key, buff, bufcx);
}

FUNCTION(fun_mailsend)
{
   CMDENT *cmdp;
   char *s_body, *s_bodyptr;
   int i_key;

   if ( !(mudconf.sideeffects & SIDE_MAIL) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || Slave(player) || return_bit(player) < mudconf.restrict_sidefx) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"mail", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "mail") ||
         cmdtest(Owner(player), "mail") || zonecmdtest(player, "mail") ) {
      notify(player, "Permission denied.");
      return;
   }

   if (!fn_range_check("MAILSEND", nfargs, 3, 4, buff, bufcx))
      return;

   if ( !*fargs[0] ) {
      safe_str("#-1 EMPTY PLAYER LIST", buff, bufcx);
      return;
   }
   if ( !*fargs[2] ) {
      safe_str("#-1 EMPTY MESSAGE BODY", buff, bufcx);
      return;
   }
   i_key = 0;
   if ( (nfargs > 3) && *fargs[3] ) {
      i_key = atoi(fargs[3]);
      if ( i_key )
         i_key = M_ANON;
   }
   i_key |= M_SEND;
   s_bodyptr = s_body = alloc_lbuf("fun_mailsend");
   if ( *fargs[1] ) {
     safe_str(fargs[1], s_body, &s_bodyptr);
     safe_str("//", s_body, &s_bodyptr);
   }
   safe_str(fargs[2], s_body, &s_bodyptr);
   do_mail(player, cause, i_key, fargs[0], s_body);
   free_lbuf(s_body);
}

FUNCTION(fun_clone)
{
   CMDENT *cmdp;
   int i_key;

   if ( !(mudconf.sideeffects & SIDE_CLONE) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || Slave(player) || return_bit(player) < mudconf.restrict_sidefx) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@clone", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@clone") ||
         cmdtest(Owner(player), "@clone") || zonecmdtest(player, "@clone") ) {
      notify(player, "Permission denied.");
      return;
   }
   if (!fn_range_check("CLONE", nfargs, 1, 3, buff, bufcx))
      return;

   i_key = 0;
   if ( (nfargs > 2) && *fargs[2] ) {
      i_key = atoi(fargs[2]);
      if ( i_key < 0 )
         i_key = 0;
      if ( Wizard(player) )
         i_key &= 3;
      else
         i_key &= 1;
   }
   switch (i_key ) {
      case 3: i_key = CLONE_PARENT | CLONE_PRESERVE;
              break;
      case 2: i_key = CLONE_PRESERVE;
              break;
      case 1: i_key = CLONE_PARENT;
              break;
      default: i_key = 0;
              break;
   }
   mudstate.store_lastcr = -1;
   do_clone(player, cause, SIDEEFFECT | i_key, fargs[0], fargs[1]);
   if ( mudconf.sidefx_returnval )
      dbval(buff, bufcx, mudstate.store_lastcr);
}

FUNCTION(fun_oemit)
{
   CMDENT *cmdp;
   int i_key;

   if ( !(mudconf.sideeffects & SIDE_OEMIT) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }

   if (!fn_range_check("OEMIT", nfargs, 2, 3, buff, bufcx))
      return;

   i_key = 0;
   if ( (nfargs > 2) && fargs[2] && *fargs[2] )
      i_key = atoi(fargs[2]);
      
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@oemit", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@oemit") ||
         cmdtest(Owner(player), "@oemit") || zonecmdtest(player, "@oemit") ) {
      notify(player, "Permission denied.");
      return;
   }
   if ( !i_key ) 
      do_pemit( player, cause, (SIDEEFFECT|PEMIT_OEMIT), fargs[0], fargs[1], cargs, ncargs);
   else
      do_pemit( player, cause, (SIDEEFFECT|PEMIT_OEMIT|PEMIT_OSTR), fargs[0], fargs[1], cargs, ncargs);
}

FUNCTION(fun_pemit)
{
   CMDENT *cmdp;
   int i_key;

   if ( !(mudconf.sideeffects & SIDE_PEMIT) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }

   if (!fn_range_check("PEMIT", nfargs, 2, 3, buff, bufcx))
      return;

   if ( !SideFX(player) || Fubar(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@pemit", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@pemit") ||
         cmdtest(Owner(player), "@pemit") || zonecmdtest(player, "@pemit") ) {
      notify(player, "Permission denied.");
      return;
   }
   if ( nfargs > 2 && *fargs[2] ) {
      i_key = atoi(fargs[2]);
   }
   if (i_key == 1 ) {
      do_pemit( player, cause, (SIDEEFFECT|PEMIT_PEMIT|PEMIT_LIST), fargs[0], fargs[1], cargs, ncargs);
   } else {
      do_pemit( player, cause, (SIDEEFFECT|PEMIT_PEMIT|PEMIT_LIST|PEMIT_NOSUB), fargs[0], fargs[1], cargs, ncargs);
   }
}

FUNCTION(fun_remit)
{
   CMDENT *cmdp;
   int i_type;

   if (!fn_range_check("REMIT", nfargs, 2, 3, buff, bufcx))
      return;
   if ( !(mudconf.sideeffects & SIDE_REMIT) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@pemit", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@pemit") ||
         cmdtest(Owner(player), "@pemit") || zonecmdtest(player, "@pemit") ) {
      notify(player, "Permission denied.");
      return;
   }
   i_type = 0;
   if ( (nfargs > 2) && *fargs[2] && *fargs[0] && (strchr(fargs[0], '/') != NULL) )
      i_type = atoi(fargs[2]);
   if ( i_type )
      do_pemit( player, cause, (SIDEEFFECT|PEMIT_CONTENTS|PEMIT_LIST|PEMIT_PEMIT|PEMIT_REALITY|PEMIT_TOREALITY), fargs[0], fargs[1], cargs, ncargs);
   else
      do_pemit( player, cause, (SIDEEFFECT|PEMIT_CONTENTS|PEMIT_LIST|PEMIT_PEMIT), fargs[0], fargs[1], cargs, ncargs);
}

FUNCTION(fun_zemit)
{
   CMDENT *cmdp;

   if ( !(mudconf.sideeffects & SIDE_ZEMIT) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@pemit", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@pemit") ||
         cmdtest(Owner(player), "@pemit") || zonecmdtest(player, "@pemit") ) {
      notify(player, "Permission denied.");
      return;
   }
   do_pemit( player, cause, (SIDEEFFECT|PEMIT_ZONE|PEMIT_LIST|PEMIT_PEMIT), fargs[0], fargs[1], cargs, ncargs);
}

FUNCTION(fun_tel)
{
   int nitems;
   char *ptrs[LBUF_SIZE / 2];
   CMDENT *cmdp;

   if ( !(mudconf.sideeffects & SIDE_TEL) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@teleport", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@teleport") ||
         cmdtest(Owner(player), "@teleport") || zonecmdtest(player, "@teleport") ) {
      notify(player, "Permission denied.");
      return;
   }

   /* Lensy: Add the quiet switch to tel() */
   if (!fn_range_check("TEL", nfargs, 2, 3, buff, bufcx))
       return;

   if (nfargs == 3 && !(*fargs[2] == '0' || *fargs[2] == '1') ) {
       notify(player, "#-1 INVALID ARG VALUE [QUIET]");
       return;
   }

   nitems = list2arr(ptrs, LBUF_SIZE / 2, fargs[1], ' ');
   do_teleport( player,
                cause,
               (SIDEEFFECT | TEL_LIST | \
           ((nfargs==3 && *fargs[2] == '1') ? TEL_QUIET : 0 )),
         fargs[0], ptrs, nitems );
}

FUNCTION(fun_cluster_wipe)
{
   CMDENT *cmdp;
   ATTR *attr;
   char *s_buff, *s_buff2, *s_buffptr, *s_return, *s_strtok, *s_strtokptr;
   int target, aflags, i_nomatch, i_nowipe, i_wipecnt, i_totobjs, i_togregexp;
   dbref aowner;
   time_t starttme, endtme;
   double timechk;

   if (!fn_range_check("CLUSTER_WIPE", nfargs, 1, 2, buff, bufcx))
      return;

   i_togregexp = 0;
   if ( (nfargs > 1) && *fargs[1] ) {
      s_return = exec(player, cause, caller,  EV_FCHECK | EV_EVAL | EV_STRIP, fargs[1], cargs, ncargs);
      if ( atoi(s_return) == 1 )
         i_togregexp = WIPE_REGEXP;
      free_lbuf(s_return);
   }
   i_totobjs = i_nomatch = i_nowipe = i_wipecnt = 0;
   if ( !(mudconf.sideeffects & SIDE_WIPE) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@wipe", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@wipe") ||
         cmdtest(Owner(player), "@wipe") || zonecmdtest(player, "@wipe") ) {
      notify(player, "Permission denied.");
      return;
   }
   s_buff = alloc_lbuf("fun_cluster_wipe");
   s_buff2 = alloc_lbuf("fun_cluster_wipe2");
   if ( strchr(fargs[0], '/') != NULL ) {
      strcpy(s_buff, fargs[0]);
      s_buffptr = s_buff;
      while ( *s_buffptr ) {
         if ( *s_buffptr == '/' ) {
            *s_buffptr = '\0';
            strcpy(s_buff2, s_buffptr+1);
            break;
         }
         s_buffptr++;
      }
   } else {
      strcpy(s_buff, fargs[0]);
   }
   s_return = exec(player, cause, caller,  EV_FCHECK | EV_EVAL | EV_STRIP, s_buff, cargs, ncargs);
   target = match_thing(player, s_return);
   free_lbuf(s_return);
   if ( !Good_chk(target) || !Cluster(target) ) {
      free_lbuf(s_buff);
      free_lbuf(s_buff2);
      safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
      return;
   }

   endtme = time(NULL);
   starttme = mudstate.chkcpu_stopper;
   if ( mudconf.cputimechk < 10 )
      timechk = 10;
   else if ( mudconf.cputimechk > 3600 )
      timechk = 3600;
   else
      timechk = mudconf.cputimechk;

   attr = atr_str("_CLUSTER");
   s_return = atr_get(target, attr->number, &aowner, &aflags);
   s_strtok = strtok_r(s_return, " ", &s_strtokptr);

   while ( s_strtok ) {
      endtme = time(NULL);
      if ( mudstate.chkcpu_toggle || ((endtme - starttme) > timechk) ) {
          mudstate.chkcpu_toggle = 1;
      }
      if ( mudstate.chkcpu_toggle ) {
         notify_quiet(player, "CPU: cluster wipe exceeded cpu limit.");
         break;
      }
      memset(s_buff, '\0', LBUF_SIZE);
      if ( s_buff2 && *s_buff2 ) {
         sprintf(s_buff, "%s/%s", s_strtok, s_buff2);
      } else {
         sprintf(s_buff, "%s", s_strtok);
      }
      i_totobjs++;
      do_wipe(player, cause, (SIDEEFFECT|i_togregexp), s_buff);
      switch ( mudstate.wipe_state ) {
         case -1: i_nomatch++;
                  break;
         case -2: i_nowipe++;
                  break;
         default: i_wipecnt+=mudstate.wipe_state;
                  break;
      }
      s_strtok = strtok_r(NULL, " ", &s_strtokptr);
   }
   if ( TogNoisy(player) ) {
      sprintf(s_buff2, "Wiped: [%d objects: %d not found, %d safe] %d attributes wiped.", 
                       i_totobjs, i_nomatch, i_nowipe, i_wipecnt);
      notify(player, s_buff2);
   }
   free_lbuf(s_return);
   free_lbuf(s_buff2);
   free_lbuf(s_buff);
}

FUNCTION(fun_wipe)
{
   CMDENT *cmdp;
   char *s_buff;

   if (!fn_range_check("WIPE", nfargs, 1, 2, buff, bufcx))
      return;

   if ( !(mudconf.sideeffects & SIDE_WIPE) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@wipe", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@wipe") ||
         cmdtest(Owner(player), "@wipe") || zonecmdtest(player, "@wipe") ) {
      notify(player, "Permission denied.");
      return;
   }

   if ( (nfargs > 1) && *fargs[1] && (atoi(fargs[1]) == 1) ) {
      do_wipe(player, cause, (SIDEEFFECT|WIPE_REGEXP), fargs[0]);
   } else {
      do_wipe(player, cause, (SIDEEFFECT), fargs[0]);
   }
   if ( TogNoisy(player) ) {
      switch( mudstate.wipe_state ) {
         case -1: notify(player, "Wipe: no match");
                  break;
         case -2: notify(player, "Wipe: Can not wipe safe/indestructable objects.");
                  break;
         default: s_buff = alloc_lbuf("fun_wipe");
                  sprintf(s_buff, "Wipe: %d attributes wiped.", mudstate.wipe_state);
                  notify(player, s_buff);
                  free_lbuf(s_buff);
                  break;
      }
   }
}

FUNCTION(fun_destroy)
{
   CMDENT *cmdp;

   if ( !(mudconf.sideeffects & SIDE_DESTROY) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( mudstate.inside_locks ) {
      notify(player, "#-1 DESTROY ATTEMPTED INSIDE LOCK");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@destroy", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@destroy") ||
         cmdtest(Owner(player), "@destroy") || zonecmdtest(player, "@destroy") ) {
      notify(player, "Permission denied.");
      return;
   }
   do_destroy(player, cause, (SIDEEFFECT), fargs[0]);
}

FUNCTION(fun_move)
{
  dbref target;
  CMDENT *cmdp;

  if ( !(mudconf.sideeffects & SIDE_MOVE) ) {
    notify(player, "#-1 FUNCTION DISABLED");
    return;
  }
  if ( !SideFX(player) || Fubar(player) || return_bit(player) < mudconf.restrict_sidefx ) {
    notify(player, "Permission denied.");
    return;
  }
  mudstate.sidefx_currcalls++;
  target = match_thing(player, fargs[0]);
  if ( !Good_obj(target) || Recover(target) || Going(target) || !Controls(player, target) ) {
    notify(player, "Permission denied.");
    return;
  }
  if ( !(isPlayer(target) || isThing(target)) ) {
    notify(player, "Permission denied.");
    return;
  }
  cmdp = (CMDENT *)hashfind((char *)"goto", &mudstate.command_htab);
  if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "goto") ||
       cmdtest(Owner(player), "goto") || zonecmdtest(player, "goto") ) {
    notify(player, "Permission denied.");
    return;
  }
  do_move(target, cause, (SIDEEFFECT), fargs[1]);
}

#endif

#ifdef REALITY_LEVELS
/*
 * Reality levels functions: hasrxlevel(), hastxlevel(),
 * rxlevel(), txlevel(), cansee(), listrlevels()
 */
FUNCTION(fun_hasrxlevel)
{
        dbref target;
        RLEVEL rl;

        init_match(player, fargs[0], NOTYPE);
        match_everything(MAT_EXIT_PARENTS);
        target = match_result();
        rl = find_rlevel(fargs[1]);
        if ((target == NOTHING) || (target == AMBIGUOUS) || (!Good_obj(target)) ||
                Recover(target) || Going(target) || !Controls(player,target) || !rl) {
        safe_str("#-1",buff,bufcx);
    }
    else {
        if((RxLevel(target) & rl) == rl)
          safe_str("1", buff, bufcx);
        else
          safe_str("0", buff, bufcx);
        }
}

FUNCTION(fun_hastxlevel)
{
    dbref target;
    RLEVEL rl;

    init_match(player, fargs[0], NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    target = match_result();
    rl = find_rlevel(fargs[1]);
    if ((target == NOTHING) || (target == AMBIGUOUS) || (!Good_obj(target)) ||
        Recover(target) || Going(target) || !Controls(player,target) || !rl) {
        safe_str("#-1",buff,bufcx);
    }
    else {
        if((TxLevel(target) & rl) == rl)
            safe_str("1", buff, bufcx);
        else
            safe_str("0", buff, bufcx);
    }
}

FUNCTION(fun_rxlevel)
{
    dbref target;
    int i, add_space, cmp_x, cmp_y, cmp_z;
#ifdef USE_SIDEEFFECT
    CMDENT *cmdp;

    if (!fn_range_check("RXLEVEL", nfargs, 1, 2, buff, bufcx))
           return;
#endif
    init_match(player, fargs[0], NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    target = match_result();
#ifdef USE_SIDEEFFECT
    if ( nfargs > 1 ) {
        mudstate.sidefx_currcalls++;
        if ( !(mudconf.sideeffects & SIDE_RXLEVEL) ) {
           notify(player, "#-1 SIDE-EFFECT PORTION OF RXLEVEL DISABLED");
           return;
        }
        if ( !SideFX(player) || Fubar(player) || Slave(player) ||
             return_bit(player) < mudconf.restrict_sidefx ) {
           notify(player, "Permission denied.");
           return;
        }
        cmdp = (CMDENT *)hashfind((char *)"@rxlevel", &mudstate.command_htab);
        if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@rxlevel") ||
              cmdtest(Owner(player), "@rxlevel") || zonecmdtest(player, "@rxlevel") ) {
           notify(player, "Permission denied.");
           return;
        }
        do_rxlevel(player, cause, 0, fargs[0], fargs[1]);
    } else {
#endif
       if ((target == NOTHING) || (target == AMBIGUOUS) || (!Good_obj(target)) ||
           Recover(target) || Going(target) || !Controls(player,target)) {
           safe_str("#-1",buff,bufcx);
       }
       else {
           cmp_x = sizeof(mudconf.reality_level);
           cmp_y = sizeof(mudconf.reality_level[0]);
           if ( cmp_y == 0 )
              cmp_z = 0;
           else
              cmp_z = cmp_x / cmp_y;
           for(add_space = i = 0; (i < mudconf.no_levels) && (i < cmp_z); ++i) {
               if((RxLevel(target) & mudconf.reality_level[i].value) ==
                   mudconf.reality_level[i].value) {
                   if(add_space)
                       safe_chr(' ', buff, bufcx);
                   safe_str(mudconf.reality_level[i].name, buff, bufcx);
                   add_space = 1;
               }
           }
       }
#ifdef USE_SIDEEFFECT
   }
#endif
}

FUNCTION(fun_listrlevels)
{
   int i, add_space, cmp_x, cmp_y, cmp_z;

   cmp_x = sizeof(mudconf.reality_level);
   cmp_y = sizeof(mudconf.reality_level[0]);
   if ( cmp_y == 0 )
      cmp_z = 0;
   else
      cmp_z = cmp_x / cmp_y;
   if ( mudconf.no_levels < 1 ) {
      safe_str("#-1 NO REALITY LEVELS DEFINED", buff, bufcx);
   } else {
      for (add_space = i = 0; (i < mudconf.no_levels) && (i < cmp_z); ++i) {
         if(add_space)
            safe_chr(' ', buff, bufcx);
         safe_str(mudconf.reality_level[i].name, buff, bufcx);
         add_space = 1;
      }
   }
}

FUNCTION(fun_txlevel)
{
    dbref target;
    int i, add_space, cmp_x, cmp_y, cmp_z;
#ifdef USE_SIDEEFFECT
    CMDENT *cmdp;

    if (!fn_range_check("TXLEVEL", nfargs, 1, 2, buff, bufcx))
           return;
#endif

    init_match(player, fargs[0], NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    target = match_result();

#ifdef USE_SIDEEFFECT
    if ( nfargs > 1 ) {
        mudstate.sidefx_currcalls++;
        if ( !(mudconf.sideeffects & SIDE_TXLEVEL) ) {
           notify(player, "#-1 SIDE-EFFECT PORTION OF TXLEVEL DISABLED");
           return;
        }
        if ( !SideFX(player) || Fubar(player) || Slave(player) ||
             return_bit(player) < mudconf.restrict_sidefx ) {
           notify(player, "Permission denied.");
           return;
        }
        cmdp = (CMDENT *)hashfind((char *)"@txlevel", &mudstate.command_htab);
        if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@txlevel") ||
              cmdtest(Owner(player), "@txlevel") || zonecmdtest(player, "@txlevel") ) {
           notify(player, "Permission denied.");
           return;
        }
        do_txlevel(player, cause, 0, fargs[0], fargs[1]);
    } else {
#endif
       if ((target == NOTHING) || (target == AMBIGUOUS) || (!Good_obj(target)) ||
           Recover(target) || Going(target) || !Controls(player,target)) {
           safe_str("#-1",buff,bufcx);
       }
       else {
          cmp_x = sizeof(mudconf.reality_level);
          cmp_y = sizeof(mudconf.reality_level[0]);
          if ( cmp_y == 0 )
             cmp_z = 0;
          else
              cmp_z = cmp_x / cmp_y;
          for(add_space = i = 0; (i < mudconf.no_levels) && (i < cmp_z); ++i) {
            if((TxLevel(target) & mudconf.reality_level[i].value) ==
                mudconf.reality_level[i].value) {
                if(add_space)
                    safe_chr(' ', buff, bufcx);
                safe_str(mudconf.reality_level[i].name, buff, bufcx);
                add_space = 1;
            }
         }
      }
#ifdef USE_SIDEEFFECT
   }
#endif
}

FUNCTION(fun_cansee)
{
    dbref who, what;

    init_match(player, fargs[0], NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    who = match_result();
    init_match(player, fargs[1], NOTYPE);
    match_everything(MAT_EXIT_PARENTS);
    what = match_result();
    if ((who == NOTHING) || (who == AMBIGUOUS) || (!Good_obj(who)) ||
        (what == NOTHING) || (what == AMBIGUOUS) || (!Good_obj(what)) ||
        Going(who) || Going(what) || Recover(who) || Recover(what) ||
        !Controls(player,who)) {
        safe_str("#-1",buff,bufcx);
        return;
    }
    if(IsReal(who, what))
        safe_str("1", buff, bufcx);
    else
        safe_str("0", buff, bufcx);
}
#endif /* REALITY_LEVELS */

/* Clustering.  Ah, lovely clustering.  here is all the main cluster functions
 * All the functions tiered to clustering go here and below
 * Clustering allows a group of dbref#'s to be virtually identified as a single entity
 */

FUNCTION(fun_cluster_vattrcnt)
{
  dbref target, aowner;
  int aflags, i_vattrcnt;
  char *s_text, *s_strtok, *s_strtokptr;
  ATTR *attr;

  target = match_thing(player, fargs[0]);
  if (!Good_obj(target) || !Examinable(player,target) || !Cluster(target) )
    safe_str("-1 NO SUCH CLUSTER", buff, bufcx);
  else {
    attr = atr_str("_CLUSTER");
    if ( attr ) {
       s_text = atr_get(target, attr->number, &aowner, &aflags);
       if ( *s_text ) {
          s_strtok = strtok_r(s_text, " ", &s_strtokptr);
          i_vattrcnt = 0;
          while (s_strtok) {
             aowner = match_thing(player, s_strtok);
             if ( Good_chk(aowner) && Cluster(aowner) )
                i_vattrcnt += (db[aowner].nvattr - 1);
             s_strtok = strtok_r(NULL, " ", &s_strtokptr);
          }
          safe_str(myitoa(i_vattrcnt), buff, bufcx);
       } else {
          safe_str("-1 NO SUCH CLUSTER", buff, bufcx);
       }
       free_lbuf(s_text);
    } else {
       safe_str("-1 NO SUCH CLUSTER", buff, bufcx);
    }
  }
}

FUNCTION(fun_cluster_attrcnt)
{
  dbref target, aowner;
  int key = 0, aflags, i_vattrcnt, i_cnt;
  char *s_text, *s_strtok, *s_strtokptr;
  ATTR *attr;

  if (!fn_range_check("CLUSTER_ATTRCNT", nfargs, 1, 2, buff, bufcx))
     return;
  if ( (nfargs > 1) && *fargs[1] )
     key = atoi(fargs[1]);
  if ( (key < 0) || (key > 3) )
     key = 3;
  target = match_thing(player, fargs[0]);
  if (!Good_obj(target) || !Examinable(player,target) || !Cluster(target) ) {
    safe_str("#-1", buff, bufcx);
  } else {
    attr = atr_str("_CLUSTER");
    if ( attr ) {
       s_text = atr_get(target, attr->number, &aowner, &aflags);
       if ( *s_text ) {
          s_strtok = strtok_r(s_text, " ", &s_strtokptr);
          i_vattrcnt = 0;
          while (s_strtok) {
             aowner = match_thing(player, s_strtok);
             i_cnt = 0;
             if ( Good_chk(aowner) && Cluster(aowner) )
                i_cnt = atrcint(player, aowner, key);
             if ( i_cnt < 0 ) {
                ival(buff, bufcx, -1);
                break;
             }
             i_vattrcnt += (i_cnt - 1);
             s_strtok = strtok_r(NULL, " ", &s_strtokptr);
          }
          safe_str(myitoa(i_vattrcnt), buff, bufcx);
       } else {
          safe_str("-1 NO SUCH CLUSTER", buff, bufcx);
       }
       free_lbuf(s_text);
    } else {
       safe_str("-1 NO SUCH CLUSTER", buff, bufcx);
    }
  }
}

FUNCTION(fun_cluster_grep)
{
   dbref object, parent, aowner;
   int i_first, aflags, i_type, i_key;
   char *ret, *s_text, *s_strtok, *s_strtokptr, *s_tmpbuff;
   ATTR *attr;

   if (!fn_range_check("CLUSTER_GREP", nfargs, 3, 5, buff, bufcx))
      return;

   i_key = i_type = 0;
   if ( (nfargs > 3) && *fargs[3] )
      i_type = atoi(fargs[3]);
   if ( (nfargs > 4) && *fargs[4] ) {
      i_key = atoi(fargs[4]);
      if ( i_key )
         i_key = 2;
   }
   object = match_thing(player, fargs[0]);
   if ( !Good_chk(object) || !Cluster(object) )
      safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
   else if (!Examinable(player, object))
      safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
   else {
      i_first = 0;
      attr = atr_str("_CLUSTER");
      if ( !attr ) {
         safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
      } else {
         s_text = atr_get(object, attr->number, &aowner, &aflags);
         if ( !*s_text ) {
            safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
         } else {
            s_strtok = strtok_r(s_text, " ", &s_strtokptr);
            s_tmpbuff = alloc_mbuf("cluster_grep");
            while ( s_strtok ) {
               parent = match_thing(player, s_strtok);
               if ( !mudstate.chkcpu_toggle && Good_chk(parent) && Examinable(player, parent) ) {
                  ret = grep_internal(player, parent, fargs[2], fargs[1], (i_key | ((i_type == 2) ? 1 : 0)) );
                  if ( *ret ) {
                     if ( i_first )
                        safe_chr(' ', buff, bufcx);
                     i_first = 1;
                     if ( i_type == 1 ) {
                        sprintf(s_tmpbuff, "[#%d] ", parent);
                        safe_str(s_tmpbuff, buff, bufcx);
                        safe_str(ret, buff, bufcx);
                     } else {
                        safe_str(ret, buff, bufcx);
                     }
                  }
                  free_lbuf(ret);
               }
               s_strtok = strtok_r(NULL, " ", &s_strtokptr);
            }
            free_mbuf(s_tmpbuff);
         }
         free_lbuf(s_text);
      }
   }
}


#define CLUSTER_GET		1
#define CLUSTER_GET_EVAL	2

void
do_cluster_get(char *buff, char **bufcx, dbref player, dbref cause, dbref caller,
             char *fargs[], int nfargs, char *cargs[], int ncargs, int i_type)
{
   char *s_buff, *s_return;
   dbref thing;
   int attrib;
   ATTR *a_attr;

   if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
      safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
      return;
   }
   if (attrib == NOTHING) {
      return;
   }
   s_buff = alloc_lbuf("cluster_get");
   s_return = find_cluster(thing, player, attrib);
   if ( strcmp(s_return, "#-1") == 0 ) {
      safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
      free_lbuf(s_buff);
      free_lbuf(s_return);
      return;
   } else {
      a_attr = atr_num(attrib);
      if ( a_attr )
         sprintf(s_buff, "%s/%s", s_return, a_attr->name);
      else
         sprintf(s_buff, "%s/-1", s_return);
   }
   free_lbuf(s_return);
   switch(i_type) {
      case CLUSTER_GET:
         fun_get(buff, bufcx, player, cause, caller, &s_buff, 1, cargs, ncargs);
         break;
      case CLUSTER_GET_EVAL:
         fun_get_eval(buff, bufcx, player, cause, caller, &s_buff, 1, cargs, ncargs);
         break;
   }
   free_lbuf(s_buff);
}

FUNCTION(fun_cluster_get)
{
    do_cluster_get(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, CLUSTER_GET);
}

FUNCTION(fun_cluster_get_eval)
{
    do_cluster_get(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, CLUSTER_GET_EVAL);
}
 
FUNCTION(fun_cluster_xget)
{
   char *s_buff, *s_return;
   dbref thing;
   int attrib;
   ATTR *a_attr;

   if ( !*fargs[0] ) {
      safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
      return;
   }
   
   if ( (nfargs < 2) || !*fargs[1] ) {
      return;
   }   

   thing = match_thing_quiet(player, fargs[0]);
   if ( !Good_chk(thing) || !Cluster(thing) ) {
      safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
      return;
   }
 
   a_attr = atr_str(fargs[1]);
   if ( !a_attr ) {
      return;
   }
   attrib = a_attr->number;

   s_return = find_cluster(thing, player, attrib);
   if ( strcmp(s_return, "#-1") == 0 ) {
      safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
      free_lbuf(s_return);
      return;
   } else {
      a_attr = atr_num(attrib);
      if ( a_attr ) {
         s_buff = alloc_lbuf("cluster_get");
         sprintf(s_buff, "%s/%s", s_return, a_attr->name);
         fun_get(buff, bufcx, player, cause, caller, &s_buff, 1, cargs, ncargs);
         free_lbuf(s_buff);
      }
   }
   free_lbuf(s_return);
}

FUNCTION(fun_cluster_flags)
{
   dbref thing;
   int attrib;
   char *s_return, *s_buff[2];
   ATTR *a_attr;

   if ( !*fargs[0] ) {
      safe_str("#-1", buff, bufcx);
      return;
   }
   
   if ( strchr(fargs[0], '/') != NULL ) {
      if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
         safe_str("#-1", buff, bufcx);
         return;
      }
      if ( attrib == NOTHING ) {
         return;
      }
   } else {
      thing = match_thing_quiet(player, fargs[0]);
      attrib = NOTHING;
   }
   if ( !Good_chk(thing) || !Cluster(thing) ) {
      safe_str("#-1", buff, bufcx);
      return;
   }
   s_buff[0] = alloc_lbuf("fun_cluster_flags");
   s_buff[1] = NULL;
   if ( attrib != NOTHING ) {
      s_return = find_cluster(thing, player, attrib);
      a_attr = atr_num(attrib);
      sprintf(s_buff[0], "%s/%s", s_return, a_attr->name);
      free_lbuf(s_return);
   } else {
      sprintf(s_buff[0], "#%d", thing);
   }
   fun_flags(buff, bufcx, player, cause, caller, s_buff, 1, cargs, ncargs);
   free_lbuf(s_buff[0]);
}

FUNCTION(fun_cluster_hasflag)
{
   dbref thing;
   int attrib;
   char *s_return, *s_buff[2];
   ATTR *a_attr;

   if ( !*fargs[0] ) {
      safe_str("#-1", buff, bufcx);
      return;
   }
   
   if ( strchr(fargs[0], '/') != NULL ) {
      if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
         safe_str("#-1", buff, bufcx);
         return;
      }
      if ( attrib == NOTHING ) {
         safe_str("0", buff, bufcx);
         return;
      }
   } else {
      thing = match_thing_quiet(player, fargs[0]);
      attrib = NOTHING;
   }
   if ( !Good_chk(thing) || !Cluster(thing) ) {
      safe_str("#-1", buff, bufcx);
      return;
   }
   s_buff[0] = alloc_lbuf("fun_cluster_hasflag");
   s_buff[1] = fargs[1];
   if ( attrib != NOTHING ) {
      s_return = find_cluster(thing, player, attrib);
      a_attr = atr_num(attrib);
      sprintf(s_buff[0], "%s/%s", s_return, a_attr->name);
      free_lbuf(s_return);
   } else {
      sprintf(s_buff[0], "#%d", thing);
   }
   fun_hasflag(buff, bufcx, player, cause, caller, s_buff, 2, cargs, ncargs);
   free_lbuf(s_buff[0]);
}

FUNCTION(fun_cluster_hasattr)
{
   dbref thing;
   int attrib;
   char *s_return, *s_buff[2];
   ATTR *a_attr;

   if (!fn_range_check("CLUSTER_HASATTR", nfargs, 1, 2, buff, bufcx))
       return;

   if ( !*fargs[0] ) {
      safe_str("#-1", buff, bufcx);
      return;
   }
   
   if ( nfargs == 1 ) {
      if (!parse_attrib(player, fargs[0], &thing, &attrib)) {
         safe_str("#-1", buff, bufcx);
         return;
      }
      if ( attrib == NOTHING ) {
         safe_str("0", buff, bufcx);
         return;
      }
      s_return = find_cluster(thing, player, attrib);
      a_attr = atr_num(attrib);
      s_buff[0] = alloc_lbuf("fun_cluster_hasattr");
      s_buff[1] = NULL;
      sprintf(s_buff[0], "%s/%s", s_return, a_attr->name);
      fun_hasattr(buff, bufcx, player, cause, caller, s_buff, 1, cargs, ncargs);
      free_lbuf(s_buff[0]);
      free_lbuf(s_return);
   } else {
      thing = match_thing_quiet(player, fargs[0]);
      if ( !Good_chk(thing) || !Cluster(thing) ) {
         safe_str("#-1", buff, bufcx);
         return;
      }
      a_attr = atr_str(fargs[1]);
      if ( !a_attr ) {
         ival(buff, bufcx, 0);
         return;
      }
      attrib = a_attr->number;
      s_return = find_cluster(thing, player, attrib);
      s_buff[0] = s_return;
      s_buff[1] = fargs[1];
      fun_hasattr(buff, bufcx, player, cause, caller, s_buff, 2, cargs, ncargs);
      free_lbuf(s_return);
   }
}

#define CLUSTER_U		1
#define CLUSTER_U2		2
#define CLUSTER_ULOCAL		3
#define CLUSTER_U2LOCAL		4
#define CLUSTER_DEFAULT		5
#define CLUSTER_EDEFAULT	6
#define CLUSTER_UDEFAULT	7
#define CLUSTER_U2DEFAULT	8
#define CLUSTER_ULDEFAULT	9
#define CLUSTER_U2LDEFAULT	10

void
do_cluster_u(char *buff, char **bufcx, dbref player, dbref cause, dbref caller,
             char *fargs[], int nfargs, char *cargs[], int ncargs, char *s_name,
             int i_is_alt, int i_is_local, int i_needeval, int i_givedefault)
{
    dbref thing;
    int anum, i, i_bypass;
    ATTR *ap;
    char *atext, *result, *xargs[MAX_ARG], *savereg[MAX_GLOBAL_REGS], *pt;

    /* We need at least one argument */

    if ( ((i_is_alt <=4) && (nfargs < 1)) || ((i_is_alt > 4) && (nfargs < 2)) ) {
       safe_str("#-1 FUNCTION (", buff, bufcx);
       safe_str(s_name, buff, bufcx);
       safe_str(") EXPECTS ", buff, bufcx);
       if ( i_is_alt < 5 )
          ival(buff, bufcx, 1);
       else
          ival(buff, bufcx, 2);
       safe_str(" OR MORE ARGUMENTS [RECEIVED ", buff, bufcx);
       ival(buff, bufcx, nfargs);
       safe_chr(']', buff, bufcx);
       return;
    }
    /* Two possibilities for the first arg: <obj>/<attr> and <attr>. */

    i_bypass = 0;
    if ( i_needeval && *fargs[0] ) {
       pt = alloc_lbuf("cluster_default_tmpbuff");
       strcpy(pt, fargs[0]);
       atext = exec(player, cause, caller,  EV_FCHECK | EV_EVAL | EV_STRIP, pt, cargs, ncargs);
       free_lbuf(pt);
    } else {
       atext = fargs[0];
    }
    if (parse_attrib(player, atext, &thing, &anum)) {
       if ((anum == NOTHING) || (!Good_obj(thing)))
           ap = NULL;
       else
           ap = atr_num(anum);
    } else {
       thing = player;
       ap = atr_str(atext);
    }
    if ( i_needeval ) {
       free_lbuf(atext);
    }
    /* Make sure we got a good attribute */
    if (!ap) {
       if ( i_givedefault )
          i_bypass = 1;
       else
          return;
    }

    /* Make sure target is valid cluster */
    if ( !Good_chk(thing) || !Cluster(thing) ) {
       if ( i_givedefault )
          i_bypass = 1;
       else
          return;
    }

    if ( !i_bypass ) {
       anum = ap->number;
    } else {
       anum = NOTHING;
    }
    result = find_cluster(thing, player, anum);
    if ( strcmp(result, "#-1") == 0 ) {
       if ( i_givedefault ) {
          i_bypass = 1;
       } else {
          free_lbuf(result);
          return;
       }
    }

    atext = alloc_lbuf("function_cluster_u");

    if ( !i_bypass ) {
       ap = atr_num(anum);
       if ( strchr(fargs[0], '/') != NULL ) {
          sprintf(atext, "%.32s/%.3960s", result, ap->name);
       } else {
          strcpy(atext, fargs[0]);
       }
    } else {
       strcpy(atext, "#-1");
    }

    xargs[0] = atext;
    for ( i = 1; i < nfargs; i++ ) {
       xargs[i] = fargs[i];
    }
    if ( i_is_local ) {
       for (i = 0; i < MAX_GLOBAL_REGS; i++) {
         savereg[i] = alloc_lbuf("cluster_ulocal_reg");
         pt = savereg[i];
         safe_str(mudstate.global_regs[i], savereg[i], &pt);
       }
    }
    switch(i_is_alt) {
       case CLUSTER_U:
          fun_u(buff, bufcx, player, cause, caller, xargs, nfargs, cargs, ncargs);
          break;
       case CLUSTER_U2: 
          fun_u2(buff, bufcx, player, cause, caller, xargs, nfargs, cargs, ncargs);
          break;
       case CLUSTER_ULOCAL:
          fun_ulocal(buff, bufcx, player, cause, caller, xargs, nfargs, cargs, ncargs);
          break;
       case CLUSTER_U2LOCAL:
          fun_u2local(buff, bufcx, player, cause, caller, xargs, nfargs, cargs, ncargs);
          break;
       case CLUSTER_DEFAULT:
          fun_default(buff, bufcx, player, cause, caller, xargs, nfargs, cargs, ncargs);
          break;
       case CLUSTER_EDEFAULT:
          fun_edefault(buff, bufcx, player, cause, caller, xargs, nfargs, cargs, ncargs);
          break;
       case CLUSTER_UDEFAULT:
          fun_udefault(buff, bufcx, player, cause, caller, xargs, nfargs, cargs, ncargs);
          break;
       case CLUSTER_U2DEFAULT:
          fun_u2default(buff, bufcx, player, cause, caller, xargs, nfargs, cargs, ncargs);
          break;
       case CLUSTER_ULDEFAULT:
          fun_uldefault(buff, bufcx, player, cause, caller, xargs, nfargs, cargs, ncargs);
          break;
       case CLUSTER_U2LDEFAULT:
          fun_u2ldefault(buff, bufcx, player, cause, caller, xargs, nfargs, cargs, ncargs);
          break;
    }
    if ( i_is_local ) {
       for (i = 0; i < MAX_GLOBAL_REGS; i++) {
         pt = mudstate.global_regs[i];
         safe_str(savereg[i], mudstate.global_regs[i], &pt);
         free_lbuf(savereg[i]);
       }
    }
    free_lbuf(result);
    free_lbuf(atext);
}

FUNCTION(fun_cluster_u)
{
    do_cluster_u(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, (char *)"CLUSTER_U", CLUSTER_U, 0, 0, 0);
}

FUNCTION(fun_cluster_u2)
{
    do_cluster_u(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, (char *)"CLUSTER_U2", CLUSTER_U2, 0, 0, 0);
}

FUNCTION(fun_cluster_ulocal)
{
    do_cluster_u(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, (char *)"CLUSTER_ULOCAL", CLUSTER_ULOCAL, 1, 0, 0);
}

FUNCTION(fun_cluster_u2local)
{
    do_cluster_u(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, (char *)"CLUSTER_U2LOCAL", CLUSTER_U2LOCAL, 1, 0, 0);
}

FUNCTION(fun_cluster_default)
{
    do_cluster_u(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, (char *)"CLUSTER_DEFAULT", CLUSTER_DEFAULT, 0, 1, 1);
}

FUNCTION(fun_cluster_edefault)
{
    do_cluster_u(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, (char *)"CLUSTER_EDEFAULT", CLUSTER_EDEFAULT, 0, 1, 1);
}

FUNCTION(fun_cluster_udefault)
{
    do_cluster_u(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, (char *)"CLUSTER_UDEFAULT", CLUSTER_UDEFAULT, 0, 1, 1);
}

FUNCTION(fun_cluster_u2default)
{
    do_cluster_u(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, (char *)"CLUSTER_U2DEFAULT", CLUSTER_U2DEFAULT, 0, 1, 1);
}

FUNCTION(fun_cluster_uldefault)
{
    do_cluster_u(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, (char *)"CLUSTER_ULDEFAULT", CLUSTER_ULDEFAULT, 1, 1, 1);
}

FUNCTION(fun_cluster_u2ldefault)
{
    do_cluster_u(buff, bufcx, player, cause, caller, fargs, nfargs, cargs, ncargs, (char *)"CLUSTER_U2LDEFAULT", CLUSTER_U2LDEFAULT, 1, 1, 1);
}

FUNCTION(fun_cluster_set)
{
   dbref thing, aowner;
   int attrib, i_flag, i_flag2, aflags;
   char *s_return, *s_instr, *s_instrptr, *s_buf1, *s_strtok, *s_strtokptr;
   CMDENT *cmdp;
   ATTR *a_clust;

   if ( !(mudconf.sideeffects & SIDE_SET) ) {
      notify(player, "#-1 FUNCTION DISABLED");
      return;
   }
   if ( !SideFX(player) || Fubar(player) || Slave(player) || return_bit(player) < mudconf.restrict_sidefx ) {
      notify(player, "Permission denied.");
      return;
   }
   mudstate.sidefx_currcalls++;
   cmdp = (CMDENT *)hashfind((char *)"@set", &mudstate.command_htab);
   if ( !check_access(player, cmdp->perms, cmdp->perms2, 0) || cmdtest(player, "@set") ||
         cmdtest(Owner(player), "@set") || zonecmdtest(player, "@set") ) {
      notify(player, "Permission denied.");
      return;
   }

   s_buf1 = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                 fargs[0], cargs, ncargs);

   attrib = NOTHING;

   i_flag2 = 0;
   i_flag = 1;

   if ( strchr(s_buf1, '/') != NULL ) {
      s_instrptr = s_buf1;
      while ( *s_instrptr && (*s_instrptr != '/') )
         s_instrptr++;
      if ( *s_instrptr ) {
         *s_instrptr = '\0';
         s_instrptr++;
      }
      thing = match_thing(player, s_buf1);
      if ( *s_instrptr ) {
         a_clust = atr_str(s_instrptr);
         if ( a_clust ) {
            attrib = a_clust->number;
            i_flag2 = 1;
            i_flag = 0;
         }
      }
   } else {
      thing = match_thing(player, s_buf1);
   }
   free_lbuf(s_buf1);

   if ( (nfargs < 2) || !*fargs[1] || (thing < 0))
      return;

   /* s_instrptr = s_instr = alloc_lbuf("fun_cluster_set"); */
   s_buf1 = exec(player, cause, caller, EV_STRIP | EV_FCHECK | EV_EVAL,
                 fargs[1], cargs, ncargs);
   s_instr = alloc_lbuf("do_cluster_set_s_instr");
   strcpy(s_instr, s_buf1);
   s_instrptr = s_instr;
   if ( (attrib == NOTHING) && (strchr(s_instr, ':') != NULL) ) {
      i_flag = 0;
      while ( *s_instrptr && (*s_instrptr != ':') ) {
         s_instrptr++;
      }
      *s_instrptr = '\0';
      a_clust = atr_str(s_instr);
      if ( a_clust )
         attrib = a_clust->number;
   } 
   free_lbuf(s_instr);

   if ( i_flag ) {
      if ( Good_chk(thing) && Cluster(thing) ) {
         a_clust = atr_str("_CLUSTER");
         if ( !a_clust ) {
            safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
         } else {
            s_return = atr_get(thing, a_clust->number, &aowner, &aflags);
            s_instr = alloc_lbuf("cluster_set_flags");
            s_strtok = strtok_r(s_return, " ", &s_strtokptr);
            s_instrptr = alloc_lbuf("cluster_set_buf1");
            while ( s_strtok ) {
               thing = match_thing(player, s_strtok);
               if ( Good_chk(thing) && Cluster(thing) ) {
                  strcpy(s_instr, s_strtok);
                  strcpy(s_instrptr, s_buf1);
                  if ( Cluster(player) && (Owner(player) == Owner(thing)) )
                     do_set(thing, thing, (SET_QUIET|SIDEEFFECT), s_instr, s_instrptr);
                  else
                     do_set(player, cause, (SET_QUIET|SIDEEFFECT), s_instr, s_instrptr);
               }
               s_strtok = strtok_r(NULL, " ", &s_strtokptr);
            }
            free_lbuf(s_return);
            free_lbuf(s_instr);
            free_lbuf(s_instrptr);
         }
      } else {
         safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
      }
   } else {
      s_return = find_cluster(thing, player, attrib);
      if ( strcmp(s_return, "#-1") == 0 ) {
         safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
      } else {
         if ( i_flag2 && (attrib > 0) ) {
            a_clust = atr_num(attrib);
            if ( a_clust ) {
               s_instr = alloc_lbuf("cluster_set_attrib_flag");
               sprintf(s_instr, "%s/%s", s_return, a_clust->name);
               strcpy(s_return, s_instr);
               free_lbuf(s_instr);
            } 
         }
         if ( Cluster(player) && Cluster(thing) && (Owner(player) == Owner(thing)) ) {
            thing = match_thing(player, s_return);
            if ( !Good_chk(thing) || !Cluster(thing) )
               do_set(player, cause, (SET_QUIET|SIDEEFFECT), s_return, s_buf1);
            else
               do_set(thing, thing, (SET_QUIET|SIDEEFFECT), s_return, s_buf1);
         } else {
            do_set(player, cause, (SET_QUIET|SIDEEFFECT), s_return, s_buf1);
         }
         if ( !i_flag2 ) {
            trigger_cluster_action(thing, player);
         }
      }
      free_lbuf(s_return);
   }
   free_lbuf(s_buf1);
}

#define CLUSTER_CHECK			0
#define CLUSTER_STAT_LIST		1
#define CLUSTER_STAT_ACTION		2
#define CLUSTER_STAT_THRESH		3
#define CLUSTER_STAT_FIND		4
#define CLUSTER_STAT_FUNCTION		5
#define CLUSTER_STAT_TIMEOUTS		6
FUNCTION(fun_cluster_stats)
{
   int i_type, aflags;
   dbref thing, aowner;
   char *s_return;
   ATTR *attr;

   if (!fn_range_check("CLUSTER_STATS", nfargs, 2, 3, buff, bufcx))
       return;

   if ( *fargs[1] )
      i_type = atoi(fargs[1]);

   if ( (i_type > 6) || (i_type < 0) )
      i_type = CLUSTER_CHECK;

   if ( !*fargs[0] ) {
      safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
      return;
   } else {
      thing = match_thing(player, fargs[0]);
      if ( !Good_chk(thing) || !Cluster(thing) ) {
         safe_str("#-1 NO SUCH CLUSTER", buff, bufcx);
         return;
      }
      if ( !Controls(player, thing) ) {
         safe_str("#-1 PERMISSION DENIED", buff, bufcx);
         return;
      }
   }

   switch(i_type) {
      case CLUSTER_CHECK: 
           ival(buff, bufcx, ((Good_chk(thing) && Cluster(thing)) ? 1 : 0));
           break;
      case CLUSTER_STAT_TIMEOUTS:
           s_return = alloc_lbuf("cluster_stat_timeout");
           sprintf(s_return, "Action: %d |ActionFunction: %d", 
                   mudconf.cluster_cap, mudconf.clusterfunc_cap);
           safe_str(s_return, buff, bufcx);
           free_lbuf(s_return);
           break;
      case CLUSTER_STAT_LIST:
           attr = atr_str("_CLUSTER");
           if ( attr ) {
              s_return = atr_get(thing, attr->number, &aowner, &aflags);
              safe_str(s_return, buff, bufcx);
              free_lbuf(s_return);
           }
           break;
      case CLUSTER_STAT_ACTION:
           attr = atr_str("_CLUSTER_ACTION");
           if ( attr ) {
              s_return = atr_get(thing, attr->number, &aowner, &aflags);
              safe_str(s_return, buff, bufcx);
              free_lbuf(s_return);
           }
           break;
      case CLUSTER_STAT_FUNCTION:
           attr = atr_str("_CLUSTER_ACTION_FUNC");
           if ( attr ) {
              s_return = atr_get(thing, attr->number, &aowner, &aflags);
              safe_str(s_return, buff, bufcx);
              free_lbuf(s_return);
           }
           break;
      case CLUSTER_STAT_THRESH:
           attr = atr_str("_CLUSTER_THRESH");
           if ( attr ) {
              s_return = atr_get(thing, attr->number, &aowner, &aflags);
              safe_str(s_return, buff, bufcx);
              free_lbuf(s_return);
           }
           break;
      case CLUSTER_STAT_FIND:
           if ( (nfargs > 2) && *fargs[2] ) {
              attr = atr_str(fargs[2]);
              if ( attr ) {
                 s_return = find_cluster(thing, player, attr->number);
                 thing = match_thing(player, s_return);
                 free_lbuf(s_return);
                 s_return = atr_get(thing, attr->number, &aowner, &aflags);
                 if ( *s_return ) {
                    dbval(buff, bufcx, thing);
                    free_lbuf(s_return);
                 }
              } else {
                 safe_str("#-1", buff, bufcx);
              }
           }
           break;
      default:
           safe_str("#-1 UNRECOGNIZED TYPE TO CLUSTER STATS", buff, bufcx);
           break;
   } /* Switch */
}

/* ---------------------------------------------------------------------------
 * flist: List of existing functions in alphabetical order.
 */

FUN flist[] =
{
    {"@@", fun_nsnull, -1, FN_NO_EVAL, CA_PUBLIC, 0},
    {"ABS", fun_abs, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ACCENT", fun_accent, 2, 0, CA_PUBLIC, 0},
    {"ACOS", fun_acos, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ADD", fun_add, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"AFLAGS", fun_aflags, 1, 0, CA_IMMORTAL, 0},
    {"AFTER", fun_after, 0, FN_VARARGS, CA_PUBLIC, 0},
    {"AINDEX", fun_aindex, 4, 0, CA_PUBLIC, CA_NO_CODE},
    {"AIINDEX", fun_aiindex, 4, 0, CA_PUBLIC, CA_NO_CODE},
    {"ALPHAMAX", fun_alphamax, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ALPHAMIN", fun_alphamin, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"AND", fun_and, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ANDCHR", fun_andchr, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"ANDFLAG", fun_andflag, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ANDFLAGS", fun_andflags, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"ANSI", fun_ansi, 0, FN_VARARGS, CA_PUBLIC, 0},
    {"APOSS", fun_aposs, 1, 0, CA_PUBLIC, 0},
    {"ARRAY", fun_array, 3, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ART", fun_art, 1, FN_VARARGS, CA_PUBLIC, 0},
    {"ASC", fun_asc, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ASIN", fun_asin, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ATAN", fun_atan, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ATTRCNT", fun_attrcnt, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"AVG", fun_avg, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"BEEP", fun_beep, 1, 0, CA_WIZARD, 0},
    {"BEFORE", fun_before, 0, FN_VARARGS, CA_PUBLIC, 0},
    {"BETWEEN", fun_between, 3, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"BITTYPE", fun_bittype, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"BOUND", fun_bound, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"BRACKETS", fun_brackets, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"BYPASS", fun_bypass, -1, 0, CA_PUBLIC, CA_NO_CODE},
    {"CAND", fun_cand, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#ifdef REALITY_LEVELS
    {"CANSEE", fun_cansee, 2, 0, CA_WIZARD, 0},
#endif /* REALITY_LEVELS */
    {"CAPLIST", fun_caplist, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE}, 
    {"CAPSTR", fun_capstr, -1, 0, CA_PUBLIC, 0},
    {"CASE", fun_case, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"CASEALL", fun_caseall, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"CAT", fun_cat, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CEIL", fun_ceil, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"CENTER", fun_center, 0, FN_VARARGS, CA_PUBLIC, 0},
    {"CHILDREN", fun_children, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CHARIN", fun_charin, 1, FN_VARARGS, CA_WIZARD, 0},
    {"CHAROUT", fun_charout, 1, FN_VARARGS, CA_WIZARD, 0},
    {"CHECKPASS", fun_checkpass,  2, 0, CA_WIZARD, 0},
    {"CHKGARBAGE", fun_chkgarbage, 2, 0, CA_WIZARD, CA_NO_CODE},
#ifdef REALITY_LEVELS
    {"CHKREALITY", fun_chkreality, 2, 0, CA_PUBLIC, CA_NO_CODE},
#endif /* REALITY_LEVELS */
    {"CHKTRACE", fun_chktrace, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"CHOMP", fun_chomp, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CHR", fun_chr, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"CITER", fun_citer, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"CLOAK", fun_cloak, 2, 0, CA_WIZARD, 0},
#ifdef USE_SIDEEFFECT
    {"CLONE", fun_clone, 1, FN_VARARGS, CA_PUBLIC, 0},
    {"CLUSTER_ADD", fun_cluster_add, 2, 0, CA_PUBLIC, CA_NO_CODE},
#endif
    {"CLUSTER_ATTRCNT", fun_cluster_attrcnt, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_DEFAULT", fun_cluster_default, 2, FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_EDEFAULT", fun_cluster_edefault, 2, FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_FLAGS", fun_cluster_flags, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_GET", fun_cluster_get, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_GET_EVAL", fun_cluster_get_eval, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_GREP", fun_cluster_grep, 3, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_HASATTR", fun_cluster_hasattr, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_HASFLAG", fun_cluster_hasflag, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_LATTR", fun_cluster_lattr, 1, FN_VARARGS, CA_PUBLIC, 0},
#ifdef USE_SIDEEFFECT
    {"CLUSTER_SET", fun_cluster_set, 2, FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#endif
    {"CLUSTER_STATS", fun_cluster_stats, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef TINY_U
    {"CLUSTER_U", fun_cluster_u2, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_U2", fun_cluster_u, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_ULOCAL", fun_cluster_u2local, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_U2LOCAL", fun_cluster_ulocal, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_UDEFAULT", fun_cluster_u2default, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_U2DEFAULT", fun_cluster_udefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_ULDEFAULT", fun_cluster_u2ldefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_U2LDEFAULT", fun_cluster_uldefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#else
    {"CLUSTER_U", fun_cluster_u, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_U2", fun_cluster_u2, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_ULOCAL", fun_cluster_ulocal, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_U2LOCAL", fun_cluster_u2local, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_UDEFAULT", fun_cluster_udefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_U2DEFAULT", fun_cluster_u2default, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_ULDEFAULT", fun_cluster_uldefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_U2LDEFAULT", fun_cluster_u2ldefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#endif
    {"CLUSTER_UEVAL", fun_cluster_ueval, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_VATTRCNT", fun_cluster_vattrcnt, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_WIPE", fun_cluster_wipe, 1, FN_NO_EVAL | FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CLUSTER_XGET", fun_cluster_xget, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"CMDS", fun_cmds, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CNAME", fun_cname, 1, 0, CA_PUBLIC, 0},
    {"COLORS", fun_colors, 1, FN_VARARGS, CA_PUBLIC, 0},
    {"COLUMNS", fun_columns, 3, FN_VARARGS, CA_PUBLIC, 0},
    {"COMP", fun_comp, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"CON", fun_con, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"CONFIG", fun_config, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"CONN", fun_conn, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CONTROLS", fun_controls, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"CONVSECS", fun_convsecs, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"CONVTIME", fun_convtime, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"COR", fun_cor, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"COS", fun_cos, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"COSH", fun_cosh, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"COUNTSPECIAL", fun_countspecial, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CRC32", fun_crc32, 1,  FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"CREPLACE", fun_creplace, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"CREATE", fun_create, 1, FN_VARARGS, CA_PUBLIC, 0},
#endif
    {"CREATETIME", fun_createtime, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"CTU", fun_ctu, 3, 0, CA_PUBLIC, CA_NO_CODE},
#ifdef MUX_INCDEC
    {"DEC", fun_xdec, 1, 0, CA_PUBLIC, CA_NO_CODE},
#else
    {"DEC", fun_dec, 1, 0, CA_PUBLIC, CA_NO_CODE},
#endif
    {"DECODE64", fun_decode64, -1, 0, CA_PUBLIC, CA_NO_CODE},
#ifdef USECRYPT
    {"DECRYPT", fun_decrypt, 2, 0, CA_PUBLIC, CA_NO_CODE},
#endif
    {"DEFAULT", fun_default, 2, FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"DELETE", fun_delete, 3, 0, CA_PUBLIC, CA_NO_CODE},
    {"DELEXTRACT", fun_delextract, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"DESTROY", fun_destroy, 1, 0, CA_PUBLIC, 0},
#endif
    {"DICE", fun_dice, 2, FN_VARARGS, CA_PUBLIC, 0},
#ifdef USE_SIDEEFFECT
    {"DIG", fun_dig, 1, FN_VARARGS, CA_PUBLIC, 0},
#endif
    {"DIGEST", fun_digest, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"DIST2D", fun_dist2d, 4, 0, CA_PUBLIC, CA_NO_CODE},
    {"DIST3D", fun_dist3d, 6, 0, CA_PUBLIC, CA_NO_CODE},
    {"DIV", fun_div, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"DOING", fun_doing, 1, FN_VARARGS, CA_WIZARD, 0},
    {"DYNHELP", fun_dynhelp, 2, FN_VARARGS, CA_WIZARD, 0},
    {"E", fun_e, 0, 0, CA_PUBLIC, CA_NO_CODE},
    {"EE", fun_ee, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"EDEFAULT", fun_edefault, 2, FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"EDIT", fun_edit, 3, FN_VARARGS, CA_PUBLIC, 0},
    {"EDITANSI", fun_editansi, 3, 0, CA_PUBLIC, 0},
    {"ELEMENTS", fun_elements, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ELEMENTSMUX", fun_elementsmux, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ELIST", fun_elist, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"ELOCK", fun_elock, 2, FN_VARARGS, CA_PUBLIC, 0},
#ifdef USE_SIDEEFFECT
    {"EMIT", fun_emit, 1, 0, CA_PUBLIC, 0},
#endif
    {"ENCODE64", fun_encode64, -1, 0, CA_PUBLIC, CA_NO_CODE},
#ifdef USECRYPT
    {"ENCRYPT", fun_encrypt, 2, 0, CA_PUBLIC, CA_NO_CODE},
#endif
    {"ENTRANCES", fun_entrances, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"EQ", fun_eq, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"ERROR", fun_error, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ESCAPE", fun_escape, -1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ESCAPEX", fun_escape, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"EVAL", fun_eval, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"EXIT", fun_exit, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"EXP", fun_exp, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"EXTRACTWORD", fun_extractword, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"EXTRACT", fun_extract, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"FBETWEEN", fun_fbetween, 3, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"FBOUND", fun_fbound, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"FDIV", fun_fdiv, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"FILTER", fun_filter, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"FINDABLE", fun_findable, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"FIRST", fun_first, 0, FN_VARARGS, CA_PUBLIC, 0},
    {"FLAGS", fun_flags, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"FLOOR", fun_floor, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"FLOORDIV", fun_floordiv, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"FMOD", fun_fmod, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"FOLD", fun_fold, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"FOLDERCURRENT", fun_foldercurrent, 0, FN_VARARGS, CA_WIZARD, 0},
    {"FOLDERLIST", fun_folderlist, 1, 0, CA_WIZARD, 0},
    {"FOREACH", fun_foreach, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"FULLNAME", fun_fullname, 1, 0, CA_PUBLIC, 0},
    {"GARBLE", fun_garble, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"GET", fun_get, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"GET_EVAL", fun_get_eval, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"GLOBALROOM", fun_globalroom, 0, 0, CA_PUBLIC, CA_NO_CODE},
    {"GRAB", fun_grab, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"GRABALL", fun_graball, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"GREP", fun_grep, 3, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"GT", fun_gt, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"GTE", fun_gte, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"GUILD", fun_guild, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"HASATTR", fun_hasattr, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"HASATTRP", fun_hasattrp, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"HASDEPOWER", fun_hasdepower, 2, 0, CA_IMMORTAL, 0},
    {"HASFLAG", fun_hasflag, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"HASPOWER", fun_haspower, 2, 0, CA_WIZARD, 0},
    {"HASQUOTA", fun_hasquota, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef REALITY_LEVELS
    {"HASRXLEVEL", fun_hasrxlevel, 2, 0, CA_PUBLIC, CA_NO_CODE},
#endif /* REALITY_LEVELS */
    {"HASTOGGLE", fun_hastoggle, 2, 0, CA_PUBLIC, CA_NO_CODE},
#ifdef REALITY_LEVELS
    {"HASTXLEVEL", fun_hastxlevel, 2, 0, CA_PUBLIC, CA_NO_CODE},
#endif /* REALITY_LEVELS */
    {"HASTYPE", fun_hastype, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"HOME", fun_home, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"IBREAK", fun_ibreak, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"IDLE", fun_idle, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"IINDEX", fun_iindex, 4, 0, CA_PUBLIC, CA_NO_CODE},
    {"IFELSE", fun_ifelse, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"ILEV", fun_ilev, 0, 0, CA_PUBLIC, CA_NO_CODE},
#ifdef MUX_INCDEC
    {"INC", fun_xinc, 1, 0, CA_PUBLIC, CA_NO_CODE},
#else
    {"INC", fun_inc, 1, 0, CA_PUBLIC, CA_NO_CODE},
#endif
    {"INDEX", fun_index, 4, 0, CA_PUBLIC, CA_NO_CODE},
    {"INPROGRAM", fun_inprogram, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"INSERT", fun_insert, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"INUM", fun_inum, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"INZONE", fun_inzone, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ISCLUSTER", fun_iscluster, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ISDBREF", fun_isdbref, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ISINT", fun_isint, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ISNUM", fun_isnum, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ISALNUM", fun_isalnum, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ISALPHA", fun_isalpha, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ISHIDDEN", fun_ishidden, 1, 0, CA_BUILDER, 0},
    {"ISPUNCT", fun_ispunct, -1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ISSPACE", fun_isspace, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ISXDIGIT", fun_isxdigit, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ISDIGIT", fun_isdigit, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ISLOWER", fun_islower, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ISUPPER", fun_isupper, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ISWORD", fun_isword, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ITER", fun_iter, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"ITEXT", fun_itext, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"KEEPFLAGS", fun_keepflags, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"KEEPTYPE", fun_keeptype, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LADD", fun_ladd, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LAND", fun_land, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LAVG", fun_lavg, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LAST", fun_last, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LASTCREATE", fun_lastcreate, 2, 0, CA_PUBLIC, 0},
    {"LATTR", fun_lattr, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LATTRP", fun_lattrp, 1, FN_VARARGS, CA_WIZARD, 0},
    {"LCMDS", fun_lcmds, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LCON", fun_lcon, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LCSTR", fun_lcstr, -1, 0, CA_PUBLIC, 0},
    {"LDELETE", fun_ldelete, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LDEPOWERS", fun_ldepowers, 1, 0, CA_IMMORTAL, 0},
    {"LDIV", fun_ldiv, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LEFT", fun_left, 2, 0, CA_PUBLIC, 0},
#ifdef USE_SIDEEFFECT
    {"LEMIT", fun_lemit, 1, 0, CA_PUBLIC, CA_NO_CODE},
#endif
    {"LEXITS", fun_lexits, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LFLAGS", fun_lflags, 1, 0, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"LINK", fun_link, 2, 0, CA_PUBLIC, 0},
#endif
    {"LIST", fun_list, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"LISTCOMMANDS", fun_listcommands, 0, 0, CA_PUBLIC, CA_NO_CODE},
    {"LISTDIFF", fun_listdiff, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LISTFUNCTIONS", fun_listfunctions, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LISTFLAGS", fun_listflags, 0, 0, CA_PUBLIC, CA_NO_CODE},
    {"LISTINTER", fun_listinter, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LISTMATCH", fun_listmatch, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LISTNEWSGROUPS", fun_listnewsgroups, 0, 1, CA_PUBLIC, CA_NO_CODE},
    {"LISTPROTECTION", fun_listprotection, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef REALITY_LEVELS
    {"LISTRLEVELS", fun_listrlevels, 0, 0, CA_PUBLIC, CA_NO_CODE},
#endif /* REALITY_LEVELS */
    {"LISTTOGGLES", fun_listtoggles, 0, 0, CA_PUBLIC, CA_NO_CODE},
    {"LISTUNION", fun_listunion, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LIT", fun_lit, -1, FN_NO_EVAL, CA_PUBLIC, 0},
    {"LJUST", fun_ljust, 0, FN_VARARGS, CA_PUBLIC, 0},
    {"LJC", fun_ljc, 2, FN_VARARGS, CA_PUBLIC, 0},
    {"LLOC", fun_lloc, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"LMAX", fun_lmax, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LMIN", fun_lmin, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LMUL", fun_lmul, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LN", fun_ln, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"LNOR", fun_lnor, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LNUM", fun_lnum, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LNUM2", fun_lnum2, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LOC", fun_loc, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"LOCALIZE", fun_localize, 1, FN_NO_EVAL | FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LOCALFUNC", fun_localfunc, 1, FN_NO_EVAL|FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LOCATE", fun_locate, 3, 0, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"LOCK", fun_lock, 1, FN_VARARGS, CA_PUBLIC, 0},
#else
    {"LOCK", fun_lock, 1, 0, CA_PUBLIC, 0},
#endif
    {"LOCKDECODE", fun_lockdecode, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"LOCKCHECK", fun_lockcheck, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LOCKENCODE", fun_lockencode, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"LOG", fun_log, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LOGTOFILE", fun_logtofile, 2, 0, CA_IMMORTAL, 0},
    {"LOGSTATUS", fun_logstatus, 0, 0, CA_IMMORTAL, 0},
    {"LOOKUP_SITE", fun_lookup_site, 1, FN_VARARGS, CA_WIZARD, CA_NO_CODE},
    {"LOR", fun_lor, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LPOWERS", fun_lpowers, 1, 0, CA_WIZARD, 0},
    {"LRAND", fun_lrand, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LREPLACE", fun_lreplace, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LROOMS", fun_lrooms, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LSUB", fun_lsub, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LT", fun_lt, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"LTE", fun_lte, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"LTOGGLES", fun_ltoggles, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"LWHO", fun_lwho, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LXNOR", fun_lxnor, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LXOR", fun_lxor, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"LZONE", fun_lzone, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"MAILALIAS", fun_mailalias, 0, FN_VARARGS, CA_WIZARD, 0},
    {"MAILSIZE", fun_mailsize, 2, 0, CA_WIZARD, 0},
    {"MAILREAD", fun_mailread, 3, FN_VARARGS, CA_WIZARD, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"MAILSEND", fun_mailsend, 3, FN_VARARGS, CA_WIZARD, CA_NO_CODE},
#endif
    {"MAILQUICK", fun_mailquick, 0, FN_VARARGS, CA_WIZARD, 0},
    {"MAILQUOTA", fun_mailquota, 1, FN_VARARGS, CA_WIZARD, 0},
    {"MAILSTATUS", fun_mailstatus, 1, FN_VARARGS, CA_WIZARD, 0},
    {"MAP", fun_map, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"MASK", fun_mask, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"MATCH", fun_match, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"MAX", fun_max, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"MEMBER", fun_member, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"MERGE", fun_merge, 3, 0, CA_PUBLIC, CA_NO_CODE},
    {"MID", fun_mid, 3, 0, CA_PUBLIC, 0},
    {"MIN", fun_min, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"MIX", fun_mix, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"MODIFYTIME", fun_modifytime, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"MODULO", fun_modulo, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"MOON", fun_moon, 1, FN_VARARGS, CA_PUBLIC, 0},
    {"MONEY", fun_money, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"MONEYNAME", fun_moneyname, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"MOVE", fun_move, 2, 0, CA_PUBLIC, 0},
#endif
    {"MSIZETOT", fun_msizetot, 0, 0, CA_IMMORTAL, 0},
    {"MUDNAME", fun_mudname, 0, 0, CA_PUBLIC, 0},
    {"MUL", fun_mul, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"MUNGE", fun_munge, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"MWORDS", fun_mwords, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"NAME", fun_name, 1, FN_VARARGS, CA_PUBLIC, 0},
#else
    {"NAME", fun_name, 1, 0, CA_PUBLIC, 0},
#endif
    {"NAMEQ", fun_nameq, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"NAND", fun_nand, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"NCOMP", fun_ncomp, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"NEARBY", fun_nearby, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"NEQ", fun_neq, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"NEXT", fun_next, 1, 0, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"NPEMIT", fun_pemit, 2, FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#endif
    {"NOR", fun_nor, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"NOSTR", fun_nostr, 2, 0, CA_PUBLIC, 0},
    {"NOT", fun_not, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"NOTCHR", fun_notchr, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"NSITER", fun_nsiter, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"NSLOOKUP", fun_nslookup, 1, 0, CA_IMMORTAL, CA_NO_CODE},
    {"NULL", fun_null, -1, 0, CA_PUBLIC, 0},
    {"NUM", fun_num, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"NUMMATCH", fun_nummatch, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"NUMWILDMATCH", fun_numwildmatch, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"NUMMEMBER", fun_nummember, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"NUMPOS", fun_numpos, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"OBJ", fun_obj, 1, 0, CA_PUBLIC, 0},
    {"OBJEVAL", fun_objeval, 2, FN_NO_EVAL|FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"OEMIT", fun_oemit, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"OPEN", fun_open, 1, FN_VARARGS, CA_PUBLIC, 0},
#endif
    {"OR", fun_or, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ORCHR", fun_orchr, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"ORFLAG", fun_orflag, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ORFLAGS", fun_orflags, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"OWNER", fun_owner, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"PACK", fun_pack, 1,  FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"PARENMATCH", fun_parenmatch, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"PARENT", fun_parent, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#else
    {"PARENT", fun_parent, 1, 0, CA_PUBLIC, CA_NO_CODE},
#endif
    {"PARENTS", fun_parents, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"PARSE", fun_parse, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"PARSESTR", fun_parsestr, 2, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#ifdef PASSWD_FUNC
    {"PASSWD", fun_passwd, 1, 0, CA_IMMORTAL, 0},
#endif
    {"PFIND", fun_pfind, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"PEDIT", fun_pedit, 3, FN_VARARGS, CA_PUBLIC, 0},
#ifdef USE_SIDEEFFECT
#ifdef SECURE_SIDEEFFECT
    {"PEMIT", fun_pemit, 2, FN_NO_EVAL|FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#else
    {"PEMIT", fun_pemit, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#endif
#endif
    {"PGREP", fun_pgrep, 3, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"PI", fun_pi, 0, 0, CA_PUBLIC, CA_NO_CODE},
    {"PID", fun_pid, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"PMATCH", fun_pmatch, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"PORT", fun_port, 1, 0, CA_WIZARD, CA_NO_CODE},
    {"POS", fun_pos, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"POSS", fun_poss, 1, 0, CA_PUBLIC, 0},
    {"POWER", fun_power, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"POWER10", fun_power10, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"PRINTF", fun_printf, 2, FN_VARARGS, CA_PUBLIC, 0},
    {"PRIVATIZE", fun_privatize, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"PROGRAMMER", fun_programmer, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"PTIMEFMT", fun_ptimefmt, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"PUSHREGS", fun_pushregs, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"QUOTA", fun_quota, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"R", fun_r, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"RACE", fun_race, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"RAND", fun_rand, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"RANDEXTRACT", fun_randextract, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"RANDMATCH", fun_randmatch, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"RANDPOS", fun_randpos, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"REBOOTTIME", fun_reboottime, 0, 0, CA_PUBLIC, CA_NO_CODE},
    {"REBOOTSECS", fun_rebootsecs, 0, 0, CA_PUBLIC, CA_NO_CODE},
    {"REMAINDER", fun_mod, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"REMFLAGS", fun_remflags, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
#ifdef SECURE_SIDEEFFECT
    {"REMIT", fun_remit, 2, FN_NO_EVAL|FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#else
    {"REMIT", fun_remit, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#endif
#endif
    {"REMOVE", fun_remove, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REMTYPE", fun_remtype, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REPEAT", fun_repeat, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"REPLACE", fun_replace, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REST", fun_rest, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"REVERSE", fun_reverse, -1, 0, CA_PUBLIC, CA_NO_CODE},
    {"REVWORDS", fun_revwords, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"RIGHT", fun_right, 2, 0, CA_PUBLIC, 0},
    {"RINDEX", fun_rindex, 4, 0, CA_PUBLIC, CA_NO_CODE},
    {"RJUST", fun_rjust, 0, FN_VARARGS, CA_PUBLIC, 0},
    {"RJC", fun_rjc, 2, FN_VARARGS, CA_PUBLIC, 0},
    {"RLOC", fun_rloc, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"RNUM", fun_rnum, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"ROMAN", fun_roman, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ROOM", fun_room, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"ROTL", fun_rotl, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"ROTR", fun_rotr, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"ROUND", fun_round, 2, 0, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"RSET", fun_rset, 2, FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#endif
#ifdef REALITY_LEVELS
#ifdef USE_SIDEEFFECT
    {"RXLEVEL", fun_rxlevel, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#else
    {"RXLEVEL", fun_rxlevel, 1, 0, CA_PUBLIC, CA_NO_CODE},
#endif /* USE_SIDEEFFECT */
#endif /* REALITY_LEVELS */
    {"S", fun_s, -1, 0, CA_PUBLIC, CA_NO_CODE},
    {"SAFEBUFF", fun_safebuff, 1, FN_VARARGS , CA_PUBLIC, CA_NO_CODE},
    {"SCRAMBLE", fun_scramble, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"SEARCH", fun_search, -1, 0, CA_PUBLIC, 0},
    {"SEARCHNG", fun_searchng, -1, 0, CA_PUBLIC, 0},
    {"SECS", fun_secs, 0, 0, CA_PUBLIC, CA_NO_CODE},
    {"SECURE", fun_secure, -1, 0, CA_PUBLIC, CA_NO_CODE},
    {"SECUREX", fun_secure, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"SEES", fun_sees, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"SET", fun_set, 2, FN_NO_EVAL, CA_PUBLIC, 0},
#endif
    {"SETDIFF", fun_setdiff, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"SETINTER", fun_setinter, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef OLD_SETQ
    {"SETQ", fun_setq_old, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"SETQ_OLD", fun_setq, 2, FN_VARARGS|FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#else
    {"SETQ", fun_setq, 2, FN_VARARGS|FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"SETQ_OLD", fun_setq_old, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#endif
    {"SETQMATCH", fun_setqmatch, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef OLD_SETQ
    {"SETR", fun_setr_old, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"SETR_OLD", fun_setr, 2, FN_VARARGS|FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#else
    {"SETR", fun_setr, 2, FN_NO_EVAL|FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"SETR_OLD", fun_setr_old, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#endif
    {"SETUNION", fun_setunion, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"SHIFT", fun_shift, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"SHL", fun_shl, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"SHR", fun_shr, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"SHUFFLE", fun_shuffle, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"SIGN", fun_sign, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"SIN", fun_sin, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"SINH", fun_sinh, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"SINGLETIME", fun_singletime, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"SIZE", fun_size, 1, FN_VARARGS, CA_WIZARD, 0},
    {"SORT", fun_sort, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"SORTBY", fun_sortby, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"SOUNDEX", fun_soundex, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"SOUNDLIKE", fun_soundlike, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"SPACE", fun_space, 1, 0, CA_PUBLIC, 0},
    {"SPELLNUM", fun_spellnum, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"SPLICE", fun_splice, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"SQRT", fun_sqrt, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"SQUISH", fun_squish, 0, FN_VARARGS, CA_PUBLIC, 0},
    {"STARTTIME", fun_starttime, 0, 0, CA_PUBLIC, CA_NO_CODE},
    {"STARTSECS", fun_startsecs, 0, 0, CA_PUBLIC, CA_NO_CODE},
    {"STATS", fun_stats, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"STEP", fun_step, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"STR", fun_str, 2, 0, CA_PUBLIC, 0},
    {"STRDISTANCE", fun_strdistance, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"STREQ", fun_streq, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"STREVAL", fun_streval, 2, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"STRIP", fun_strip, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"STRIPACCENTS", fun_stripaccents, 1, 0, CA_PUBLIC, 0},
    {"STRIPANSI", fun_stripansi, 1, 0, CA_PUBLIC, 0},
    {"STRCAT", fun_strcat, 0,  FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"STRFUNC", fun_strfunc, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"STRLEN", fun_strlen, -1, 0, CA_PUBLIC, CA_NO_CODE},
    {"STRLENRAW", fun_strlenraw, -1, 0, CA_IMMORTAL, CA_NO_CODE},
    {"STRLENVIS", fun_strlenvis, -1, 0, CA_PUBLIC, CA_NO_CODE},
    {"STRMATCH", fun_strmatch, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"STRMATH", fun_strmath, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"SUB", fun_sub, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"SUBJ", fun_subj, 1, 0, CA_PUBLIC, 0},
    {"SUBNETMATCH", fun_subnetmatch, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"SWITCH", fun_switch, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"SWITCHALL", fun_switchall, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"T", fun_t, 1, 1, CA_PUBLIC, CA_NO_CODE},
    {"TAN", fun_tan, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"TANH", fun_tanh, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"TEL", fun_tel, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#endif
    {"TEXTFILE", fun_textfile, 1, FN_VARARGS, CA_WIZARD, 0},
    {"TIME", fun_time, 0, 0, CA_PUBLIC, CA_NO_CODE},
    {"TIMEFMT", fun_timefmt, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"TOBIN", fun_tobin, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"TODEC", fun_todec, 1, 0, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"TOGGLE", fun_toggle, 2, 0, CA_PUBLIC, CA_NO_CODE},
#endif
    {"TOHEX", fun_tohex, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"TOOCT", fun_tooct, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"TOTCMDS", fun_totcmds, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"TOTMATCH", fun_totmatch, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"TOTWILDMATCH", fun_totwildmatch, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"TOTMEMBER", fun_totmember, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"TOTPOS", fun_totpos, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"TR", fun_tr, 3, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"TRACE", fun_trace, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"TRANSLATE", fun_translate, 2, FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"TRIM", fun_trim, 0, FN_VARARGS, CA_PUBLIC, 0},
    {"TRUNC", fun_trunc, 1, 0, CA_PUBLIC, CA_NO_CODE},
#ifdef REALITY_LEVELS
#ifdef USE_SIDEEFFECT
    {"TXLEVEL", fun_txlevel, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#else
    {"TXLEVEL", fun_txlevel, 1, 0, CA_PUBLIC, CA_NO_CODE},
#endif /* USE_SIDEEFFECT */
#endif /* REALITY_LEVELS */
    {"TYPE", fun_type, 1, 0, CA_PUBLIC, CA_NO_CODE},
#ifdef TINY_U
    {"U", fun_u2, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#else
    {"U", fun_u, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#endif
    {"UCSTR", fun_ucstr, -1, 0, CA_PUBLIC, 0},
#ifdef TINY_U
    {"UDEFAULT", fun_u2default, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#else
    {"UDEFAULT", fun_udefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#endif
    {"UEVAL", fun_ueval, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef TINY_U
    {"ULDEFAULT", fun_u2ldefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"ULOCAL", fun_u2local, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"U2", fun_u, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"U2DEFAULT", fun_udefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"U2LDEFAULT", fun_uldefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"U2LOCAL", fun_ulocal, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#else
    {"ULDEFAULT", fun_uldefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"ULOCAL", fun_ulocal, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"U2", fun_u2, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"U2DEFAULT", fun_u2default, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"U2LDEFAULT", fun_u2ldefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"U2LOCAL", fun_u2local, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#endif
    {"UNPACK", fun_unpack, 1,  FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"V", fun_v, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"VADD", fun_vadd, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"VALID", fun_valid, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"VATTRCNT", fun_vattrcnt, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"VCROSS", fun_vcross, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"VDIM", fun_vdim, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"VDOT", fun_vdot, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"VERSION", fun_version, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"VISIBLE", fun_visible, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"VISIBLEMUX", fun_visiblemux, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"VMAG", fun_vmag, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"VMUL", fun_vmul, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"VSUB", fun_vsub, 2, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"VUNIT", fun_vunit, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"WILDMATCH", fun_wildmatch, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
    {"WIPE", fun_wipe, 1, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#endif
    {"WHERE", fun_where, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"WHILE", fun_while, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"WORDPOS", fun_wordpos, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"WORDS", fun_words, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"WRAP", fun_wrap, 2, FN_VARARGS, CA_PUBLIC, 0},
    {"WRAPCOLUMNS", fun_wrapcolumns, 3, FN_VARARGS, CA_PUBLIC, 0},
    {"WRITABLE", fun_writable, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"XCON", fun_xcon, 3, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"XGET", fun_xget, 2, 0, CA_PUBLIC, CA_NO_CODE},
#ifdef MUX_INCDEC
    {"XDEC", fun_dec, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"XINC", fun_inc, 1, 0, CA_PUBLIC, CA_NO_CODE},
#else
    {"XDEC", fun_xdec, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {"XINC", fun_xinc, 1, 0, CA_PUBLIC, CA_NO_CODE},
#endif
    {"XNOR", fun_xnor, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"XOR", fun_xor, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"XORCHR", fun_xorchr, 2, 0, CA_PUBLIC, CA_NO_CODE},
    {"XORFLAG", fun_xorflag, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef USE_SIDEEFFECT
#ifdef SECURE_SIDEEFFECT
    {"ZEMIT", fun_zemit, 2, FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#else
    {"ZEMIT", fun_zemit, 2, 0, CA_PUBLIC, CA_NO_CODE},
#endif
#endif
#ifdef TINY_U
    {"ZFUN", fun_zfun2, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ZFUNDEFAULT", fun_zfun2default, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#else
    {"ZFUN", fun_zfun, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ZFUNDEFAULT", fun_zfundefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
#endif
    {"ZFUNEVAL", fun_zfuneval, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#ifdef TINY_U
    {"ZFUNLDEFAULT", fun_zfun2ldefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"ZFUNLOCAL", fun_zfun2local, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ZFUN2", fun_zfun, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ZFUN2DEFAULT", fun_zfundefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"ZFUN2LDEFAULT", fun_zfunldefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"ZFUN2LOCAL", fun_zfunlocal, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#else
    {"ZFUNLDEFAULT", fun_zfunldefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"ZFUNLOCAL", fun_zfunlocal, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ZFUN2", fun_zfun2, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
    {"ZFUN2DEFAULT", fun_zfun2default, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"ZFUN2LDEFAULT", fun_zfun2ldefault, 0, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, CA_NO_CODE},
    {"ZFUN2LOCAL", fun_zfun2local, 0, FN_VARARGS, CA_PUBLIC, CA_NO_CODE},
#endif
    {"ZWHO", fun_zwho, 1, 0, CA_PUBLIC, CA_NO_CODE},
    {NULL, NULL, 0, 0, 0, 0}
};


void
NDECL(init_ansitab)
{
   PENNANSI *cm;

   hashinit(&mudstate.ansi_htab, 131);
   for (cm = penn_namecolors; cm->name; cm++) {
      hashadd2(cm->name, (int *) cm, &mudstate.ansi_htab, 1);
   }
}

void
NDECL(init_functab)
{
    FUN *fp;
    char *buff, *cp, *dp;

    buff = alloc_sbuf("init_functab");
    hashinit(&mudstate.func_htab, 521);
    for (fp = flist; fp->name; fp++) {
      cp = (char *) fp->name;
      dp = buff;
      while (*cp) {
         *dp = ToLower((int)*cp);
         cp++;
         dp++;
      }
      *dp = '\0';
      hashadd2(buff, (int *) fp, &mudstate.func_htab, 1);
    }
    free_sbuf(buff);

    ufun_head = NULL;
    ulfun_head = NULL;
    hashinit(&mudstate.ufunc_htab, 131);
    hashinit(&mudstate.ulfunc_htab, 131);
}

void
do_function(dbref player, dbref cause, int key, char *fname, char *target)
{
    UFUN *ufp, *ufp2, *ufp3;
    ATTR *ap;
    char *np, *bp, *tpr_buff, *tprp_buff, *atext, *tpr2_buff, *tprp2_buff, 
         s_funlocal[100], s_minargs[4], s_maxargs[4], *s_chkattr, *s_chkattrptr,
         *s_buffptr;
    int atr, aflags, count, i_tcount, count_owner, i_local, i_array[LIMIT_MAX], i, aflags2;
    dbref obj, aowner, aowner2;

    i_local = 0;
    if ( key & FN_LOCAL ) {
       i_local = 1;
       key &= ~FN_LOCAL;
    }
    /* Make a local uppercase copy of the function name */
    if ( (key & (FN_LIST|FN_DEL)) && (key & (FN_PRES|FN_PROTECT)) ) {
       notify(player, "Illegal combination of switches.");
       return;
    }

    if ( (key & FN_DEL) && !*fname ) {
       notify(player, "Delete what function?");
       return;
    }
    memset(s_funlocal, '\0', sizeof(s_funlocal));
    if ( (key & (FN_MAX|FN_MIN)) ) {
       tprp_buff = tpr_buff = alloc_lbuf("do_function");
       bp = np = alloc_sbuf("add_user_func");
       safe_sb_str(fname, np, &bp);
       *bp = '\0';
       for (bp = np; *bp; bp++)
          *bp = ToLower((int)*bp);
       if ( i_local ) {
          sprintf(s_funlocal, "%d_%s", Owner(player), np);
          ufp = (UFUN *) hashfind(s_funlocal, &mudstate.ulfunc_htab);
          if ( !ufp || ((ufp->owner != Owner(player)) && !controls(player, ufp->owner)) )
             ufp = NULL;
       } else {
          ufp = (UFUN *) hashfind(np, &mudstate.ufunc_htab);
       }
       if ( !ufp ) {
          notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Function %s not found.", fname));
       } else {
          i_tcount = atoi(target);
          if ( !(((i_tcount > 0) && (i_tcount < MAX_ARGS)) || (i_tcount == -1)) ) {
             notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, 
                          "Value out of range.  Expect -1 or between 1 and %d.", MAX_ARGS - 1));
          } else {
             if ( key & FN_MIN) {
                notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Function %s MINARGS set to %d.", fname, i_tcount));
                ufp->minargs = i_tcount;
             } else {
                notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Function %s MAXARGS set to %d.", fname, i_tcount));
                ufp->maxargs = i_tcount;
             }
          }
       }
       free_sbuf(np);
       free_lbuf(tpr_buff);
       return;
    }
    if ( (key & FN_DISPLAY) && (Wizard(player) || i_local) ) {
       if ( !*fname ) {
          notify(player, "Display details on what function?");
          return;
       } else {
          bp = np = alloc_sbuf("add_user_func");
          safe_sb_str(fname, np, &bp);
          *bp = '\0';
          for (bp = np; *bp; bp++)
              *bp = ToLower((int)*bp);
          if ( i_local ) {
             if ( Immortal(player) && (*np == '#') ) {
                sprintf(s_funlocal, "%s", np+1);
             } else {
                sprintf(s_funlocal, "%d_%s", Owner(player), np);
             }
          }
          if ( !(i_local && (ufp = (UFUN *) hashfind(s_funlocal, &mudstate.ulfunc_htab))) &&
               !(!i_local && (ufp = (UFUN *) hashfind(np, &mudstate.ufunc_htab))) ) {
             notify(player, "No matching user-defined function.");
          } else {
             if ( i_local && (!ufp || ((ufp->owner != Owner(player)) && !controls(player, ufp->owner))) ) {
                notify(player, "No matching user-defined function.");
             } else {
                notify(player, "---------------------------------------"\
                               "---------------------------------------");
                ap = atr_num(ufp->atr);
                tprp_buff = tpr_buff = alloc_lbuf("do_function");
                tprp2_buff = tpr2_buff = alloc_lbuf("do_function");
                notify(player, safe_tprintf(tpr_buff, &tprp_buff,
                                            "Function Name: %s\n"\
                                            "Target Object: #%d\n"\
                                            "Attribute Name: %s\n"\
                                            "Arguments: min %d, max %d\n"\
                                            "Permissions:%s%s%s%s\n"\
                                            "Owner: #%d", 
                                            ufp->name, 
                                            ((!i_local || (i_local && (Good_chk(ufp->obj) && (Owner(ufp->obj) == ufp->orig_owner)))) 
                                                ? ufp->obj : -1), 
                                            ((ap != NULL) ? ap->name : "(DELETED)"),
                                            ufp->minargs, ufp->maxargs,
                                            ((ufp->flags & FN_PRIV) ? " WIZARD " : " "),
                                            ((ufp->flags & FN_PRES) ? "PRESERVED " : " "),
                                            ((ufp->flags & FN_NOTRACE) ? "NOTRACE " : " "),
                                            ((ufp->flags & FN_PROTECT) ? "PROTECTED" : " "), 
                                            ufp->owner) );
                listset_nametab(player, access_nametab, access_nametab2, ufp->perms, ufp->perms2, "Flags:", 1);
                tprp_buff = tpr_buff;
                atext = NULL;
                if ( Good_chk(ufp->obj) ) {
                   atext = alloc_lbuf("do_function_display");
                   sprintf(atext, "#%d/%s", ufp->obj, ((ap != NULL ? ap->name : "#-1")));
                   fun_parenmatch(tpr2_buff, &tprp2_buff, player, cause, cause, &atext, 1, (char **)NULL, 0);
                   free_lbuf(atext);
                }
                notify(player, safe_tprintf(tpr_buff, &tprp_buff,
                                            "Attribute Contents: %s",
                                            (*tpr2_buff ? tpr2_buff : "(EMPTY)")) );
                notify(player, "---------------------------------------"\
                               "---------------------------------------");
                free_lbuf(tpr_buff);
                free_lbuf(tpr2_buff);
             }
          }
          free_sbuf(np);
       }
       return;
    } else if (key & FN_DISPLAY) {
       notify(player, "Can not display details on that function.");
       return;
    }
    if ( (key & FN_LIST) || !*fname || !fname ) {
       tprp_buff = tpr_buff = alloc_lbuf("do_function");
       if ( i_local ) {
          notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-24s   %-8s  %-26s %-10s%-22s Min/Max Flgs",
                 "Function Name", "DBref#", "Attribute", "#dbref", "[Owner]"));
          tprp_buff = tpr_buff;
          notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%24s   %8s  %26s %10s %3s %3s %-4s",
                 "------------------------", "--------",
                 "--------------------------", "--------------------------------", "---", "---", "----"));
       } else {
          notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-24s   %-8s  %-26s Min/Max Flgs",
                 "Function Name", "DBref#", "Attribute"));
          tprp_buff = tpr_buff;
          notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%24s   %8s  %26s %3s %3s %-4s",
                 "------------------------", "--------",
                 "--------------------------", "---", "---", "----"));
       }
       i_tcount = count = count_owner = 0;
       memset(s_minargs, '\0', sizeof(s_minargs));
       memset(s_maxargs, '\0', sizeof(s_maxargs));
       for (ufp2 = (i_local ? ulfun_head : ufun_head); ufp2; ufp2 = ufp2->next) {
          if ( i_local && ((ufp2->owner != Owner(player)) && !controls(player, ufp2->owner)) ) 
             continue;
          ap = atr_num(ufp2->atr);
          if (ap) {
             if ( ufp2->owner == Owner(player) )
                count_owner++;

             i_tcount++;
             if ( fname && *fname ) {
                if ( !(fname && *fname && wild_match(fname, (char *)ufp2->name, (char **)NULL, 0, 1)) )
                   continue;
             }
             tprp_buff = tpr_buff;
             if ( ufp2->maxargs == -1 ) {
                if ( ufp2->minargs != -1 ) {
                   strcpy(s_maxargs, (char *)"Inf");
                } else {
                   strcpy(s_maxargs, (char *)"   ");
                }
             } else {
                sprintf(s_maxargs, "%-3d", ufp2->maxargs);
             }
             if ( ufp2->minargs == -1 ) {
                if ( ufp2->maxargs != -1 ) {
                   sprintf(s_minargs, "%-3d", (int) 1);
                } else {
                   strcpy(s_minargs, (char *)"   ");
                }
             } else {
                sprintf(s_minargs, "%-3d", ufp2->minargs);
             }
             if ( i_local ) {
                notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-24.24s   #%-7d  %-26.26s %-10d[%-20.20s] %-3s %-3s %c%c%c%c",
                                  ufp2->name, 
                                  ((Good_chk(ufp2->obj) && (Owner(ufp2->obj) == ufp2->orig_owner)) ? ufp2->obj : -1), 
                                  ((ap != NULL) ? ap->name : "(INVALID ATRIBUTE)"),
                                  ufp2->owner, 
                                  (Good_chk(ufp2->owner) ? Name(ufp2->owner) : "(invalid)"),
                                  s_minargs, 
                                  s_maxargs, 
                                  ((ufp2->flags & FN_PRIV) ? 'W' : '-'),
                                  ((ufp2->flags & FN_PRES) ? 'p' : '-'), 
                                  ((ufp2->flags & FN_PROTECT) ? '+' : '-'), 
                                  ((ufp2->flags & FN_NOTRACE) ? 't' : '-')));
             } else {
                notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%-24.24s   #%-7d  %-26.26s %-3s %-3s %c%c%c%c",
                                  ufp2->name, 
                                  ufp2->obj,
                                  ((ap != NULL) ? ap->name : "(INVALID ATRIBUTE)"),
                                  s_minargs, 
                                  s_maxargs, 
                                  ((ufp2->flags & FN_PRIV) ? 'W' : '-'),
                                  ((ufp2->flags & FN_PRES) ? 'p' : '-'), 
                                  ((ufp2->flags & FN_PROTECT) ? '+' : '-'), 
                                  ((ufp2->flags & FN_NOTRACE) ? 't' : '-')));
             }
             count++;
          }
       }
       tprp_buff = tpr_buff;
       if ( i_local ) {
          notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%24s   %8s  %26s %10s %3s %3s %-4s",
                 "------------------------", "--------",
                 "--------------------------", "--------------------------------", "---", "---", "----"));
       } else {
          notify(player, safe_tprintf(tpr_buff, &tprp_buff, "%24s   %8s  %26s %3s %3s %-4s",
                 "------------------------", "--------",
                 "--------------------------", "---", "---", "----"));
       }
       s_chkattr = atr_get(player, A_DESTVATTRMAX, &aowner2, &aflags2);
       i_array[0] = i_array[2] = 0;
       i_array[4] = i_array[1] = i_array[3] = -2;
       if ( *s_chkattr ) {
          for (s_buffptr = (char *) strtok_r(s_chkattr, " ", &s_chkattrptr), i = 0;
               s_buffptr && (i < LIMIT_MAX);
               s_buffptr = (char *) strtok_r(NULL, " ", &s_chkattrptr), i++) {
               i_array[i] = atoi(s_buffptr);
          }
          if ( i_array[4] == -2 ) {
             i_array[4] = mudconf.lfunction_max;
          }
       } else {
          i_array[4] = mudconf.lfunction_max;
       }
       free_lbuf(s_chkattr);
       tprp_buff = tpr_buff;
       if ( i_tcount != count ) {
          if ( i_local ) {
             notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Total LOCAL User-Defined Functions: %d [%d matched] [%d defined of %d max allowed]", 
                    i_tcount, count, count_owner, i_array[4]));
          } else {
             notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Total User-Defined Functions: %d [%d matched]", 
                    i_tcount, count));
          }
       } else {
          if ( i_local ) {
             notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Total LOCAL User-Defined Functions: %d [%d defined of %d max allowed]", 
                    count, count_owner, i_array[4]));
          } else {
             notify(player, safe_tprintf(tpr_buff, &tprp_buff, "Total User-Defined Functions: %d", 
                    count));
          }
       }
       free_lbuf(tpr_buff);
       return;
    }

    bp = np = alloc_sbuf("add_user_func");
    safe_sb_str(fname, np, &bp);
    *bp = '\0';
    for (bp = np; *bp; bp++)
        *bp = ToLower((int)*bp);

    /* Verify that the function doesn't exist in the builtin table */

    if (hashfind(np, &mudstate.func_htab) != NULL) {
        if ( key & FN_DEL )
            notify_quiet(player, "Can not delete built in function.");
        else
            notify_quiet(player,
                          "Function already defined in builtin function table.");
        free_sbuf(np);
        return;
    }

    if ( (key & FN_DEL) ) {
       count = 0;
       if ( i_local ) {
          if ( (*np == '#') && Builder(player) )
             sprintf(s_funlocal, "%s", np+1);
          else
             sprintf(s_funlocal, "%d_%s", Owner(player), np);
          ufp = (UFUN *)hashfind(s_funlocal, &mudstate.ulfunc_htab);
       } else {
          ufp = (UFUN *)hashfind(np, &mudstate.ufunc_htab);
       }
       
       if ( !ufp ) {
          tprp_buff = tpr_buff = alloc_lbuf("do_function");
          notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Function %s not found.", fname));
          free_lbuf(tpr_buff);
       } else if ( i_local && ufp && !((ufp->owner == Owner(player)) || controls(player, ufp->owner) || Immortal(player)) ) {
          tprp_buff = tpr_buff = alloc_lbuf("do_function");
          notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Function %s not found.", fname));
          free_lbuf(tpr_buff);
       } else {
          if ( i_local ) {
             ufp3 = ulfun_head;
             for (ufp2 = ulfun_head; ufp2; ufp2 = ufp2->next) {
                if ( (strcmp(ufp->name, ufp2->name) == 0) && (ufp->owner == ufp2->owner) ) {
                   if ( (strcmp(ufp->name, ulfun_head->name) == 0) && (ufp->owner == ulfun_head->owner) ) {
                      ulfun_head = ulfun_head->next;
                   } else {
                      ufp3->next = ufp2->next;
                   }
                   count = 1;
                   break;
                }
                ufp3 = ufp2;
             }
          } else {
             ufp3 = ufun_head;
             for (ufp2 = ufun_head; ufp2; ufp2 = ufp2->next) {
                if ( strcmp(ufp->name, ufp2->name) == 0 ) {
                   if ( strcmp(ufp->name, ufun_head->name) == 0 ) {
                      ufun_head = ufun_head->next;
                   } else {
                      ufp3->next = ufp2->next;
                   }
                   count = 1;
                   break;
                }
                ufp3 = ufp2;
             }
          }
          tprp_buff = tpr_buff = alloc_lbuf("do_function");
          if ( count ) {
             if ( i_local && (*np == '#') && (s_buffptr = strchr(fname, '_')) ) {
                notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, 
                "%sunction %s deleted (owned by #%d).", (i_local ? "LOCAL F" : "F"), ufp2->name, ufp2->owner));
             } else {
                notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, 
                "%sunction %s deleted.", (i_local ? "LOCAL F" : "F"), ufp2->name));
             }
             if ( ufp2->name )
                free((void *)ufp2->name);
             free(ufp2);
             if ( i_local ) {
                hashdelete(s_funlocal, &mudstate.ulfunc_htab);
             } else {
                hashdelete(np, &mudstate.ufunc_htab);
             }
          } else {
             notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "Warning: Error freeing function %s.", fname));
          }
          free_lbuf(tpr_buff);
       }
       free_sbuf(np);
       return;
    }

    /* Make sure the target object exists */
    if (!parse_attrib(player, target, &obj, &atr)) {
       notify_quiet(player, "I don't see that here.");
       free_sbuf(np);
       return;
    }

    /* If local, make sure you control the target object */
    if ( i_local ) {
       if ( !Good_chk(obj) || !(Immortal(player) || Controls(player, obj)) ) {
          notify_quiet(player, "Permission denied.");
          free_sbuf(np);
          return;
       }
    }

    /* Make sure the attribute exists */
    if (atr == NOTHING) {
       notify_quiet(player, "No such attribute.");
       free_sbuf(np);
       return;
    }

    /* Make sure attribute is readably by me */
    ap = atr_num(atr);
    if (!ap) {
       notify_quiet(player, "No such attribute.");
       free_sbuf(np);
       return;
    }

    /* Make sure we have permission */
    atr_get_info(obj, atr, &aowner, &aflags);
    if (!See_attr(player, obj, ap, aowner, aflags, 0)) {
       notify_quiet(player, "Permission denied.");
       free_sbuf(np);
       return;
    }

    /* Privileged functions require you control the obj.  */
    if ((key & FN_PRIV) && !Controls(player, obj)) {
       notify_quiet(player, "Permission denied.");
       free_sbuf(np);
       return;
    }

    /* Privalaged functions require printable characters -- no ansi */
    if (i_local) {
       s_buffptr = np;
       while ( *s_buffptr ) {
          if ( !isprint(*s_buffptr) || isspace(*s_buffptr) ) {
             free_sbuf(np);
             notify_quiet(player, "LOCAL functions require non white-space printable characters.");
             return;
          }
          s_buffptr++;
       }
    }

   /* See if function already exists.  If so, redefine it */
   if ( i_local ) {
       if ( (*np == '#') && Builder(player) ) {
          sprintf(s_funlocal, "%s", np+1);
          ufp = (UFUN *) hashfind(s_funlocal, &mudstate.ulfunc_htab);
          if ( ufp && !(Immortal(player) || (Builder(player) && Controls(player, ufp->owner))) ) {
             ufp = NULL;
          }
       } else {
          sprintf(s_funlocal, "%d_%s", Owner(player), np);
       }
   }
   if ( i_local ) {
      ufp = (UFUN *) hashfind(s_funlocal, &mudstate.ulfunc_htab);
      if ( !ufp && (*np == '#') ) {
         notify_quiet(player, "You can not start local functions with a '#' character.");
         free_sbuf(np);
         return;
      }
   } else {
      ufp = (UFUN *) hashfind(np, &mudstate.ufunc_htab);
   }
   count = 0;
   if ( i_local && !ufp ) {
      /* You're only allowed 20 local functions total */
      for (ufp3 = ulfun_head; ufp3; ufp3 = ufp3->next) {
         if ( ufp3->owner == Owner(player) ) {
            count++;
         }
      }
   }

   s_chkattr = atr_get(player, A_DESTVATTRMAX, &aowner2, &aflags2);
   i_array[0] = i_array[2] = 0;
   i_array[4] = i_array[1] = i_array[3] = -2;
   if ( *s_chkattr ) {
      for (s_buffptr = (char *) strtok_r(s_chkattr, " ", &s_chkattrptr), i = 0;
           s_buffptr && (i < LIMIT_MAX);
           s_buffptr = (char *) strtok_r(NULL, " ", &s_chkattrptr), i++) {
           i_array[i] = atoi(s_buffptr);
      }
      if ( i_array[4] == -2 ) {
         i_array[4] = mudconf.lfunction_max;
      }
   } else {
      i_array[4] = mudconf.lfunction_max;
   }
   free_lbuf(s_chkattr);

   if ( count >= i_array[4] ) {
      notify(player, unsafe_tprintf("Maximum number of LOCAL functions have been defined for your userid [%d].", i_array[4]));
      free_sbuf(np);
      return;
   }
   count = 0;
   if (!ufp) {
      count = 1;
      ufp = (UFUN *) malloc(sizeof(UFUN));
      ufp->name = strsave(np);
      for (bp = (char *) ufp->name; *bp; bp++)
         *bp = ToUpper((int)*bp);
      ufp->obj = obj;
      ufp->atr = atr;
      ufp->minargs = -1;
      ufp->maxargs = -1;
      ufp->perms = CA_PUBLIC;
      ufp->perms2 = 0;
      ufp->next = NULL;
      ufp->owner = Owner(player);
      ufp->orig_owner = Owner(obj);
      if ( i_local ) {
         if (!ulfun_head) {
            ulfun_head = ufp;
         } else {
            for (ufp2 = ulfun_head; ufp2->next; ufp2 = ufp2->next);
            ufp2->next = ufp;
         }
         hashadd2(s_funlocal, (int *) ufp, &mudstate.ulfunc_htab,1);
      } else {
         if (!ufun_head) {
            ufun_head = ufp;
         } else {
            for (ufp2 = ufun_head; ufp2->next; ufp2 = ufp2->next);
            ufp2->next = ufp;
         }
         hashadd2(np, (int *) ufp, &mudstate.ufunc_htab,1);
      }
   }
   ufp->obj = obj;
   ufp->atr = atr;
   ufp->flags = key;
   ufp->orig_owner = Owner(obj);
   if ( i_local && (!controls(ufp->owner, ufp->obj)) ) {
      notify_quiet(player, unsafe_tprintf("Warning: SECURITY RISK -- Assigned function %s to object #%d which player #%d does not control!", ufp->name, ufp->obj, ufp->owner));
   }
   if (!Quiet(player)) {
       tprp_buff = tpr_buff = alloc_lbuf("do_function");
       if ( i_local && (*np == '#') ) {
          if ( !count && (s_buffptr = strchr(fname, '_')) ) {
             notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "%sunction %s %sefined (owned by #%d).",
                                               (i_local ? "LOCAL F":"F"), s_buffptr+1, (count ? "d" : "re-d"), ufp->owner));
          } else {
             notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "%sunction %s %sefined.",
                                               (i_local ? "LOCAL F":"F"), fname, (count ? "d" : "re-d")));
          }
       } else {
          notify_quiet(player, safe_tprintf(tpr_buff, &tprp_buff, "%sunction %s %sefined.",
                                            (i_local ? "LOCAL F":"F"), fname, (count ? "d" : "re-d")));
       }
       free_lbuf(tpr_buff);
   }
   free_sbuf(np);
}

/* ---------------------------------------------------------------------------
 * list_functable: List available functions.
 */


void list_functable2(dbref player, char *buff, char **bufcx, int key)
{
    FUN *fp;
    UFUN *ufp;
    const char *ptrs[LBUF_SIZE/2], *ptrs2[LBUF_SIZE/2], *ptrs3[LBUF_SIZE/2]; 
    char *tbuff[LBUF_SIZE/2];
    int f_int, i, j, nptrs, nptrs2, nptrs3;

    memset(tbuff, '\0', sizeof(tbuff));
    if ( !key || (key == 1) ) {
       nptrs = 0;
       for (fp = (FUN *) hash_firstentry2(&mudstate.func_htab, 1); fp;
                fp = (FUN *) hash_nextentry(&mudstate.func_htab)) {
          if (check_access(player, fp->perms, fp->perms2, 0)) {
             ptrs[nptrs] = fp->name;
             nptrs++;
          }
       }
       qsort(ptrs, nptrs, sizeof(char *), s_comp);
       f_int = 0;
       for (i = 0 ; i < nptrs ; i++) {
          if ( f_int )
             safe_chr(' ', buff, bufcx);
          f_int = 1;
          safe_str((char *) ptrs[i], buff, bufcx);
       }
    }
    if ( !key || (key == 2) ) {
       nptrs2 = 0;
       for (ufp = ufun_head; ufp; ufp = ufp->next) {
         if (check_access(player, ufp->perms, ufp->perms2, 0)) {
            ptrs2[nptrs2] = ufp->name;
            nptrs2++;
         }
       }
       qsort(ptrs2, nptrs2, sizeof(char *), s_comp);
       f_int = 0;
       for (i = 0 ; i < nptrs2 ; i++) {
         if ( f_int )
            safe_chr(' ', buff, bufcx);
         f_int = 1;
         safe_str((char *) ptrs2[i], buff, bufcx);
       }
    }
    if ( !key || (key == 4) ) {
       j = nptrs3 = 0;
       for (ufp = ulfun_head; ufp; ufp = ufp->next) {
         if (check_access(player, ufp->perms, ufp->perms2, 0)) {
            if ( ufp->owner == player ) {
               ptrs3[nptrs3] = ufp->name;
               nptrs3++;
            } else if ( controls(player, ufp->owner) ) {
               tbuff[j] = alloc_mbuf("listing functions");
               sprintf(tbuff[j], "%s[#%d]", ufp->name, ufp->owner);
               ptrs3[nptrs3] = tbuff[j];
               nptrs3++;
               j++;
            }
         }
       }
       qsort(ptrs3, nptrs3, sizeof(char *), s_comp);
       f_int = 0;
       for (i = 0 ; i < nptrs3 ; i++) {
         if ( f_int )
            safe_chr(' ', buff, bufcx);
         f_int = 1;
         safe_str((char *) ptrs3[i], buff, bufcx);
       }
       for ( i = 0; i < j; i++ ) {
          free_mbuf(tbuff[i]);
       }
    }
}

void
list_functable(dbref player)
{
    char *buf, *bp, *buf2, *bp2, *buf3, *bp3;

    bp = buf = alloc_lbuf("list_functable");
    bp2 = buf2 = alloc_lbuf("list_functable2");
    bp3 = buf3 = alloc_lbuf("list_functable3");
    safe_str((char *) "Functions : ", buf, &bp);
    list_functable2(player, buf, &bp, 1);
    notify(player, buf);
    free_lbuf(buf);
    safe_str((char *) "User-Functions : ", buf2, &bp2);
    list_functable2(player, buf2, &bp2, 2);
    notify(player, buf2);
    free_lbuf(buf2);
    safe_str((char *) "Local-Functions: ", buf3, &bp3);
    list_functable2(player, buf3, &bp3, 4);
    notify(player, buf3);
    free_lbuf(buf3);
}

/* ---------------------------------------------------------------------------
 * cf_func_access: set access on functions
 */

CF_HAND(cf_func_access)
{
    FUN *fp;
    UFUN *ufp;
    char *ap;

    for (ap = str; *ap && !isspace((int)*ap); ap++);
       if (*ap)
          *ap++ = '\0';

    for (fp = (FUN *) hash_firstentry2(&mudstate.func_htab, 1); fp;
               fp = (FUN *) hash_nextentry(&mudstate.func_htab)) {
       if (!string_compare(fp->name, str)) {
           return (cf_modify_multibits(&fp->perms, &fp->perms2, ap, extra, extra2,
                     player, cmd));
       }
    }
    for (ufp = ufun_head; ufp; ufp = ufp->next) {
       if (!string_compare(ufp->name, str)) {
           return (cf_modify_multibits(&ufp->perms, &ufp->perms2, ap, extra, extra2,
                     player, cmd));
       }
    }
    for (ufp = ulfun_head; ufp; ufp = ufp->next) {
       if (!string_compare(ufp->name, str)) {
           return (cf_modify_multibits(&ufp->perms, &ufp->perms2, ap, extra, extra2,
                     player, cmd));
       }
    }
    cf_log_notfound(player, cmd, "Function", str);
    return -1;
}

/* copyright and code borrowed from pom.c */
/*
 * Copyright (c) 1989, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software posted to USENET.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
