/* help.c -- commands for giving help */

#include "copyright.h"
#include "autoconf.h"

#include <fcntl.h>

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "interface.h"
#include "externs.h"
#include "help.h"
#include "htab.h"
#include "alloc.h"
#include "rhost_ansi.h"

extern void fun_strdistance(char *, char **, dbref, dbref, dbref, char **, int, char **, int);
extern void do_regedit(char *, char **, dbref, dbref, dbref, char **, int, char **, int, int);
extern int FDECL(parse_dynhelp, (dbref, dbref, int, char *, char *, char *, char *, int, int, char *));

int
pstricmp(char *buf1, char *buf2, int len)
{
    char *p1, *p2;
    int cntr = 0;

    p1 = buf1;
    p2 = buf2;
    if ( len < 1 )
       return 0;
    while ( (cntr < len) && (*p1 != '\0') && (*p2 != '\0') && (tolower(*p1) == tolower(*p2))) {
        p1++;
        p2++;
        cntr++;
    }
    if ( cntr == len )
        return 0;
    if ((*p1 == '\0') && (*p2 == '\0'))
        return 0;
    if (*p1 == '\0')
        return -1;
    if (*p2 == '\0')
        return 1;
    if (*p1 < *p2)
        return -1;
    return 1;
}

/* Pointers to this struct is what gets stored in the help_htab's */
struct help_entry {
    int pos;			/* Position, copied from help_indx */
    char original;		/* 1 for the longest name for a topic.
				   0 for abbreviations */
    char *key;			/* The key this is stored under. */
    char *keyorig;		/* The uncut original key */
};

int 
helpindex_read(HASHTAB * htab, char *filename)
{
    help_indx entry;
    char *p, fullFilename[129 + 32], *korig;
    int count;
    FILE *fp;
    struct help_entry *htab_entry;

    for (htab_entry = (struct help_entry *) hash_firstentry(htab);
	htab_entry;
	htab_entry = (struct help_entry *) hash_nextentry(htab)) {
	free(htab_entry->key);
	free(htab_entry->keyorig);
	free(htab_entry);
    }

    hashflush(htab, 0);
    sprintf(fullFilename, "%s/%s", mudconf.txt_dir, filename);
    if ((fp = tf_fopen(fullFilename, O_RDONLY)) == NULL) {
	STARTLOG(LOG_PROBLEMS, "HLP", "RINDX")
	    p = alloc_lbuf("helpindex_read.LOG");
	sprintf(p, "Can't open %.3900s for reading.", filename);
	log_text(p);
	free_lbuf(p);
	ENDLOG
	    return -1;
    }
    count = 0;
    while ((fread((char *) &entry, sizeof(help_indx), 1, fp)) == 1) {

	/* Lowercase the entry and add all leftmost substrings.
	 * Substrings already added will be rejected by hashadd.
	 */
	for (p = entry.topic; *p; p++)
	    *p = ToLower((int)*p);

	htab_entry = (struct help_entry *) malloc(sizeof(struct help_entry));

	htab_entry->pos = entry.pos;
	htab_entry->original = 1;	/* First is the longest */
	htab_entry->key = (char *) malloc(strlen(entry.topic) + 1);
	htab_entry->keyorig = (char *) malloc(strlen(entry.topic) + 1);
	strcpy(htab_entry->key, entry.topic);
	strcpy(htab_entry->keyorig, entry.topic);
        korig = (char *) malloc(strlen(entry.topic) + 1);
	strcpy(korig, entry.topic);
	while (p > entry.topic) {
	  p--;
	  if (!isspace((int)*p) && (hashadd2(entry.topic, (int *) htab_entry, htab, htab_entry->original) == 0)) 
	    count++;
	  else {
	    free(htab_entry->key);
	    free(htab_entry->keyorig);
	    free(htab_entry);
	  }
	  *p = '\0';
	  htab_entry = (struct help_entry *) malloc(sizeof(struct help_entry));
	  
	  htab_entry->pos = entry.pos;
	  htab_entry->original = 0;
	  htab_entry->key = (char *) malloc(strlen(entry.topic) + 1);
	  htab_entry->keyorig = (char *) malloc(strlen(korig) + 1);
	  strcpy(htab_entry->key, entry.topic);
	  strcpy(htab_entry->keyorig, korig);
	}
	free(htab_entry->key);
	free(htab_entry->keyorig);
	free(htab_entry);
        free(korig);
    }
    tf_fclose(fp);
    hashreset(htab);
    return count;
}

void 
helpindex_load(dbref player)
{
#ifndef PLUSHELP
    int news, help, whelp, err;
#else
    int news, help, whelp, err, plushelp;
#endif

    news = helpindex_read(&mudstate.news_htab, mudconf.news_indx);
    help = helpindex_read(&mudstate.help_htab, mudconf.help_indx);
    whelp = helpindex_read(&mudstate.wizhelp_htab, mudconf.whelp_indx);
    err = helpindex_read(&mudstate.error_htab, mudconf.error_indx);
#ifdef PLUSHELP
    plushelp = helpindex_read(&mudstate.plushelp_htab, mudconf.plushelp_indx);
#endif
    if ((player != NOTHING) && !Quiet(player))
#ifndef PLUSHELP
	notify(player,
	       unsafe_tprintf("Index entries: News...%d  Help...%d  Wizhelp...%d  Error...%d",
		       news, help, whelp, err));
#else
	notify(player,
	       unsafe_tprintf("Index entries: News...%d  Help...%d  Wizhelp...%d  Error...%d Plushelp...%d",
		       news, help, whelp, err, plushelp));
#endif
    mudstate.errornum = err;
}

void 
NDECL(helpindex_init)
{
    hashinit(&mudstate.news_htab, 521);
    hashinit(&mudstate.help_htab, 5147);
    hashinit(&mudstate.wizhelp_htab, 5147);
    hashinit(&mudstate.error_htab, 521);
#ifdef PLUSHELP
    hashinit(&mudstate.plushelp_htab, 521);
#endif
    helpindex_load(NOTHING);
}

