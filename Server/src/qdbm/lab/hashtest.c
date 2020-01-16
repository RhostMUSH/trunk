/*************************************************************************************************
 * Test unit for hash functions
 *************************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#define RECBUFSIZ      32
#define RANDDATAMIN    4
#define RANDDATAMAX    12


/* function prototypes */
int main(int argc, char **argv);
void usage(const char *progname);
int setrandstr(char *buf);
void printcol(const char *name, int (*hfunc)(const char *, int), int rnum, int bnum);
int dpfirsthash(const char *kbuf, int ksiz);
int dpsecondhash(const char *kbuf, int ksiz);
int dpthirdhash(const char *kbuf, int ksiz);


/* main routine */
int main(int argc, char **argv){
  int rnum, bnum, i, len;
  char buf[RECBUFSIZ];
  rnum = argc > 1 ? atoi(argv[1]) : -1;
  bnum = argc > 2 ? atoi(argv[2]) : -1;
  srand(time(NULL));
  if(rnum > 0 && bnum > 0){
    printf("number of records: %d\n", rnum);
    printf("number of buckets: %d\n", bnum);
    printcol("dpfirsthash", dpfirsthash, rnum, bnum);
    printcol("dpsecondhash", dpsecondhash, rnum, bnum);
    printcol("dpthirdhash", dpthirdhash, rnum, bnum);
  } else if(rnum > 0){
    for(i = 1; i < rnum; i++){
      len = sprintf(buf, "%X", i);
      printf("%s", buf);
      printf("\t%10d", dpfirsthash(buf, len));
      printf("\t%10d", dpsecondhash(buf, len));
      printf("\t%10d", dpthirdhash(buf, len));
      printf("\n");
    }
  } else {
    usage(argv[0]);
  }
  return 0;
}


/* show usage and exit */
void usage(const char *progname){
  fprintf(stderr, "%s: test unit for hash functions\n\n", progname);
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "  %s rnum bnum  : print collision rate\n", progname);
  fprintf(stderr, "  %s rnum       : print values\n", progname);
  exit(1);
}


/* set randum string and return its length */
int setrandstr(char *buf){
  int i, len;
  len = rand() % (RANDDATAMAX - RANDDATAMIN) + RANDDATAMIN + 1;
  for(i = 0; i < len; i++){
    buf[i] = rand() % 256;
  }
  buf[len] = '\0';
  return len;
}


/* print collision rate of each hash functioon */
void printcol(const char *name, int (*hfunc)(const char *, int), int rnum, int bnum){
  int *array, i, len, col;
  char buf[RECBUFSIZ];
  if(!(array = calloc(bnum, sizeof(int)))) return;
  for(i = 1; i <= rnum; i++){
    len = setrandstr(buf);
    array[hfunc(buf, len) % bnum]++;
  }
  col = 0;
  for(i = 0; i < bnum; i++){
    if(array[i] > 1) col += array[i] - 1;
  }
  printf("collision rate: %5.2f%% (%8d) by %s\n",
         ((double)col / (double)rnum) * 100.0, col, name);
  free(array);
}


/* hash function to select a bucket element */
int dpfirsthash(const char *kbuf, int ksiz){
  const unsigned char *p;
  unsigned int sum;
  int i;
  assert(kbuf && ksiz >= 0);
  p = (const unsigned char *)kbuf;
  sum = 751;
  for(i = 0; i < ksiz; i++){
    sum = sum * 31 + p[i];
  }
  return (sum * 87767623) & 0x7FFFFFFF;
}


/* hash function to select a right/left node of binary search tree */
int dpsecondhash(const char *kbuf, int ksiz){
  const unsigned char *p;
  unsigned int sum;
  int i;
  assert(kbuf && ksiz >= 0);
  p = (const unsigned char *)kbuf;
  sum = 19780211;
  for(i = ksiz - 1; i >= 0; i--){
    sum = sum * 37 + p[i];
  }
  return (sum * 43321879) & 0x7FFFFFFF;
}


/* hash function for its applications */
int dpthirdhash(const char *kbuf, int ksiz){
  const unsigned char *p;
  unsigned int sum;
  int i;
  assert(kbuf && ksiz >= 0);
  p = (const unsigned char *)kbuf;
  sum = 774831917;
  for(i = ksiz - 1; i >= 0; i--){
    sum = sum * 29 + p[i];
  }
  return (sum * 5157883) & 0x7FFFFFFF;
}



/* END OF FILE */
