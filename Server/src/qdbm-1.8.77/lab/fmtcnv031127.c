/*************************************************************************************************
 * Format converter from older than 2003-11-27 to latest
 *************************************************************************************************/


#include <stdio.h>
#include <string.h>

#define HEADSIZ        48
#define OLDFSIZOFF     16
#define OLDBNUMOFF     24
#define OLDRNUMOFF     32
#define NEWFLAGSOFF    16
#define NEWFSIZOFF     24
#define NEWBNUMOFF     32
#define NEWRNUMOFF     40


/* function prototypes */
int main(int argc, char **argv);


/* main routine */
int main(int argc, char **argv){
  char head[HEADSIZ];
  int i, vb, zb, flags, fsiz, bnum, rnum, c;
  vb = 0;
  zb = 0;
  for(i = 1; i < argc; i++){
    if(!strcmp(argv[i], "-v")) vb = 1;
    if(!strcmp(argv[i], "-z")) zb = 1;
  }
  for(i = 0; i < HEADSIZ; i++){
    head[i] = getchar();
  }
  fsiz = *(int *)(head + OLDFSIZOFF);
  bnum = *(int *)(head + OLDBNUMOFF);
  rnum = *(int *)(head + OLDRNUMOFF);
  flags = 0;
  if(vb) flags |= 1 << 0;
  if(zb) flags |= 1 << 1;
  *(int *)(head + NEWFLAGSOFF) = flags;
  *(int *)(head + NEWFSIZOFF) = fsiz;
  *(int *)(head + NEWBNUMOFF) = bnum;
  *(int *)(head + NEWRNUMOFF) = rnum;
  for(i = 0; i < HEADSIZ; i++){
    putchar(head[i]);
  }
  while((c = getchar()) != EOF){
    putchar(c);
  }
  return 0;
}



/* END OF FILE */
