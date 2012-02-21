/* mkindx.c -- make help/news file indexes */

#include "copyright.h"
#include "autoconf.h"

#include  "help.h"

char line[LINE_SIZE + 1];
int main(int argc, char *argv[])
{
    long pos;
    int i, n, lineno, ntopics, naliases, topic_indx;
    char *s, *topic;
    help_indx entry[1000];
    FILE *rfp, *wfp;

    if (argc < 2 || argc > 3) {
	printf("Usage:\tmkindx <file_to_be_indexed> <output_index_filename>\n");
	exit(-1);
    }
    if ((rfp = fopen(argv[1], "r")) == NULL) {
	fprintf(stderr, "can't open %s for reading\n", argv[1]);
	exit(-1);
    }
    if ((wfp = fopen(argv[2], "w")) == NULL) {
	fprintf(stderr, "can't open %s for writing\n", argv[2]);
	exit(-1);
    }
    pos = 0L;
    lineno = 0;
    ntopics = 0;
    naliases = 0;
    topic_indx = 0;
    while (fgets(line, LINE_SIZE, rfp) != NULL) {
	++lineno;

	n = strlen(line);
	if (line[n - 1] != '\n') {
	    fprintf(stderr, "line %d: line too long\n", lineno);
	}
	if (line[0] == '&') {
	    for (topic = &line[1];
	      (*topic == ' ' || *topic == '\t') && *topic != '\0'; topic++);
	    for (i = -1, s = topic; *s != '\n' && *s != '\0'; s++) {
		if (i >= TOPIC_NAME_LEN - 1)
		    break;
		if (*s != ' ' || entry[topic_indx].topic[i] != ' ')
		    entry[topic_indx].topic[++i] = *s;
	    }
	    entry[topic_indx].topic[++i] = '\0';
            topic_indx++;
            if ( topic_indx >= 1000 ) {
               fprintf(stderr, "Maximum indexed aliases of 1000 reached.");
               fclose(rfp);
               fclose(wfp);
            }
	    ++ntopics;
	} else {
           for ( i = 0; i < topic_indx; i++ ) {
              entry[i].pos = pos;
              if (fwrite(&entry[i], sizeof(help_indx), 1, wfp) < 1) {
                 fprintf(stderr, "error writing %s\n", argv[2]);
                 exit(-1);
              }
           }
           if ( topic_indx > 1 ) {
              naliases = naliases + topic_indx - 1;
           }
           topic_indx = 0;
        }
	pos += n;
    }
    for ( i = 0; i < topic_indx; i++ ) {
       entry[i].pos = pos;
       if (fwrite(&entry[i], sizeof(help_indx), 1, wfp) < 1) {
          fprintf(stderr, "error writing %s\n", argv[2]);
          exit(-1);
       }
    }
    if ( topic_indx > 1 ) {
       naliases = naliases + topic_indx - 1;
    }
    fclose(rfp);
    fclose(wfp);

    printf("%d topics indexed (%d marked as topic aliases)\n", ntopics, naliases);
    return 0;
}
