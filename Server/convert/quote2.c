/* Coded by Ashen-Shugar (c) 1999, all rights reserved
 * This program strips quotes that MUX uses for attribute definition,
 * combines attributes that MUX tends to break up, and strips dual '//' 
 * that MUX tends to add as well.  How lame.
 */
#include <stdio.h>
#include <stdlib.h>

main(int argc, char **argv)
{
   FILE *fp_in  = NULL,
        *fp_out = NULL;
   char s_in[20000],
        s_out[20000],
        *sp_in = NULL, *sp_out = NULL;
   int in_attr = 0,
       i_indata = 0,
       i_inlock = 0,
       i_brkcnt = 0,
       i_isbetween = 1,
       i_inescape = 0;

   if ( argc < 3 ) {
      printf("syntax: %s <filein> <fileout>\n", argv[0]);
      exit(1);
   }
   if ( (fp_in = fopen(argv[1], "r")) == NULL ) {
      printf("Error opening file %s for reading.\n", argv[1]);
      exit(1);
   }
   if ( (fp_out = fopen(argv[2], "w")) == NULL ) {
      printf("Error opening file %s for writing.\n", argv[2]);
      fclose(fp_in);
      exit(1);
   }
   while ( !feof(fp_in) ) {
      fflush(fp_out);
      memset(s_in, '\0', sizeof(s_in));
      memset(s_out, '\0', sizeof(s_out));
      fgets(s_in, 19999, fp_in);
      if ( !i_isbetween && s_in[0] == '\n' ) {
         fprintf(fp_out, "%r");
         continue;
      }
      if ( in_attr && s_in[0] == '"' ) {
         i_isbetween = 0;
         sp_in = s_in;
         sp_out = s_out;
         sp_in++;
         if ( *sp_in == '\n' ) {
            *sp_out = ' ';
            sp_out++;
            *sp_out = *sp_in;
            sp_out++;
            sp_in++;
            i_isbetween = 1;
         }
         while ( *sp_in ) {
            if ( *sp_in == '\\' && !i_inescape ) {
               i_inescape = 1;
               sp_in++;
            }
            if ( !i_inescape && *sp_in == '"' && (*(sp_in+1) == '\n') ) {
               i_isbetween = 1;
               sp_in++;
            }
            if ( i_inescape ) {
               i_inescape = 0;
            }
            if ( !i_isbetween && *sp_in == '\n' ) {
               sp_in++;
               *sp_out = '%';
               sp_out++;
               *sp_out = 'r';
               sp_out++;
               continue;
            }
            if ( i_isbetween && *(sp_in-2) == '\015' && *(sp_in) == '\n' ) {
               *sp_out = '%';
               sp_out++;
               *sp_out = 'r';
               sp_out++;
/*             *sp_out = ' ';
               sp_out++; */
            }
            *sp_out = *sp_in;
            sp_in++;
            sp_out++;
         }
         *sp_out = '\0';
         fputs(s_out, fp_out);
      } else if ( !i_isbetween ) {
         sp_in = s_in;
         sp_out = s_out;
         while ( *sp_in ) {
            if ( *sp_in == '\\' && !i_inescape ) {
               i_inescape = 1;
               sp_in++;
            }
            if ( !i_inescape && *sp_in == '"' && (*(sp_in+1) == '\n') ) {
               i_isbetween = 1;
               sp_in++;
            }
            if ( i_inescape ) {
               i_inescape = 0;
            }
            if ( !i_isbetween && *sp_in == '\n' ) {
               *sp_out = '%';
               sp_out++;
               *sp_out = 'r';
               sp_out++;
               sp_in++;
               continue;
            }
            if ( i_isbetween && !(*(sp_in-1) == s_in[0]) && *(sp_in-2) == '\015' && *(sp_in) == '\n' ) {
               *sp_out = '%';
               sp_out++;
               *sp_out = 'r';
               sp_out++;
/*             *sp_out = ' ';
               sp_out++; */
            }
            *sp_out = *sp_in;
            sp_in++;
            sp_out++;
         }
         *sp_out = '\0';
         fputs(s_out, fp_out);
      } else if ( i_indata && s_in[0] == '(' ) {
         sp_in = s_in;
         sp_out = s_out;
         i_inlock = 1;
         while ( *sp_in ) {
            if ( *sp_in == '\\' && !i_inescape ) {
               i_inescape = 1;
               sp_in++;
               continue;
            }
            if ( i_inescape ) {
               i_inescape = 0;
            }
            if ( *sp_in == '\n' || *sp_in == '\r' ) {
               sp_in++;
               continue;
            }
            if ( *sp_in == '(' )
               i_brkcnt++;
            if ( *sp_in == ')' )
               i_brkcnt--;
            *sp_out = *sp_in;
            sp_in++;
            sp_out++;
         }
         *sp_out = '\0';
         fputs(s_out, fp_out);
         if ( !i_brkcnt ) {
            i_inlock = 0;
            fputs((char *)"\n", fp_out);
         }
      } else if ( i_inlock ) {
         sp_in = s_in;
         sp_out = s_out;
         while ( *sp_in ) {
            if ( *sp_in == '\\' && !i_inescape ) {
               i_inescape = 1;
               sp_in++;
               continue;
            }
            if ( i_inescape ) {
               i_inescape = 0;
            }
            if ( *sp_in == '\n' || *sp_in == '\r' ) {
               sp_in++;
               continue;
            }
            if ( *sp_in == '(' )
               i_brkcnt++;
            if ( *sp_in == ')' )
               i_brkcnt--;
            *sp_out = *sp_in;
            sp_in++;
            sp_out++;
         }
         *sp_out = '\0';
         fputs(s_out, fp_out);
         if ( !i_brkcnt ) {
            i_inlock = 0;
            fputs((char *)"\n", fp_out);
         }
      } else {
         fputs(s_in, fp_out);
      }
      if ( (s_in[0] == '+') && (s_in[1] == 'A') )
         in_attr = 1;
      else if ( s_in[0] == '>' && isdigit(s_in[1]) && i_isbetween ) {
         in_attr = 1;
         i_indata = 0;
      } else if ( s_in[0] == '!' && isdigit(s_in[1]) ) {
         in_attr = 1;
         i_indata = 1;
      } else if ( !i_isbetween ) {
         in_attr = 1;
      } else if ( s_in[0] == '<' && (s_in[1] == '\n' || s_in[1] == '\r')) {
         in_attr = 0;
         i_indata = 0;
      } else
         in_attr = 0;
   }
   if ( fp_in )
      fclose(fp_in);
   if ( fp_out )
      fclose(fp_out);
   return(0);
}
