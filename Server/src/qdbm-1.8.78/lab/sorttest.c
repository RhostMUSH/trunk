/*************************************************************************************************
 * Test unit for sorting functions
 *************************************************************************************************/


#include <cabin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define INPUTBIAS      0


/* function prototypes */
int main(int argc, char **argv);
void usage(const char *progname);
int strpcmp(const void *ap, const void *bp);


/* main routine */
int main(int argc, char **argv){
  int i, rnum, len;
  char **ivector, buf[32];
  if(argc != 3) usage(argv[0]);
  if((rnum = atoi(argv[2])) < 1) usage(argv[0]);
  if(!strchr("ishqQp", argv[1][0])) usage(argv[0]);
  ivector = cbmalloc(rnum * sizeof(ivector[0]));
  for(i = 0; i < rnum; i++){
    len = sprintf(buf, "%08d", rand() % rnum + i * INPUTBIAS + 1);
    ivector[i] = cbmemdup(buf, len);
  }
  switch(argv[1][0]){
  case 'i':
    cbisort(ivector, rnum, sizeof(ivector[0]), strpcmp);
    break;
  case 's':
    cbssort(ivector, rnum, sizeof(ivector[0]), strpcmp);
    break;
  case 'h':
    cbhsort(ivector, rnum, sizeof(ivector[0]), strpcmp);
    break;
  case 'q':
    cbqsort(ivector, rnum, sizeof(ivector[0]), strpcmp);
    break;
  case 'Q':
    qsort(ivector, rnum, sizeof(ivector[0]), strpcmp);
    break;
  }
  if(strchr(argv[1], 'p')){
    for(i = 0; i < rnum; i++){
      printf("%s\n", ivector[i]);
      free(ivector[i]);
    }
  } else {
    for(i = 0; i < rnum; i++){
      free(ivector[i]);
    }
  }
  free(ivector);
  return 0;
}


/* show usage and exit */
void usage(const char *progname){
  fprintf(stderr, "%s: usage: %s i|s|h|q|Q|p|ip|sp|hp|qp|Qp rnum\n", progname, progname);
  exit(1);
}


/* comparing function */
int strpcmp(const void *ap, const void *bp){
  char *a, *b;
  a = *(char **)ap;
  b = *(char **)bp;
  return strcmp(a, b);
}



/* END OF FILE */
