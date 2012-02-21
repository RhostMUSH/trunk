/*************************************************************************************************
 * CGI script for full-text search
 *                                                      Copyright (C) 2000-2003 Mikio Hirabayashi
 * This file is part of QDBM, Quick Database Manager.
 * QDBM is free software; you can redistribute it and/or modify it under the terms of the GNU
 * Lesser General Public License as published by the Free Software Foundation; either version
 * 2.1 of the License or any later version.  QDBM is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * You should have received a copy of the GNU Lesser General Public License along with QDBM; if
 * not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA.
 *************************************************************************************************/


#include <depot.h>
#include <cabin.h>
#include <odeum.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#undef TRUE
#define TRUE           1                 /* boolean true */
#undef FALSE
#define FALSE          0                 /* boolean false */

#define CONFFILE    "qfts.conf"          /* name of the configuration file */
#define DEFENC      "US-ASCII"           /* default encoding */
#define DEFLANG     "en"                 /* default language */
#define DEFTITLE    "Odeum on WWW"       /* default title */
#define DEFINDEX    "casket"             /* directory containing database files */
#define DEFPREFIX   "./"                 /* prefix of the URI of a document */
#define RDATAMAX    262144               /* max size of data to read */
#define NUMBUFSIZ   32                   /* size of a buffer for a number */
#define DEFMAX      16                   /* default max number of shown documents */
#define SUMTOP      24                   /* number of adopted words as top of summary */
#define SUMWIDTH    16                   /* number of adopted words around a keyword */
#define SUMWORDMAX  96                   /* max number of words in summary */
#define KEYWORDS    16                   /* number of shown keywords */
#define EMCLASSNUM  6                    /* number of classes for em elements */
#define SCDBNAME    "_score"             /* name of the database for scores */
#define SCSHOWNUM   8                    /* number of shown scores */
#define RELKEYNUM   16                   /* number of words to use as relational search */
#define RELDOCMAX   2048                 /* number of target words as relational search */
#define RELVECNUM   32                   /* number of dimension of score vector */
#define PATHBUFSIZ  1024                 /* size of a path buffer */
#define PATHCHR     '/'                  /* delimiter character of path */

enum {
  UNITAND,
  UNITOR
};


/* for Win32 */
#if defined(_WIN32)
#undef PATHCHR
#define PATHCHR     '\\'
#endif


/* global variables */
const char *scriptname;                  /* name of the script */
const char *enc;                         /* encoding of the page */
const char *lang;                        /* language of the page */
const char *title;                       /* title of the page */


/* function prototypes */
int main(int argc, char **argv);
const char *skiplabel(const char *str);
CBMAP *getparams(void);
void senderror(int code, const char *tag, const char *message);
void htmlprintf(const char *format, ...);
void printmime(void);
void printdecl(void);
void printhead(void);
void showform(const char *phrase, int unit, const char *except, int max);
CBLIST *getwords(const char *phrase);
void setovec(CBMAP *scores, int *vec);
void settvec(CBMAP *osc, CBMAP *tsc, int *vec);
void showrelresult(int rel, int max, int skip, const char *index,
                   const char *prefix, const char *diridx, int decuri);
void showresult(const CBLIST *words, const char *phrase, int unit,
                const CBLIST *ewords, const char *except, int max, int skip,
                const char *index, const char *prefix, const char *diridx, int decuri);
void showsummary(const ODDOC *doc, const CBLIST *kwords, const char *phrase, int unit,
                 const char *except, int max, CBMAP *scores);
CBMAP *listtomap(const CBLIST *list);
void showwords(int id, const CBLIST *words, const char *phrase, int unit, const char *except,
               int max, const char *index, const char *prefix, const char *diridx, int decuri);
void showhelp(const CBLIST *help);


