/* Original Code provided by Zenty (c) 1999, All rights reserved
 * Modified by Ashen-Shugar for attribute allignment and functionary locks
 * (c) 1999, All rights reserved
 */
#include <stdio.h>
#include <string.h>
#define IMMORTAL  0x00200000
#define ADMIN     0x00000100
#define RANSI     0x02000000
#define ANSICOLOR 0x04000000
#define BUILDER   0x20000000
#define ROYALTY   0x20000000
#define STAFF     0x00010000
#define ANSI      0x00000200
#define MAGE      0x00000001
#define WIZARD    0x00000010
#define MUXANSI   0x00000200
#define TINYANSI  0x00040000

#define LBUF_SIZE 65535
#define mush_gets(x)    fgets(x, LBUF_SIZE - 1, stdin)


int main(void) {
	int val, flag1, flag2, flag3, nflag1, nflag2, nflag3, obj;
	int mage, royalty, staff, ansi, immortal, offsetchk, atrcnt;
	char f[LBUF_SIZE], *q, *f1, *f2;
	memset(f,'\0', sizeof(f));
	q = f;
	f1 = f+1;
	f2 = f+2;
	offsetchk = atrcnt = 0;
	
	mush_gets(q);
	while(q != NULL && !feof(stdin) ) {
		if(f[0] == '!') {
                        if ( atrcnt > 750 ) {
                           fprintf(stderr, "Notice: Object #%d has more than 750 attributes (%d total).\n", obj, atrcnt);
                        }
			obj=atoi(f1);
                        atrcnt = 0;
			/* object conversion */
			printf("%s",q);
			mush_gets(q); printf("%s",q); /* name */
			mush_gets(q); printf("%s",q); /* location */
/*  			mush_gets(q); *//*printf("%s",q); *//* zone */
			mush_gets(q); printf("%s",q); /* Contents */
			mush_gets(q); printf("%s",q); /* Exits */
			mush_gets(q); printf("%s",q); /* Link */
			mush_gets(q); printf("%s",q); /* Next */
			mush_gets(q);
			   if ( strchr(f, '/') == 0 && strchr(f, ':') == 0 ) {
				   printf("%s", q); /* Bool */
			   } else {
				   printf("%s", q); /* Functionary Lock */
  				   mush_gets(q);printf("%s",q);     /* Bool */
				   offsetchk = 1;
			   }
			/* If previous was null, last was owner, not bool */
  			if ( (!( q && *q) && offsetchk) || !offsetchk ) { 
			   mush_gets(q); printf("%s",q); /* Owner */
  			} 
			offsetchk = 0;
			mush_gets(q); printf("%s",q); /* Parent */
			mush_gets(q); printf("%s",q); /* Pennies */
			/* flag conv */
			mush_gets(q); flag1 = atoi(q); 
  			mush_gets(q); flag2 = atoi(q);
			nflag1 = (flag1 & 0xFFDFFFFF);
                        if ( nflag2 & TINYANSI )
			   nflag2 = (flag2 & 0xD60000FF);
                        else
			   nflag2 = (flag2 & 0xD00000FF);
			nflag3 = 0;			
			if((obj == 1) && !(nflag1 & IMMORTAL))
			  nflag1 |= IMMORTAL; // This should not happen.
			printf("%i\n",nflag1);
			printf("%i\n",nflag2);
  			printf("%i\n",nflag3);
			fflush(stdout);
			printf("0\n"); /* Flags4 */
			printf("0\n"); /* toggles */
                        printf("0\n"); /* toggles1 */
			printf("0\n"); /* toggles2 */
			printf("0\n"); /* toggles3 */
			printf("0\n"); /* toggles4 */
			printf("0\n"); /* toggles5 */
			printf("0\n"); /* toggles6 */
			printf("0\n"); /* toggles7 */
			printf("-1\n"); /* Unknown */
			fflush(stdout);
			mush_gets(q);
		} else 
		    if(f[0] == '+') {
			    if((f[1] == 'A') || (f[1] == 'N')) {
				    val = atoi(f2);
				    if(val > 255)
					val = val + 256;
				    printf("+%c%d\n",f[1],val);
				    fflush(stdout);
				    mush_gets(q);
			    } else if(f[1] == 'X') {
				    printf("+V74247\n",val);
				    fflush(stdout);
				    mush_gets(q);
			    } else {
				    printf("%s",q);
				    fflush(stdout);
				    mush_gets(q);
			    }
		    } else
		    if(f[0] == '>') {
                            atrcnt++;
			    val = atoi(f1);
			    if(val > 255)
				val = val + 256;
			    /* attr conversion */
			    if((val == 96) || ((val > 199) && (val < 222))) {
				    mush_gets(q);
                                    fprintf(stderr, "Warning: Object #%d has attribute (%d) unused by RhostMUSH.  Contents: %s\n", obj, val, q);
				    mush_gets(q);
			    } else  {
				    printf(">%d\n",val);
				    fflush(stdout);
				    mush_gets(q);
				    printf("%s",q);
				    mush_gets(q);
			    }
		    } else 
		    if(f[0] == '-') {
			    mush_gets(q);
		    } else {
			    printf("%s",q);
			    fflush(stdout);
			    mush_gets(q);
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
