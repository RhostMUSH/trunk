#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utf8helpers.h"

char ansi_mods[5] = {'n', 'h', 'u', 'f', 'i'};
char *ansi_base_fg[16] = { "x",  "r",  "g",  "y",  "b",  "m",  "c",  "w",
                           "hx", "hr", "hg", "hy", "hb", "hm", "hc", "hw" };
char ansi_base_bg[16] = { 'X', 'R', 'G', 'Y', 'B', 'M', 'C', 'W' };


int utf8_chk(char *ptr) {
   if ( (*ptr && IS_4BYTE(*ptr)) && (*(ptr+1) && IS_CBYTE(*(ptr+1)))
        && (*(ptr+2) && IS_CBYTE(*(ptr+2))) && (*(ptr+3) && IS_CBYTE(*(ptr+3))) )
      return 4;

   if ( (*ptr && IS_3BYTE(*ptr)) && (*(ptr+1) && IS_CBYTE(*(ptr+1)))
        && (*(ptr+2) && IS_CBYTE(*(ptr+2))) )
      return 3;

   if ( (*ptr && IS_2BYTE(*ptr)) && (*(ptr+1) && IS_CBYTE(*(ptr+1))) )
      return 2;

   return 0;
}

int utf8toucp(char *utf)
{
   char *ucp, *ptr, tmp[3];
   int i_b1, i_b2, i_b3, i_b4, i_bytecnt, i_ucp;

   ucp = (char*)malloc(12);
   memset(ucp, '\0', sizeof(ucp));
   memset(tmp, '\0', sizeof(tmp));

   i_bytecnt = strlen(utf) / 2;

   // Convert UTF8 bytes to unicode code point
   if (i_bytecnt == 1) {
      return atoi(utf);
   } else if (i_bytecnt == 2) {
      strncpy(tmp, utf, 2);
      i_b1 = strtol(tmp, &ptr, 16);
      strncpy(tmp, utf+2, 2);
      i_b2 = strtol(tmp, &ptr, 16);
      i_ucp = ((i_b1 - 192) * 64) + (i_b2 - 128);
      sprintf(ucp, "%04X", i_ucp);
   } else if (i_bytecnt == 3) {
      strncpy(tmp, utf, 2);
      i_b1 = strtol(tmp, &ptr, 16);
      strncpy(tmp, utf+2, 2);
      i_b2 = strtol(tmp, &ptr, 16);
      strncpy(tmp, utf+4, 2);
      i_b3 = strtol(tmp, &ptr, 16);
      i_ucp = ((i_b1 - 224) * 4096) + ((i_b2 - 128) * 64) + (i_b3 - 128);
      sprintf(ucp, "%04X", i_ucp);
   } else if (i_bytecnt == 4) {
      strncpy(tmp, utf, 2);
      i_b1 = strtol(tmp, &ptr, 16);
      strncpy(tmp, utf+2, 2);
      i_b2 = strtol(tmp, &ptr, 16);
      strncpy(tmp, utf+4, 2);
      i_b3 = strtol(tmp, &ptr, 16);
      strncpy(tmp, utf+6, 2);
      i_b4 = strtol(tmp, &ptr, 16);
      i_ucp = ((i_b1 =240) * 262114) + ((i_b2 - 128) * 4096) + ((i_b3 - 128) * 64) + (i_b4 - 128);
      sprintf(ucp, "%04X", i_ucp);
   } else {
      sprintf(ucp, " ");
   }

   // Free allocated memory
   free(ucp);

   return i_ucp;
}

int is_color(int code) {
   if (code >= ANSI_MODS_LOW && code <= ANSI_MODS_HIGH) {
      return ANSI_MOD;
   } else if (code >= ANSI_BFG_LOW && code <= ANSI_BFG_HIGH) {
      return ANSI_BASE_FG;
   } else if (code >= ANSI_XFG_LOW && code <= ANSI_XFG_HIGH) {
      return ANSI_XTERM_FG;
   } else if (code >= ANSI_BBG_LOW && code <= ANSI_BBG_HIGH) {
      return ANSI_BASE_BG;
   } else if (code >= ANSI_XBG_LOW && code <= ANSI_XBG_HIGH) {
      return ANSI_XTERM_BG;
   } else if (code >= ANSI_ADJ_LOW && code <= ANSI_ADJ_HIGH) {
      return ANSI_ADJ;
   }

   return 0;
}

