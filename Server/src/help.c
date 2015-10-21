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
    char *p, *line;
    int offset, i_first, i_found, matched;
    struct help_entry *htab_entry;
    char *topic_list, *buffp, *mybuff, *myp;
    char realFilename[129 + 32], *s_tmpbuff, *s_ptr;

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
       for (htab_entry = (struct help_entry *) hash_firstentry(htab);
            htab_entry != NULL;
            htab_entry = (struct help_entry *) hash_nextentry(htab)) {
          if ( !htab_entry->original )
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
              free_lbuf(topic_list);
              free_lbuf(s_tmpbuff);
              tf_fclose(fp);
              return;
          }
          i_found = i_first = 0;
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
                i_found = matched = 1;
		safe_str(htab_entry->key, topic_list, &buffp);
		safe_str((char *) "  ", topic_list, &buffp);
                break;
             }
          }
          if ( !i_found && quick_wild(topic, s_tmpbuff) ) {
             matched = 1;
             safe_str(htab_entry->key, topic_list, &buffp);
             safe_str((char *) "  ", topic_list, &buffp);
          }
          memset(s_tmpbuff, '\0', LBUF_SIZE);
          s_ptr = s_tmpbuff;
       }
       free_lbuf(s_tmpbuff);
       if (matched == 0)
          notify(player, unsafe_tprintf("No entry for contents '%s'.", topic));
       else {
          notify(player, unsafe_tprintf("Here are the entries which contain '%s':", topic));
          *buffp = '\0';
          notify(player, topic_list);
       }
       free_lbuf(topic_list);
       free_lbuf(line);
       tf_fclose(fp);
       return;
    }

    htab_entry = (struct help_entry *) hashfind(topic, htab);
    if (htab_entry)
	offset = htab_entry->pos;
    else {
	matched = 0;
	for (htab_entry = (struct help_entry *) hash_firstentry(htab);
	     htab_entry != NULL;
	     htab_entry = (struct help_entry *) hash_nextentry(htab)) {
	    if (htab_entry->original &&
		quick_wild(topic, htab_entry->key)) {
		if (matched == 0) {
		    matched = 1;
		    topic_list = alloc_lbuf("help_write");
		    buffp = topic_list;
		}
		safe_str(htab_entry->key, topic_list, &buffp);
		safe_str((char *) "  ", topic_list, &buffp);
	    }
	}
	if (matched == 0)
	    notify(player, unsafe_tprintf("No entry for '%s'.", topic));
	else {
	    notify(player, unsafe_tprintf("Here are the entries which match '%s':", topic));
	    *buffp = '\0';
	    notify(player, topic_list);
	    free_lbuf(topic_list);
	}
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
    for (;;) {
	if (fgets(line, LBUF_SIZE - 1, fp) == NULL)
	    break;
	if (line[0] == '&')
	    break;
	for (p = line; *p != '\0'; p++)
	    if ( (*p == '\n') || (*p == '\r') )
		*p = '\0';
	noansi_notify(player, line);
    }
    free_lbuf(line);
    tf_fclose(fp);
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
   int first, found, matched, one_through, space_compress;
   FILE *fp_indx, *fp_help;
   char filename[129 + 40], *mybuff, *myp;

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

   if ( key & DYN_SEARCH ) {
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
            fclose(fp_help);
            tf_fclose(fp_indx);
            return 1;
         }
         for (;;) {
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
               if ( first ) {
                  if ( i_type ) {
                     safe_str(sep, tmp, &p_tmp);
                  } else {
                     safe_str("  ", tmp, &p_tmp);
                  }
               }
               safe_str(entry.topic, tmp, &p_tmp);
               first = 1;
               break;
            }
         }
      }
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
      free_lbuf(msg);
      free_lbuf(tmp);
      free_lbuf(line);
      fclose(fp_help);
      tf_fclose(fp_indx);
      return 0;
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
                  safe_str("  ", tmp, &p_tmp);
               }
            }
            safe_str(entry.topic, tmp, &p_tmp);
            first = 1;
         }
      }
   }

   tf_fclose(fp_indx);
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
         return(1);
      }
      line = alloc_lbuf("help_write");
      one_through=0;
      space_compress = mudconf.space_compress;
      mudconf.space_compress=0;
      if ( !i_type && !t_val ) {
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
            result = exec(player, cause, cause,
                         EV_STRIP | EV_FCHECK | EV_EVAL, line, (char**)NULL, 0, (char **)NULL, 0);
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
         if ( t_val )
            safe_str(unsafe_tprintf("No entry for '%s'.", msg), t_buff, &t_bufptr);
         else
            notify(player, unsafe_tprintf("No entry for '%s'.", msg));
         STARTLOG(LOG_PROBLEMS, "DYN", "INDX")
         line = alloc_lbuf("help_write.LOG.seek");
         sprintf(line, "Missmatched index for %.3900s[.indx/.txt].", fhelp);
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
         if ( !i_type )
            safe_str(unsafe_tprintf("No entry for '%s'.", msg), t_buff, &t_bufptr);
      } else {
         if ( !i_type )
            notify(player, unsafe_tprintf("No entry for '%s'.", msg));
      }
   }
   free_lbuf(tmp);
   tf_fclose(fp_help);
   free_lbuf(msg);
   return(0);
}

void
do_dynhelp(dbref player, dbref cause, int key, char *fhelp, char *msg)
{
   int retval;
   dbref it;
   char *line, *in_topic, *p_in_topic, *in_file;

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
      in_file = strtok(fhelp, "/");
      safe_str(strtok(NULL, "\0"), in_topic, &p_in_topic);
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
    char *topic_list, *buffp, filename[129 + 32];
    HASHTAB *htab;
    static char errbuf[LBUF_SIZE];
    char *dmsg = "Huh?  (Type \"help\" for help.)";

    if (VanillaErrors(player)) {
      strcpy(errbuf,dmsg);
      return errbuf;
    }
    i_trace = 0;
    htab = &mudstate.error_htab;
    strcpy(errbuf,myitoa(random() % (mudstate.errornum)));
    htab_entry = (struct help_entry *) hashfind(errbuf, htab);
    if (htab_entry)
	offset = htab_entry->pos;
    else {
	matched = 0;
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
		safe_str((char *) "  ", topic_list, &buffp);
	    }
	}
	if (matched == 0)
	    strcpy(errbuf, dmsg);
	else {
	    strcpy(errbuf, dmsg);
	    free_lbuf(topic_list);
	}
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
       buffp = exec(player, player, player, EV_STRIP | EV_FCHECK | EV_EVAL, errbuf+1, (char **) NULL, 0, (char **)NULL, 0);
       strcpy(errbuf, buffp);
       free_lbuf(buffp);
       mudstate.notrace = i_trace;
    }
    return errbuf;
}
