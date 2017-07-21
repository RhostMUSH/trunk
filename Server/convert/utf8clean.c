#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utf8helpers.h"

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

   gets(line);
   while( line != NULL && !feof(stdin) ) {
      if (line[0] == '!') {
         // Output the current line
         printf("%s\n",line);

         // Reset the extansi state
         has_extansi = 0;

         // Get the next line (name)
         gets(line);

         // Strip all UTF8 from the name, and echo it back
         strcpy(input, line);
         // TODO: Add return value to denote has color
         has_color = strip_utf8(input, clean);
         printf("%s\n", clean);

         // Store name with color codes, and update extansi toggle if needed
         if (has_color) {
            convert_markup(input, color, ansic, 0);

            for (i = 0; i < 14; i++) {
               gets(line); printf("%s\n", line);
            }
            gets(line);
            toggles2 = atoi(line);
            toggles2 |= EXTANSI_TOGGLE;
            printf("%d\n", toggles2);
         }
      } else if (line[0] == '>') {
         // If color name is saved set it and reset color flag
         // tag this object as having extansi already set
         if (has_color) {
            printf(">%d\n%s\n",EXTANSI_ATTR, color);
            has_color = 0;
            has_extansi = 1;
         }
         // We have an attribute, get the number
         attrnum = atoi(line+1);

         // Don't output anything and skip ahead if this is the 
         // extansi attr and we set one earlier
         if (attrnum == EXTANSI_ATTR && has_extansi) {
            gets(line);
            gets(line);
            continue;
         } else {
            printf("%s\n", line);
         }

         // Get the next line, the attribute content
         gets(line);

         // Get the full markup for the attr contents
         strcpy(input, line);
         convert_markup(input, full, ansic, 1);
         printf("%s\n", full);
      } else {
         printf("%s\n", line);
      }

      // Get the next line of input
      gets(line);
   }

   return 0;
}
