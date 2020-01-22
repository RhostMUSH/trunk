/* htab.c - table hashing routines */

#include "copyright.h"
#include "autoconf.h"

#ifndef STANDALONE
#include "db.h"
#include "externs.h"
#include "htab.h"
#include "alloc.h"
#endif /* STANDALONE */

#include "mudconf.h"

#define FILENUM HTAB_c

char *gtpr_buff, *gtprp_buff; 
#define LOGTEXT(DOM, MSG) \
    STARTLOG(LOG_BUGS, "HTAB", DOM)\
       gtprp_buff = gtpr_buff = alloc_lbuf("LOGTEXT");\
       log_text(safe_tprintf(gtpr_buff, &gtprp_buff, "(%s:%d)", fileName, lineNo));\
       gtprp_buff = gtpr_buff;\
       log_text(safe_tprintf(gtpr_buff, &gtprp_buff, "  %s", (char *) MSG)); \
       free_lbuf(gtpr_buff); \
    ENDLOG

/* ---------------------------------------------------------------------------
 * hashval: Compute hash value of a string for a hash table.
 */

int real_hashval(char *str, int hashmask, const char *fileName, int lineNo)
{
    int hash;
    char *sp;

    /*
     * If the string pointer is null, return 0.  If not, add up the
     * numeric value of all the characters and return the sum,
     * modulo the size of the hash table.
     */

    if (str == NULL)
	return 0;
    hash = 0;
    for (sp = str; *sp; sp++)
	hash = (hash << 5) + hash + *sp;
    return (hash & hashmask);
}

#ifndef STANDALONE

/* ----------------------------------------------------------------------
 * get_hashmask: Get hash mask for mask-style hashing.
 */

int real_get_hashmask(int *size, const char *fileName, int lineNo)
{
    int tsize;

    /* Get next power-of-two >= size, return power-1 as the mask
     * for ANDing
     */

    for (tsize = 1; tsize < *size; tsize = tsize << 1);
    *size = tsize;
    return tsize - 1;
}

/* ---------------------------------------------------------------------------
 * hashinit: Initialize a new hash table.
 */

void real_hashinit(HASHTAB * htab, int size, const char *fileName, int lineNo)
{
    int i;

    if (htab == NULL) {
      LOGTEXT("ERR", "hashinit was passed a NULL htab.");
      return;
    }
    if (size < 0) {
      LOGTEXT("WRN", "hashinit was passed a size < 0.");
      size = 50;
    }

    htab->mask = real_get_hashmask(&size, fileName, lineNo);
    htab->hashsize = size;
    htab->checks = 0;
    htab->scans = 0;
    htab->max_scan = 0;
    htab->hits = 0;
    htab->entries = 0;
    htab->deletes = 0;
    htab->nulls = size;
    htab->entry =
	(HASHARR *) malloc(size * sizeof(struct hashentry *));

    for (i = 0; i < size; i++)
	htab->entry->element[i] = NULL;
}

/* ---------------------------------------------------------------------------
 * hashreset: Reset hash table stats.
 */

void real_hashreset(HASHTAB * htab, const char *fileName, int lineNo)
{
    if (htab == NULL) {
      LOGTEXT("ERR", "hashreset was passed a NULL htab.");
      return;
    }
    htab->checks = 0;
    htab->scans = 0;
    htab->hits = 0;
}

/* ---------------------------------------------------------------------------
 * hashfind: Look up an entry in a hash table and return a pointer to its
 * hash data.
 */
int * real_hashfind(char *str, HASHTAB * htab, const char *fileName, int lineNo) {
  return (int *) hashfind2(str, htab, 0);
}

