/* mkhtml.c -- convert help/news files to html */

#include "autoconf.h"

#include  "help.h"

/* This code is very wastefull of memory, but hey its just a quicky program */

#define MAXTOPICS  1500
#define MAXTEXTLEN 64000

char bigtextbuff[MAXTEXTLEN];

help_indx alltopics[MAXTOPICS];
help_indx alltopicslc[MAXTOPICS];

int ntopics = 0;

/* Slow bubble sorts R' us */
void sorttopics( void )
{
  int j, k;
  help_indx temp;

  for( j = 0; j < ntopics; j++ ) {
    for( k = j + 1; k < ntopics; k++ ) {
      if( strcmp(alltopicslc[j].topic, alltopicslc[k].topic) > 0 ) {
        memcpy( &temp, &alltopicslc[j], sizeof(help_indx));
        memcpy( &alltopicslc[j], &alltopicslc[k], sizeof(help_indx));
        memcpy( &alltopicslc[k], &temp, sizeof(help_indx));
        memcpy( &temp, &alltopics[j], sizeof(help_indx));
        memcpy( &alltopics[j], &alltopics[k], sizeof(help_indx));
        memcpy( &alltopics[k], &temp, sizeof(help_indx));
      }
    }
  }
}

void outputurltext( FILE* hfp, char *str )
{
  char *pp;

  for( pp = str; *pp; pp++ ) {
    switch(*pp) {
      case '&':
        fprintf(hfp, "AMPER");
        break;
      case '"':
        fprintf(hfp, "QUOTE");
        break;
      case '$':
        fprintf(hfp, "DOLAR");
        break;
      case '<':
        fprintf(hfp, "LT");
        break;
      case '>':
        fprintf(hfp, "GT");
        break;
      case '!':
        fprintf(hfp, "BANG");
        break;
      case '+':
        fprintf(hfp, "PLUS");
        break;
      case '-':
        fprintf(hfp, "MINUS");
        break;
      default:
        fputc(*pp, hfp);
        break;
    }
  }
}

void outputstring( FILE* hfp, char *str )
{
  char *pp;

  for( pp = str; *pp; pp++ ) {
    switch(*pp) {
      case '&':
        fprintf(hfp, "&amp;");
        break;
      case '"':
        fprintf(hfp, "&quot;");
        break;
      case '<':
        fprintf(hfp, "&lt;");
        break;
      case '>':
        fprintf(hfp, "&gt;");
        break;
      default:
        fputc(*pp, hfp);
        break;
    }
  }
}

int main(int argc, char *argv[])
{
    int idx, ckval;
    int topicidx;
    int columns = 0;
    help_indx entry;
    FILE *tfp, *ifp, *hfp;
    time_t run_time;

    if ( argc != 4 ) {
	printf("Usage:\tmkhtml <txt_file> <indx_file> <output_html_file>\n");
	exit(-1);
    }
    if ((tfp = fopen(argv[1], "r")) == NULL) {
	fprintf(stderr, "can't open %s for reading\n", argv[1]);
	exit(-1);
    }
    if ((ifp = fopen(argv[2], "r")) == NULL) {
	fprintf(stderr, "can't open %s for reading\n", argv[2]);
	exit(-1);
    }
    if ((hfp = fopen(argv[3], "w")) == NULL) {
	fprintf(stderr, "can't open %s for writing\n", argv[2]);
	exit(-1);
    }
    printf("Scanning topics...\n");
    
    while (fread(&alltopics[ntopics], sizeof(entry), 1, ifp) != 0) {
      memcpy(&alltopicslc[ntopics], &alltopics[ntopics], sizeof(help_indx));
      for( idx = 0; idx < strlen(alltopicslc[ntopics].topic); idx++ ) {
        alltopicslc[ntopics].topic[idx] = 
          tolower(alltopicslc[ntopics].topic[idx]);
      }
      ++ntopics;
      if( ntopics >= MAXTOPICS ) {
        fprintf(stderr, "Too many topics to convert.\n");
        exit(-1);
      }
    }

    sorttopics();

    printf("Converting topics...\n");

    fprintf(hfp, "<HTML><HEAD><TITLE>RhostMUSH Help File [HTML Version]"
                 "</TITLE></HEAD><BODY>\n"
                 "<H1>RhostMUSH Help File [HTML Version]</H1>\n");

    run_time = time(NULL);
    fprintf(hfp, "Generated: %s", ctime(&run_time) );

    fprintf(hfp, "<HR><A NAME=\"topic index\"><H2>Topic Index</H2></A><PRE>\n");

    for( topicidx = 0; topicidx < ntopics; topicidx++ ) {
      fprintf(hfp, "<A HREF=\"#");
      outputurltext(hfp, alltopicslc[topicidx].topic);
      fprintf(hfp, "\">");
      outputstring(hfp, alltopics[topicidx].topic);
      fprintf(hfp, "</A>");

      if( ++columns == 3 ) {
        columns = 0;
        fprintf(hfp, "\n");
      }
      else {
        ckval = strlen(alltopics[topicidx].topic);
        if ( ckval > 26 )
           ckval = 26;
        for( idx = 0; idx < 26 - ckval; idx++ ) { 
          fprintf(hfp, " ");
        }
      }
    }

    for( topicidx = 0; topicidx < ntopics; topicidx++ ) {
      fprintf(hfp, "<HR><A NAME=\"");
      outputurltext(hfp, alltopicslc[topicidx].topic);
      fprintf(hfp, "\"><H3>");
      outputstring(hfp, alltopics[topicidx].topic);
      fprintf(hfp, "</H3></A><PRE>\n");

      fseek(tfp, alltopics[topicidx].pos, SEEK_SET);

      if( alltopics[topicidx].len >= MAXTEXTLEN - 1 ) {
        fprintf(stderr, "Topic too long: %s\n", alltopics[topicidx].topic);
        exit(-1);
      }
     
/*
      fread(bigtextbuff, alltopics[topicidx].len, 1, tfp);

      bigtextbuff[alltopics[topicidx].len] = '\0';

      outputstring(hfp, bigtextbuff);
*/
      for (;;) {
         if ( fgets(bigtextbuff, (MAXTEXTLEN - 1), tfp) == NULL )
            break;
         if (bigtextbuff[0] == '&')
            break;
         outputstring(hfp, bigtextbuff);
      }

      fprintf(hfp, "</PRE>\n"/*, entry.topic ??? */);
      if( topicidx ) {
        fprintf(hfp, "<A HREF=\"#");
        outputurltext(hfp, alltopicslc[topicidx-1].topic);
        fprintf(hfp, "\">[PREV]</A>\n");
      }

      fprintf(hfp, "<A HREF=\"#topic index\">[TOP]</A>\n");

      if( topicidx < ntopics - 1) {
        fprintf(hfp, "<A HREF=\"#");
        outputurltext(hfp, alltopicslc[topicidx+1].topic);
        fprintf(hfp, "\">[NEXT]</A>\n");
      }
      fprintf(hfp, "<BR>\n");
    }
    fprintf(hfp, "</BODY></HTML>\n");
    fclose(tfp);
    fclose(ifp);
    fclose(hfp);

    printf("%d topics converted\n", ntopics);
    return 0;
}
