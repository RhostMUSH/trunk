/* Original Code provided by Zenty (c) 1999, All rights reserved
 * Modified by Ashen-Shugar for attribute allignment and functionary locks
 * (c) 1999, All rights reserved
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* 1st word */
#define IMMORTAL  0x00200000
/* 2nd word */
#define MUXAUDIT  0x00000100
#define MUXANSI   0x00000200
#define MUXFIXED  0x00000800
#define MUXNOCMD  0x00002000
#define MUXKPALV  0x00004000
#define MUXBLIND  0x08000000
#define RHOANSI   0x02000000
#define RHOANSICO 0x04000000
/* 3rd word */
#define MUXCMDCHK 0x00000800
#define MUXMARK0  0x00400000
#define MUXMARK1  0x00800000
#define MUXMARK2  0x01000000
#define MUXMARK3  0x02000000
#define MUXMARK4  0x04000000
#define MUXMARK5  0x08000000
#define MUXMARK6  0x10000000
#define MUXMARK7  0x20000000
#define MUXMARK8  0x40000000
#define MUXMARK9  0x80000000
#define RHOCMDCHK 0x00000200
#define RHONOCMD  0x00008000
#define RHOAUDIT  0x00010000
#define RHOFIXED  0x00080000

/* 4th word */
#define RHOMARK0  0x00000010
#define RHOMARK1  0x00000020
#define RHOMARK2  0x00000040
#define RHOMARK3  0x00000080
#define RHOMARK4  0x00000100
#define RHOMARK5  0x00000200
#define RHOMARK6  0x00000400
#define RHOMARK7  0x00000800
#define RHOMARK8  0x00001000
#define RHOMARK9  0x00002000
#define RHOBLIND  0x00100000

/* 2nd word toggle */
#define RHOKPALV  0x00010000

#define LBUF_SIZE 65535
#define mush_gets(x)	fgets(x, LBUF_SIZE - 1, stdin)

char s_out[LBUF_SIZE];

char *
juggle_attrib(char *s_in)
{
   int val;
   char *s_base, *s_ptr, *s_owner, *s_flags, *s_string;

   s_flags = '\0';
   s_string = '\0';
   memset(s_out, '\0', LBUF_SIZE);
   if ( s_in && *s_in ) {
      if ( *s_in == '' ) {
         /* format ^Aowner:flags:string */
         s_base = s_in;
         s_owner++; /* to owner */
         s_flags = strchr(s_base, ':'); /* to owner */
         if ( *s_flags ) {
            s_string = strchr(s_flags+1, ':'); /* String */
         }
         /* Set nulls */
         if ( *s_flags ) {
            *s_flags = '\0';
            s_flags++;
         }
         if ( *s_string ) {
            *s_string = '\0';
            s_string++;
         }

         val = 0;
         if ( *s_flags )
            val = atoi(s_flags);
        
         /* Strip unneeded */
         val &= ~0x10ce2000;

         /* Let's now do flag conversion */
         if ( val & 0x00000400 ) { /* is_lock */
            val &= ~0x00000400;
            val |=  0x00001000;
         }
         if ( val & 0x00000800 ) { /* Visual */
            val &= ~0x00000800;
            val |=  0x00020000;
         }
         if ( val & 0x00001000 ) { /* Private */
            val &= ~0x00001000;
            val |=  0x00008000;
         }
         if ( val & 0x00004000 ) { /* NoParse */
            val &= ~0x00004000;
            val |=  0x00400000;
         }
         if ( val & 0x00008000 ) { /* RegExp  */
            val &= ~0x00008000;
            val |=  0x20000000;
         }
         if ( val & 0x00010000 ) { /* NoClone */
           val &= ~0x00010000;
           val |=  0x00200000;
         }
         sprintf(s_out, "%s:%d:%s", s_base, val, s_string);
      } else {
         strcpy(s_out, s_in);
      }
   }
   return(s_out);
}

char *
juggle_attrib2(char *s_in, int isexit)  {
   char *snip = NULL;

   if ( isexit ) {
      snip = strchr(s_in, ';');
      if ( snip ) {
         *snip = '\0';
      }
   }
   return(s_in);
}


