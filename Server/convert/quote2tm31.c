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
   char ch_in   = '\0',
        lstchr  = '\0';
   int esc_char = 0,
       hard_return = 0,
       obj_trans=0,
       chk_lock=0,
       do_pass=0,
       chk_return=0,
       i_markup = 0,
       st_attr  = 0;

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
      ch_in = getc(fp_in);
      if ( ch_in == '\\' )
         esc_char++;
      if ( ch_in == '"' && !(esc_char % 2) ) {
	 st_attr = (st_attr ? 0 : 1);
         if ( !st_attr && chk_return )
            putc(' ', fp_out);
         continue;
      }
      if ( ch_in == '\\' && (esc_char % 2) && do_pass ) {
         do_pass = 0;
         if ( i_markup )
	    fprintf(fp_out, "\r\n");
         i_markup = 0;
	 continue;
      } else
         do_pass = 1; 
/*
      if ( ch_in == '\\' )
         esc_char--;
*/
      if ( ch_in == '(' && !st_attr )
	      chk_lock++;
      if ( ch_in == ')' && !st_attr )
	      chk_lock--;
      if ( !st_attr && chk_lock && (ch_in == '\n' || ch_in == '\r') ) {
	      continue;
      }
      if ( st_attr && chk_lock )
	      chk_lock = 0;
      if ( obj_trans == 2 && !(ch_in == '\n' || ch_in == '\r') )
	      obj_trans = 0;
      if ( st_attr && ch_in == '\0') {
	 continue; 
      }
      if ( st_attr && ch_in == '\013' )  {
	 hard_return = 1;
         chk_return=1;
      } else if ( !hard_return && st_attr && (ch_in == '\n' || ch_in == '\r') ) {
         putc(ch_in, fp_out);
         i_markup = 1;
	 continue;
      } else if ( hard_return ) {
	 fprintf(fp_out, "%s", "\015 ");
	 hard_return = 0;
         lstchr = ch_in;
	 continue;
      }
      i_markup = 0;
      if ( !feof(fp_in) )
         putc(ch_in, fp_out);
      hard_return = 0;
      if ( ch_in != '\013' )
         chk_return = 0;
      if ( ch_in != '\\' )
         esc_char = 0;
      lstchr = ch_in;
   }
   if ( fp_in )
      fclose(fp_in);
   if ( fp_out )
      fclose(fp_out);
   return(0);
}