void 
help_write(dbref player, char *topic, HASHTAB * htab, char *filename, int key)
{
    FILE *fp;
    char *p, *line, *sh_key1, *sh_key2, *sh_tmp;
    int offset, i_first, i_found, matched, i, i_tier0, i_tier1, i_tier2, i_tier3, i_header,
        i_cntr, i_tier0chk, i_shkey, i_shcnt, i_shflag, i_magic;
    struct help_entry *htab_entry;
    char *topic_list, *buffp, *mybuff, *myp, *help_array[4], *s_buff2, *s_buff2ptr;
    char realFilename[129 + 32], *s_tmpbuff, *s_ptr, *s_hbuff, *s_hbuff2;
    char *s_tier0[3], *s_tier1[3], *s_tier2[3], *s_tier3[3], *s_buff, *s_buffptr, *s_nbuff[2];

    i_magic = 0;

    if (*topic == '\0')
	topic = (char *) "help";
    else
	for (p = topic; *p; p++)
	    *p = ToLower((int)*p);

    if ( key ) {
       sprintf(realFilename, "%s/%s", mudconf.txt_dir, filename);
       if ((fp = tf_fopen(realFilename, O_RDONLY)) == NULL) {
          notify(player, "Sorry, that function is temporarily unavailable.");
          STARTLOG(LOG_PROBLEMS, "HLP", "OPEN")
             line = alloc_lbuf("help_write.LOG.open");
             sprintf(line, "Can't open %.3900s for reading.", filename);
             log_text(line);
             free_lbuf(line);
          ENDLOG
          return;
       }
       line = alloc_lbuf("help_write");
       buffp = topic_list = alloc_lbuf("help_write");
       i_found = i_first = matched = 0;
       s_ptr = s_tmpbuff = alloc_lbuf("help_write_search");
       i_found = i_first = 0;
       s_buff = alloc_lbuf("help_query");
       s_buff2ptr = s_buff2 = alloc_lbuf("help_query2");
       i_cntr = 0;
       s_hbuff2 = alloc_lbuf("help_buff");
       if ( *(mudconf.help_separator) ) {
          strcpy(s_hbuff2, mudconf.help_separator);
          s_hbuff = exec(GOD, GOD, GOD, EV_FIGNORE | EV_EVAL | EV_NOFCHECK, s_hbuff2,
                         (char **)NULL, 0, (char **)NULL, 0);
          if ( !*s_hbuff ) {
             sprintf(s_hbuff2, "%s", (char *)"  ");
          } else {
             sprintf(s_hbuff2, "%s", s_hbuff);
          }
          free_lbuf(s_hbuff);
       } else {
          sprintf(s_hbuff2, "%s", (char *)"  ");
       }
       for (htab_entry = (struct help_entry *) hash_firstentry(htab);
            htab_entry != NULL;
            htab_entry = (struct help_entry *) hash_nextentry(htab)) {
          if ( !htab_entry->original )
             continue;
          if ( i_cntr > 2000 )
             continue;
          if (fseek(fp, htab_entry->pos, 0) < 0L) {
              notify(player,
                     "Sorry, that function is temporarily unavailable.");
              STARTLOG(LOG_PROBLEMS, "HLP", "SEEK")
                 line = alloc_lbuf("help_write.LOG.seek");
                 sprintf(line, "Seek error in file %.3900s.", filename);
                 log_text(line);
                 free_lbuf(line);
              ENDLOG
              free_lbuf(line);
              free_lbuf(s_buff);
              free_lbuf(s_buff2);
              free_lbuf(topic_list);
              free_lbuf(s_tmpbuff);
              tf_fclose(fp);
              return;
          }
          i_header = 0;
          for (;;) {
             if (fgets(line, LBUF_SIZE - 1, fp) == NULL)
                break;
             if (line[0] == '&')
                break;
             for (p = line; *p != '\0'; p++) {
                *p = ToLower((int)*p);
                if ( (*p == '\n') || (*p == '\r') )
                   *p = '\0';
             }
             if ( i_first ) 
                safe_chr(' ', s_tmpbuff, &s_ptr);
             i_first = 1;
             safe_str(line, s_tmpbuff, &s_ptr);
             if ( quick_wild( topic, line ) ) {
                if ( key == 2 ) {
                   if ( !i_header ) {
#ifdef ZENTY_ANSI
                      sprintf(s_buff, "%s%s%s:", SAFE_ANSI_HILITE, htab_entry->key, SAFE_ANSI_NORMAL);
#else
                      sprintf(s_buff, "%s%s%s:", ANSI_HILITE, htab_entry->key, ANSI_NORMAL);
#endif
                      notify(player, s_buff);
                      i_header = 1;
                   }
#ifdef ZENTY_ANSI
                  sprintf(s_buff, "%s$0%s", SAFE_ANSI_RED, SAFE_ANSI_NORMAL);
#else
                  sprintf(s_buff, "%s$0%s", ANSI_RED, ANSI_NORMAL);
#endif
                   s_buffptr = topic;
                   memset(s_buff2, '\0', LBUF_SIZE);
                   s_buff2ptr = s_buff2;
                   i_magic = 0;
                   while ( *s_buffptr ) {
                      if ( *s_buffptr == '*' ) {
                         if ( isprint(*(s_buffptr+1)) ) {
                            if ( i_magic ) {
                               safe_str((char *)".*", s_buff2, &s_buff2ptr);
                            }
                         }
                         s_buffptr++;
                         continue;
                      }
                      i_magic = 1;
                      if ( *s_buffptr == '?' ) {
                         safe_chr('.', s_buff2, &s_buff2ptr);
                      } else {
                         safe_chr(*s_buffptr, s_buff2, &s_buff2ptr);
                      }
                      s_buffptr++;
                   }
                   help_array[0] = line;
                   help_array[1] = s_buff2;
                   help_array[2] = s_buff;
                   help_array[3] = NULL;
                   buffp = topic_list;
                   do_regedit(topic_list, &buffp, GOD, GOD, GOD, help_array, 3, (char **)NULL, 0, 8);
                   sprintf(s_buff, "   ...%.*s", (LBUF_SIZE - 20), topic_list);
                   notify(player, s_buff);
                   i_found = matched = 1;
                   i_cntr++;
                } else {
                   if ( i_found ) {
                      safe_str(s_hbuff2, topic_list, &buffp);
                   }
                   i_found = matched = 1;
		   safe_str(htab_entry->key, topic_list, &buffp);
                   break;
                }
             }
          }
          if ( (key != 2) && (!i_found && quick_wild(topic, s_tmpbuff)) ) {
             if ( matched ) {
                safe_str(s_hbuff2, topic_list, &buffp);
             }
             matched = 1;
             safe_str(htab_entry->key, topic_list, &buffp);
          }
          memset(s_tmpbuff, '\0', LBUF_SIZE);
          s_ptr = s_tmpbuff;
       }
       free_lbuf(s_tmpbuff);
       if (matched == 0) {
          notify(player, unsafe_tprintf("No entry for contents '%s'.", topic));
       } else if ( key != 2 ) {
          notify(player, unsafe_tprintf("Here are the entries which contain '%s':", topic));
          *buffp = '\0';
          notify(player, topic_list);
       }
       free_lbuf(topic_list);
       free_lbuf(line);
       free_lbuf(s_buff);
       free_lbuf(s_buff2);
       tf_fclose(fp);
       if ( i_cntr > 2000 ) {
          notify(player, "Warning: /query matches discarded after 2000 matches.");
       }
       free_lbuf(s_hbuff2);
       return;
    }

    htab_entry = (struct help_entry *) hashfind(topic, htab);
    if (htab_entry) {
	offset = htab_entry->pos;
    } else {
	matched = 0;
        i_tier0 = i_tier1 = i_tier2 = i_tier3 = 0;
        for (i=0; i < 3; i++ ) {
           s_tier0[i] = alloc_sbuf("s_tier1");
           s_tier1[i] = alloc_sbuf("s_tier1");
           s_tier2[i] = alloc_sbuf("s_tier2");
           s_tier3[i] = alloc_sbuf("s_tier3");
        }
        s_buffptr = s_buff = alloc_lbuf("help_topical");
        s_tmpbuff = alloc_lbuf("help_wild_checker");
        s_hbuff2 = alloc_lbuf("help_buff");
        if ( *(mudconf.help_separator) ) {
           strcpy(s_hbuff2, mudconf.help_separator);
           s_hbuff = exec(GOD, GOD, GOD, EV_FIGNORE | EV_EVAL | EV_NOFCHECK, s_hbuff2,
                          (char **)NULL, 0, (char **)NULL, 0);
           if ( !*s_hbuff ) {
              sprintf(s_hbuff2, "%s", (char *)"  ");
           } else {
              sprintf(s_hbuff2, "%s", s_hbuff);
           }
           free_lbuf(s_hbuff);
        } else {
           sprintf(s_hbuff2, "%s", (char *)"  ");
        }
	for (htab_entry = (struct help_entry *) hash_firstentry(htab);
	     htab_entry != NULL;
	     htab_entry = (struct help_entry *) hash_nextentry(htab)) {
	    if (htab_entry->original &&
		quick_wild(topic, htab_entry->key)) {
		if (matched == 0) {
		    topic_list = alloc_lbuf("help_write");
		    buffp = topic_list;
		}
                if ( matched ) {
		   safe_str(s_hbuff2, topic_list, &buffp);
                }
		matched = 1;
		safe_str(htab_entry->key, topic_list, &buffp);
            }
            if ( !matched && !strcmp(htab_entry->key, htab_entry->keyorig) ) {
               s_nbuff[0] = topic;
               s_nbuff[1] = htab_entry->keyorig;
               s_buffptr = s_buff;
               i_tier0chk = 0;
               if ( i_tier0 < 3 ) {
                  sprintf(s_tmpbuff, "*%.*s*", LBUF_SIZE-100, topic);
                  if ( quick_wild(s_tmpbuff, htab_entry->keyorig) ) {
                     sprintf(s_tier0[i_tier0], "%.*s", SBUF_SIZE - 1, s_nbuff[1]);
                     i_tier0++;
                     i_tier0chk = 1;
                  }
               }
               if ( !i_tier0chk ) {
                  fun_strdistance(s_buff, &s_buffptr, 1, 1, 1, s_nbuff, 2, (char **)NULL, 0);
                  switch(atoi(s_buff)) {
                     case 1: /* case 1 */
                        if ( i_tier1 < 3 ) {
                           sprintf(s_tier1[i_tier1], "%.*s", SBUF_SIZE - 1, s_nbuff[1]);
                           i_tier1++;
                        }
                        break;
                     case 2: /* case 2 */
                        if ( i_tier2 < 3 ) {
                           sprintf(s_tier2[i_tier2], "%.*s", SBUF_SIZE - 1, s_nbuff[1]);
                           i_tier2++;
                        }
                        break;
                     case 3: /* case 3 */
                        if ( i_tier3 < 3 ) {
                           sprintf(s_tier3[i_tier3], "%.*s", SBUF_SIZE - 1, s_nbuff[1]);
                           i_tier3++;
                        }
                        break;
                  }
               }
            }
	}
        free_lbuf(s_tmpbuff);
	if (matched == 0) {
            if ( i_tier0 || i_tier1 || i_tier2 || i_tier3 ) {
               s_buffptr = s_buff;
	       safe_str(unsafe_tprintf("No entry for '%s'.  Suggestions:  ", topic), s_buff, &s_buffptr);
               if ( i_tier0 > 0 ) {
                  for (i=0; i<i_tier0; i++) {
                     if ( matched ) {
                        safe_str(s_hbuff2, s_buff, &s_buffptr);
                     }
                     safe_str(s_tier0[i], s_buff, &s_buffptr);
                     matched = 1;
                  }
               }
               if ( i_tier1 > 0 ) {
                  for (i=0; i<i_tier1; i++) {
                     if ( matched ) {
                        safe_str(s_hbuff2, s_buff, &s_buffptr);
                     }
                     safe_str(s_tier1[i], s_buff, &s_buffptr);
                     matched = 1;
                  }
               }
               if ( !i_tier1 && (i_tier2 > 0) ) {
                  for (i=0; i<i_tier2; i++) {
                     if ( matched ) {
                        safe_str(s_hbuff2, s_buff, &s_buffptr);
                     }
                     safe_str(s_tier2[i], s_buff, &s_buffptr);
                     matched = 1;
                  }
               }
               if ( !i_tier1 && !i_tier2 && (i_tier3 > 0) ) {
                  for (i=0; i<i_tier3; i++) {
                     if ( matched ) {
                        safe_str(s_hbuff2, s_buff, &s_buffptr);
                     }
                     safe_str(s_tier3[i], s_buff, &s_buffptr);
                     matched = 1;
                  }
               }
               notify(player, s_buff);
            } else {
	       notify(player, unsafe_tprintf("No entry for '%s'.  No suggestions available.", topic));
            }
	} else {
	    notify(player, unsafe_tprintf("Here are the entries which match '%s':", topic));
	    *buffp = '\0';
	    notify(player, topic_list);
	    free_lbuf(topic_list);
	}
        for (i=0; i<3; i++) {
           free_sbuf(s_tier0[i]);
           free_sbuf(s_tier1[i]);
           free_sbuf(s_tier2[i]);
           free_sbuf(s_tier3[i]);
        }
        free_lbuf(s_buff);
        free_lbuf(s_hbuff2);
	return;
    }

    sprintf(realFilename, "%s/%s", mudconf.txt_dir, filename);
    if ((fp = tf_fopen(realFilename, O_RDONLY)) == NULL) {
	notify(player,
	       "Sorry, that function is temporarily unavailable.");
	STARTLOG(LOG_PROBLEMS, "HLP", "OPEN")
	    line = alloc_lbuf("help_write.LOG.open");
	sprintf(line, "Can't open %.3900s for reading.", filename);
	log_text(line);
	free_lbuf(line);
	ENDLOG
	    return;
    }
    if (fseek(fp, offset, 0) < 0L) {
	notify(player,
	       "Sorry, that function is temporarily unavailable.");
	STARTLOG(LOG_PROBLEMS, "HLP", "SEEK")
	    line = alloc_lbuf("help_write.LOG.seek");
	sprintf(line, "Seek error in file %.3900s.", filename);
	log_text(line);
	free_lbuf(line);
	ENDLOG
	    tf_fclose(fp);
	return;
    }
    myp = mybuff = alloc_lbuf("ANSI_HELP");
#ifdef ZENTY_ANSI
    safe_str(SAFE_ANSI_HILITE, mybuff, &myp);
    for ( p = htab_entry->keyorig; *p; p++ ) {
       safe_chr(ToUpper(*p), mybuff, &myp);
    }
    safe_str(SAFE_ANSI_NORMAL, mybuff, &myp);
#else
    safe_str(ANSI_HILITE, mybuff, &myp);
    for ( p = htab_entry->keyorig; *p; p++ ) {
       safe_chr(ToUpper(*p), mybuff, &myp);
    }
    safe_str(ANSI_NORMAL, mybuff, &myp);
#endif
    notify(player, mybuff);
    free_lbuf(mybuff);
    line = alloc_lbuf("help_write");
    i_shkey = 0;
    for (;;) {
	if (fgets(line, LBUF_SIZE - 1, fp) == NULL)
	    break;
	if (line[0] == '&')
	    break;
        i_shkey++;
        if ( (line[0] == '!') && (line[1] == '!') && (strchr(line, '/') != NULL) ) {
           i_shflag = 0;
           mudstate.help_shell++;
           sh_key1 = alloc_lbuf("help_shell1");
           sh_key2 = alloc_lbuf("help_shell1");
           sh_tmp = strchr(line, '/');           
           *sh_tmp = '\0';
           switch (line[2]) {
              case '~': /* Parse */
                 i_shflag = DYN_PARSE;
                 sprintf(sh_key1, "%.*s", LBUF_SIZE - 10, line+3);
                 break;
              case '-': /* No-Eval */
                 i_shflag = DYN_SUBEVAL;
                 sprintf(sh_key1, "%.*s", LBUF_SIZE - 10, line+3);
                 break;
              default: /* Handle as normal */
                 sprintf(sh_key1, "%.*s", LBUF_SIZE - 10, line+2);
                 break;
           }
           *sh_tmp = '/';
           sprintf(sh_key2, "%.*s", LBUF_SIZE - 10, sh_tmp+1);
	   for (p = sh_key2; *p != '\0'; p++) {
	      if ( (*p == '\n') || (*p == '\r') ) {
		 *p = '\0';
              }
           }
           /* Warning: parse_dynhelp will close existing file pointer -- it will handle empty args to sh_key1 and sh_key2 */
           parse_dynhelp(player, player, (DYN_NOLABEL|i_shflag), sh_key1, sh_key2, (char *)NULL, (char *)NULL, 0, 0, (char *)NULL);
           free_lbuf(sh_key1);
           free_lbuf(sh_key2);

           /* We must reopen the file here and re-seek + offset since parse_dynhelp and help_write
            * share a common file pointer which is closed than opened per call 
            */

           /* Re-open original file */
           if ( (fp = tf_fopen(realFilename, O_RDONLY)) == NULL) {
              /* This shouldn't happen but we need to cover a falied re-open for paranoia */
              notify(player, "#-1 FAILURE TO RE-READ HELP FILE.");
              i_shkey = -1;  /* Set bypass since file pointer is not open at this point */
              mudstate.help_shell--;
              break;
           }

           /* This is fine as we seeked this before */
           if (fseek(fp, offset, 0) < 0L) {
              /* This shouldn't happen but we need to cover a falied re-open for paranoia */
              notify(player, "#-1 FAILURE TO RE-SEEK HELP FILE.");
              mudstate.help_shell--;
              break;
           }
           for ( i_shcnt = 0; i_shcnt < i_shkey; i_shcnt++ ) {
              /* Insurance we don't reach the end of the list */
              if (fgets(line, LBUF_SIZE - 1, fp) == NULL)
                 break;
           } 
           mudstate.help_shell--;
           continue;
        } 
	for (p = line; *p != '\0'; p++)
	    if ( (*p == '\n') || (*p == '\r') )
		*p = '\0';
	noansi_notify(player, line);
    }
    free_lbuf(line);
    /* Only close the file if it's opened */
    if ( i_shkey != -1 ) {
       tf_fclose(fp);
    }
}