/* main routine */
int main(int argc, char **argv){
  CBMAP *params;
  CBLIST *lines, *help, *words, *ewords;
  const char *tmp, *index, *prefix, *except, *diridx, *phrase;
  int i, decuri, unit, max, skip, id, rel;
  /* set configurations */
  cbstdiobin();
  scriptname = argv[0];
  if((tmp = getenv("SCRIPT_NAME")) != NULL) scriptname = tmp;
  enc = NULL;
  lang = NULL;
  title = NULL;
  index = NULL;
  prefix = NULL;
  diridx = NULL;
  decuri = FALSE;
  help = cblistopen();;
  if((lines = cbreadlines(CONFFILE)) != NULL){
    for(i = 0; i < cblistnum(lines); i++){
      tmp = cblistval(lines, i, NULL);
      if(cbstrfwmatch(tmp, "encoding:")){
        enc = skiplabel(tmp);
      } else if(cbstrfwmatch(tmp, "lang:")){
        lang = skiplabel(tmp);
      } else if(cbstrfwmatch(tmp, "title:")){
        title = skiplabel(tmp);
      } else if(cbstrfwmatch(tmp, "index:")){
        index = skiplabel(tmp);
      } else if(cbstrfwmatch(tmp, "prefix:")){
        prefix = skiplabel(tmp);
      } else if(cbstrfwmatch(tmp, "diridx:")){
        diridx = skiplabel(tmp);
      } else if(cbstrfwmatch(tmp, "decuri:")){
        decuri = !strcmp(skiplabel(tmp), "true");
      } else if(cbstrfwmatch(tmp, "help:")){
        cblistpush(help, skiplabel(tmp), -1);
      }
    }
  }
  if(!enc) enc = DEFENC;
  if(!lang) lang = DEFLANG;
  if(!title) title = DEFTITLE;
  if(!index) index = DEFINDEX;
  if(!prefix) prefix = DEFPREFIX;
  /* read parameters */
  phrase = NULL;
  except = NULL;
  unit = UNITAND;
  max = 0;
  skip = 0;
  id = 0;
  rel = 0;
  params = getparams();
  if((tmp = cbmapget(params, "phrase", -1, NULL)) != NULL) phrase = tmp;
  if((tmp = cbmapget(params, "except", -1, NULL)) != NULL) except = tmp;
  if((tmp = cbmapget(params, "unit", -1, NULL)) != NULL) unit = atoi(tmp);
  if((tmp = cbmapget(params, "max", -1, NULL)) != NULL) max = atoi(tmp);
  if((tmp = cbmapget(params, "skip", -1, NULL)) != NULL) skip = atoi(tmp);
  if((tmp = cbmapget(params, "id", -1, NULL)) != NULL) id = atoi(tmp);
  if((tmp = cbmapget(params, "rel", -1, NULL)) != NULL) rel = atoi(tmp);
  if(!phrase) phrase = "";
  if(!except) except = "";
  if(max < 1) max = DEFMAX;
  if(skip < 0) skip = 0;
  /* show page */
  printmime();
  printdecl();
  htmlprintf("<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"%s\" lang=\"%s\">\n",
             lang, lang);
  printhead();
  htmlprintf("<body>\n");
  showform(phrase, unit, except, max);
  htmlprintf("<div class=\"result\">\n");
  words = getwords(phrase);
  ewords = getwords(except);
  if(id > 0){
    showwords(id, words, phrase, unit, except, max, index, prefix, diridx, decuri);
  } else if(rel > 0){
    showrelresult(rel, max, skip, index, prefix, diridx, decuri);
  } else if(cblistnum(words) > 0){
    showresult(words, phrase, unit, ewords, except, max, skip, index, prefix, diridx, decuri);
  } else if(strlen(phrase) > 0){
    htmlprintf("<p>No effective word was extracted from the phrase.</p>\n");
  } else {
    showhelp(help);
  }
  cblistclose(ewords);
  cblistclose(words);
  htmlprintf("</div>\n");
  htmlprintf("</body>\n");
  htmlprintf("</html>\n");
  /* release resources */
  cbmapclose(params);
  if(lines) cblistclose(lines);
  cblistclose(help);
  return 0;
}


/* skip the label of a line */
const char *skiplabel(const char *str){
  if(!(str = strchr(str, ':'))) return str;
  str++;
  while(*str != '\0' && (*str == ' ' || *str == '\t')){
    str++;
  }
  return str;
}


/* get a map of the CGI parameters */
CBMAP *getparams(void){
  CBMAP *params;
  CBLIST *pairs;
  char *rbuf, *buf, *key, *val, *dkey, *dval;
  const char *tmp;
  int i, len, c;
  params = cbmapopen();
  rbuf = NULL;
  buf = NULL;
  if((tmp = getenv("CONTENT_LENGTH")) != NULL && (len = atoi(tmp)) > 0 && len <= RDATAMAX){
    rbuf = cbmalloc(len + 1);
    for(i = 0; i < len && (c = getchar()) != EOF; i++){
      rbuf[i] = c;
    }
    rbuf[i] = '\0';
    if(i == len) buf = rbuf;
  } else {
    buf = getenv("QUERY_STRING");
  }
  if(buf != NULL){
    buf = cbmemdup(buf, -1);
    pairs = cbsplit(buf, -1, "&");
    for(i = 0; i < cblistnum(pairs); i++){
      key = cbmemdup(cblistval(pairs, i, NULL), -1);
      if((val = strchr(key, '=')) != NULL){
        *(val++) = '\0';
        dkey = cburldecode(key, NULL);
        dval = cburldecode(val, NULL);
        cbmapput(params, dkey, -1, dval, -1, FALSE);
        free(dval);
        free(dkey);
      }
      free(key);
    }
    cblistclose(pairs);
    free(buf);
  }
  free(rbuf);
  return params;
}


/* send error status */
void senderror(int code, const char *tag, const char *message){
  printf("Status: %d %s\r\n", code, tag);
  printf("Content-Type: text/plain; charset=US-ASCII\r\n");
  printf("\r\n");
  printf("%s\n", message);
}


