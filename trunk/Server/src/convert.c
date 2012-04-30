#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define	ToLower(s) (isupper((int)s) ? tolower((int)s) : (s))
#define	ToUpper(s) (islower((int)s) ? toupper((int)s) : (s))

typedef struct hstruct {
  short int oldix;
  short int newix;
  int from;
  char *data;
  struct hstruct *next;
} HSTRUCT;

typedef struct mstruct {
  short int oldix;
  char *data;
  struct mstruct *next;
} MSTRUCT;

typedef struct pstruct {
  short int sndix;
  HSTRUCT *header;
  MSTRUCT *message;
} PSTRUCT;

#define BIGNUM 100000
PSTRUCT master[BIGNUM];
FILE *filept;
FILE *filept2;
char fbuff[100];
char fbuff2[100];
char buff[12000];
char obuff1[12000];
char obuff2[12000];

void cleanup()
{
  int x;
  HSTRUCT *pt1, *pt2;
  MSTRUCT *pt3, *pt4;

  for (x = 0; x < BIGNUM; x++) {
    pt1 = master[x].header;
    while (pt1) {
      pt2 = pt1->next;
      free(pt1->data);
      free(pt1);
      pt1 = pt2;
    }
    pt3 = master[x].message;
    while (pt3) {
      pt4 = pt3->next;
      free(pt3->data);
      free(pt3);
      pt3 = pt4;
    }
  }
}

MSTRUCT *find_msg(int player, short int old)
{
  MSTRUCT *pt1;

  pt1 = master[player].message;
  while (pt1 && (pt1->oldix != old))
    pt1 = pt1->next;
  return pt1;
}

void push_head(short int inx, char *pt1, int player, int from, short int inx2)
{
  HSTRUCT *dpt1, *dpt2;

  dpt1 = malloc(sizeof(HSTRUCT));
  if (!dpt1) {
    fprintf(stderr,"Big problems, no memory\n");
    exit(4);
  }
  dpt1->oldix = inx;
  dpt1->newix = inx2;
  dpt1->from = from;
  dpt1->data = pt1;
  dpt1->next = master[player].header;
  master[player].header = dpt1;
}

void push_mesg(short int inx, char *pt1, int player)
{
  MSTRUCT *dpt1, *dpt2;

  dpt1 = malloc(sizeof(MSTRUCT));
  if (!dpt1) {
    fprintf(stderr,"Big problems, no memory\n");
    exit(4);
  }
  dpt1->oldix = inx;
  dpt1->data = pt1;
  dpt1->next = master[player].message;
  master[player].message = dpt1;
}

void setup()
{
  int x;

  for (x = 0; x < BIGNUM; x++) {
    master[x].sndix = 1;
    master[x].header = NULL;
    master[x].message = NULL;
  }
}

void readin(int start)
{
  char in, *pt1;

  pt1 = buff + start;
  in = fgetc(filept);
  while ((in != EOF) && (in) && (in != '\1')) {
    *pt1++ = in;
    in = fgetc(filept);
  }
  *pt1 = '\0';
  if (in == '\1')
    fgetc(filept);
}

int string_compare(const char *s1, const char *s2)
{
    while (isspace((int)*s1))
      s1++;
    while (isspace((int)*s2))
      s2++;
    while (*s1 && *s2 && ((ToLower(*s1) == ToLower(*s2)) ||
			  (isspace((int)*s1) && isspace((int)*s2)))) {
      if (isspace((int)*s1) && isspace((int)*s2)) {	/* skip all other spaces */
        while (isspace((int)*s1))
	  s1++;
        while (isspace((int)*s2))
	  s2++;
      } else {
        s1++;
        s2++;
      }
    }
    if ((*s1) && (*s2))
      return (1);
    if (isspace((int)*s1)) {
      while (isspace((int)*s1))
        s1++;
      return (*s1);
    }
    if (isspace((int)*s2)) {
      while (isspace((int)*s2))
        s2++;
      return (*s2);
    }
    if ((*s1) || (*s2))
      return (1);
    return (0);
}

char *myitoa(int n)
{
  static char itoabuf[40];
  int i, sign;

  if ((sign = n) < 0)
    n = -n;
  itoabuf[39] = '\0';
  i = 38;
  do {
    itoabuf[i--] = n % 10 + '0';
  } while ((n /= 10) > 0);
  if (sign < 0)
    itoabuf[i] = '-';
  else
    i++;
  return (itoabuf + i);
}

