#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
FILE *fpin, *fpout, *fpout2;
char *s_in, s_buf[100000];
int  i_dbref;
   
   memset(s_buf, '\0', sizeof(s_buf));

   if ( argc != 3 ) {
      fprintf(stderr, "Syntax: %s <flatfile> <outfile>\n", argv[0]);
      exit(1);
   }
   if ( (fpin = fopen(argv[1], "r")) == NULL ) {
      fprintf(stderr, "Error opening flatfile for reading.\n");
      fprintf(stderr, "Syntax: %s <flatfile> <outfile>\n", argv[0]);
      exit(1);
   }
   
   if ( (fpout = fopen(argv[2], "w")) == NULL ) {
      fclose(fpin);
      fprintf(stderr, "Error opening outfile for writing.\n");
      fprintf(stderr, "Syntax: %s <flatfile> <outfile>\n", argv[0]);
      exit(1);
   }
   if ( (fpout2 = fopen("muxlock.chk", "w")) == NULL ) {
      fclose(fpin);
      fclose(fpout);
      fprintf(stderr, "Error opening outfile for writing.\n");
      fprintf(stderr, "Syntax: %s <flatfile> <outfile>\n", argv[0]);
      exit(1);
   }
   
   while ( !feof(fpin) ) {
      fgets( s_buf, 99999, fpin );
      if ( *s_buf == '!' ) {
         i_dbref = atoi(s_buf+1);
         continue;
      }
      if ( (*s_buf == '>') && (atoi(s_buf+1) == 42) ) {
         fgets( s_buf, 99999, fpin );
         fprintf(fpout, "@lock #%d=%s", i_dbref, s_buf);
         fprintf(fpout2, "%d\n", i_dbref);
         continue;
      }
   }

   fclose(fpin);
   fclose(fpout);
   fclose(fpout2);
}
