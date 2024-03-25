#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utf8helpers.h"

#define mush_gets(x)    fgets(x, BUFFER_SIZE - 1, stdin)

int main(int argc, char* argv[])
{
   char line[BUFFER_SIZE], input[BUFFER_SIZE];
   char full[BUFFER_SIZE], color[BUFFER_SIZE], clean[BUFFER_SIZE];
   char ansic, *arg;
   int i, obj, attrnum, has_color, has_extansi, toggles2;

   memset(line, '\0', sizeof(line));
   memset(input, '\0', sizeof(input));
   memset(clean, '\0', sizeof(clean));
   memset(color, '\0', sizeof(color));
   memset(full, '\0', sizeof(full));

   while (*argv) {
      arg = *argv;

      if (strcmp(arg, "-x") == 0) {
         ansic = 'x';
      } else if (strcmp(arg, "-m") == 0) {
         ansic = 'm';
      } else {
         ansic = 'c';
      }

      *argv++;
   }

   mush_gets(line);
   while( line != NULL && !feof(stdin) ) {
      if (line[0] == '!') {
         // Output the current line
         printf("%s",line);

         // Reset the extansi state
         has_extansi = 0;

         // Get the next line (name)
         mush_gets(line);

         // Strip all UTF8 from the name, and echo it back
         strcpy(input, line);
         // TODO: Add return value to denote has color
         has_color = strip_utf8(input, clean);
         printf("%s", clean);

         // Store name with color codes, and update extansi toggle if needed
         if (has_color) {
            convert_markup(input, color, ansic, 0);

            for (i = 0; i < 14; i++) {
               mush_gets(line); printf("%s", line);
            }
            mush_gets(line);
            toggles2 = atoi(line);
            toggles2 |= EXTANSI_TOGGLE;
            printf("%d\n", toggles2);
         }
      } else if (line[0] == '>') {
         // If color name is saved set it and reset color flag
         // tag this object as having extansi already set
         if (has_color) {
            printf(">%d\n%s",EXTANSI_ATTR, color);
            has_color = 0;
            has_extansi = 1;
         }
         // We have an attribute, get the number
         attrnum = atoi(line+1);

         // Don't output anything and skip ahead if this is the 
         // extansi attr and we set one earlier
         if (attrnum == EXTANSI_ATTR && has_extansi) {
            mush_gets(line);
            mush_gets(line);
            continue;
         } else {
            printf("%s", line);
         }

         // Get the next line, the attribute content
         mush_gets(line);

         // Get the full markup for the attr contents
         strcpy(input, line);
         convert_markup(input, full, ansic, 1);
         printf("%s", full);
      } else {
         printf("%s", line);
      }

      // Get the next line of input
      mush_gets(line);
   }

   return 0;
}