static const char *monthtab[] =
{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static const char daystab[] =
{31, 29, 31, 30, 31, 30,
 31, 31, 30, 31, 30, 31};

#define get_substr(buf, p) { \
	p = (char *)strchr(buf, ' '); \
	if (p) { \
		*p++ = '\0'; \
		while (*p == ' ') p++; \
	} \
}

int 
do_convtime(char *str, struct tm *ttm)
{
    char *buf, *p, *q;
    int i;

    if (!str || !ttm)
	return 0;
    while (*str == ' ')
	str++;
    buf = p = obuff2;
    strcpy(buf,str);

    get_substr(buf, p);		/* day-of-week or month */
    if (!p || strlen(buf) != 3) {
	return 0;
    }
    for (i = 0; (i < 12) && string_compare(monthtab[i], p); i++);
    if (i == 12) {
	get_substr(p, q);	/* month */
	if (!q || strlen(p) != 3) {
	    return 0;
	}
	for (i = 0; (i < 12) && string_compare(monthtab[i], p); i++);
	if (i == 12) {
	    return 0;
	}
	p = q;
    }
    ttm->tm_mon = i;

    get_substr(p, q);		/* day of month */
    if (!q || (ttm->tm_mday = atoi(p)) < 1 || ttm->tm_mday > daystab[i]) {
	return 0;
    }
    p = (char *) strchr(q, ':');	/* hours */
    if (!p) {
	return 0;
    }
    *p++ = '\0';
    if ((ttm->tm_hour = atoi(q)) > 23 || ttm->tm_hour < 0) {
	return 0;
    }
    if (ttm->tm_hour == 0) {
	while (isspace((int)*q))
	    q++;
	if (*q != '0') {
	    return 0;
	}
    }
    q = (char *) strchr(p, ':');	/* minutes */
    if (!q) {
	return 0;
    }
    *q++ = '\0';
    if ((ttm->tm_min = atoi(p)) > 59 || ttm->tm_min < 0) {
	return 0;
    }
    if (ttm->tm_min == 0) {
	while (isspace((int)*p))
	    p++;
	if (*p != '0') {
	    return 0;
	}
    }
    get_substr(q, p);		/* seconds */
    if (!p || (ttm->tm_sec = atoi(q)) > 59 || ttm->tm_sec < 0) {
	return 0;
    }
    if (ttm->tm_sec == 0) {
	while (isspace((int)*q))
	    q++;
	if (*q != '0') {
	    return 0;
	}
    }
    get_substr(p, q);		/* year */
    if ((ttm->tm_year = atoi(p)) == 0) {
	while (isspace((int)*p))
	    p++;
	if (*p != '0') {
	    return 0;
	}
    }
    if (ttm->tm_year > 100)
	ttm->tm_year -= 1900;
    if (ttm->tm_year < 0)
	return 0;
#define LEAPYEAR_1900(yr) ((yr)%400==100||((yr)%100!=0&&(yr)%4==0))
    return (ttm->tm_mday != 29 || i != 1 || LEAPYEAR_1900(ttm->tm_year));
#undef LEAPYEAR_1900
}

void proc_head()
{
  int player, from;
  short int index, in2;
  char *pt1, *pt2;
  char *newinfo;
  struct tm ttm;
  time_t timeinfo;

  pt1 = strchr(buff+1,'i');
  *pt1 = '\0';
  player = atoi(buff+1);
  index = (short int)atoi(pt1+1);
  readin(0);
  *obuff1 = '\0';
  pt1 = strchr(buff,'<');
  *pt1 = '\0';
  from = atoi(buff+3);
  strcpy(obuff1,buff+3);
  strcat(obuff1,"/");
  pt2 = strchr(pt1+1,'>');
  *pt2 = '\0';
  do_convtime(pt1+1,&ttm);
  timeinfo = mktime(&ttm);
  strcat(obuff1,myitoa(timeinfo));
  strcat(obuff1,"/");
  in2 = master[from].sndix;
  master[from].sndix += 1;
  strcat(obuff1,myitoa(in2));
  strcat(obuff1,"/");
  pt1 = strchr(pt2+1,'(');
  *pt1 = '\0';
  pt2 = strchr(pt1+1,' ');
  *pt2 = '\0';
  strcat(obuff1,pt1+1);
  strcat(obuff1,"/");
  pt1 = obuff1+strlen(obuff1);
  *pt1 = buff[0];
  *(pt1+1) = buff[2];
  if (buff[1] == 'F')
    *(pt1+2) = 'F';
  else
    *(pt1+2) = 'N';
  *(pt1+3) = '\0';
  newinfo = malloc(strlen(obuff1) + 1);
  if (!newinfo) {
    fprintf(stderr,"Big problem, no memory\n");
    exit(4);
  }
  strcpy(newinfo,obuff1);
  push_head(index,newinfo,player,from,in2);
}


