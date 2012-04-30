#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LBUF_SIZE 4000
#define MBUF_SIZE 200

int
main(int argc, char *argv[])
{
   FILE *fptr1, *fptr2, *fptr3;
   char s_buff[LBUF_SIZE+1], *s_buffptr, m_buff[MBUF_SIZE+1], *m_buffptr;
   int i_attr_chk, i;

   if ( (argc != 3) || !*argv[1] || !*argv[2] ) {
      fprintf(stderr, "%s expects 2 arguments.\nSyntax: %s <input-flatfile> <output-flatfile>\n",
              argv[0], argv[0]);
      exit(1);
   }
   if ( (fptr1 = fopen(argv[1], "r")) == NULL ) {
      fprintf(stderr, "Unable to open file %s for reading.\n", argv[1]);
      exit(1);
   }
   if ( (fptr2 = fopen(argv[2], "w")) == NULL ) {
      fclose(fptr1);
      fprintf(stderr, "Unable to open file %s for writing.\n", argv[2]);
      exit(1);
   }

   if ( (fptr3 = fopen("pulled_attribs.txt", "w")) == NULL ) {
      fclose(fptr1);
      fclose(fptr2);
      fprintf(stderr, "Unable to open attribute table pulled_attribs.txt\n");
      exit(1);
   }
   for ( i=0; i<=256; i++ )
      fprintf(fptr3, ">%d\n", i);
   fclose(fptr3);

   sprintf(s_buff, "grep \"^>[1-9]*\" %s|sort -u >> pulled_attribs.txt", argv[1]);
   system(s_buff);

   if ( (fptr3 = fopen("pulled_attribs.txt", "r")) == NULL ) {
      fclose(fptr1);
      fclose(fptr2);
      fprintf(stderr, "Unable to open attribute table pulled_attribs.txt\n");
      exit(1);
   }

   memset(m_buff, '\0', sizeof(m_buff)); 
   memset(s_buff, '\0', sizeof(s_buff)); 

   i_attr_chk = 1;
   while ( !feof(fptr1) ) {
      fgets(s_buff, LBUF_SIZE, fptr1);
      if ( (i_attr_chk < 2) && (s_buff[0] == '+') && (s_buff[1] == 'A') ) {
         rewind(fptr3);
         i_attr_chk = 0;
         while ( !feof(fptr3) ) {
            fgets(m_buff, MBUF_SIZE, fptr3);
            if ( strcmp(s_buff+2, m_buff+1) == 0 ) {
               i_attr_chk = 1;
               break;
            }
         }
      }
      if ( s_buff[0] == '!' )
         i_attr_chk = 2;

      if ( i_attr_chk && !feof(fptr1)) {
         fprintf(fptr2, "%s", s_buff);
      }
   }
   fclose(fptr3);
   fclose(fptr2);
   fclose(fptr1);
   return(0);
}