/* HTML-oriented printf */
void htmlprintf(const char *format, ...){
  va_list ap;
  char *tmp;
  unsigned char c;
  va_start(ap, format);
  while(*format != '\0'){
    if(*format == '%'){
      format++;
      switch(*format){
      case 's':
        tmp = va_arg(ap, char *);
        if(!tmp) tmp = "(null)";
        printf("%s", tmp);
        break;
      case 'd':
        printf("%d", va_arg(ap, int));
        break;
      case '@':
        tmp = va_arg(ap, char *);
        if(!tmp) tmp = "(null)";
        while(*tmp){
          switch(*tmp){
          case '&': printf("&amp;"); break;
          case '<': printf("&lt;"); break;
          case '>': printf("&gt;"); break;
          case '"': printf("&quot;"); break;
          default: putchar(*tmp); break;
          }
          tmp++;
        }
        break;
      case '?':
        tmp = va_arg(ap, char *);
        if(!tmp) tmp = "(null)";
        while(*tmp){
          c = *(unsigned char *)tmp;
          if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
             (c >= '0' && c <= '9') || (c != '\0' && strchr("_-.", c))){
            putchar(c);
          } else if(c == ' '){
            putchar('+');
          } else {
            printf("%%%02X", c);
          }
          tmp++;
        }
        break;
      case '%':
        putchar('%');
        break;
      }
    } else {
      putchar(*format);
    }
    format++;
  }
  va_end(ap);
}


/* print mime headers */
void printmime(void){
  printf("Content-Type: text/html; charset=%s\r\n", enc);
  printf("Cache-Control: no-cache, must-revalidate\r\n");
  printf("Pragma: no-cache\r\n");
  printf("\r\n");
  fflush(stdout);
}


/* print the declarations of XHTML */
void printdecl(void){
  htmlprintf("<?xml version=\"1.0\" encoding=\"%s\"?>\n", enc);
  htmlprintf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
             "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n");
}


/* print headers */
void printhead(void){
  htmlprintf("<head>\n");
  htmlprintf("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%s\" />\n", enc);
  htmlprintf("<meta http-equiv=\"Content-Style-Type\" content=\"text/css\" />\n");
  htmlprintf("<link rel=\"contents\" href=\"./\" />\n");
  htmlprintf("<title>%@</title>\n", title);
  htmlprintf("<style type=\"text/css\">\n");
  htmlprintf("body { background-color: #eeeeee; color: #111111;"
             " margin: 0em 0em; padding: 0em 0em; }\n");
  htmlprintf("h1,h2,p { margin: 0.5em 0.8em; }\n");
  htmlprintf("a { color: #0022aa; text-decoration: none; }\n");
  htmlprintf("a:hover { color: #1144ff; text-decoration: underline; }\n");
  htmlprintf("a.head { color: #111111; text-decoration: none; }\n");
  htmlprintf("em { font-weight: bold; font-style: normal; color: #001166; }\n");
  htmlprintf("form { background-color: #ddddee; margin: 0em 0em; padding: 0.7em 0.8em;"
             " border-bottom: 1pt solid #bbbbcc; }\n");
  htmlprintf("dd { margin: 0.1em 0.3em 0.1em 1.8em; font-size: smaller; }\n");
  htmlprintf(".result { margin: 0em 0em; padding: 0.5em 1.0em; }\n");
  htmlprintf(".title { font-weight: bold; }\n");
  htmlprintf(".summary { background-color: #ddddee; padding: 0.2em 0.3em;"
             " border: 1pt solid #bbbbcc; }\n");
  htmlprintf(".summary em { border: 1pt solid #bbcccc; padding: 0.0em 0.2em; }\n");
  htmlprintf(".key0 { background-color: #ddeeee; color: #001177; }\n");
  htmlprintf(".key1 { background-color: #eeddee; color: #002255; }\n");
  htmlprintf(".key2 { background-color: #eeeedd; color: #112233; }\n");
  htmlprintf(".key3 { background-color: #ccddee; color: #112233; }\n");
  htmlprintf(".key4 { background-color: #ddffee; color: #112233; }\n");
  htmlprintf(".key5 { background-color: #ffeedd; color: #112233; }\n");
  htmlprintf(".note { margin: 0.5em 0.5em; text-align: right; color: #666666; }\n");
  htmlprintf(".blur { color: #666666; }\n");
  htmlprintf(".missing { color: #888888; }\n");
  htmlprintf("</style>\n");
  htmlprintf("</head>\n");
}


/* show search form */
void showform(const char *phrase, int unit, const char *except, int max){
  int i;
  htmlprintf("<form action=\"%@\" method=\"get\">\n", scriptname);
  htmlprintf("<div>\n");
  htmlprintf("Phrase: <input type=\"text\" name=\"phrase\" value=\"%@\" size=\"48\" />\n",
             phrase);
  htmlprintf("<select name=\"unit\">\n");
  htmlprintf("<option value=\"%d\"%s>all of them</option>\n",
             UNITAND, unit == UNITAND ? " selected=\"selected\"" : "");
  htmlprintf("<option value=\"%d\"%s>any of them</option>\n",
             UNITOR, unit == UNITOR ? " selected=\"selected\"" : "");
  htmlprintf("</select>\n");
  htmlprintf("/\n");
  htmlprintf("Except: <input type=\"text\" name=\"except\" value=\"%@\" size=\"16\" />\n",
             except);
  htmlprintf("/\n");
  htmlprintf("<select name=\"max\">\n");
  for(i = 1; i <= 256; i *= 2){
    htmlprintf("<option value=\"%d\"%s>%d per page</option>\n",
               i, i == max ? " selected=\"selected\"" : "", i);
  }
  htmlprintf("</select>\n");
  htmlprintf("/\n");
  htmlprintf("<input type=\"submit\" value=\"Search\" />\n");
  htmlprintf("</div>\n");
  htmlprintf("</form>\n");
}