int * real_hashfind2(char *str, HASHTAB * htab, int bNeedOriginal, const char *fileName, int lineNo)
{
    int hval, numchecks;
    HASHENT *hptr;

    if (str == NULL) {
      LOGTEXT("ERR", "hashfind2 was passed a NULL key-string");
      return NULL;
    } 

    if (htab == NULL) {
      LOGTEXT("ERR", "hashfind2 was passed a NULL htab");
      return NULL;
    }

    if (htab->entry == NULL ) {
      LOGTEXT("ERR", "hashfind2 was passed a NULL htab->entry");
      return NULL;
    }

    numchecks = 0;
    htab->scans++;
    hval = hashval(str, htab->mask);

    for (hptr = htab->entry->element[hval]; hptr != NULL; hptr = hptr->next) {
	numchecks++;
	if (strcmp(str, hptr->target) == 0) {
	  if (bNeedOriginal) {
	    if (hptr->bIsOriginal) {
	      htab->hits++;
	      if (numchecks > htab->max_scan)
		htab->max_scan = numchecks;
	      htab->checks += numchecks;
	      htab->last_entry = hptr;
	      return hptr->data;
	    } 
	  } else {
	    htab->hits++;
	    if (numchecks > htab->max_scan)
	      htab->max_scan = numchecks;
	    htab->checks += numchecks;
	    htab->last_entry = hptr;	    
	    return hptr->data;
	  }
	}
    }
    if (numchecks > htab->max_scan)
	htab->max_scan = numchecks;
    htab->checks += numchecks;
    return NULL;
}

/* ---------------------------------------------------------------------------
 * hashadd: Add a new entry to a hash table.
 */

int real_hashadd(char *str, int *hashdata, HASHTAB *htab, const char *fileName, int lineNo) {
  return (int) real_hashadd2(str, hashdata, htab, 0, fileName, lineNo);
}

int real_hashadd2(char *str, int *hashdata, HASHTAB * htab, int bOriginal, const char *fileName, int lineNo)
{
    int hval;
    HASHENT *hptr;

    if (str == NULL || *str == '\0') {
      LOGTEXT("ERR", "hashadd2 was passed an empty string.");
      return -1;
    }
    if (hashdata == NULL) {
      LOGTEXT("ERR", "hashadd2 was passed a NULL data structure.");
      return -1;
    }
    if (htab == NULL) {
      LOGTEXT("ERR", "hashadd2 was passed a NULL htab.");
      return -1;
    }
    if (bOriginal !=0 && bOriginal != 1) {
      LOGTEXT("ERR", "hashadd2 was passed an invalid bOriginal value.");
      return -1;
    }
    /*
     * Make sure that the entry isn't already in the hash table.  If it
     * is, exit with an error.  Otherwise, create a new hash block and
     * link it in at the head of its thread.
     */

    if (bOriginal) {
      int *p;
      p = hashfind2(str, htab, 1);

      if (p) {
	return -1;
      } else if (hashfind2(str, htab, 0)) {
	hashdelete(str, htab);
      } 
    } else {
      if (hashfind2(str, htab, 0) != NULL)
	return (-1);
    }

    hval = hashval(str, htab->mask);
    htab->entries++;
    if (htab->entry->element[hval] == NULL)
      htab->nulls--;
    hptr = (HASHENT *) malloc(sizeof(HASHENT));
    hptr->target = (char *) strsave(str);
    hptr->data = hashdata;
    hptr->next = htab->entry->element[hval];
    hptr->bIsOriginal = bOriginal;
    htab->entry->element[hval] = hptr;
    return (0);
}

/* ---------------------------------------------------------------------------
 * hashdelete: Remove an entry from a hash table.
 */

void real_hashdelete(char *str, HASHTAB * htab, const char *fileName, int lineNo)
{
    int hval;
    HASHENT *hptr, *last;

    if (htab == NULL) {
      LOGTEXT("ERR", "hashdelete was passed a NULL htab.");
      return;
    }

    if (str == NULL) {
      LOGTEXT("ERR", "hashdelete was passed a NULL string.");
      return;
    }

    hval = hashval(str, htab->mask);
    last = NULL;
    for (hptr = htab->entry->element[hval];
	 hptr != NULL;
	 last = hptr, hptr = hptr->next) {
	if (strcmp(str, hptr->target) == 0) {
	    if (last == NULL)
		htab->entry->element[hval] = hptr->next;
	    else
		last->next = hptr->next;
	    free(hptr->target);
	    free(hptr);
	    htab->deletes++;
	    htab->entries--;
	    if (htab->entry->element[hval] == NULL)
		htab->nulls++;
	    return;
	}
    }
}

