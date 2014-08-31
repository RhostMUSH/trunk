/* Original Code provided by Zenty (c) 1999, All rights reserved
 * Modified by Ashen-Shugar for attribute allignment and functionary locks
 * (c) 1999, All rights reserved
 */
#include <stdio.h>
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

#define LBUF_SIZE 3998

int main(void) {
	int val, flag1, flag2, flag3, nflag1, nflag2, nflag3, nflag4, tog2,obj;
	int mage, royalty, staff, ansi, immortal, atrcnt;
	char f[16384], *q, *f1, *f2;
	memset(f,'\0', sizeof(f));
	q = f;
	f1 = f+1;
	f2 = f+2;
	atrcnt = 0;
	
	gets(q);
	while(q != NULL && !feof(stdin) ) {
		if(f[0] == '!') {
                        if ( atrcnt > 750 ) {
                           fprintf(stderr, "Notice: Object #%d has more than 750 attributes (%d total).\n", obj, atrcnt);
                        }
			obj=atoi(f1);
                        atrcnt = 0;
			/* object conversion */
			printf("%s\n",q);          /* Object ID */
			gets(q); printf("%s\n",q); /* name */
			gets(q); printf("%s\n",q); /* location */
  			gets(q);                   /* zone */
			gets(q); printf("%s\n",q); /* Contents */
			gets(q); printf("%s\n",q); /* Exits */
			gets(q); printf("%s\n",q); /* Link */
			gets(q); printf("%s\n",q); /* Next */
                        gets(q); printf("%s\n",q); /* Owner */
			gets(q); printf("%s\n",q); /* Parent */
			gets(q); printf("%s\n",q); /* Pennies */
			/* flag conv */
			gets(q); flag1 = atoi(q);  /* Flag Word 1 */
			gets(q); flag2 = atoi(q);  /* Flag Word 2 */
			gets(q); flag3 = atoi(q);  /* Flag Word 3 */
			nflag1 = (flag1 & 0xDFDFFFFF); /* Convert/Strip Flag 1 */
                        nflag2 = (flag2 & 0xD00000FF); /* Convert/Strip Flag 2 */
			nflag3 = nflag4 = tog2 = 0;    /* Nullify Flag 3 , initialize flag 4 */
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

			if((obj == 1) && !(nflag1 & IMMORTAL))
			  nflag1 |= IMMORTAL; /* This should not happen. */
			printf("%u\n",nflag1); /* flags1 */
			printf("%u\n",nflag2); /* flags2 */
			printf("%u\n",nflag3); /* flags3 */
			fflush(stdout);
			gets(q); /* power 1 */
			gets(q); /* power 2 */
			printf("%u\n",nflag4); /* Flags4 */
			printf("0\n"); /* toggles1 */
                        printf("%u\n", tog2); /* toggles2 */
			printf("0\n"); /* powers1 */
			printf("0\n"); /* powers2 */
			printf("0\n"); /* powers3 */
			printf("0\n"); /* depowers1 */
			printf("0\n"); /* depowers2 */
			printf("0\n"); /* depowers3 */
			printf("-1\n"); /* Zone(s) */
			fflush(stdout);
			gets(q);
		} else if(f[0] == '+') {
			    if((f[1] == 'A') || (f[1] == 'N')) {
				    val = atoi(f2);
				    if(val > 255)
					val = val + 256;
				    printf("+%c%d\n",f[1],val);
				    fflush(stdout);
				    gets(q);
			    } else if(f[1] == 'X' || f[1] == 'T') {
				    printf("+V74247\n",val);
				    fflush(stdout);
				    gets(q);
			    } else {
				    printf("%s\n",q);
				    fflush(stdout);
				    gets(q);
			    }
		    } else if(f[0] == '>') {
                            atrcnt++;
			    val = atoi(f1);
                            if(val > 255)
				val = val + 256;
			    /* attr conversion */
                            if( (val == 5) && (obj == 1) ) {
                               printf(">5\n");
                               gets(q); /* read password and toss away */
                               if ( strstr(q, "$SHA1$") != NULL ) {
                                  fprintf(stderr, "Warning: #1's password in SHA1 encryption.  Reset to 'Nyctasia' using crypt.\n");
                                  printf("XXXXFe7xx3Zo2\n"); /* set password for #1 to Nyctasia */
                               } else
                                  printf("%s\n",q); /* Just store the password as is */
                               gets(q);
                            } else if ((val >= 129) && (val <= 142)) {
			       printf(">%d\n",val+30);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 127) {
			       printf(">%d\n",235);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 143) {
			       printf(">%d\n",174);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 144) {
			       printf(">%d\n",246);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 200) {
			       printf(">%d\n",177);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 203) {
			       printf(">%d\n",193);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 209) {
			       printf(">%d\n",199);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 213) {
			       printf(">%d\n",237);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 218) {
			       printf(">%d\n",227);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 219) {
			       printf(">%d\n",228);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 241) {
			       printf(">%d\n",225);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 242) {
			       printf(">%d\n",224);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 243) {
			       printf(">%d\n",245);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 199) {
			       printf(">%d\n",220);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 214) {
			       printf(">%d\n",236);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 216) {
			       printf(">%d\n",248);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 217) {
			       printf(">%d\n",218);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 226) {
			       printf(">%d\n",217);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
			    } else if ( (val == 96) || ((val > 199) && (val < 252)) ||
                                        ((val >= 145) && (val <= 148)) ) {
			       gets(q);
                               fprintf(stderr, "Warning: Object #%d has attribute (%d) unused by RhostMUSH.  Contents: %s\n", obj, val, q);
			       gets(q);
			    } else  {
			       printf(">%d\n",val);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",q);
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
			    }
		    } else if(f[0] == '-') {
			    gets(q);
		    } else {
			    printf("%s\n",q);
			    fflush(stdout);
			    gets(q);
		    }
		
		if(strstr(f, "***END OF DUMP***") != NULL )
		  {
			  printf("***END OF DUMP***\n");
			  fflush(stdout);
			  q = NULL;
			  return(0);
		  }
	} /* while */
	return(0);	
}