/* break phrase into words */
CBLIST *getwords(const char *phrase){
  CBLIST *words, *tmp;
  char *normal;
  int i;
  words = cblistopen();
  tmp = odbreaktext(phrase);
  for(i = 0; i < cblistnum(tmp); i++){
    normal = odnormalizeword(cblistval(tmp, i, NULL));
    if(strlen(normal) > 0) cblistpush(words, normal, -1);
    free(normal);
  }
  cblistclose(tmp);
  return words;
}


/* set the original score vector */
void setovec(CBMAP *scores, int *vec){
  int i;
  const char *kbuf;
  cbmapiterinit(scores);
  for(i = 0; i < RELVECNUM; i++){
    if((kbuf = cbmapiternext(scores, NULL)) != NULL){
      vec[i] = atoi(cbmapget(scores, kbuf, -1, NULL));
    } else {
      vec[i] = 0;
    }
  }
}


/* set the target score vector */
void settvec(CBMAP *osc, CBMAP *tsc, int *vec){
  int i;
  const char *kbuf, *vbuf;
  if(tsc){
    cbmapiterinit(osc);
    for(i = 0; i < RELVECNUM; i++){
      if((kbuf = cbmapiternext(osc, NULL)) != NULL){
        vbuf = cbmapget(tsc, kbuf, -1, NULL);
        vec[i] = vbuf ? atoi(vbuf) : 0;
      } else {
        vec[i] = 0;
      }
    }
  } else {
    for(i = 0; i < RELVECNUM; i++){
      vec[i] = 0;
    }
  }
}