/* ---------------------------------------------------------------------------
 * hashflush: free all the entries in a hashtable.
 */

void real_hashflush(HASHTAB * htab, int size, const char *fileName, int lineNo)
{
    HASHENT *hent, *thent;
    int i;

    if (htab == NULL) {
      LOGTEXT("ERR", "hashflush was passed a NULL htab.");
      return;
    }
    if (size < 0) {
      LOGTEXT("WRN", "hashflush was passed a size < 0.");
      size = 50;
    }

    for (i = 0; i < htab->hashsize; i++) {
	hent = htab->entry->element[i];
	while (hent != NULL) {
	    thent = hent;
	    hent = hent->next;
	    free(thent->target);
	    free(thent);
	}
	htab->entry->element[i] = NULL;
    }

    /* Resize if needed.  Otherwise, just zero all the stats */

    if ((size > 0) && (size != htab->hashsize)) {
	free(htab->entry);
	hashinit(htab, size);
    } else {
	htab->checks = 0;
	htab->scans = 0;
	htab->max_scan = 0;
	htab->hits = 0;
	htab->entries = 0;
	htab->deletes = 0;
	htab->nulls = htab->hashsize;
    }
}

/* ---------------------------------------------------------------------------
 * hashrepl: replace the data part of a hash entry.
 */

int real_hashrepl(char *str, int *hashdata, HASHTAB *htab, const char *fileName, int lineNo) {
  return (int) real_hashrepl2(str, hashdata, htab, 0, fileName, lineNo);
}

