/* addflag.c version 3.00 by Echemus (bean@lintilla.foobar.co.uk) */
#include <stdio.h>
#include <string.h>

#define LINES 14

int addflag(FILE *file, FILE *file2, char *buf);
char *my_fgets(char *buf, int size, FILE *file);
char *readline(FILE *file, char *buf);

void main(int argc, char **argv)
{ char *buf = (char *) malloc(10240 * sizeof(char));
  FILE *file, *file2;
  int a = 0;

  if (argc < 2)
  {
    printf("Usage: %s <flatfile>\n", argv[0]);
    exit(1);
  }

  file = fopen(argv[1], "r");
  if (!file)
  {
    printf("Filename invalid.\n");
    exit(2);
  }

  sprintf(buf, "%s.new", argv[1]);
  file2 = fopen(buf, "w");
  if (!file2)
  {
    printf("Output file invalid.\n");
    exit(3);
  }

  /* look for #0 */
  while((!feof(file)) && !a)
  {
    if (!strcmp(my_fgets(buf, 3, file), "\n!0"))
      a = addflag(file, file2, buf);
    else
      fputc(*buf, file2);
  }

  /* find other objects */
  while(!feof(file))
    if (!strcmp(my_fgets(buf, 3, file), "<\n!"))
     addflag(file, file2, buf);
    else
      fputc(*buf, file2);
}

int addflag(FILE *file, FILE *file2, char *buf)
{ int a;

  fseek(file, ftell(file) - 1, SEEK_SET);
  /* move through existing flag/lock information */
  for (a = 0; a < LINES; a++)
    fprintf(file2, "%s\n", readline(file, buf));
  /* add in extra rhost flag/power space */
  fprintf(file2, "0\n0\n0\n0\n0\n0\n0\n0\n0\n0\n-1\n");

  return -1;
}

char *my_fgets(char *buf, int size, FILE *file)
{ int a = ftell(file), b;

  for (b = 0; b < size; buf[b++] = fgetc(file));
  buf[size] = '\0';
  fseek(file, ++a, SEEK_SET);

  return buf;
}

char *readline(FILE *file, char *buf)
{ char *b = buf;

  while(!feof(file) && ((*b++ = fgetc(file)) > 31));
  *--b = '\0';

  return buf;
}