/* show relational search result */
void showrelresult(int rel, int max, int skip, const char *index,
                   const char *prefix, const char *diridx, int decuri){
  ODEUM *odeum;
  DEPOT *scdb;
  ODDOC *doc, *tdoc;
  CBMAP *scores, *tsc;
  CBLIST *words;
  ODPAIR *pairs, *last, *tmp;
  const char *uri, *word, *title, *date, *author;
  char path[PATHBUFSIZ], *ubuf, *mbuf, *tmbuf, *tubuf, numbuf[NUMBUFSIZ];
  int i, j, ulen, msiz, tmsiz, pnum, hnum, lnum, tnum;
  int ovec[RELVECNUM], tvec[RELVECNUM], id, dnum, wnum;
  double ival, ustime, sstime, uetime, setime;
  cbproctime(&ustime, &sstime);
  if(!(odeum = odopen(index, OD_OREADER))){
    htmlprintf("<p>The index cannot be open because of `%@'.</p>\n", dperrmsg(dpecode));
    return;
  }
  sprintf(path, "%s%c%s", index, PATHCHR, SCDBNAME);
  if(!(scdb = dpopen(path, DP_OREADER, -1))){
    htmlprintf("<p>The score database cannot be open because of `%@'.</p>\n", dperrmsg(dpecode));
    odclose(odeum);
    return;
  }
  if(!(doc = odgetbyid(odeum, rel))){
    htmlprintf("<p>The document cannot be got because of `%@'.</p>\n", dperrmsg(dpecode));
    dpclose(scdb);
    odclose(odeum);
  }
  ubuf = decuri ? cburldecode(oddocuri(doc), NULL) : cbmemdup(oddocuri(doc), -1);
  if(diridx && cbstrbwmatch(ubuf, diridx)){
    ulen = strlen(ubuf) - strlen(diridx);
    if(ulen > 1 && ubuf[ulen-1] == '/') ubuf[ulen] = '\0';
  }
  uri = ubuf;
  if(cbstrfwmatch(uri, DEFPREFIX)) uri += strlen(DEFPREFIX);
  if(!(mbuf = dpget(scdb, (char *)&rel, sizeof(int), 0, -1, &msiz))){
    htmlprintf("<p>Scores cannot be got because of `%@'.</p>\n", dperrmsg(dpecode));
    free(ubuf);
    oddocclose(doc);
    dpclose(scdb);
    odclose(odeum);
  }
  scores = cbmapload(mbuf, msiz);
  words = cbmapkeys(scores);
  last = NULL;
  lnum = 0;
  for(i = 0; i < RELKEYNUM && i < cblistnum(words); i++){
    word = cblistval(words, i, NULL);
    if(!(pairs = odsearch(odeum, word, RELDOCMAX, &pnum))) continue;
    if((hnum = odsearchdnum(odeum, word)) < 0) hnum = 0;
    ival = odlogarithm(hnum);
    ival = (ival * ival) / 4.0;
    if(ival < 4.0) ival = 4.0;
    for(j = 0; j < pnum; j++){
      pairs[j].score /= ival;
    }
    if(last){
      tmp = odpairsor(last, lnum, pairs, pnum, &tnum);
      free(last);
      free(pairs);
      last = tmp;
      lnum = tnum;
    } else {
      last = pairs;
      lnum = pnum;
    }
  }
  if(last && lnum > 0){
    setovec(scores, ovec);
    for(i = 0; i < lnum; i++){
      if((tmbuf = dpget(scdb, (char *)&(last[i].id), sizeof(int), 0, -1, &tmsiz)) != NULL){
        tsc = cbmapload(tmbuf, tmsiz);
        free(tmbuf);
      } else {
        tsc = NULL;
      }
      settvec(scores, tsc, tvec);
      if(tsc) cbmapclose(tsc);
      last[i].score = odvectorcosine(ovec, tvec, RELVECNUM) * 10000;
      if(last[i].score >= 9999) last[i].score = 10000;
    }
    odpairssort(last, lnum);
    for(i = 0; i < lnum; i++){
      if(last[i].score < 1){
        lnum = i;
        break;
      }
    }
  }
  if(last && lnum > 0){
    htmlprintf("<p>Related documents with <a href=\"%@%@\">%@%@</a> : <em>%d</em> hits</p>\n",
               prefix, uri, prefix, uri, lnum);
    for(i = skip; i < lnum && i < max + skip; i++){
      if(!(tdoc = odgetbyid(odeum, last[i].id))){
        htmlprintf("<dl>\n");
        htmlprintf("<dt class=\"missing\">%d: (replaced or purged)</dt>\n", i + 1);
        htmlprintf("</dl>\n");
        continue;
      }
      tubuf = decuri ? cburldecode(oddocuri(tdoc), NULL) : cbmemdup(oddocuri(tdoc), -1);
      if(diridx && cbstrbwmatch(tubuf, diridx)){
        ulen = strlen(tubuf) - strlen(diridx);
        if(ulen > 1 && tubuf[ulen-1] == '/') tubuf[ulen] = '\0';
      }
      uri = tubuf;
      if(cbstrfwmatch(uri, DEFPREFIX)) uri += strlen(DEFPREFIX);
      title = oddocgetattr(tdoc, "title");
      date = oddocgetattr(tdoc, "date");
      author = oddocgetattr(tdoc, "author");
      htmlprintf("<dl>\n");
      sprintf(numbuf, "%.2f", (double)last[i].score / 100.0);
      htmlprintf("<dt class=\"blur\">%d: <a href=\"%@%@\" class=\"title\">%@</a>"
                 " (%@%%)</dt>\n", i + 1, prefix, uri,
                 title && strlen(title) > 0 ? title : "(untitled)", numbuf);
      tsc = NULL;
      if(scdb){
        id = oddocid(tdoc);
        if((tmbuf = dpget(scdb, (char *)&id, sizeof(int), 0, -1, &tmsiz)) != NULL){
          tsc = cbmapload(tmbuf, tmsiz);
          free(tmbuf);
        }
      }
      showsummary(tdoc, NULL, "", 0, "", max, tsc);
      if(tsc) cbmapclose(tsc);
      htmlprintf("<dd class=\"blur\">");
      htmlprintf("%@%@", prefix, uri);
      if(date) htmlprintf(" (%@)", date);
      if(author) htmlprintf(" (%@)", author);
      htmlprintf("</dd>\n");
      htmlprintf("</dl>\n");
      free(tubuf);
      oddocclose(tdoc);
    }
  } else {
    htmlprintf("<p>No document hits.</p>\n");
  }
  if(last) free(last);
  htmlprintf("<div class=\"note\">");
  if(skip > 0){
    htmlprintf("<a href=\"%@?rel=%d&amp;max=%d&amp;skip=%d\">[PREV]</a>",
               scriptname, rel, max, skip - max);
  } else {
    htmlprintf("<span class=\"blur\">[PREV]</span>");
  }
  htmlprintf(" ");
  if(i < lnum){
    htmlprintf("<a href=\"%@?rel=%d&amp;max=%d&amp;skip=%d\">[NEXT]</a>",
               scriptname, rel, max, skip + max);
  } else {
    htmlprintf("<span class=\"blur\">[NEXT]</span>");
  }
  htmlprintf("</div>\n");
  cbproctime(&uetime, &setime);
  uetime -= ustime;
  setime -= sstime;
  dnum = oddnum(odeum);
  wnum = odwnum(odeum);
  cblistclose(words);
  cbmapclose(scores);
  free(mbuf);
  free(ubuf);
  oddocclose(doc);
  dpclose(scdb);
  odclose(odeum);
  printf("<div class=\"note\">\n");
  printf("The index contains %d documents and %d words.\n", dnum, wnum);
  printf("Processing time is %.2f sec. (user:%.2f + sys:%.2f)\n",
         uetime + setime, uetime, setime);
  printf("</div>\n");
}