/* ---------------------------------------------------------------------------
 * do_help: display information from new-format news and help files
 */

int
parse_dynhelp(dbref player, dbref cause, int key, char *fhelp, char *msg2, 
              char *t_buff, char *t_bufptr, int t_val, int i_type, char *sep)
{
   help_indx entry;
   char *tmp, *p_tmp, *p, *q, *line, *msg, *p_msg, *result, *tpass;
   char filename[129 + 40], *mybuff, *myp;
   char *s_tier0[3], *s_tier1[3], *s_tier2[3], *s_tier3[3], *s_tmpbuff, *s_buff2,
        *s_buff, *s_buffptr, *s_nbuff[2], *s_hbuff, *s_hbuff2, *help_array[4], *s_buff2ptr; 
   int first, found, matched, one_through, space_compress, i_noindex, i_header, i_magic;
   int i_tier0, i_tier1, i_tier2, i_tier3, i_suggest, i, i_cntr, i_tier0chk, i_bufffilled;
   FILE *fp_indx, *fp_help;

   /* Recursion isn't possible right now, but when it is we want this */
   if ( mudstate.help_shell > 10 ) {
      notify(player, "#-1 TOO MUCH RECURSION IN HELP ENTRY");
      return(1);
   }

   if ( ((key & DYN_SEARCH) || (key & DYN_QUERY)) && (key & DYN_NOLABEL) ) {
      if ( t_val ) {
         safe_str("Illegal combination of switches.", t_buff, &t_bufptr);
      } else {
         notify_quiet(player, "Illegal combination of switches.");
      }
      return(1);
   }

   i_magic = i_bufffilled = 0;

   i_noindex = i_suggest = 0;
   if ( key & DYN_NOLABEL ) {
      key &= ~DYN_NOLABEL;
      i_noindex = 1;
   }
   if ( key & DYN_SUGGEST ) {
      key &= ~DYN_SUGGEST;
      i_suggest = 1;
   }
   fp_help = NULL;
   memset(filename, 0, sizeof(filename));
   tpass = fhelp;
   while (*tpass) {
     if (*tpass == '^')
        *tpass = '/';
     if (*tpass == '.')
        *tpass = '_';
     tpass++;
   }
   sprintf(filename, "%.128s/%.32s.indx", mudconf.txt_dir, fhelp);
   p_msg = msg = alloc_lbuf("temp_messaging.help");
   if ( !*msg2 || !msg2 )
      safe_str("help", msg, &p_msg);
   else
      safe_str(msg2, msg, &p_msg);
   if ( (fp_indx = tf_fopen(filename, O_RDONLY)) == NULL ) {
      if ( t_val )
         safe_str("Sorry, that file doesn't seem to exist.", t_buff, &t_bufptr);
      else
         notify(player, "Sorry, that file doesn't seem to exist.");
      free_lbuf(msg);
      return(1);
   }
   p_tmp = tmp = alloc_lbuf("topic_listing.help");
   for (q = msg; *q; q++)
       *q = ToLower((int)*q);
   found = first = matched = 0;


   if ( (key & DYN_SEARCH) || (key & DYN_QUERY) ) {
      sprintf(filename, "%.128s/%.32s.txt", mudconf.txt_dir, fhelp);
      if ( (fp_help = fopen(filename, "r")) == NULL ) {
         if ( t_val )
            safe_str("Sorry, that file doesn't seem to exist.", t_buff, &t_bufptr);
         else
            notify(player, "Sorry, that file doesn't seem to exist.");
         free_lbuf(msg);
         free_lbuf(tmp);
         tf_fclose(fp_indx);
         return(1);
      }
      line = alloc_lbuf("help_write");
      memset(line, '\0', LBUF_SIZE);
      s_buff = alloc_lbuf("help_query");
      s_buff2ptr = s_buff2 = alloc_lbuf("help_query2");
      i_cntr = 0;
      s_hbuff2 = alloc_lbuf("help_buff");
      if ( *(mudconf.help_separator) ) {
         strcpy(s_hbuff2, mudconf.help_separator);
         s_hbuff = exec(GOD, GOD, GOD, EV_FIGNORE | EV_EVAL | EV_NOFCHECK, s_hbuff2,
                        (char **)NULL, 0, (char **)NULL, 0);
         if ( !*s_hbuff ) {
            sprintf(s_hbuff2, "%s", (char *)"  ");
         } else {
            sprintf(s_hbuff2, "%s", s_hbuff);
         }
         free_lbuf(s_hbuff);
      } else {
         sprintf(s_hbuff2, "%s", (char *)"  ");
      }
      while ( fread((char *)&entry, sizeof(help_indx), 1, fp_indx) == 1 ) { 
         for (p = entry.topic; *p; p++)
             *p = ToLower((int)*p);
         if (fseek(fp_help, entry.pos, 0) < 0L) {
            if ( t_val )
               safe_str("Error on index seeking.", t_buff, &t_bufptr);
            else
               notify(player, "Error on index seeking.");
            free_lbuf(msg);
            free_lbuf(tmp);
            free_lbuf(line);
            free_lbuf(s_buff);
            free_lbuf(s_buff2);
            fclose(fp_help);
            tf_fclose(fp_indx);
            return 1;
         }
         if ( i_bufffilled ) {
            break;
         }
         if ( i_cntr > 2000 ) {
            break;
         }
         i_header = 0;
         for (;;) {
            if ( i_bufffilled ) {
               break;
            }
            if ( fgets(line, (LBUF_SIZE - 1), fp_help) == NULL ) 
               break;
            if (line[0] == '&')
               break;
            for (p = line; *p != '\0'; p++) {
               *p = ToLower((int)*p);
               if ( (*p == '\n') || (*p == '\r') )
                   *p = '\0'; 
            }
            if ( quick_wild(msg, line) ) { 
               if ( key & DYN_QUERY ) {
                  if ( !i_header ) {
#ifdef ZENTY_ANSI
                     sprintf(s_buff, "%s%s%s:", SAFE_ANSI_HILITE, entry.topic, SAFE_ANSI_NORMAL);
#else
                     sprintf(s_buff, "%s%s%s:", ANSI_HILITE, entry.topic, ANSI_NORMAL);
#endif
                     if ( t_val ) {
                        safe_str(s_buff, t_buff, &t_bufptr);
                        safe_str((char *)"\r\n", t_buff, &t_bufptr);
                        if ( strlen(t_buff) > (LBUF_SIZE - (LBUF_SIZE/8)) ) {
                           safe_str((char *)"\r\nWarning: /query matches discarded with buffer limit.", t_buff, &t_bufptr);
                           i_bufffilled = 1;
                           break;
                        }
                     } else {
                        notify(player, s_buff);
                     }
                     i_header = 1;
                  }
#ifdef ZENTY_ANSI
                  sprintf(s_buff, "%s$0%s", SAFE_ANSI_RED, SAFE_ANSI_NORMAL);
#else
                  sprintf(s_buff, "%s$0%s", ANSI_RED, ANSI_NORMAL);
#endif
                  s_buffptr = msg;
                  memset(s_buff2, '\0', LBUF_SIZE);
                  s_buff2ptr = s_buff2;
                  i_magic = 0;
                  while ( *s_buffptr ) {
                    if ( *s_buffptr == '*' ) {
                       if ( isprint(*(s_buffptr+1)) ) {
                          if ( i_magic ) {
                             safe_str((char *)".*", s_buff2, &s_buff2ptr);
                          }
                       }
                       s_buffptr++;
                       continue;
                    }
                    i_magic = 1;
                    if ( *s_buffptr == '?' ) {
                       safe_chr('.', s_buff2, &s_buff2ptr);
                    } else {
                       safe_chr(*s_buffptr, s_buff2, &s_buff2ptr);
                    }
                    s_buffptr++;
                  }
                  help_array[0] = line;
                  help_array[1] = s_buff2;
                  help_array[2] = s_buff;
                  help_array[3] = NULL;
                  p_tmp = tmp;
                  do_regedit(tmp, &p_tmp, GOD, GOD, GOD, help_array, 3, (char **)NULL, 0, 8);
                  sprintf(s_buff, "   ...%.*s", (LBUF_SIZE - 20), tmp);
                  if ( t_val ) {
                     safe_str(s_buff, t_buff, &t_bufptr);
                     safe_str((char *)"\r\n", t_buff, &t_bufptr);
                     if ( strlen(t_buff) > (LBUF_SIZE - (LBUF_SIZE/8)) ) {
                        safe_str((char *)"\r\nWarning: /query matches discarded with buffer limit.", t_buff, &t_bufptr);
                        i_bufffilled = 1;
                        break;
                     }
                  } else {
                     notify(player, s_buff);
                  }
                  first = 1;
                  i_cntr++;
               } else {
                  if ( first ) {
                     if ( i_type ) {
                        safe_str(sep, tmp, &p_tmp);
                        if ( strlen(t_buff) > (LBUF_SIZE - (LBUF_SIZE/8)) ) {
                           safe_str((char *)"\r\nWarning: /query matches discarded with buffer limit.", t_buff, &t_bufptr);
                           i_bufffilled = 1;
                        }
                     } else {
                        safe_str(s_hbuff2, tmp, &p_tmp);
                     }
                  }
                  safe_str(entry.topic, tmp, &p_tmp);
                  first = 1;
                  break;
               }
            }
         }
      }
      free_lbuf(s_hbuff2);
      if ( i_cntr > 2000 ) {
         if ( t_val ) {
            safe_str((char *)"Warning: /query matches discarded after 2000 matches.", t_buff, &t_bufptr);
         } else {
            notify(player, "Warning: /query matches discarded after 2000 matches.");
         }
      }
      free_lbuf(s_buff);
      free_lbuf(s_buff2);
      if ( !(key & DYN_QUERY) ) {
         if ( t_val ) {
            if ( first ) {
               if ( !i_type )
                  safe_str(unsafe_tprintf("Here are the entries which match content '%s':\r\n", msg), t_buff, &t_bufptr);
               safe_str(tmp, t_buff, &t_bufptr);
            } else {
               if ( !i_type )
                  safe_str(unsafe_tprintf("There are no entries which match content '%s'.", msg), t_buff, &t_bufptr);
            }
         } else {
            if ( first ) {
               if ( !i_type )
                  notify(player, unsafe_tprintf("Here are the entries which match content '%s':", msg));
               notify(player, tmp);
            } else {
               if ( !i_type )
                  notify(player, unsafe_tprintf("There are no entries which match content '%s'.", msg));
            }
         }
      } else if ( !first ) {
         if ( t_val ) {
            safe_str(unsafe_tprintf("There are no entries which match content '%s'.", msg), t_buff, &t_bufptr);
         } else {
            notify(player, unsafe_tprintf("There are no entries which match content '%s'.", msg));
         }
      }
      free_lbuf(msg);
      free_lbuf(tmp);
      free_lbuf(line);
      fclose(fp_help);
      tf_fclose(fp_indx);
      return 0;
   }
   
   if ( i_suggest ) {
      for ( i=0; i<3; i++ ) {
         s_tier0[i] = alloc_sbuf("dynhelp_suggest");
         s_tier1[i] = alloc_sbuf("dynhelp_suggest");
         s_tier2[i] = alloc_sbuf("dynhelp_suggest");
         s_tier3[i] = alloc_sbuf("dynhelp_suggest");
      }
      s_tmpbuff = alloc_lbuf("dynhelp_suggest_wildcard");
      s_buffptr = s_buff = alloc_lbuf("dynhelp_topical");
      i_tier0 = i_tier1 = i_tier2 = i_tier3 = 0;
   }
   s_hbuff2 = alloc_lbuf("help_buff");
   if ( *(mudconf.help_separator) ) {
      strcpy(s_hbuff2, mudconf.help_separator);
      s_hbuff = exec(GOD, GOD, GOD, EV_FIGNORE | EV_EVAL | EV_NOFCHECK, s_hbuff2,
                     (char **)NULL, 0, (char **)NULL, 0);
      if ( !*s_hbuff ) {
         sprintf(s_hbuff2, "%s", (char *)"  ");
      } else {
         sprintf(s_hbuff2, "%s", s_hbuff);
      }
      free_lbuf(s_hbuff);
   } else {
      sprintf(s_hbuff2, "%s", (char *)"  ");
   }
   while (!found && ((fread((char *)&entry, sizeof(help_indx), 1, fp_indx) == 1))) {
      for (p = entry.topic; *p; p++)
          *p = ToLower((int)*p);

      if ( pstricmp(entry.topic, msg, strlen(msg)) == 0 ) {
         found = 1;
      } else {
         if ( quick_wild(msg, entry.topic) ) {
            matched = 1;
            if ( first ) {
               if ( i_type ) {
                  safe_str(sep, tmp, &p_tmp);
               } else {
                  if ( matched ) {
                     safe_str(s_hbuff2, tmp, &p_tmp);
                  }
               }
            }
            safe_str(entry.topic, tmp, &p_tmp);
            first = 1;
         }
      }
      if ( i_suggest && !matched ) {
         i_tier0chk = 0;
         if ( i_tier0 < 3 ) {
            sprintf(s_tmpbuff, "*%.*s*", LBUF_SIZE - 100, msg);
            if ( quick_wild(s_tmpbuff, entry.topic) ) {
               sprintf(s_tier0[i_tier0], "%.*s", SBUF_SIZE-1, entry.topic);
               i_tier0++;
               i_tier0chk = 1;
            }
         }
         s_buffptr = s_buff;
         s_nbuff[0] = msg;
         s_nbuff[1] = entry.topic;
         if ( !i_tier0chk ) {
            fun_strdistance(s_buff, &s_buffptr, 1, 1, 1, s_nbuff, 2, (char **)NULL, 0);
            switch(atoi(s_buff)) {
               case 1: /* case 1 */
                  if ( i_tier1 < 3 ) {
                     sprintf(s_tier1[i_tier1], "%.*s", SBUF_SIZE - 1, s_nbuff[1]);
                     i_tier1++;
                  }
                  break;
               case 2: /* case 2 */
                  if ( i_tier2 < 3 ) {
                     sprintf(s_tier2[i_tier2], "%.*s", SBUF_SIZE - 1, s_nbuff[1]);
                     i_tier2++;
                  }
                  break;
               case 3: /* case 3 */
                  if ( i_tier3 < 3 ) {
                     sprintf(s_tier3[i_tier3], "%.*s", SBUF_SIZE - 1, s_nbuff[1]);
                     i_tier3++;
                  }
                  break;
            }
         }
      }
   }
   if ( i_suggest ) {
      free_lbuf(s_tmpbuff);
      if ( i_tier0 || i_tier1 || i_tier2 || i_tier3 ) {
         s_buffptr = s_buff;
	 safe_str(unsafe_tprintf("No entry for '%s'.  Suggestions:  ", msg), s_buff, &s_buffptr);
         if ( i_tier0 > 0 ) {
            for (i=0; i<i_tier0; i++) {
               if ( matched ) {
                  safe_str(s_hbuff2, s_buff, &s_buffptr);
               }
               safe_str(s_tier0[i], s_buff, &s_buffptr);
               matched = 1;
            }
         }
         if ( i_tier1 > 0 ) {
            for (i=0; i<i_tier1; i++) {
               if ( matched ) {
                  safe_str(s_hbuff2, s_buff, &s_buffptr);
               }
               safe_str(s_tier1[i], s_buff, &s_buffptr);
               matched = 1;
            }
         }
         if ( !i_tier1 && (i_tier2 > 0) ) {
            for (i=0; i<i_tier2; i++) {
               if ( matched ) {
                  safe_str(s_hbuff2, s_buff, &s_buffptr);
               }
               safe_str(s_tier2[i], s_buff, &s_buffptr);
               matched = 1;
            }
         }
         if ( !i_tier1 && !i_tier2 && (i_tier3 > 0) ) {
            for (i=0; i<i_tier3; i++) {
               if ( matched ) {
                  safe_str(s_hbuff2, s_buff, &s_buffptr);
               }
               safe_str(s_tier3[i], s_buff, &s_buffptr);
               matched = 1;
            }
         }
      } else {
	 safe_str(unsafe_tprintf("No entry for '%s'.  No suggestions.", msg), s_buff, &s_buffptr);
      }
      matched = 0;
   }
   tf_fclose(fp_indx);
   free_lbuf(s_hbuff2);
   if ( found ) {
      memset(filename, 0, sizeof(filename));
      tpass = fhelp;
      while (*tpass) {
        if (*tpass == '^')
           *tpass = '/';
        if (*tpass == '.')
           *tpass = '_';
        tpass++;
      }
      sprintf(filename, "%.128s/%.32s.txt", mudconf.txt_dir, fhelp);   
      if ( (fp_help = tf_fopen(filename, O_RDONLY)) == NULL ) {
         if ( t_val )
            safe_str("Sorry, that file doesn't seem to exist.", t_buff, &t_bufptr);
         else
            notify(player, "Sorry, that file doesn't seem to exist.");
         free_lbuf(tmp);
         free_lbuf(msg);
         if ( i_suggest ) {
            for ( i=0; i<3; i++ ) {
               free_sbuf(s_tier0[i]);
               free_sbuf(s_tier1[i]);
               free_sbuf(s_tier2[i]);
               free_sbuf(s_tier3[i]);
            }
            free_lbuf(s_buff);
         }
         return(1);
      } 
      if (fseek(fp_help, entry.pos, 0) < 0L) {
         if ( t_val )
            safe_str("Sorry, that function is temporarily unavailable.", t_buff, &t_bufptr);
         else
            notify(player, "Sorry, that function is temporarily unavailable.");
         STARTLOG(LOG_PROBLEMS, "DYN", "SEEK")
         line = alloc_lbuf("help_write.LOG.seek");
         sprintf(line, "Seek error in file %.3900s[.indx/.txt].", fhelp);
         log_text(line);
         free_lbuf(line);
         ENDLOG
         free_lbuf(tmp);
         tf_fclose(fp_help);
         free_lbuf(msg);
         if ( i_suggest ) {
            for ( i=0; i<3; i++ ) {
               free_sbuf(s_tier0[i]);
               free_sbuf(s_tier1[i]);
               free_sbuf(s_tier2[i]);
               free_sbuf(s_tier3[i]);
            }
            free_lbuf(s_buff);
         }
         return(1);
      }
      line = alloc_lbuf("help_write");
      one_through=0;
      space_compress = mudconf.space_compress;
      mudconf.space_compress=0;
      if ( !i_noindex && !i_type && !t_val ) {
         myp = mybuff = alloc_lbuf("ANSI_DYNHELP");
#ifdef ZENTY_ANSI
         safe_str(SAFE_ANSI_HILITE, mybuff, &myp);
         for ( p = entry.topic; *p; p++ ) {
            safe_chr(ToUpper(*p), mybuff, &myp);
         }
         safe_str(SAFE_ANSI_NORMAL, mybuff, &myp);
#else
         safe_str(ANSI_HILITE, mybuff, &myp);
         for ( p = entry.topic; *p; p++ ) {
            safe_chr(ToUpper(*p), mybuff, &myp);
         }
         safe_str(ANSI_NORMAL, mybuff, &myp);
#endif
         notify(player, mybuff);
         free_lbuf(mybuff);
      }
      for (;;) {
         if (fgets(line, LBUF_SIZE - 1, fp_help) == NULL)
            break;
         if (line[0] == '&')
            break;
         for (p = line; *p != '\0'; p++)
            if ( (*p == '\n') || (*p == '\r') )
                *p = '\0';
         if ( key & DYN_PARSE ) {
            result = cpuexec(player, cause, cause,
                            EV_STRIP | EV_FCHECK | EV_EVAL, line, (char**)NULL, 0, (char **)NULL, 0);
            if ( t_val ) {
               safe_str(result, t_buff, &t_bufptr);
               safe_str("\r\n", t_buff, &t_bufptr);
            } else
               notify(player, result);
            free_lbuf(result);
         } else if ( key & DYN_SUBEVAL ) {
            result = cpuexec(player, cause, cause,
                            EV_FIGNORE | EV_NOFCHECK | EV_EVAL, line, (char**)NULL, 0, (char **)NULL, 0);
            if ( t_val ) {
               safe_str(result, t_buff, &t_bufptr);
               safe_str("\r\n", t_buff, &t_bufptr);
            } else
               notify(player, result);
            free_lbuf(result);
         } else {
            if ( t_val ) {
               safe_str(line, t_buff, &t_bufptr);
               safe_str("\r\n", t_buff, &t_bufptr);
            } else
               noansi_notify(player, line);
         }
         one_through=1;
      }
      mudconf.space_compress = space_compress;
      free_lbuf(line);
      if ( !one_through ) {
         if ( t_val ) {
            if ( i_suggest ) {
               safe_str(s_buff, t_buff, &t_bufptr);
            } else {
               safe_str(unsafe_tprintf("No entry for '%s'.", msg), t_buff, &t_bufptr);
            }
         } else {
            if ( i_suggest ) {
               notify(player, s_buff);
            } else {
               notify(player, unsafe_tprintf("No entry for '%s'.", msg));
            }
         }
         STARTLOG(LOG_PROBLEMS, "DYN", "INDX")
         line = alloc_lbuf("help_write.LOG.seek");
         sprintf(line, "Mismatched index for %.3900s[.indx/.txt].", fhelp);
         log_text(line);
         free_lbuf(line);
         ENDLOG
      }
   } else if ( matched ) {
      if ( t_val ) {
         if ( !i_type )
            safe_str(unsafe_tprintf("Here are the entries which match '%s':\r\n", msg), t_buff, &t_bufptr);
         safe_str(tmp, t_buff, &t_bufptr);
      } else {
         if ( !i_type )
            notify(player, unsafe_tprintf("Here are the entries which match '%s':", msg));
         notify(player, tmp);
      }
   } else {
      if ( t_val ) {
         if ( !i_type ) {
            if ( i_suggest ) {
               safe_str(s_buff, t_buff, &t_bufptr);
            } else {
               safe_str(unsafe_tprintf("No entry for '%s'.", msg), t_buff, &t_bufptr);
            }
         }
      } else {
         if ( !i_type ) {
            if ( i_suggest ) {
               notify(player, s_buff);
            } else {
               notify(player, unsafe_tprintf("No entry for '%s'.", msg));
            }
         }
      }
   }
   free_lbuf(tmp);
   tf_fclose(fp_help);
   free_lbuf(msg);
   if ( i_suggest ) {
      for ( i=0; i<3; i++ ) {
         free_sbuf(s_tier0[i]);
         free_sbuf(s_tier1[i]);
         free_sbuf(s_tier2[i]);
         free_sbuf(s_tier3[i]);
      }
      free_lbuf(s_buff);
   }
   return(0);
}

