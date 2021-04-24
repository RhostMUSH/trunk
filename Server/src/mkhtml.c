/* mkhtml.c -- convert help/news files to html */

#include "autoconf.h"

#include  "help.h"

/* This code is very wastefull of memory, but hey its just a quicky program */

#define MAXTOPICS  5000
#define MAXTEXTLEN 64000

/* Values used for CSS and HTML output.
 *
 * Typically left as is and changed through additional CSS or JS.
 * Only need to change if not using these and don't like the default.
 *
 * Default: vt100 style output
 */

#define CSSFONT    "monospace"  /* Only use fixed-width fonts */
#define CSSBG      "#000000"    /* Black Background */
#define CSSFG      "#ffffff"    /* White Foreground */
#define CSSLINK    "#ffff00"    /* Yellow Links     */
#define CSSWIDTH   "60em"
#define HTMLTITLE  "RhostMUSH Help File [HTML Version]"
#define HTMLH1     HTMLTITLE    /* Does not need to match title */
#define HTMLH2     "Topic Index"

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
    int idx;
    int topicidx;
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

/* FILE OUTPUT: BEGIN */

/* HTML HEAD */

    fprintf(hfp, "<!DOCTYPE html>\n"
                 "<html lang=\"en\">\n"
                 "<head>\n"
                 "<meta charset=\"utf-8\" />\n"
                 "<style type=\"text/css\">\n"
                 "body {\n"
                 "\tbackground: %s;\n"
                 "\tcolor: %s;\n"
                 "\tfont-family: %s;\n"
                 "}\n\n"
                 "a, a:hover, a:visited {\n"
                 "\tcolor: %s;\n"
                 "}\n\n"
                 "code {\n"
                 "\twhite-space: pre;\n"
                 "}\n\n"
                 "#help-toc,\n"
                 "\t.help-topic,\n"
                 "\t.help-navigation {\n"
                 "\twidth: %s;\n"
                 "}\n\n"
                 "#help-toc ul {\n"
                 "\tlist-style-type: none;\n"
                 "\tmargin: 0;\n"
                 "\tpadding: 0;\n"
                 "\tcolumns: 3;\n"
                 "}\n"
                 "</style>\n"
                 "<title>%s</title>\n"
                 "</head>\n\n",
	    CSSBG, CSSFG, CSSFONT, CSSLINK, CSSWIDTH, HTMLTITLE);

/* HTML BODY */

    fprintf(hfp, "<body>\n"
		 "<h1>%s</h1>\n\n",
            HTMLH1);

    run_time = time(NULL);
    fprintf(hfp, "Generated: %s\n", ctime(&run_time) );

/* HTML BODY: Table of Contents */

    fprintf(hfp, "<div id=\"help-toc\">\n"
                 "\t<h2>%s</h2>\n"
                 "\t<ul>\n",
	    HTMLH2);

    for( topicidx = 0; topicidx < ntopics; topicidx++ ) {
      fprintf(hfp, "\t\t<li><a href=\"#");
      outputurltext(hfp, alltopicslc[topicidx].topic);
      fprintf(hfp, "\">");
      outputstring(hfp, alltopics[topicidx].topic);
      fprintf(hfp, "</a></li>\n");
    }
    
    fprintf(hfp, "\t</ul>\n"
                 "</div>\n\n");

/* HTML BODY: Help Topics */

/* HELP TOPIC: Topic Title */

    for( topicidx = 0; topicidx < ntopics; topicidx++ ) {
      fprintf(hfp, "<div class=\"help-topic\" id=\"");
      outputurltext(hfp, alltopicslc[topicidx].topic);
      fprintf(hfp, "\">\n"
                   "\t<h3>");
      outputstring(hfp, alltopics[topicidx].topic);
      fprintf(hfp, "</h3>\n");

      fseek(tfp, alltopics[topicidx].pos, SEEK_SET);

      if( alltopics[topicidx].len >= MAXTEXTLEN - 1 ) {
        fprintf(stderr, "Topic too long: %s\n", alltopics[topicidx].topic);
        exit(-1);
      }

/* HELP TOPIC: Topic Content */

      fprintf(hfp, "\t<code>\n");

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
     
      fprintf(hfp, "\t</code>\n"
                   "</div>\n\n");

/* HTML BODY: Help Navigation */

      fprintf(hfp, "<div class=\"help-navigation\">\n");
      if( topicidx ) {
        fprintf(hfp, "\t<a href=\"#");
        outputurltext(hfp, alltopicslc[topicidx-1].topic);
        fprintf(hfp, "\">[PREV]</a>\n");
      }

      fprintf(hfp, "\t<a href=\"#help-toc\">[TOP]</a>\n");

      if( topicidx < ntopics - 1) {
        fprintf(hfp, "\t<a href=\"#");
        outputurltext(hfp, alltopicslc[topicidx+1].topic);
        fprintf(hfp, "\">[NEXT]</a>\n");
      }
      fprintf(hfp, "</div>\n\n");
    }
    fprintf(hfp, "</body>\n"
                 "</html>\n");
    fclose(tfp);
    fclose(ifp);
    fclose(hfp);

/* FILE OUTPUT: END */

    printf("%d topics converted\n", ntopics);
    return 0;
}