int main(void) {
   unsigned long flag1, flag2, flag3, nflag1, nflag2, nflag3, nflag4;
   int val, tog2, obj, mage, royalty, staff, ansi, immortal, atrcnt, i_dbref, i_isexit;
   char f[LBUF_SIZE], *q, *f1, *f2, *t, fstr[60];
   FILE *fpin;

   memset(f,'\0', sizeof(f));
   memset(fstr, '\0', sizeof(fstr));
   q = f;
   f1 = f+1;
   f2 = f+2;
   atrcnt = 0;

   if ( (fpin = fopen("muxlock.chk", "r")) == NULL ) {
      fprintf(stderr, "ERROR: Unable to open the MUX lock check muxlock.chk file.\n");
      exit(1);
   }
   if ( !feof(fpin) ) {
      fgets(fstr, 59, fpin);
      i_dbref = atoi(fstr);
   } else {
      i_dbref = -1;
   }
   mush_gets(q);
   while(q != NULL && !feof(stdin) ) {
      if(f[0] == '!') {
         if ( atrcnt > 750 ) {
            fprintf(stderr, "Notice: Object #%d has more than 750 attributes (%d total).\n", obj, atrcnt);
         }
         obj=atoi(f1);
         atrcnt = 0;
         /* object conversion */
         printf("%s",q);          /* Object ID */
         mush_gets(q); printf("%s",q); /* name */
         mush_gets(q); printf("%s",q); /* location */
         mush_gets(q);                   /* zone */
         mush_gets(q); printf("%s",q); /* Contents */
         mush_gets(q); printf("%s",q); /* Exits */
         mush_gets(q); printf("%s",q); /* Link */
         mush_gets(q); printf("%s",q); /* Next */
         if ( i_dbref == obj ) {
            printf("%d\n", obj);/* UNLock -- Mux removed LOCK from structure, rhost needs it populated */
            if ( !feof(fpin) ) {
               fgets(fstr, 59, fpin);
               i_dbref = atoi(fstr);
            } 
         } else {
            printf("%s\n", (char *)"");/* UNLock -- Mux removed LOCK from structure, rhost needs it populated */
         }
         mush_gets(q); printf("%s",q); /* Owner */
         mush_gets(q); printf("%s",q); /* Parent */
         mush_gets(q); printf("%s",q); /* Pennies */

         /* flag conv */
         mush_gets(q); flag1 = atol(q);  /* Flag Word 1 */
         mush_gets(q); flag2 = atol(q);  /* Flag Word 2 */
         mush_gets(q); flag3 = atol(q);  /* Flag Word 3 */
         nflag1 = (flag1 & 0xDFDFFFFF); /* Convert/Strip Flag 1 */
         nflag2 = (flag2 & 0xD00000FF); /* Convert/Strip Flag 2 */
         nflag3 = nflag4 = tog2 = 0;    /* Nullify Flag 3 , initialize flag 4 */
         i_isexit = 0;
         if ( nflag1 & 0x2 ) {
            i_isexit = 1;
         }
         if ( flag2 & MUXANSI )
            nflag2 = (nflag2 | 0x06000000);
         if ( flag2 & MUXAUDIT )
            nflag3 = (nflag3 | RHOAUDIT);
         if ( flag2 & MUXFIXED )
            nflag3 = (nflag3 | RHOFIXED);
         if ( flag2 & MUXNOCMD )
            nflag3 = (nflag3 | RHONOCMD);
         if ( flag2 & MUXKPALV )
            tog2 = (tog2 | RHOKPALV);
         if ( flag3 & MUXCMDCHK )
            nflag3 = (nflag3 | RHOCMDCHK);
         if ( flag2 & MUXBLIND )
            nflag4 = (nflag4 | RHOBLIND);
         if ( flag3 & MUXMARK0 )
            nflag4 = (nflag4 | RHOMARK0);
         if ( flag3 & MUXMARK1 )
            nflag4 = (nflag4 | RHOMARK1);
         if ( flag3 & MUXMARK2 )
            nflag4 = (nflag4 | RHOMARK2);
         if ( flag3 & MUXMARK3 )
            nflag4 = (nflag4 | RHOMARK3);
         if ( flag3 & MUXMARK4 )
            nflag4 = (nflag4 | RHOMARK4);
         if ( flag3 & MUXMARK5 )
            nflag4 = (nflag4 | RHOMARK5);
         if ( flag3 & MUXMARK6 )
            nflag4 = (nflag4 | RHOMARK6);
         if ( flag3 & MUXMARK7 )
            nflag4 = (nflag4 | RHOMARK7);
         if ( flag3 & MUXMARK8 )
            nflag4 = (nflag4 | RHOMARK8);
         if ( flag3 & MUXMARK9 )
            nflag4 = (nflag4 | RHOMARK9);

         if ((obj == 1) && !(nflag1 & IMMORTAL))
            nflag1 |= IMMORTAL; /* This should not happen. */
         printf("%d\n",nflag1); /* flags1 */
         printf("%d\n",nflag2); /* flags2 */
         printf("%d\n",nflag3); /* flags3 */
         fflush(stdout);
         mush_gets(q); /* power 1 */
         mush_gets(q); /* power 2 */
         printf("%d\n",nflag4); /* Flags4 */
         printf("0\n"); /* toggles1 */
         printf("%d\n", tog2); /* toggles2 */
         printf("0\n"); /* powers1 */
         printf("0\n"); /* powers2 */
         printf("0\n"); /* powers3 */
         printf("0\n"); /* depowers1 */
         printf("0\n"); /* depowers2 */
         printf("0\n"); /* depowers3 */
         printf("-1\n"); /* Zone(s) */
         fflush(stdout);
         mush_gets(q);
      } else if(f[0] == '+') {
         if ((f[1] == 'A') || (f[1] == 'N')) {
            val = atoi(f2);
            if (val > 255)
               val = val + 256;
            printf("+%c%d\n",f[1],val);
            fflush(stdout);
            if ( f[1] == 'N' ) {
               mush_gets(q);
            } else {
               mush_gets(q);

               /* this will be attribute contents -- wig it -- first char is a '"' */
               val = atoi(q); /* Grab the flags */
               t = strchr(q, ':'); /* grab the rest */
               /* Let's mangle the flags -- first strip out not needed */
               val &= ~0x10ce2000;
               /* Let's now do flag conversion */
               if ( val & 0x00000400 ) { /* is_lock */
                  val &= ~0x00000400;
                  val |=  0x00001000;
               }
               if ( val & 0x00000800 ) { /* Visual */
                  val &= ~0x00000800;
                  val |=  0x00020000;
               }
               if ( val & 0x00001000 ) { /* Private */
                  val &= ~0x00001000;
                  val |=  0x00008000;
               }
               if ( val & 0x00004000 ) { /* NoParse */
                  val &= ~0x00004000;
                  val |=  0x00400000;
               }
               if ( val & 0x00008000 ) { /* RegExp  */
                  val &= ~0x00008000;
                  val |=  0x20000000;
               }
               if ( val & 0x00010000 ) { /* NoClone */
                  val &= ~0x00010000;
                  val |=  0x00200000;
               }
               printf("%d%s", val, t);
               fflush(stdout);
               mush_gets(q);
            }
         } else if(f[1] == 'X' || f[1] == 'T') {
            printf("+V74247\n",val);
            fflush(stdout);
            mush_gets(q);
         } else {
            printf("%s",q);
            fflush(stdout);
            mush_gets(q);
         }
      } else if(f[0] == '>') {
         atrcnt++;
         val = atoi(f1);
         if (val > 255)
            val = val + 256;
         /* attr conversion */
         if( (val == 5) && (obj == 1) ) { /* Password */
            printf(">5\n");
            mush_gets(q); /* read password and toss away */
            if ( strstr(q, "$SHA1$") != NULL ) {
               fprintf(stderr, "Warning: #1's password in SHA1 encryption.  Reset to 'Nyctasia' using crypt.\n");
               printf("XXXXFe7xx3Zo2\n"); /* set password for #1 to Nyctasia */
            } else {
               printf("%s",juggle_attrib(q)); /* Just store the password as is */
            }
            mush_gets(q);
         } else if (val == 42 ) { /* Lock attribute -- handled by a separate routine */
            /* Eat it */
            mush_gets(q);
            mush_gets(q);
         } else if ((val >= 129) && (val <= 142)) { /* These are 1 to 1 but a 30 offset between MUX and Rhost */
            printf(">%d\n",val+30);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 127) { /* Getlock */
            printf(">%d\n",235);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 143) { /* Teleport from fail */
            printf(">%d\n",174);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 144) { /* LastIP */
            printf(">%d\n",246);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 200) { /* Last Paged */
            printf(">%d\n",177);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 203) { /* Mail signature */
            printf(">%d\n",193);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 209) { /* Speech Lock */
            printf(">%d\n",199);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 213) { /* Create Array -- holds last created dbref#s */
            printf(">%d\n",237);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 218) { /* Create time */
            printf(">%d\n",228);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 219) { /* Modify time */
            printf(">%d\n",227);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 224) { /* ConnInfo */
            printf(">%d\n",2100000000); /* Extended inaternal attributes */
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 225) { /* Mail Lock */
            printf(">%d\n",157);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 241) { /* Exit Format */
            printf(">%d\n",225);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 242) { /* Content Format */
            printf(">%d\n",224);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 243) { /* Name Format */
            printf(">%d\n",245);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 199) { /* Moniker/Ansiname */
            printf(">%d\n",220);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib2(q, i_isexit));
            mush_gets(q);
         } else if (val == 214) { /* Saystring */
            printf(">%d\n",236);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 216) { /* @exitto */
            printf(">%d\n",248);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 217) { /* Chown Lock */
            printf(">%d\n",218);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 226) { /* Open Lock */
            printf(">%d\n",217);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if (val == 128) { /* Mail Fail/Reject */
            printf(">%d\n",2100000002);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         } else if ( (val == 96) || ((val > 199) && (val < 252)) ||
                     ((val >= 145) && (val <= 148)) ) { /* Eat Remaining and report */
            mush_gets(q);
            fprintf(stderr, "Warning: Object #%d has attribute (%d) unused by RhostMUSH.  Contents: %s\n", obj, val, q);
            mush_gets(q);
         } else  {  /* Write out user-defined (> 256) attribute */
            printf(">%d\n",val);
            fflush(stdout);
            mush_gets(q);
            if ( strlen(q) > LBUF_SIZE )
               fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
            printf("%s",juggle_attrib(q));
            mush_gets(q);
         }
      } else if(f[0] == '-') {
         mush_gets(q);
      } else {
         printf("%s",q);
         fflush(stdout);
         mush_gets(q);
      }
		
      if (strstr(f, "***END OF DUMP***") != NULL ) {
         printf("***END OF DUMP***\n");
         fflush(stdout);
         q = NULL;
         fclose(fpin);
         return(0);
      }
   } /* while */
   fclose(fpin);
   return(0);	
}