void
do_dynhelp(dbref player, dbref cause, int key, char *fhelp, char *msg)
{
   int retval;
   dbref it;
   char *line, *in_topic, *p_in_topic, *in_file, *tstrtokr;

   if ( !*msg || !msg ) {
      it = player;
   } else {
      line = exec(player, cause, cause, EV_STRIP | EV_FCHECK | EV_EVAL, msg, (char **) NULL, 0, (char **)NULL, 0);
      it = lookup_player(player, line, 1);
      if (it == NOTHING || !Good_obj(it) || !isPlayer(it) || Going(it) || Recover(it) ) {
         notify_quiet(player, unsafe_tprintf("Unknown player '%s'.", line));
         free_lbuf(line);
         return;
      }
      free_lbuf(line);
   }
  
   p_in_topic = in_topic = alloc_lbuf("in_topic.dynhelp");
   if ( strchr(fhelp, '/') ) {
      in_file = strtok_r(fhelp, "/", &tstrtokr);
      safe_str(strtok_r(NULL, "\0", &tstrtokr), in_topic, &p_in_topic);
   } else {
      in_file = fhelp;
   } 
   retval = parse_dynhelp(it, cause, key, in_file, in_topic, (char *)NULL, (char *)NULL, 0, 0, (char *)NULL);
   free_lbuf(in_topic);
   if ( retval ) {
	STARTLOG(LOG_PROBLEMS, "DYN", "FILE")
	line = alloc_lbuf("dynamic_file.LOG.open");
	sprintf(line, "Can't open %.3900s[.txt/.indx] for reading.", fhelp);
	log_text(line);
	free_lbuf(line);
	ENDLOG
   }
}

