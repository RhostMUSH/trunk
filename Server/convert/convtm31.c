/* Original Code provided by Zenty (c) 1999, All rights reserved
 * Modified by Ashen-Shugar for attribute allignment and functionary locks
 * (c) 1999, All rights reserved
 */
#include <stdio.h>
#include <string.h>
/* 1st word of flags */
#define IMMORTAL  0x00200000
/* 2nd word of flags */
#define TM3AUDIT  0x00000100
#define TM3ANSI   0x00000200
#define TM3FIXED  0x00000800
#define TM3GAGGED 0x00040000
#define TM3CMDS   0x00080000
#define TM3STOP   0x00100000
#define TM3BOUNCE 0x00200000
#define TM3CONST  0x00800000
#define TM3BLIND  0x08000000
#define RHOFIXED  0x00080000
#define RHOGAGGED 0x00010000
/* 3rd word of flags */
#define TM3NODFLT 0x00000020
#define TM3MARK0  0x00400000
#define TM3MARK1  0x00800000
#define TM3MARK2  0x01000000
#define TM3MARK3  0x02000000
#define TM3MARK4  0x04000000
#define TM3MARK5  0x08000000
#define TM3MARK6  0x10000000
#define TM3MARK7  0x20000000
#define TM3MARK8  0x40000000
#define TM3MARK9  0x80000000
#define RHOAUDIT  0x00010000
#define RHOSTOP   0x00002000
#define RHOCONST  0x00000100
/* 4th word of flags */
#define RHOCMDS   0x00000008
#define RHOBOUNCE 0x00004000
#define RHOBLIND  0x00100000
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
/* 2nd word of toggles */
#define RHONODFLT 0x08000000

#define LBUF_SIZE 65535
#define mush_gets(x)	fgets(x, LBUF_SIZE - 1, stdin)

char *rewrite_atr(char *);

