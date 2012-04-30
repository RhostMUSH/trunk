/* This is not intended to be pretty code, just do the job.
   It takes as input the name of the flat file and will output a striped
   file with .new appended. */
/* Credits:  Seawolf@RhostMUSH */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXGET 5000

int numtotal;
int nextattr;
char mainbuff[MAXGET];
char s1b[MAXGET];
char s2b[MAXGET];
int *amask;
FILE *orig, *newf, *newf2;


void setup()
{
  char *pt1;
  int x;

  pt1 = mainbuff;
  fgets(mainbuff,MAXGET,orig);
  fputs(mainbuff,newf);
  fgets(mainbuff,MAXGET,orig);
  fputs(mainbuff,newf);
  fgets(mainbuff,MAXGET,orig);
  fputs(mainbuff,newf);
  numtotal = atoi(pt1+2);
  nextattr = numtotal;
  if ((nextattr & 0x7f) == 0)
    nextattr++;
  numtotal *= 2;
  amask = (int *)malloc(sizeof(int) * numtotal);
  if (!amask) {
    fclose(orig);
    fclose(newf);
    fprintf(stderr,"Could not allocate memory required.\n");
    exit(4);
  }
  for (x = 0; x < numtotal; x++)
    amask[x] = 0;
}

void skip1()
{
  char *pt1;
  int x;
  
  pt1 = mainbuff;
  fgets(mainbuff,MAXGET,orig);
  while (*pt1 != '!') {
    x = atoi(pt1+2);
      if ((x & 0x7f) == 0) {
	amask[x] = nextattr++;
	if ((nextattr & 0x7f) == 0)
	  nextattr++;
      }
      else
        amask[x] = x;
      fprintf(newf,"+A%d\n",amask[x]);
      fgets(mainbuff,MAXGET,orig);
      fputs(mainbuff,newf);
    fgets(mainbuff,MAXGET,orig);
  }
  fputs(mainbuff,newf);
}

void skip2()
{
  int x;

  *s1b = '\0';
  *s2b = '\0';
  x = 0;
  while (((*mainbuff != '<') && (*mainbuff != '>')) || (x < 5)) {
    fgets(mainbuff,MAXGET,orig);
    x++;
    if (((*mainbuff != '<') && (*mainbuff != '>')) || (x < 5)) {
      if (*s1b)
        fputs(s1b,newf);
      strcpy(s1b,s2b);
      strcpy(s2b,mainbuff);
    }
  }
}

void procflags() 		/* Add fix stuff later */
{
  int x;

  x = atoi(s1b);
  fprintf(newf,"%d\n",x);
  x = atoi(s2b);
  fprintf(newf,"%d\n",x);
}

void rattr()
{
  char *p;
  int c, lastc;

  p = mainbuff;
  c = '\0';
  for (;;) {
    lastc = c;
    c = fgetc(orig);
    if (!c || (c == EOF)) {
      *p = '\0';
      return;
    }
    if ((c == '\n') && (lastc != '\r')) {
      *p = '\0';
      return;
    }
    *p++ = (char)c;
  }
}

void skipattrs()
{
  char *pt1, *c1, *c2;
  int x, i1, i2, i3, i4, i5, test;

  pt1 = mainbuff;
  if (*pt1 == '<')
    fputs(mainbuff,newf);
  else if (*pt1 == '>') {	/* do attr number conversion here */
    x = atoi(pt1+1);
    if ((x >= 126) && (x <= 151))
      x += 3;
    else if (x == 152)
      x = 126;
    else if (x == 153)
      x = 127;
    else if (x == 154)
      x = 197;
    if ((x < 256) || amask[x])
      test = 1;
    else
      test = 0;
    if (test) {
      if (x < 256)
        fprintf(newf,">%d\n",x);
      else
	fprintf(newf,">%d\n",amask[x]);
    }
    while (*pt1 != '<') {
      rattr();
      if (test) {
	  fputs(mainbuff,newf);
	  fputc('\n',newf);
      }
      fgets(mainbuff,MAXGET,orig);
      if (*pt1 != '>')
        fputs(mainbuff,newf);
      else {
        x = atoi(pt1+1);
        if ((x >= 126) && (x <= 151))
          x += 3;
	else if (x == 152)
	  x = 126;
        else if (x == 153)
          x = 127;
        else if (x == 154)
          x = 197;
        if ((x < 256) || amask[x])
          test = 1;
        else
          test = 0;
	if (test) {
	  if (x < 256)
            fprintf(newf,">%d\n",x);
	  else
	    fprintf(newf,">%d\n",amask[x]);
	}
      }
    }
  }
}

void pass1()
{
  char *pt1;
  int numt;

  pt1 = mainbuff;
  skip1();
  while(1) {
    skip2();
    procflags();
    skipattrs();
    if (*pt1 == '<') {
      fgets(mainbuff,MAXGET,orig);
      fputs(mainbuff,newf);
      if (!strncmp(mainbuff,"***END OF DUMP***",17)) break;
    }
  }
}

void pass2()
{
  fgets(mainbuff,MAXGET,newf);
  fputs(mainbuff,newf2);
  fgets(mainbuff,MAXGET,newf);
  fputs(mainbuff,newf2);
  fgets(mainbuff,MAXGET,newf);
  fprintf(newf2,"+N%d\n",nextattr);
  fgets(mainbuff,MAXGET,newf);
  while (1) {
    fputs(mainbuff,newf2);
    if (!strncmp(mainbuff,"***END OF DUMP***",17)) break;
    fgets(mainbuff,MAXGET,newf);
  }
}

void main(argc, argv) 
  int argc;
  char *argv[];
{
  char buff[100];

  if (argc != 2) {
    fprintf(stderr,"Incorrect number of arguments\n");
    exit(1);
  }
  strcpy(buff,argv[1]);
  orig = fopen(buff,"r");
  if (!orig) {
    fprintf(stderr,"Could not open file: %s\n",buff);
    exit(2);
  }
  strcat(buff,".new");
  newf = fopen(buff,"w+");
  strcat(buff,".2");
  newf2 = fopen(buff,"w+");
  if (!newf || !newf2) {
    fprintf(stderr,"Could not open new file: %s\n",buff);
    exit(3);
  }
  printf("Initializng.\n");
  setup();
  printf("Starting pass 1.\n");
  pass1();
  printf("Pass 1 Complete.  Starting pass 2.\n");
  rewind(newf);
  pass2();
  printf("Pass 2 Complete.\n");
  fclose(orig);
  fclose(newf);
  fclose(newf2);
  free(amask);
  exit(0);
}