void proc_mesg()
{
  int player;
  short int index;
  char *pt1;

  pt1 = strchr(buff+1,'i');
  *pt1 = '\0';
  player = atoi(buff+1);
  index = (short int)atoi(pt1+1);
  readin(0);
  pt1 = malloc(strlen(buff) + 1);
  if (!pt1) {
    fprintf(stderr,"Big problems, no memory\n");
    exit(4);
  }
  strcpy(pt1,buff);
  push_mesg(index,pt1,player);
}

void output()
{
  int x, y, z;
  HSTRUCT *pt1;
  MSTRUCT *pt2;

  strcpy(fbuff2,fbuff);
  strcat(fbuff,".1");
  strcat(fbuff2,".2");
  filept = fopen(fbuff,"w+");
  filept2 = fopen(fbuff2,"w+");
  if (!filept || !filept2) {
    fprintf(stderr,"Can't open output file\n");
    exit(5);
  }
  for (x = 0; x < BIGNUM; x++) {
    if (master[x].header) {
      for (pt1 = master[x].header, y = 1; pt1; pt1 = pt1->next, y++) {
	pt2 = find_msg(x,pt1->oldix);
	if (pt2) {
	  *obuff1 = 'E';
	  strcpy(obuff1+1,myitoa(pt1->from));
	  strcat(obuff1,"/");
	  strcat(obuff1,myitoa(pt1->newix));
	  strcat(obuff1,"\1\n");
	  fputs(obuff1,filept);
	  fputs(pt2->data,filept);
	  fputs("\1\n",filept);
	  *obuff1 = 'D';
	  fputs(obuff1,filept);
	  fputs("1/",filept);
	  fputs(myitoa(x),filept);
	  fputc('/',filept);
	  fputs(myitoa(y),filept);
	  fputs("\1\n",filept);
	}
	fputc('C',filept);
	fputs(myitoa(x),filept);
	fputc('/',filept);
	fputs(myitoa(y),filept);
	fputs("\1\n",filept);
	fputs(pt1->data,filept);
	fputs("\1\n",filept);
      }
      strcpy(obuff1,myitoa(y-1));
      strcat(obuff1,"/");
      for (z = 1; z < (y-1); z++) {
	strcat(obuff1,myitoa(z));
	strcat(obuff1,"/");
      }
      strcat(obuff1,myitoa(z));
      strcat(obuff1,"\1\n");
      fputc('B',filept2);
      fputs(myitoa(x),filept2);
      fputc('/',filept2);
      fputs("Incoming\1\n",filept2);
      fputs(obuff1,filept2);
      fputc('A',filept);
      fputs(myitoa(x),filept);
      fputs("\1\n",filept);
      fputs(myitoa(y),filept);
      fputs("/0/",filept);
      fputs(obuff1,filept);
    }
    if (master[x].sndix > 1) {
      fputc('B',filept);
      fputs(myitoa(x),filept);
      fputs("\1\n",filept);
      fputs(myitoa(master[x].sndix),filept);
      fputs("/0/",filept);
      fputs(myitoa(master[x].sndix-1),filept);
      fputc('/',filept);
      for (z = 1; z < (master[x].sndix - 1); z++) {
        fputs(myitoa(z),filept);
        fputc('/',filept);
      }
      fputs(myitoa(z),filept);
      fputs("\1\n",filept);
    }
  }
  fclose(filept);
  fclose(filept2);
}

void main(int argc, char *argv[])
{

  if (argc != 2) {
    fprintf(stderr,"Incorrect number of arguments\n");
    exit(1);
  }
  strcpy(fbuff,argv[1]);
  filept = fopen(fbuff,"r");
  if (!filept) {
    fprintf(stderr,"Couldn't open file for reading\n");
    exit(2);
  }
  setup();
  while ((buff[0] = fgetc(filept)) != EOF) {
    readin(1);
    while ((buff[0] != 'm') && (buff[0] != 'h')) {
      readin(0);
      if ((buff[0] = fgetc(filept)) == EOF)
	break;
      readin(1);
    }
    if (buff[0] == EOF)
      break;
    if (buff[0] == 'h')
      proc_head();
    else
      proc_mesg();
  }
  fclose(filept);
  output();
  cleanup();
}