/* show search result */
void showresult(const CBLIST *words, const char *phrase, int unit,
                const CBLIST *ewords, const char *except, int max, int skip,
                const char *index, const char *prefix, const char *diridx, int decuri){
  ODEUM *odeum;
  DEPOT *scdb;
  ODPAIR *pairs, *last, *tmp;
  ODDOC *doc;
  CBMAP *scores;
  const char *word, *uri, *title, *date, *author;
  char path[PATHBUFSIZ], *ubuf, *mbuf;
  int i, j, smax, pnum, ezpnum, lnum, hnum, tnum, ulen, id, msiz, dnum, wnum;
  double ival, ustime, sstime, uetime, setime;
  cbproctime(&ustime, &sstime);
  if(!(odeum = odopen(index, OD_OREADER))){
    htmlprintf("<p>The index cannot be open because of `%@'.</p>\n", dperrmsg(dpecode));
    return;
  }
  sprintf(path, "%s%c%s", index, PATHCHR, SCDBNAME);
  scdb = dpopen(path, DP_OREADER, -1);
  htmlprintf("<p>");
  last = NULL;
  lnum = 0;
  smax = -1;
  ezpnum = -1;
  if(cblistnum(words) < 2 && cblistnum(ewords) < 1) smax = max + skip;
  for(i = 0; i < cblistnum(words); i++){
    word = cblistval(words, i, NULL);
    if(!(pairs = odsearch(odeum, word, smax, &pnum))) continue;
    if(smax >= 0) ezpnum = odsearchdnum(odeum, word);
    if((hnum = odsearchdnum(odeum, word)) < 0) hnum = 0;
    ival = odlogarithm(hnum);
    ival = (ival * ival) / 2.0;
    if(ival < 2.0) ival = 2.0;
    for(j = 0; j < pnum; j++){
      pairs[j].score /= ival;
    }
    if(last) htmlprintf(" %@ ", unit == UNITOR ? "+" : "*");
    htmlprintf("<em>%@</em> (%d)", word, hnum);
    if(last){
      if(unit == UNITOR){
        tmp = odpairsor(last, lnum, pairs, pnum, &tnum);
      } else {
        tmp = odpairsand(last, lnum, pairs, pnum, &tnum);
      }
      free(last);
      free(pairs);
      last = tmp;
      lnum = tnum;
    } else {
      last = pairs;
      lnum = pnum;
    }
  }
  for(i = 0; i < cblistnum(ewords); i++){
    word = cblistval(ewords, i, NULL);
    if(!(pairs = odsearch(odeum, word, -1, &pnum))) continue;
    if((hnum = odsearchdnum(odeum, word)) < 0) hnum = 0;
    htmlprintf(" - <em>%@</em> (%d)", word, hnum);
    if(last){
      tmp = odpairsnotand(last, lnum, pairs, pnum, &tnum);
      free(last);
      free(pairs);
      last = tmp;
      lnum = tnum;
    } else {
      free(pairs);
    }
  }
  htmlprintf(" = <em>%d</em> hits</p>\n", ezpnum >= 0 ? ezpnum : lnum);
  if(last && lnum > 0){
    for(i = skip; i < lnum && i < max + skip; i++){
      if(!(doc = odgetbyid(odeum, last[i].id))){
        htmlprintf("<dl>\n");
        htmlprintf("<dt class=\"missing\">%d: (replaced or purged)</dt>\n", i + 1);
        htmlprintf("</dl>\n");
        continue;
      }
      ubuf = decuri ? cburldecode(oddocuri(doc), NULL) : cbmemdup(oddocuri(doc), -1);
      if(diridx && cbstrbwmatch(ubuf, diridx)){
        ulen = strlen(ubuf) - strlen(diridx);
        if(ulen > 1 && ubuf[ulen-1] == '/') ubuf[ulen] = '\0';
      }
      uri = ubuf;
      if(cbstrfwmatch(uri, DEFPREFIX)) uri += strlen(DEFPREFIX);
      title = oddocgetattr(doc, "title");
      date = oddocgetattr(doc, "date");
      author = oddocgetattr(doc, "author");
      htmlprintf("<dl>\n");
      htmlprintf("<dt class=\"blur\">%d: <a href=\"%@%@\" class=\"title\">%@</a> (%d pt.)</dt>\n",
                 i + 1, prefix, uri,
                 title && strlen(title) > 0 ? title : "(untitled)", last[i].score);
      scores = NULL;
      if(scdb){
        id = oddocid(doc);
        if((mbuf = dpget(scdb, (char *)&id, sizeof(int), 0, -1, &msiz)) != NULL){
          scores = cbmapload(mbuf, msiz);
          free(mbuf);
        }
      }
      showsummary(doc, words, phrase, unit, except, max, scores);
      if(scores) cbmapclose(scores);
      htmlprintf("<dd class=\"blur\">");
      htmlprintf("%@%@", prefix, uri);
      if(date) htmlprintf(" (%@)", date);
      if(author) htmlprintf(" (%@)", author);
      htmlprintf("</dd>\n");
      htmlprintf("</dl>\n");
      free(ubuf);
      oddocclose(doc);
    }
  } else {
    htmlprintf("<p>No document hits.</p>\n");
  }
  if(last) free(last);
  if(ezpnum >= 0) lnum = ezpnum;
  htmlprintf("<div class=\"note\">");
  if(skip > 0){
    htmlprintf("<a href=\"%@?phrase=%?&amp;unit=%d&amp;except=%?&amp;max=%d&amp;skip=%d\">"
               "[PREV]</a>", scriptname, phrase, unit, except, max, skip - max);
  } else {
    htmlprintf("<span class=\"blur\">[PREV]</span>");
  }
  htmlprintf(" ");
  if(i < lnum){
    htmlprintf("<a href=\"%@?phrase=%?&amp;unit=%d&amp;except=%?&amp;max=%d&amp;skip=%d\">"
               "[NEXT]</a>", scriptname, phrase, unit, except, max, skip + max);
  } else {
    htmlprintf("<span class=\"blur\">[NEXT]</span>");
  }
  htmlprintf("</div>\n");
  cbproctime(&uetime, &setime);
  uetime -= ustime;
  setime -= sstime;
  dnum = oddnum(odeum);
  wnum = odwnum(odeum);
  if(scdb) dpclose(scdb);
  odclose(odeum);
  printf("<div class=\"note\">\n");
  printf("The index contains %d documents and %d words.\n", dnum, wnum);
  printf("Processing time is %.2f sec. (user:%.2f + sys:%.2f)\n",
         uetime + setime, uetime, setime);
  printf("</div>\n");
}