int main(int argc, char **argv) {
	int val, flag1, flag2, flag3, nflag1, nflag2, nflag3, nflag4, tog2, obj, inattr, variable;
	int mage, royalty, staff, ansi, immortal, offsetchk, invar, atrcnt, loc, origattr, sanitattr;
	char f[LBUF_SIZE], *q, *f1, *f2;
	memset(f,'\0', sizeof(f));
	q = f;
	f1 = f+1;
	f2 = f+2;
	offsetchk = atrcnt = inattr = loc = origattr = sanitattr = 0;
	
        invar = 0;
	gets(q);
	while(q != NULL && !feof(stdin) ) {
		if(f[0] == '!') {
                        invar = 0;
                        if ( atrcnt > 750 ) {
                           fprintf(stderr, "Notice: Object #%d has more than 750 attributes (%d total).\n", obj, atrcnt);
                        }
			obj=atoi(f1);
                        atrcnt = variable = 0;
			/* object conversion */
			printf("%s\n",q);
			gets(q); printf("%s\n",q); /* name */
			gets(q); printf("%s\n",q); /* location */
                        loc = atoi(q);
  			gets(q);                   /* zone */
			gets(q); printf("%s\n",q); /* Contents */
			gets(q); printf("%s\n",q); /* Exits */
			gets(q); 
                        if ( atoi(q) == -4 ) {     /* if variable exit */
                           variable = 1;
                           printf("%d\n", loc);
                        } else
                           printf("%s\n",q); /* Link */
			gets(q); printf("%s\n",q); /* Next */
			gets(q);
			   if ( strchr(f, '/') == 0 && strchr(f, ':') == 0 ) {
				   printf("%s\n", q); /* Bool */
			   } else {
				   printf("%s\n", q); /* Functionary Lock */
				   gets(q);printf("%s\n",q); /* Bool */
				   offsetchk = 1;
			   }
			/* If previous was null, last was owner, not bool */
			if ( (!( q && *q) && offsetchk) || !offsetchk ) {
			   gets(q); printf("%s\n",q); /* Owner */
			}
			offsetchk = 0;
			gets(q); printf("%s\n",q); /* Parent */
			gets(q); printf("%s\n",q); /* Pennies */
			/* flag conv */
			gets(q); flag1 = atoi(q); 
			gets(q); flag2 = atoi(q);
			gets(q); flag3 = atoi(q);
                        nflag1 = nflag2 = nflag3 = nflag4 = 0;
			nflag1 = (flag1 & 0xDFDFFFFF); /* Strip royalty and immortal */
			nflag2 = (flag2 & 0xD00000FF);
                        if ( flag2 & TM3ANSI )
			   nflag2 = (nflag2 | 0x06000000);
			nflag3 = nflag4 = tog2 = 0;			
                        if ( flag2 & TM3AUDIT )
                           nflag3 = nflag3 | RHOAUDIT;
                        if ( flag2 & TM3FIXED )
                           nflag2 = nflag2 | RHOFIXED;
                        if ( flag2 & TM3GAGGED )
                           nflag2 = nflag2 | RHOGAGGED;
                        if ( flag2 & TM3CMDS )
                           nflag4 = nflag4 | RHOCMDS;
                        if ( flag2 & TM3STOP )
                           nflag3 = nflag3 | RHOSTOP;
                        if ( flag2 & TM3BOUNCE )
                           nflag4 = nflag4 | RHOBOUNCE;
                        if ( flag2 & TM3CONST)
                           nflag3 = nflag3 | RHOCONST;
                        if ( flag2 & TM3BLIND )
                           nflag4 = nflag4 | RHOBLIND;
                        if ( flag3 & TM3NODFLT )
                           tog2 = tog2 | RHONODFLT;
                        if ( flag3 & TM3MARK0 )
                           nflag4 = nflag4 | RHOMARK0;
                        if ( flag3 & TM3MARK1 )
                           nflag4 = nflag4 | RHOMARK1;
                        if ( flag3 & TM3MARK2 )
                           nflag4 = nflag4 | RHOMARK2;
                        if ( flag3 & TM3MARK3 )
                           nflag4 = nflag4 | RHOMARK3;
                        if ( flag3 & TM3MARK4 )
                           nflag4 = nflag4 | RHOMARK4;
                        if ( flag3 & TM3MARK5 )
                           nflag4 = nflag4 | RHOMARK5;
                        if ( flag3 & TM3MARK6 )
                           nflag4 = nflag4 | RHOMARK6;
                        if ( flag3 & TM3MARK7 )
                           nflag4 = nflag4 | RHOMARK7;
                        if ( flag3 & TM3MARK8 )
                           nflag4 = nflag4 | RHOMARK8;
                        if ( flag3 & TM3MARK9 )
                           nflag4 = nflag4 | RHOMARK9;
			if((obj == 1) && !(nflag1 & IMMORTAL))
			  nflag1 |= IMMORTAL; /* This should not happen. */
                        if ((obj != 1) && (nflag1 & IMMORTAL))
                          nflag1 = (nflag1 & ~IMMORTAL); /* Yank immortal */
			printf("%u\n",nflag1); /* Flags1 */
			printf("%u\n",nflag2); /* Flags2 */
			printf("%u\n",nflag3); /* Flags3 */
			printf("%u\n",nflag4); /* Flags4 */
			fflush(stdout);
			gets(q); /* power 1 */
			gets(q); /* power 2 */
                        gets(q); /* Timestamp - Access */
                        gets(q); /* Timestamp - Modify */
			printf("0\n"); /* toggles0 */
                        if ( variable ) 
                           printf("%u\n", 32768); /* variable toggle */
                        else
                           printf("0\n"); /* toggles1 */
                        if ( tog2 > 0 )
                           printf("%u\n", tog2);
                        else
			   printf("0\n"); /* toggles2 */
			printf("0\n"); /* toggles3 */
			printf("0\n"); /* toggles4 */
			printf("0\n"); /* toggles5 */
			printf("0\n"); /* toggles6 */
			printf("0\n"); /* toggles7 */
			printf("-1\n"); /* Zones */
			fflush(stdout);
			gets(q);
		} else 
		    if( (f[0] == '+') && !invar ) {
			    if((f[1] == 'A') || (f[1] == 'N')) {
                                    if ( f[1] == 'A' )
                                       inattr = 1;
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
		    } else
		    if( (f[0] == '>') && isdigit(f[1]) ) {
                            atrcnt++;
                            invar = 1;
			    val = atoi(f1);
			    if(val > 255)
				val = val + 256;
			    /* attr conversion */
                            if ((val >= 129) && (val <=142)) {
                               printf(">%d\n", val+30);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",rewrite_atr(q));
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 143) {
                               printf(">%d\n", 174);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",rewrite_atr(q));
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if ((val >= 144) && (val <= 199)) { /* Eat it */
			       gets(q);
			       gets(q);
                            } else if (val == 203) {
                               printf(">%d\n", 193);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",rewrite_atr(q));
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 209) {
                               printf(">%d\n", 199);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",rewrite_atr(q));
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 213) {
                               printf(">%d\n", 237);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",rewrite_atr(q));
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 214) {
                               printf(">%d\n", 224);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",rewrite_atr(q));
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 215) {
                               printf(">%d\n", 225);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",rewrite_atr(q));
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 216) {
                               printf(">%d\n", 248);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",rewrite_atr(q));
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 217) {
                               printf(">%d\n", 218);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",rewrite_atr(q));
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
                            } else if (val == 218) {
                               printf(">%d\n", 246);
			       fflush(stdout);
			       gets(q);
			       printf("%s\n",rewrite_atr(q));
                               if ( strlen(q) > LBUF_SIZE )
                                  fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
			       gets(q);
			    } else if((val == 96) || ((val > 199) && (val < 252))) { /* Eat it */
				    gets(q);
                                    fprintf(stderr, "Warning: Object #%d has attribute (%d) unused by RhostMUSH.  Contents: %s\n", obj, val, q);
				    gets(q);
			    } else  {
				    printf(">%d\n",val);
				    fflush(stdout);
				    gets(q);
				    printf("%s\n",rewrite_atr(q));
                                    if ( strlen(q) > LBUF_SIZE )
                                       fprintf(stderr, "Warning: Object #%d has attribute (%d) over LBUF.\n", obj, val);
				    gets(q);
			    }
		    } else 
		    if( (f[0] == '-') && !invar ) {
			    gets(q);
		    } else {
                            if ( inattr ) {
                               inattr = 0;
                               origattr = (atoi(q) | 1);
                               sanitattr = (origattr & 0x000003FF);
                               if ( origattr & 0x00000400 )
                                  sanitattr = (sanitattr | 0x00001000);
                               if ( origattr & 0x00000800 )
                                  sanitattr = (sanitattr | 0x00020000);
                               if ( origattr & 0x00001000 )
                                  sanitattr = (sanitattr | 0x00008000);
                               if ( origattr & 0x00004000 )
                                  sanitattr = (sanitattr | 0x00040000);
                               if ( origattr & 0x00008000 )
                                  sanitattr = (sanitattr | 0x20000000);
                               if ( origattr & 0x00010000 )
                                  sanitattr = (sanitattr | 0x00200000);
                               if ( origattr & 0x00200000 )
                                  sanitattr = (sanitattr | 0x04000000);
                               printf("%d%s\n", sanitattr, strchr(q, ':'));
                            } else
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

char *
rewrite_atr(char *in_str)
{
   int sanitattr, origattr;
   char *tck_buff, *tck_val1, *tck_val2, *tck_val3;
   static char f[LBUF_SIZE];
   
   strcpy(f, in_str);
   sanitattr = origattr = 0;
   tck_val1 = tck_val2 = tck_val3 = NULL;

   if ( strchr(in_str, '\001') == NULL ) {
      return(in_str);
   } else {
      strcpy(f, in_str);
      tck_val1 = strtok_r(in_str, ":", &tck_buff);
      if ( tck_val1 )
         tck_val2 = strtok_r(NULL, ":", &tck_buff);
      if ( tck_val2 )
         tck_val3 = strtok_r(NULL, "\0", &tck_buff);
      if ( !tck_val2 )
         strcpy(in_str, f);
      else {
         origattr = atoi(tck_val2);
         sanitattr = (origattr & 0x000003FF);
         if ( origattr & 0x00000400 )
            sanitattr = (sanitattr | 0x00001000);
         if ( origattr & 0x00000800 )
            sanitattr = (sanitattr | 0x00020000);
         if ( origattr & 0x00001000 )
            sanitattr = (sanitattr | 0x00008000);
         if ( origattr & 0x00004000 )
            sanitattr = (sanitattr | 0x00040000);
         if ( origattr & 0x00008000 )
            sanitattr = (sanitattr | 0x20000000);
         if ( origattr & 0x00010000 )
            sanitattr = (sanitattr | 0x00200000);
         if ( origattr & 0x00200000 )
            sanitattr = (sanitattr | 0x04000000);
         sprintf(f, "%s:%d:%s", tck_val1, sanitattr, tck_val3);
      }
      return(f);
   }
}