void 
do_help(dbref player, dbref cause, int key, char *message)
{
    char *buf;
    int keyval;

    keyval = 0;
    if ( key & HELP_SEARCH ) {
       key = (key & ~HELP_SEARCH);
       keyval = 1;
    }
    if ( key & HELP_QUERY ) {
       key = (key & ~HELP_QUERY);
       keyval = 2;
    }
    switch (key) {
#ifdef PLUSHELP
    case HELP_PLUSHELP:
	help_write(player, message, &mudstate.plushelp_htab,
		   mudconf.plushelp_file, keyval);
	break;
#endif
    case HELP_HELP:
	help_write(player, message, &mudstate.help_htab,
		   mudconf.help_file, keyval);
	break;
    case HELP_NEWS:
	help_write(player, message, &mudstate.news_htab,
		   mudconf.news_file, keyval);
	break;
    case HELP_WIZHELP:
	help_write(player, message, &mudstate.wizhelp_htab,
		   mudconf.whelp_file, keyval);
	break;
    default:
	STARTLOG(LOG_BUGS, "BUG", "HELP")
	    buf = alloc_mbuf("do_help.LOG");
	sprintf(buf, "Unknown help file number: %d", key);
	log_text(buf);
	free_mbuf(buf);
	ENDLOG
    }
}