/* show summary of a document */
void showsummary(const ODDOC *doc, const CBLIST *kwords, const char *phrase, int unit,
                 const char *except, int max, CBMAP *scores){
  const CBLIST *nwords, *awords;
  CBMAP *kmap, *map;
  const char *normal, *asis, *kbuf;
  int i, j, num, lnum, tnum, nwsiz, awsiz, pv, bi, first, em;
  htmlprintf("<dd class=\"summary\">");
  num = SUMWORDMAX;
  nwords = oddocnwords(doc);
  awords = oddocawords(doc);
  kmap = kwords ? listtomap(kwords) : cbmapopen();
  map = kwords ? listtomap(kwords) : cbmapopen();
  lnum = cblistnum(nwords);
  tnum = 0;
  first = TRUE;
  em = FALSE;
  for(i = 0; i < lnum && tnum < (kwords ? SUMTOP : SUMWORDMAX); i++){
    normal = cblistval(nwords, i, &nwsiz);
    asis = cblistval(awords, i, &awsiz);
    if(awsiz < 1) continue;
    cbmapout(map, normal, nwsiz);
    if(normal[0] != '\0' && cbmapget(kmap, normal, nwsiz, NULL)){
      if(!first) htmlprintf(" ");
      if(!em) htmlprintf("<em class=\"key%d\">",
                         kwords ? cblistlsearch(kwords, normal, nwsiz) % EMCLASSNUM : 0);
      em = TRUE;
    } else {
      if(em) htmlprintf("</em>");
      if(!first) htmlprintf(" ");
      em = FALSE;
    }
    htmlprintf("%@", asis);
    first = FALSE;
    tnum++;
    num--;
  }
  if(em) htmlprintf("</em>");
  htmlprintf(" ...");
  em = FALSE;
  pv = i;
  while(i < lnum){
    if(cbmaprnum(map) < 1){
      cbmapclose(map);
      map = kwords ? listtomap(kwords) : cbmapopen();
    }
    normal = cblistval(nwords, i, &nwsiz);
    if(cbmapget(map, normal, nwsiz, NULL)){
      bi = i - SUMWIDTH / 2;
      bi = bi > pv ? bi : pv;
      for(j = bi; j < lnum && j <= bi + SUMWIDTH; j++){
        normal = cblistval(nwords, j, &nwsiz);
        asis = cblistval(awords, j, &awsiz);
        if(awsiz < 1) continue;
        cbmapout(map, normal, nwsiz);
        if(normal[0] != '\0' && cbmapget(kmap, normal, nwsiz, NULL)){
          htmlprintf(" ");
          if(!em) htmlprintf("<em class=\"key%d\">",
                             kwords ? cblistlsearch(kwords, normal, nwsiz) % EMCLASSNUM : 0);
          htmlprintf("%@", asis);
          em = TRUE;
        } else {
          if(em) htmlprintf("</em>");
          htmlprintf(" ");
          htmlprintf("%@", asis);
          em = FALSE;
        }
        num--;
      }
      if(em) htmlprintf("</em>");
      htmlprintf(" ...");
      em = FALSE;
      i = j;
      pv = i;
    } else {
      i++;
    }
    if(num <= 0) break;
  }
  if(pv < lnum - SUMWIDTH && num > SUMWIDTH){
    for(i = pv; i < lnum && num > 0; i++){
      asis = cblistval(awords, i, NULL);
      htmlprintf(" %@", asis);
      num--;
    }
    htmlprintf(" ...");
  }
  htmlprintf(" <a href=\"%@?id=%d&amp;phrase=%?&amp;unit=%d&amp;except=%?&amp;max=%d\">"
             "[more]</a>", scriptname, oddocid(doc), phrase, unit, except, max);
  if(scores){
    htmlprintf("<div class=\"blur\">(");
    cbmapiterinit(scores);
    for(i = 0; i < SCSHOWNUM && (kbuf = cbmapiternext(scores, NULL)) != NULL; i++){
      if(i > 0) htmlprintf(" / ");
      if(strlen(phrase) > 0){
        htmlprintf("<a href=\"%@?phrase=%?+%?&amp;&amp;unit=%d&amp;except=%?&amp;max=%d\">%@</a>",
                   scriptname, phrase, kbuf, unit, except, max, kbuf);
      } else {
        htmlprintf("<a href=\"%@?phrase=%?&amp;&amp;unit=%d&amp;except=%?&amp;max=%d\">%@</a>",
                   scriptname, kbuf, unit, except, max, kbuf);
      }
    }
    htmlprintf(") : <a href=\"%@?max=%d&amp;rel=%d\">[related]</a></div>",
               scriptname, max, oddocid(doc));
  }
  cbmapclose(map);
  cbmapclose(kmap);
  htmlprintf("</dd>\n");
}