int real_hashrepl2(char *str, int *hashdata, HASHTAB * htab, int bOriginal, const char *fileName, int lineNo)
{
    HASHENT *hptr;
    int hval;

    if (str == NULL || *str == '\0') {
      LOGTEXT("ERR", "hashrepl2 was passed an empty string.");
      return 0;
    }
    if (hashdata == NULL) {
      LOGTEXT("ERR", "hashrepl2 was passed a NULL data structure.");
      return 0;
    }
    if (htab == NULL) {
      LOGTEXT("ERR", "hashrepl2 was passed a NULL htab.");
      return 0;
    }
    if (bOriginal !=0 && bOriginal != 1) {
      LOGTEXT("ERR", "hashrepl2 was passed an invalid bOriginal value.");
      return 0;
    }

    hval = hashval(str, htab->mask);
    for (hptr = htab->entry->element[hval];
	 hptr != NULL;
	 hptr = hptr->next) {
	if (strcmp(str, hptr->target) == 0) {
	    hptr->data = hashdata;
	    hptr->bIsOriginal = bOriginal;
	    return 1;
	}
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * hashinfo: return an mbuf with hashing stats
 */

char * real_hashinfo(const char *tab_name, HASHTAB * htab, const char *fileName, int lineNo)
{
    char *buff;
    const char *errstr="<\?\?\?>";

    if (htab == NULL) {
      LOGTEXT("ERR", "hashinfo was passed a NULL htab.");
      return 0;
    }
   if (tab_name == NULL) {
      LOGTEXT("WRN", "hashinfo was passed an empty tab_name.");
      tab_name = errstr;
    }
    buff = alloc_mbuf("hashinfo");
    sprintf(buff, "%-15s %4d%8d%8d%8d%8d%8d%8d%8d",
	    tab_name, htab->hashsize, htab->entries, htab->deletes,
	    htab->nulls, htab->scans, htab->hits, htab->checks,
	    htab->max_scan);
    return buff;
}

/* Returns the key for the first hash entry in 'htab'. */

int * real_hash_firstentry(HASHTAB * htab, const char *fileName, int lineNo) {
  return real_hash_firstentry2(htab, 0, fileName, lineNo);
}

int * real_hash_firstentry2(HASHTAB * htab, int bOnlyOriginals, const char *fileName, int lineNo)
{
    int hval;
    HASHENT *hEntPtr;

    if (htab == NULL) {
      LOGTEXT("ERR", "hash_firstentry2 was passed a NULL htab.");
      return 0;
    }

    if (bOnlyOriginals !=0 && bOnlyOriginals != 1) {
      LOGTEXT("ERR", "hash_firstentry2 was passed an invalid bOriginal value.");
      return 0;
    }

    for (hval = 0; hval < htab->hashsize; hval++)
      if (htab->entry->element[hval] != NULL) {

	/* For each element in the hash
	 * walk through the list, looking for a valid bucket.
	 */
	for (hEntPtr = htab->entry->element[hval];
	     hEntPtr != NULL;
	     hEntPtr = hEntPtr->next) {
	  if (!bOnlyOriginals || hEntPtr->bIsOriginal == 1) {
	    htab->last_hval = hval;
	    htab->last_entry = hEntPtr;
	    htab->bOnlyOriginals = bOnlyOriginals;
	    return hEntPtr->data;
	  }
	}
      }
    return NULL;
}

static HASHENT * real_next_hashent(HASHTAB * htab, const char *fileName, int lineNo);
int * real_hash_nextentry(HASHTAB * htab, const char *fileName, int lineNo) {
  int bOnlyOriginals = htab->bOnlyOriginals;
  HASHENT *hptr;

  if (htab == NULL) {
    LOGTEXT("ERR", "hash_nextentry was passed a NULL htab.");
    return NULL;
  }

  while (1) {
    hptr = real_next_hashent(htab, fileName, lineNo);
    
    if (!hptr || !bOnlyOriginals || hptr->bIsOriginal) {
      break;
    }
  }
  return (hptr == NULL) ? NULL : hptr->data;
}

static HASHENT * real_next_hashent(HASHTAB * htab, const char *fileName, int lineNo) {
    int hval;
    HASHENT *hptr;

    if (htab == NULL) {
      LOGTEXT("ERR", "next_hashent was passed a NULL htab.");
      return NULL;
    }

    hval = htab->last_hval;
    hptr = htab->last_entry;
    if (hptr->next != NULL) {	/* We can stay in the same chain */
	htab->last_entry = hptr->next;
	return hptr->next;
    }
    /* We were at the end of the previous chain, go to the next one */
    hval++;
    while (hval < htab->hashsize) {
      if (htab->entry->element[hval] != NULL) {
	    htab->last_hval = hval;
	    htab->last_entry = htab->entry->element[hval];
	    return htab->entry->element[hval];
	}
	hval++;
    }
    return NULL;
}

/* ---------------------------------------------------------------------------
 * nhashfind: Look up an entry in a numeric hash table and return a pointer
 * to its hash data.
 */

int * real_nhashfind(int val, NHSHTAB * htab, const char *fileName, int lineNo)
{
    int hval, numchecks;
    NHSHENT *hptr;

    if (htab == NULL) {
      LOGTEXT("ERR", "nhashfind was passed a NULL htab.");
      return NULL;
    }

    numchecks = 0;
    htab->scans++;
    hval = (val & htab->mask);
    for (hptr = htab->entry->element[hval]; hptr != NULL; hptr = hptr->next) {
	numchecks++;
	if (val == hptr->target) {
	    htab->hits++;
	    if (numchecks > htab->max_scan)
		htab->max_scan = numchecks;
	    htab->checks += numchecks;
	    return hptr->data;
	}
    }
    if (numchecks > htab->max_scan)
	htab->max_scan = numchecks;
    htab->checks += numchecks;
    return NULL;
}

/* ---------------------------------------------------------------------------
 * nhashadd: Add a new entry to a numeric hash table.
 */

int real_nhashadd(int val, int *hashdata, NHSHTAB * htab, const char *fileName, int lineNo)
{
    int hval;
    NHSHENT *hptr;

    if (htab == NULL) {
      LOGTEXT("ERR", "nhashadd was passed a NULL htab.");
      return -1;
    }
    if (hashdata == NULL) {
      LOGTEXT("ERR", "nhashadd was passed a NULL hashdata.");
      return -1;
    }
    /*
     * Make sure that the entry isn't already in the hash table.  If it
     * is, exit with an error.  Otherwise, create a new hash block and
     * link it in at the head of its thread.
     */

    if (nhashfind(val, htab) != NULL)
	return (-1);
    hval = (val & htab->mask);
    htab->entries++;
    if (htab->entry->element[hval] == NULL)
	htab->nulls--;
    hptr = (NHSHENT *) malloc(sizeof(NHSHENT));
    hptr->target = val;
    hptr->data = hashdata;
    hptr->next = htab->entry->element[hval];
    htab->entry->element[hval] = hptr;
    return (0);
}

/* ---------------------------------------------------------------------------
 * nhashdelete: Remove an entry from a numeric hash table.
 */

void real_nhashdelete(int val, NHSHTAB * htab, const char *fileName, int lineNo)
{
    int hval;
    NHSHENT *hptr, *last;

    if (htab == NULL) {
      LOGTEXT("ERR", "nhashdelete was passed a NULL htab.");
      return;
    }

    hval = (val & htab->mask);
    last = NULL;
    for (hptr = htab->entry->element[hval];
	 hptr != NULL;
	 last = hptr, hptr = hptr->next) {
	if (val == hptr->target) {
	    if (last == NULL)
		htab->entry->element[hval] = hptr->next;
	    else
		last->next = hptr->next;
	    free(hptr);
	    htab->deletes++;
	    htab->entries--;
	    if (htab->entry->element[hval] == NULL)
		htab->nulls++;
	    return;
	}
    }
}

/* ---------------------------------------------------------------------------
 * nhashflush: free all the entries in a hashtable.
 */

void real_nhashflush(NHSHTAB * htab, int size, const char *fileName, int lineNo)
{
    NHSHENT *hent, *thent;
    int i;

    if (htab == NULL) {
      LOGTEXT("ERR", "nhashflush was passed a NULL htab.");
      return;
    }
    if (size < 0) {
      LOGTEXT("WRN", "nhashflush was passed a size < 0.");
      size = 50;
    }

    for (i = 0; i < htab->hashsize; i++) {
	hent = htab->entry->element[i];
	while (hent != NULL) {
	    thent = hent;
	    hent = hent->next;
	    free(thent);
	}
	htab->entry->element[i] = NULL;
    }

    /* Resize if needed.  Otherwise, just zero all the stats */

    if ((size > 0) && (size != htab->hashsize)) {
	free(htab->entry);
	nhashinit(htab, size);
    } else {
	htab->checks = 0;
	htab->scans = 0;
	htab->max_scan = 0;
	htab->hits = 0;
	htab->entries = 0;
	htab->deletes = 0;
	htab->nulls = htab->hashsize;
    }
}

/* ---------------------------------------------------------------------------
 * nhashrepl: replace the data part of a hash entry.
 */

int real_nhashrepl(int val, int *hashdata, NHSHTAB * htab, const char *fileName, int lineNo)
{
    NHSHENT *hptr;
    int hval;

    if (htab == NULL) {
      LOGTEXT("ERR", "nhashrepl was passed a NULL htab.");
      return 0;
    }
    if (hashdata == NULL) {
      LOGTEXT("ERR", "nhashrepl was passed a NULL hashdata.");
      return 0;
    }

    hval = (val & htab->mask);
    for (hptr = htab->entry->element[hval];
	 hptr != NULL;
	 hptr = hptr->next) {
	if (hptr->target == val) {
	    hptr->data = hashdata;
	    return 1;
	}
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * search_nametab: Search a name table for a match and return the flag value.
 */

int real_search_nametab(dbref player, NAMETAB * ntab, char *flagname, const char *fileName, int lineNo)
{
    NAMETAB *nt;

    if (ntab == NULL) {
      LOGTEXT("ERR", "search_nametab was passed a NULL ntab.");
      return -1;
    }
    if (flagname == NULL || !*flagname) {
      LOGTEXT("ERR", "search_nametab was passed a NULL flagname.");
      return -1;
    }

    for (nt = ntab; nt->name; nt++) {
	if (minmatch(flagname, nt->name, nt->minlen)) {
	    if (check_access(player, nt->perm, nt->perm2, 0)) {
		return nt->flag;
	    }
	}
    }
    return -1;
}

/* ---------------------------------------------------------------------------
 * find_nametab_ent: Search a name table for a match and return a pointer to it.
 */

NAMETAB * real_find_nametab_ent(dbref player, NAMETAB * ntab, char *flagname, const char *fileName, int lineNo)
{
    NAMETAB *nt;

    if (ntab == NULL) {
      LOGTEXT("ERR", "find_nametab_ent was passed a NULL ntab.");
      return NULL;
    }
    if (flagname == NULL || !*flagname) {
      LOGTEXT("ERR", "find_nametab_ent was passed a NULL flagname.");
      return NULL;
    }

    for (nt = ntab; nt->name; nt++) {
	if (minmatch(flagname, nt->name, nt->minlen)) {
	    if (check_access(player, nt->perm, nt->perm2, 0)) {
		return nt;
	    }
	}
    }
    return NULL;
}

/* ---------------------------------------------------------------------------
 * display_nametab: Print out the names of the entries in a name table.
 */

void real_display_nametab(dbref player, NAMETAB *ntab, char *prefix, int list_if_none, const char *fileName, int lineNo)
{
    char *buf, *bp, *cp;
    NAMETAB *nt;
    int got_one;

    if (ntab == NULL) {
      LOGTEXT("ERR", "display_nametab was passed a NULL ntab.");
      return;
    }
    if (prefix == NULL || !*prefix) {
      LOGTEXT("ERR", "display_nametab was passed a NULL prefix.");
      return;
    }

    buf = alloc_lbuf("display_nametab");
    bp = buf;
    got_one = 0;
    for (cp = prefix; *cp; cp++)
	*bp++ = *cp;
    for (nt = ntab; nt->name; nt++) {
	if (God(player) || check_access(player, nt->perm, nt->perm2, 0)) {
	    *bp++ = ' ';
	    for (cp = nt->name; *cp; cp++)
		*bp++ = *cp;
	    got_one = 1;
	}
    }
    *bp = '\0';
    if (got_one || list_if_none)
	notify(player, buf);
    free_lbuf(buf);
}



/* ---------------------------------------------------------------------------
 * interp_nametab: Print values for flags defined in name table.
 */

void real_interp_nametab(dbref player, NAMETAB * ntab, int flagword,
			 char *prefix, char *true_text, char *false_text,
			 const char *fileName, int lineNo)
{
    char *buf, *bp, *cp;
    NAMETAB *nt;

    if (ntab == NULL) {
      LOGTEXT("ERR", "interp_nametab was passed a NULL ntab.");
      return;
    }

    buf = alloc_lbuf("interp_nametab");
    bp = buf;
    for (cp = prefix; *cp; cp++)
	*bp++ = *cp;
    nt = ntab;
    while (nt->name) {
	if (God(player) || check_access(player, nt->perm, nt->perm2, 0)) {
	    *bp++ = ' ';
	    for (cp = nt->name; *cp; cp++)
		*bp++ = *cp;
	    *bp++ = '.';
	    *bp++ = '.';
	    *bp++ = '.';
	    if ((flagword & nt->flag) != 0)
		cp = true_text;
	    else
		cp = false_text;
	    while (*cp)
		*bp++ = *cp++;
	    if ((++nt)->name)
		*bp++ = ';';
	}
    }
    *bp = '\0';
    notify(player, buf);
    free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * listset_nametab: Print values for flags defined in name table.
 */

void real_listset_nametab(dbref player, NAMETAB *ntab, NAMETAB *ntab2, int flagword, int flagword2,
			  char *prefix, int list_if_none, const char *fileName, int lineNo)
{
    char *buf, *bp, *cp, *buf2, *bp2;
    NAMETAB *nt;
    int got_one, got_two;

    if (ntab == NULL) {
      LOGTEXT("ERR", "listset_nametab was passed a NULL ntab.");
      return;
    }

    buf = bp = alloc_lbuf("listset_nametab");
    for (cp = prefix; *cp; cp++)
	*bp++ = *cp;
    nt = ntab;
    got_one = got_two = 0;
    while (nt->name) {
	if (((flagword & nt->flag) != 0) &&
	    (God(player) || check_access(player, nt->perm, nt->perm2, 0))) {
	    *bp++ = ' ';
	    for (cp = nt->name; *cp; cp++)
		*bp++ = *cp;
	    got_one = 1;
	}
	nt++;
    }
    if ( ntab2 ) {
       nt = ntab2;
       bp2 = buf2 = alloc_lbuf("listset_nametab2");
       while (nt->name) {
	   if (((flagword2 & nt->flag) != 0) &&
	       (God(player) || check_access(player, nt->perm2, nt->perm2, 0))) {
               if ( !got_two )
	          *bp2++ = '[';
               else
	          *bp2++ = ' ';
	       for (cp = nt->name; *cp; cp++)
		   *bp2++ = *cp;
	       got_two = 1;
	   }
	   nt++;
       }
       if ( got_two )
          *bp2++ = ']';
       *bp2 = '\0'; 
       *bp++ = ' ';
       safe_str(buf2, buf, &bp);
       free_lbuf(buf2);
    }
    got_one |= got_two;
    *bp = '\0';
    if (got_one || list_if_none)
	notify(player, buf);
    free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * cf_ntab_access: Change the access on a nametab entry.
 */

extern CF_HDCL(cf_modify_bits);
extern void FDECL(cf_log_notfound, (dbref, char *, const char *, char *));

CF_HAND(cf_ntab_access)
{
    NAMETAB *np;
    char *ap;

    for (ap = str; *ap && !isspace((int)*ap); ap++);
    if (*ap)
	*ap++ = '\0';
    while (*ap && isspace((int)*ap))
	ap++;
    for (np = (NAMETAB *) vp; np->name; np++) {
	if (minmatch(str, np->name, np->minlen)) {
	    return cf_modify_multibits((int *)&(np->perm), (int *)&(np->perm2), ap, extra, extra2,
				     player, cmd);
	}
    }
    cf_log_notfound(player, cmd, "Entry", str);
    return -1;
}

/* Nothing calls this function, but it's really usefull for debugging a faulty
 * hashtable.
 *                                                                       Lensy
 */
void hashwalk_dump(HASHTAB *pHtab, char *ref) {
  HASHENT *hEntPtr;
  int i;
  FILE *pFile;
  
  if ( (pFile = fopen(unsafe_tprintf("htab_debug_%s", ref), "w+")) == NULL) {
    return;
  }
       
  for (i = 0 ; i < pHtab->hashsize ; i++) {
    fprintf(pFile, "--> Element (%d)\n", i);
    
    for (hEntPtr = pHtab->entry->element[i];
	 hEntPtr != NULL;
	 hEntPtr = hEntPtr->next) {
#ifdef BIT64
      fprintf(pFile, "{target = \"%s\", data = %lx, bIsOriginal = %d}\n",
#else
      fprintf(pFile, "{target = \"%s\", data = %x, bIsOriginal = %d}\n",
#endif
	      hEntPtr->target,
	      (pmath2) hEntPtr->data,
	      hEntPtr->bIsOriginal);
    }
  }
  
  fclose(pFile);
}
#endif
  