char *
errmsg(dbref player)
{
    FILE *fp;
    char *p, *line;
    int offset, first, i_trace, matched;
    struct help_entry *htab_entry;
    char *topic_list, *buffp, filename[129 + 32], *s_hbuff, *s_hbuff2;
    HASHTAB *htab;
    static char errbuf[LBUF_SIZE];
    char *dmsg = "Huh?  (Type \"help\" for help.)";

    if (VanillaErrors(player)) {
      strcpy(errbuf,dmsg);
      return errbuf;
    }
    i_trace = 0;
    htab = &mudstate.error_htab;
    if ( mudstate.errornum == 0 ) {
       strcpy(errbuf, (char *)"0");
    } else {
       strcpy(errbuf,myitoa(random() % (mudstate.errornum)));
    }
    htab_entry = (struct help_entry *) hashfind(errbuf, htab);
    if (htab_entry)
	offset = htab_entry->pos;
    else {
	matched = 0;
        s_hbuff2 = alloc_lbuf("help_buff");
        if ( *(mudconf.help_separator) ) {
           strcpy(s_hbuff2, mudconf.help_separator);
           s_hbuff = exec(GOD, GOD, GOD, EV_FIGNORE | EV_EVAL | EV_NOFCHECK, s_hbuff2,
                          (char **)NULL, 0, (char **)NULL, 0);
           if ( !*s_hbuff ) {
              sprintf(s_hbuff2, "%s", (char *)"  ");
           } else {
              sprintf(s_hbuff2, "%s", s_hbuff);
           }
           free_lbuf(s_hbuff);
        } else {
           sprintf(s_hbuff2, "%s", (char *)"  ");
        }
	for (htab_entry = (struct help_entry *) hash_firstentry(htab);
	     htab_entry != NULL;
	     htab_entry = (struct help_entry *) hash_nextentry(htab)) {
	    if (htab_entry->original &&
		quick_wild(errbuf, htab_entry->key)) {
		if (matched == 0) {
		    matched = 1;
		    topic_list = alloc_lbuf("help_write");
		    buffp = topic_list;
		}
		safe_str(htab_entry->key, topic_list, &buffp);
		safe_str(s_hbuff2, topic_list, &buffp);
	    }
	}
	if (matched == 0)
	    strcpy(errbuf, dmsg);
	else {
	    strcpy(errbuf, dmsg);
	    free_lbuf(topic_list);
	}
        free_lbuf(s_hbuff2);
	return errbuf;
    }
    sprintf(filename, "%s/%s", mudconf.txt_dir, mudconf.error_file);
    if ((fp = tf_fopen(filename, O_RDONLY)) == NULL) {
	strcpy(errbuf, dmsg);
	STARTLOG(LOG_PROBLEMS, "HLP", "OPEN")
	    line = alloc_lbuf("help_write.LOG.open");
	sprintf(line, "Can't open %.3900s for reading.", mudconf.error_file);
	log_text(line);
	free_lbuf(line);
	ENDLOG
	    return errbuf;
    }
    if (fseek(fp, offset, 0) < 0L) {
	strcpy(errbuf, dmsg);
	STARTLOG(LOG_PROBLEMS, "HLP", "SEEK")
	    line = alloc_lbuf("help_write.LOG.seek");
	sprintf(line, "Seek error in file %.3900s.", mudconf.error_file);
	log_text(line);
	free_lbuf(line);
	ENDLOG
	    tf_fclose(fp);
	return errbuf;
    }
    line = alloc_lbuf("help_write");
    *errbuf = '\0';
    first = 1;
    for (;;) {
	if (fgets(line, LBUF_SIZE - 1, fp) == NULL)
	    break;
	if (line[0] == '&')
	    break;
	for (p = line; *p != '\0'; p++)
	    if ( (*p == '\n') || (*p == '\r') )
		*p = '\0';
	if (!first)
	    strcat(errbuf, "\r\n");
	else
	    first = 0;
	strcat(errbuf, line);
    }
    free_lbuf(line);
    tf_fclose(fp);
    if ( *errbuf == '!' ) {
       i_trace = mudstate.notrace;
       mudstate.notrace = 1;
       buffp = cpuexec(player, player, player, EV_STRIP | EV_FCHECK | EV_EVAL, errbuf+1, (char **) NULL, 0, (char **)NULL, 0);
       strcpy(errbuf, buffp);
       free_lbuf(buffp);
       mudstate.notrace = i_trace;
    }
    return errbuf;
}