void get_color_markup(int code, char ansic, char *output) {
   int offset, color_type;

   memset(output, '\0', sizeof(output));

   color_type = is_color(code);
   switch (color_type) {
      case ANSI_MOD:
         offset = code - ANSI_MODS_LOW;
         sprintf(output, "%c%c%c", '%', ansic, ansi_mods[offset]);
         break;
      case ANSI_BASE_FG:
         offset = code - ANSI_BFG_LOW;
         sprintf(output, "%c%c%s", '%', ansic, ansi_base_fg[offset]);
         break;
      case ANSI_XTERM_FG:
         offset = code - ANSI_BFG_LOW;
         sprintf(output, "%c%c0x%02X", '%', ansic, offset);
         break;
      case ANSI_BASE_BG:
         offset = code - ANSI_BBG_LOW;
         sprintf(output, "%c%c%c", '%', ansic, ansi_base_bg[offset]);
         break;
      case ANSI_XTERM_BG:
         offset = code - ANSI_BBG_LOW;
         sprintf(output, "%c%c0X%02X", '%', ansic, offset);
         break;
   }
}

void get_full_markup(int code, char ansic, char *output) {
   if (is_color(code)) {
      get_color_markup(code, ansic, output);
   } else {
      memset(output, '\0', sizeof(output));
      sprintf(output, "%c<u%04X>", '%', code);
   }
}

int strip_utf8(char *input, char *output) {
   char tmpbuf[5], utfbuf[15];
   char *inptr, *outptr, *tmpptr, *utfptr;
   int i, utf8bytes, ucp, has_color = 0;

   memset(utfbuf, '\0', sizeof(utfbuf));
   memset(tmpbuf, '\0', sizeof(tmpbuf));

   inptr = input;
   outptr = output;
   while (inptr && *inptr) {
      if (utf8bytes = utf8_chk(inptr)) {
         if (!has_color) {
            utfptr = utfbuf;
            for (i = 0; i < utf8bytes; i++) {
               sprintf(tmpbuf,"%02X",(int)(unsigned char)*inptr++);
               *utfptr++ = tmpbuf[0];
               *utfptr++ = tmpbuf[1];
            }
            *utfptr++ = '\0';
            ucp = utf8toucp(utfbuf);
            has_color = is_color(ucp);
         } else {
            inptr+=utf8bytes;
         }
      } else {
         *outptr++ = *inptr++;
      }
   }

   // Null terminate the string
   *outptr++ = '\0';

   return has_color;
}

void convert_markup(char *input, char *output, char ansic, int full) {
   char tmpbuf[5], utfbuf[15], *colorbuf, *fullbuf;
   char *inptr, *outptr, *utfptr, *colorptr, *fullptr;
   int ucp, utf8bytes, i;
   
   colorbuf = malloc(12);
   fullbuf = malloc(18);
   memset(colorbuf, '\0', sizeof(colorbuf));
   memset(fullbuf, '\0', sizeof(fullbuf));
   memset(tmpbuf, '\0', sizeof(tmpbuf));
   memset(utfbuf, '\0', sizeof(utfbuf));

   inptr = input;
   outptr = output;
   while (inptr && *inptr) {
      if (utf8bytes = utf8_chk(inptr)) {
         utfptr = utfbuf;
         for (i = 0; i < utf8bytes; i++) {
            sprintf(tmpbuf,"%02X",(int)(unsigned char)*inptr++);
            *utfptr++ = tmpbuf[0];
            *utfptr++ = tmpbuf[1];
         }
         *utfptr++ = '\0';
         ucp = utf8toucp(utfbuf);

         if (full) {
            get_full_markup(ucp, ansic, fullbuf);
            fullptr = fullbuf;
            while (fullptr && *fullptr) {
               *outptr++ = *fullptr++;
            }
         } else {
            get_color_markup(ucp, ansic, colorbuf);
            colorptr = colorbuf;
            while (colorptr && *colorptr) {
               *outptr++ = *colorptr++;
            }
         }
      } else {
         *outptr++ = *inptr++;
      }

      if ( (int)outptr - (int)output > (BUFFER_SIZE - 20) ) {
         break;
      }
   }

   // Free allocated memory
   free(colorbuf);
   free(fullbuf);

   // null terminate the string
   *outptr++ = '\0';
}