/* get a map made from a list */
CBMAP *listtomap(const CBLIST *list){
  CBMAP *map;
  const char *tmp;
  int i, tsiz;
  map = cbmapopen();
  for(i = 0; i < cblistnum(list); i++){
    tmp = cblistval(list, i, &tsiz);
    cbmapput(map, tmp, tsiz, "", 0, FALSE);
  }
  return map;
}


/* show words in a document */
void showwords(int id, const CBLIST *words, const char *phrase, int unit, const char *except,
               int max, const char *index, const char *prefix, const char *diridx, int decuri){
  ODEUM *odeum;
  ODDOC *doc;
  const CBLIST *awords, *nwords;
  CBLIST *kwords;
  CBMAP *kmap;
  const char *uri, *title, *date, *author, *kword, *asis, *normal;
  char *ubuf;
  int i, wnum, nwsiz, awsiz, first, em, ulen;
  if(!(odeum = odopen(index, OD_OREADER))){
    htmlprintf("<p>The index cannot be open because of `%@'.</p>\n", dperrmsg(dpecode));
    return;
  }
  if((doc = odgetbyid(odeum, id)) != NULL){
    ubuf = decuri ? cburldecode(oddocuri(doc), NULL) : cbmemdup(oddocuri(doc), -1);
    if(diridx && cbstrbwmatch(ubuf, diridx)){
      ulen = strlen(ubuf) - strlen(diridx);
      if(ulen > 1 && ubuf[ulen-1] == '/') ubuf[ulen] = '\0';
    }
    uri = ubuf;
    if(cbstrfwmatch(uri, DEFPREFIX)) uri += strlen(DEFPREFIX);
    title = oddocgetattr(doc, "title");
    date = oddocgetattr(doc, "date");
    author = oddocgetattr(doc, "author");
    awords = oddocawords(doc);
    nwords = oddocnwords(doc);
    htmlprintf("<div>ID: %d</div>\n", oddocid(doc));
    htmlprintf("<div>URI: <a href=\"%@%@\">%@%@</a></div>\n", prefix, uri, prefix, uri);
    if(title) htmlprintf("<div>Title: <span class=\"title\">%@</span></div>\n", title);
    if(date) htmlprintf("<div>Date: %@</div>\n", date);
    if(author) htmlprintf("<div>Author: %@</div>\n", author);
    kmap = oddocscores(doc, KEYWORDS, odeum);
    kwords = cbmapkeys(kmap);
    htmlprintf("<div>Keywords: ");
    for(i = 0; i < cblistnum(kwords); i++){
      kword = cblistval(kwords, i, NULL);
      if(i > 0) htmlprintf(", ");
      if(strlen(phrase) > 0){
        htmlprintf("<a href=\"%@?phrase=%?+%?&amp;&amp;unit=%d&amp;except=%?&amp;max=%d\">%@</a>",
                   scriptname, phrase, kword, unit, except, max, kword);
      } else {
        htmlprintf("<a href=\"%@?phrase=%?&amp;&amp;unit=%d&amp;except=%?&amp;max=%d\">%@</a>",
                   scriptname, kword, unit, except, max, kword);
      }
    }
    htmlprintf(" : <a href=\"%@?max=%d&amp;rel=%d\">[related]</a>", scriptname, max, id);
    htmlprintf("</div>\n");
    cblistclose(kwords);
    cbmapclose(kmap);
    wnum = cblistnum(awords);
    kmap = listtomap(words);
    htmlprintf("<dl>\n");
    htmlprintf("<dt>Words: %d (or more)</dt>\n", wnum);
    htmlprintf("<dd class=\"summary\">");
    first = TRUE;
    em = FALSE;
    for(i = 0; i < wnum; i++){
      normal = cblistval(nwords, i, &nwsiz);
      asis = cblistval(awords, i, &awsiz);
      if(awsiz < 1) continue;
      if(normal[0] != '\0' && cbmapget(kmap, normal, nwsiz, NULL)){
        if(!first) htmlprintf(" ");
        if(!em) htmlprintf("<em class=\"key%d\">",
                           cblistlsearch(words, normal, nwsiz) % EMCLASSNUM);
        em = TRUE;
      } else {
        if(em) htmlprintf("</em>");
        if(!first) htmlprintf(" ");
        em = FALSE;
      }
      htmlprintf("%@", asis);
      first = FALSE;
    }
    if(em) htmlprintf("</em>");
    htmlprintf(" ...");
    htmlprintf("</dd>\n");
    htmlprintf("</dl>\n");
    cbmapclose(kmap);
    free(ubuf);
    oddocclose(doc);
  } else {
    htmlprintf("<p>Retrieving the document failed because of `%@'.</p>\n", dperrmsg(dpecode));
  }
  odclose(odeum);
}


/* show help message */
void showhelp(const CBLIST *help){
  int i;
  for(i = 0; i < cblistnum(help); i++){
    htmlprintf("%s\n", cblistval(help, i, NULL));
  }
  htmlprintf("<div class=\"note\">Powered by QDBM %@.</div>\n", dpversion);
}



/* END OF FILE */
