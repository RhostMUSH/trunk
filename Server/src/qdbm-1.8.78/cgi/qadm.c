/*************************************************************************************************
 * CGI script for administration of database files
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
#include <curia.h>
#include <cabin.h>
#include <villa.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#undef TRUE
#define TRUE           1                 /* boolean true */
#undef FALSE
#define FALSE          0                 /* boolean false */

#define CONFFILE    "qadm.conf"          /* name of the configuration file */
#define DEFENC      "US-ASCII"           /* default encoding */
#define DEFLANG     "en"                 /* default language */
#define DEFTITLE    "QDBM on WWW"        /* default title */
#define DEFDATADIR  "qadmdir"            /* directory containing database files */
#define RDATAMAX    262144               /* max size of data to read */
#define DBNAMEMAX   128                  /* max size of the name of a database */
#define PATHBUFSIZ  1024                 /* size of a buffer for path string */
#define TSTRBUFSIZ  256                  /* size of a buffer for time string */

enum {
  ACTNONE,
  ACTDBDOWN,
  ACTDBCREATE,
  ACTDBREMOVE,
  ACTRECLIST,
  ACTRECPUTOVER,
  ACTRECPUTKEEP,
  ACTRECPUTCAT,
  ACTRECPUTDUP,
  ACTRECOUTONE,
  ACTRECOUTLIST
};

enum {
  DBTDEPOT,
  DBTCURIA,
  DBTVILLA,
  DBTERROR
};


/* global variables */
const char *scriptname;                  /* name of the script */
const char *enc;                         /* encoding of the page */
const char *lang;                        /* language of the page */
const char *title;                       /* title of the page */
int keychop;                             /* whether to chop keys */
int valchop;                             /* whether to chop values */


/* function prototypes */
int main(int argc, char **argv);
const char *skiplabel(const char *str);
CBMAP *getparams(void);
char *getpathinfo(void);
void senderror(int code, const char *tag, const char *message);
void dodbdown(const char *dbname);
void dorecdown(const char *dbname, const char *tkey);
void htmlprintf(const char *format, ...);
void printmime(void);
void printdecl(void);
void printhead(void);
const char *getdbattrs(const char *dbname, int *dbtp, int *rnump, int *fsizp, time_t *mtp);
const char *dbtstr(int dbt);
const char *timestr(time_t t);
void dodblist();
void dodbcreate(const char *dbname, int dbt);
void dodbremove(const char *dbname, int sure);
void doreclist(const char *dbname, int act, const char *key, const char *val, int sure);
void strchop(char *str);
void updatedepot(const char *dbname, int act, const char *key, const char *val);
void updatecuria(const char *dbname, int act, const char *key, const char *val);
void updatevilla(const char *dbname, int act, const char *key, const char *val);
void depotreclist(const char *dbname);
void curiareclist(const char *dbname);
void villareclist(const char *dbname);


/* main routine */
int main(int argc, char **argv){
  CBMAP *params;
  CBLIST *lines;
  const char *tmp, *datadir, *dbname, *key, *val;
  char *pinfo, *tkey;
  int i, act, sure, dbt;
  /* set configurations */
  cbstdiobin();
  scriptname = argv[0];
  if((tmp = getenv("SCRIPT_NAME")) != NULL) scriptname = tmp;
  enc = NULL;
  lang = NULL;
  title = NULL;
  datadir = NULL;
  keychop = FALSE;
  valchop = FALSE;
  if((lines = cbreadlines(CONFFILE)) != NULL){
    for(i = 0; i < cblistnum(lines); i++){
      tmp = cblistval(lines, i, NULL);
      if(cbstrfwmatch(tmp, "encoding:")){
        enc = skiplabel(tmp);
      } else if(cbstrfwmatch(tmp, "lang:")){
        lang = skiplabel(tmp);
      } else if(cbstrfwmatch(tmp, "title:")){
        title = skiplabel(tmp);
      } else if(cbstrfwmatch(tmp, "datadir:")){
        datadir = skiplabel(tmp);
      } else if(cbstrfwmatch(tmp, "keychop:")){
        if(!strcmp(skiplabel(tmp), "true")) keychop = TRUE;
      } else if(cbstrfwmatch(tmp, "valchop:")){
        if(!strcmp(skiplabel(tmp), "true")) valchop = TRUE;
      }
    }
  }
  if(!enc) enc = DEFENC;
  if(!lang) lang = DEFLANG;
  if(!title) title = DEFTITLE;
  if(!datadir) datadir = DEFDATADIR;
  /* read parameters */
  dbname = NULL;
  act = ACTNONE;
  sure = FALSE;
  dbt = DBTERROR;
  key = NULL;
  val = NULL;
  params = getparams();
  if((tmp = cbmapget(params, "act", -1, NULL)) != NULL) act = atoi(tmp);
  if((tmp = cbmapget(params, "sure", -1, NULL)) != NULL) sure = atoi(tmp);
  if((tmp = cbmapget(params, "dbname", -1, NULL)) != NULL) dbname = tmp;
  if((tmp = cbmapget(params, "dbt", -1, NULL)) != NULL) dbt = atoi(tmp);
  if((tmp = cbmapget(params, "key", -1, NULL)) != NULL) key = tmp;
  if((tmp = cbmapget(params, "val", -1, NULL)) != NULL) val = tmp;
  if(dbname && strlen(dbname) > DBNAMEMAX) dbname = NULL;
  pinfo = getpathinfo();
  /* show page or send data */
  if(chdir(datadir) == -1){
    senderror(500, "Internal Server Error", "Could not change the current directory.");
  } else if(act == ACTDBDOWN){
    dodbdown(dbname);
  } else if(pinfo && (tkey = strchr(pinfo + 1, '/')) != NULL){
    dbname = pinfo + 1;
    *tkey = '\0';
    tkey++;
    dorecdown(dbname, tkey);
  } else {
    printmime();
    printdecl();
    htmlprintf("<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"%s\" lang=\"%s\">\n",
               lang, lang);
    printhead();
    htmlprintf("<body>\n");
    if(act == ACTNONE){
      htmlprintf("<h1>%@</h1>\n", title);
    } else {
      htmlprintf("<div class=\"note\"><a href=\"%s\">[BACK]</a></div>\n", scriptname);
    }
    htmlprintf("<hr />\n");
    switch(act){
    case ACTDBCREATE:
      dodbcreate(dbname, dbt);
      break;
    case ACTDBREMOVE:
      dodbremove(dbname, sure);
      break;
    case ACTRECLIST:
    case ACTRECPUTOVER:
    case ACTRECPUTKEEP:
    case ACTRECPUTCAT:
    case ACTRECPUTDUP:
    case ACTRECOUTONE:
    case ACTRECOUTLIST:
      doreclist(dbname, act, key, val, sure);
      break;
    default:
      dodblist();
      break;
    }
    htmlprintf("<hr />\n");
    if(act == ACTNONE)
      htmlprintf("<div class=\"note\">Powered by QDBM %@.</div>\n", dpversion);
    htmlprintf("</body>\n");
    htmlprintf("</html>\n");
  }
  /* release resources */
  cbmapclose(params);
  if(lines) cblistclose(lines);
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


/* get a string for PATH_INFO */
char *getpathinfo(void){
  const char *tmp;
  if((tmp = getenv("PATH_INFO")) != NULL){
    return cburldecode(tmp, NULL);
  }
  return NULL;
}


/* send error status */
void senderror(int code, const char *tag, const char *message){
  printf("Status: %d %s\r\n", code, tag);
  printf("Content-Type: text/plain; charset=US-ASCII\r\n");
  printf("\r\n");
  printf("%s\n", message);
}


/* send a database */
void dodbdown(const char *dbname){
  FILE *ifp;
  int c;
  if(!dbname || dbname[0] == '/' || strstr(dbname, "..") || !(ifp = fopen(dbname, "rb"))){
    senderror(404, "File Not Found", "The database file was not found.");
  } else {
    printf("Content-Type: application/x-qdbm\r\n");
    printf("\r\n");
    while((c = fgetc(ifp)) != EOF){
      putchar(c);
    }
    fclose(ifp);
  }
}


/* send the value of a record */
void dorecdown(const char *dbname, const char *tkey){
  char vlpath[PATHBUFSIZ], crpath[PATHBUFSIZ], dppath[PATHBUFSIZ], *vbuf;
  int vsiz;
  DEPOT *depot;
  CURIA *curia;
  VILLA *villa;
  vbuf = NULL;
  vsiz = 0;
  sprintf(vlpath, "%s.vl", dbname);
  sprintf(crpath, "%s.cr", dbname);
  sprintf(dppath, "%s.dp", dbname);
  if((villa = vlopen(vlpath, VL_OREADER, VL_CMPLEX)) != NULL){
    vbuf = vlget(villa, tkey, -1, &vsiz);
    vlclose(villa);
  } else if((curia = cropen(crpath, CR_OREADER, -1, -1)) != NULL){
    vbuf = crget(curia, tkey, -1, 0, -1, &vsiz);
    crclose(curia);
  } else if((depot = dpopen(dppath, DP_OREADER, -1)) != NULL){
    vbuf = dpget(depot, tkey, -1, 0, -1, &vsiz);
    dpclose(depot);
  }
  if(vbuf){
    printf("Content-Type: text/plain; charset=%s\r\n", enc);
    printf("Content-Length: %d\r\n", vsiz);
    printf("\r\n");
    printf("%s", vbuf);
    free(vbuf);
  } else {
    senderror(404, "Record Not Found", "There is no corresponding record.");
  }
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
             " margin: 0em 0em; padding: 0em 1em; }\n");
  htmlprintf(".note { margin: 0.5em 0.5em; text-align: right; }\n");
  htmlprintf("h1,h2,p { margin: 0.5em 0.5em; }\n");
  htmlprintf("hr { margin-top: 1.2em; margin-bottom: 0.8em; height: 1pt;");
  htmlprintf(" color: #888888; background-color: #888888; border: none; }\n");
  htmlprintf("a { color: #0022aa; text-decoration: none; }\n");
  htmlprintf("a:hover { color: #1144ff; text-decoration: underline; }\n");
  htmlprintf("a.head { color: #111111; text-decoration: none; }\n");
  htmlprintf("em { font-weight: bold; font-style: normal; }\n");
  htmlprintf("form { margin: 0em 0em; padding: 0em 0em; }\n");
  htmlprintf("div,table { margin: 0em 1em; padding: 0em 0em;"
             " border: none; border-collapse: collapse; }\n");
  htmlprintf("th,td { margin: 0em 0em; padding: 0.2em 0.5em; vertical-align: top; }\n");
  htmlprintf("th { font-size: smaller; font-weight: normal; text-align: left; }\n");
  htmlprintf("td { background-color: #ddddee; border: solid 1pt #888888; }\n");
  htmlprintf("td.str { text-align: left; }\n");
  htmlprintf("td.num { text-align: right; }\n");
  htmlprintf("td.raw { text-align: left; white-space: pre; font-family: monospace; }\n");
  htmlprintf("</style>\n");
  htmlprintf("</head>\n");
}


/* get the attributes of a database */
const char *getdbattrs(const char *dbname, int *dbtp, int *rnump, int *fsizp, time_t *mtp){
  static char name[PATHBUFSIZ], *tmp;
  DEPOT *depot;
  CURIA *curia;
  VILLA *villa;
  struct stat sbuf;
  strcpy(name, dbname);
  *dbtp = DBTERROR;
  *fsizp = -1;
  *rnump = -1;
  *mtp = -1;
  if((tmp = strrchr(name, '.')) != NULL){
    *tmp = '\0';
    tmp++;
    if(!strcmp(tmp, "dp")){
      *dbtp = DBTDEPOT;
    } else if(!strcmp(tmp, "cr")){
      *dbtp = DBTCURIA;
    } else if(!strcmp(tmp, "vl")){
      *dbtp = DBTVILLA;
    }
  }
  switch(*dbtp){
  case DBTDEPOT:
    if((depot = dpopen(dbname, DP_OREADER, -1)) != NULL){
      *fsizp = dpfsiz(depot);
      *rnump = dprnum(depot);
      dpclose(depot);
    }
    break;
  case DBTCURIA:
    if((curia = cropen(dbname, CR_OREADER, -1, -1)) != NULL){
      *fsizp = crfsiz(curia);
      *rnump = crrnum(curia);
      crclose(curia);
    }
    break;
  case DBTVILLA:
    if((villa = vlopen(dbname, VL_OREADER, VL_CMPLEX)) != NULL){
      *fsizp = vlfsiz(villa);
      *rnump = vlrnum(villa);
      vlclose(villa);
    }
    break;
  default:
    break;
  }
  if(stat(dbname, &sbuf) == 0){
    *mtp = sbuf.st_mtime;
  }
  return name;
}


/* get the string of a database type */
const char *dbtstr(int dbt){
  switch(dbt){
  case DBTDEPOT: return "Depot";
  case DBTCURIA: return "Curia";
  case DBTVILLA: return "Villa";
  default: break;
  }
  return "ERROR";
}


/* get the string of the time */
const char *timestr(time_t t){
  static char str[TSTRBUFSIZ];
  struct tm *tsp;
  tsp = localtime(&t);
  sprintf(str, "%04d/%02d/%02d %02d:%02d:%02d",
          tsp->tm_year + 1900, tsp->tm_mon + 1, tsp->tm_mday,
          tsp->tm_hour, tsp->tm_min, tsp->tm_sec);
  return str;
}


/* list existing databases */
void dodblist(){
  CBLIST *files;
  const char *tmp, *name;
  int i, dbt, rnum, fsiz;
  time_t mt;
  if(!(files = cbdirlist("."))){
    htmlprintf("<p>The data directory could not be scanned.</p>\n");
    return;
  }
  cblistsort(files);
  htmlprintf("<p>The following are existing database files."
             "  Select a database file and its action.</p>\n");
  htmlprintf("<form action=\"%@\" method=\"post\">\n", scriptname);
  htmlprintf("<table summary=\"databases\">\n");
  htmlprintf("<tr>\n");
  htmlprintf("<th abbr=\"name\">Name</th>\n");
  htmlprintf("<th abbr=\"dbt\">Type of Database</th>\n");
  htmlprintf("<th abbr=\"fsiz\">Size</th>\n");
  htmlprintf("<th abbr=\"rnum\">Records</th>\n");
  htmlprintf("<th abbr=\"last\">Last Modified</th>\n");
  htmlprintf("<th abbr=\"acts\">Actions</th>\n");
  htmlprintf("</tr>\n");
  for(i = 0; i < cblistnum(files); i++){
    tmp = cblistval(files, i, NULL);
    if(!strcmp(tmp, ".") || !strcmp(tmp, "..")) continue;
    name = getdbattrs(tmp, &dbt, &rnum, &fsiz, &mt);
    htmlprintf("<tr>\n");
    htmlprintf("<td class=\"str\"><em>%@</em></td>\n", name);
    htmlprintf("<td class=\"str\">%@</td>\n", dbtstr(dbt));
    htmlprintf("<td class=\"num\">%d</td>\n", fsiz);
    htmlprintf("<td class=\"num\">%d</td>\n", rnum);
    htmlprintf("<td class=\"str\">%s</td>\n", timestr(mt));
    htmlprintf("<td class=\"str\">");
    htmlprintf("<a href=\"%s?act=%d&amp;dbname=%?\">[OPEN]</a>",
               scriptname, ACTRECLIST, tmp);
    htmlprintf(" <a href=\"%s?act=%d&amp;dbname=%?&amp;sure=0\">[REMOVE]</a>",
               scriptname, ACTDBREMOVE, tmp);
    if(dbt != DBTCURIA){
      htmlprintf(" <a href=\"%s?act=%d&amp;dbname=%?\">[D/L]</a>",
                 scriptname, ACTDBDOWN, tmp);
    }
    htmlprintf("</td>\n");
    htmlprintf("</tr>\n");
  }
  htmlprintf("</table>\n");
  htmlprintf("</form>\n");
  htmlprintf("<hr />\n");
  htmlprintf("<p>This form is used in order to create a new database.</p>\n");
  htmlprintf("<form action=\"%@\" method=\"post\">\n", scriptname);
  htmlprintf("<table summary=\"new database\">\n");
  htmlprintf("<tr>\n");
  htmlprintf("<th abbr=\"name\">Name</th>\n");
  htmlprintf("<th abbr=\"dbt\">Type of Database</th>\n");
  htmlprintf("<th abbr=\"act\">Action</th>\n");
  htmlprintf("</tr>\n");
  htmlprintf("<tr>\n");
  htmlprintf("<td class=\"str\">"
             "<input type=\"text\" name=\"dbname\" size=\"32\" /></td>\n");
  htmlprintf("<td class=\"str\"><select name=\"dbt\">\n");
  htmlprintf("<option value=\"%d\">%@</option>\n", DBTDEPOT, dbtstr(DBTDEPOT));
  htmlprintf("<option value=\"%d\">%@</option>\n", DBTCURIA, dbtstr(DBTCURIA));
  htmlprintf("<option value=\"%d\">%@</option>\n", DBTVILLA, dbtstr(DBTVILLA));
  htmlprintf("</select></td>\n");
  htmlprintf("<td class=\"str\"><input type=\"submit\" value=\"Create\" /></td>\n");
  htmlprintf("</tr>\n");
  htmlprintf("</table>\n");
  htmlprintf("<div><input type=\"hidden\" name=\"act\" value=\"%d\" /></div>\n", ACTDBCREATE);
  htmlprintf("</form>\n");
  cblistclose(files);
}


/* create a new database */
void dodbcreate(const char *dbname, int dbt){
  CBLIST *files;
  char path[PATHBUFSIZ];
  int i, err;
  DEPOT *depot;
  CURIA *curia;
  VILLA *villa;
  if(!dbname || strlen(dbname) < 1){
    htmlprintf("<p>The name of the database is not defined.</p>\n");
    return;
  }
  if(strchr(dbname, '/')){
    htmlprintf("<p>The name of the database includes invalid characters: `/'.</p>\n");
    return;
  }
  if(dbt == DBTERROR){
    htmlprintf("<p>The parameters are invalid.</p>\n");
    return;
  }
  if(!(files = cbdirlist("."))){
    htmlprintf("<p>The data directory could not be scanned.</p>\n");
    return;
  }
  for(i = 0; i < cblistnum(files); i++){
    sprintf(path, "%s.dp", dbname);
    if(cbstrfwmatch(cblistval(files, i, NULL), path)){
      cblistclose(files);
      htmlprintf("<p>`%@' is duplicated.</p>\n", dbname);
      return;
    }
    sprintf(path, "%s.cr", dbname);
    if(cbstrfwmatch(cblistval(files, i, NULL), path)){
      cblistclose(files);
      htmlprintf("<p>`%@' is duplicated.</p>\n", dbname);
      return;
    }
    sprintf(path, "%s.vl", dbname);
    if(cbstrfwmatch(cblistval(files, i, NULL), path)){
      cblistclose(files);
      htmlprintf("<p>`%@' is duplicated.</p>\n", dbname);
      return;
    }
  }
  cblistclose(files);
  sprintf(path, "%s.%s", dbname, dbt == DBTDEPOT ? "dp" : dbt == DBTCURIA ? "cr" : "vl");
  err = FALSE;
  switch(dbt){
  case DBTDEPOT:
    if(!(depot = dpopen(path, DP_OWRITER | DP_OCREAT | DP_OTRUNC, -1)) ||
       !dpclose(depot)) err = TRUE;
    break;
  case DBTCURIA:
    if(!(curia = cropen(path, CR_OWRITER | CR_OCREAT | CR_OTRUNC, -1, -1)) ||
       !crclose(curia)) err = TRUE;
    break;
  case DBTVILLA:
    if(!(villa = vlopen(path, VL_OWRITER | VL_OCREAT | VL_OTRUNC, VL_CMPLEX)) ||
       !vlclose(villa)) err = TRUE;
    break;
  }
  if(err){
    htmlprintf("<p>`%@' could not be created.</p>\n", dbname);
  } else {
    htmlprintf("<p>`<em>%@</em>' was created successfully.</p>\n", dbname);
  }
}


/* remove a database */
void dodbremove(const char *dbname, int sure){
  const char *name;
  int dbt, rnum, fsiz, err;
  time_t mt;
  if(!dbname || strlen(dbname) < 1){
    htmlprintf("<p>The name of the database is not defined.</p>\n");
    return;
  }
  name = getdbattrs(dbname, &dbt, &rnum, &fsiz, &mt);
  if(!sure){
    htmlprintf("<p>Do you really remove `<em>%@</em>'?</p>\n", name);
    htmlprintf("<div><a href=\"%s?act=%d&amp;dbname=%?&amp;sure=1\">[REMOVE]</a></div>\n",
               scriptname, ACTDBREMOVE, dbname);
    return;
  }
  err = FALSE;
  switch(dbt){
  case DBTDEPOT:
    if(!dpremove(dbname)) err = TRUE;
    break;
  case DBTCURIA:
    if(!crremove(dbname)) err = TRUE;
    break;
  case DBTVILLA:
    if(!vlremove(dbname)) err = TRUE;
    break;
  default:
    err = TRUE;
    break;
  }
  if(err){
    htmlprintf("<p>`%@' could not be removed.</p>\n", name);
  } else {
    htmlprintf("<p>`<em>%@</em>' was removed successfully.</p>\n", name);
  }
}


/* open a database */
void doreclist(const char *dbname, int act, const char *key, const char *val, int sure){
  const char *name;
  int dbt, rnum, fsiz;
  time_t mt;
  if(!dbname || strlen(dbname) < 1){
    htmlprintf("<p>The name of the database is not defined.</p>\n");
    return;
  }
  if(strchr(dbname, '/')){
    htmlprintf("<p>The name of the database includes invalid characters: `/'.</p>\n");
    return;
  }
  name = getdbattrs(dbname, &dbt, &rnum, &fsiz, &mt);
  switch(dbt){
  case DBTDEPOT:
    updatedepot(dbname, act, key, val);
    break;
  case DBTCURIA:
    updatecuria(dbname, act, key, val);
    break;
  case DBTVILLA:
    updatevilla(dbname, act, key, val);
    break;
  default:
    htmlprintf("<p>The type of the database is invalid.</p>\n");
    break;
  }
  name = getdbattrs(dbname, &dbt, &rnum, &fsiz, &mt);
  htmlprintf("<table summary=\"attributes\">\n");
  htmlprintf("<tr>\n");
  htmlprintf("<th abbr=\"name\">Name</th>\n");
  htmlprintf("<th abbr=\"dbt\">Type of Database</th>\n");
  htmlprintf("<th abbr=\"fsiz\">Size</th>\n");
  htmlprintf("<th abbr=\"rnum\">Records</th>\n");
  htmlprintf("<th abbr=\"last\">Last Modified</th>\n");
  htmlprintf("<th abbr=\"acts\">Actions</th>\n");
  htmlprintf("</tr>\n");
  htmlprintf("<tr>\n");
  htmlprintf("<td class=\"str\"><em>%@</em></td>\n", name);
  htmlprintf("<td class=\"str\">%s</td>\n", dbtstr(dbt));
  htmlprintf("<td class=\"num\">%d</td>\n", fsiz);
  htmlprintf("<td class=\"num\">%d</td>\n", rnum);
  htmlprintf("<td class=\"str\">%s</td>\n", timestr(mt));
  htmlprintf("<td class=\"str\">");
  htmlprintf(" <a href=\"%s?act=%d&amp;dbname=%?&amp;sure=0\">[REMOVE]</a>",
             scriptname, ACTDBREMOVE, dbname);
  if(dbt != DBTCURIA){
    htmlprintf(" <a href=\"%s?act=%d&amp;dbname=%?\">[D/L]</a>",
               scriptname, ACTDBDOWN, dbname);
  }
  htmlprintf("</td>\n");
  htmlprintf("</tr>\n");
  htmlprintf("</table>\n");
  htmlprintf("<hr />\n");
  htmlprintf("<p>This form is used in order to update the database.</p>\n");
  htmlprintf("<form action=\"%@\" method=\"post\">\n", scriptname);
  htmlprintf("<table summary=\"update\">\n");
  htmlprintf("<tr>\n");
  htmlprintf("<th abbr=\"act\">Action:</th>\n");
  htmlprintf("<td class=\"str\">\n");
  htmlprintf("<select name=\"act\">\n");
  htmlprintf("<option value=\"%d\"%s>PUT (over)</option>\n",
             ACTRECPUTOVER, act == ACTRECPUTOVER ? " selected=\"selected\"" : "");
  htmlprintf("<option value=\"%d\"%s>PUT (keep)</option>\n",
             ACTRECPUTKEEP, act == ACTRECPUTKEEP ? " selected=\"selected\"" : "");
  if(dbt == DBTVILLA){
    htmlprintf("<option value=\"%d\"%s>PUT (dup)</option>\n",
               ACTRECPUTDUP, act == ACTRECPUTDUP ? " selected=\"selected\"" : "");
  } else {
    htmlprintf("<option value=\"%d\"%s>PUT (cat)</option>\n",
               ACTRECPUTCAT, act == ACTRECPUTCAT ? " selected=\"selected\"" : "");
  }
  if(dbt == DBTVILLA){
    htmlprintf("<option value=\"%d\"%s>OUT (one)</option>\n",
               ACTRECOUTONE, act == ACTRECOUTONE ? " selected=\"selected\"" : "");
    htmlprintf("<option value=\"%d\"%s>OUT (list)</option>\n",
               ACTRECOUTLIST, act == ACTRECOUTLIST ? " selected=\"selected\"" : "");
  } else {
    htmlprintf("<option value=\"%d\"%s>OUT</option>\n",
               ACTRECOUTONE, act == ACTRECOUTONE ? " selected=\"selected\"" : "");
  }
  htmlprintf("</select>\n");
  htmlprintf("<input type=\"submit\" value=\"Submit\" />\n");
  htmlprintf("<input type=\"reset\" value=\"Reset\" />\n");
  htmlprintf("</td>\n");
  htmlprintf("</tr>\n");
  htmlprintf("<tr>\n");
  htmlprintf("<th abbr=\"key\">Key:</th>\n");
  htmlprintf("<td class=\"str\">\n");
  if(keychop){
    htmlprintf("<input type=\"text\" name=\"key\" size=\"48\" />\n");
  } else {
    htmlprintf("<textarea name=\"key\" cols=\"48\" rows=\"3\"></textarea>\n");
  }
  htmlprintf("</td>\n");
  htmlprintf("</tr>\n");
  htmlprintf("<tr>\n");
  htmlprintf("<th abbr=\"val\">Value:</th>\n");
  htmlprintf("<td class=\"str\">\n");
  if(valchop){
    htmlprintf("<input type=\"text\" name=\"val\" size=\"48\" />\n");
  } else {
    htmlprintf("<textarea name=\"val\" cols=\"48\" rows=\"3\"></textarea>\n");
  }
  htmlprintf("</td>\n");
  htmlprintf("</tr>\n");
  htmlprintf("</table>\n");
  htmlprintf("<div><input type=\"hidden\" name=\"dbname\" value=\"%@\" /></div>\n", dbname);
  htmlprintf("<div><input type=\"hidden\" name=\"sure\" value=\"%d\" /></div>\n", sure);
  htmlprintf("</form>\n");
  htmlprintf("<hr />\n");
  if(sure){
    htmlprintf("<p>The following are all records of the database."
               " : <a href=\"%s?act=%d&amp;dbname=%?&amp;sure=0\">"
               "[HIDE RECORDS]</a></p>\n", scriptname, ACTRECLIST);
    switch(dbt){
    case DBTDEPOT: depotreclist(dbname); break;
    case DBTCURIA: curiareclist(dbname); break;
    case DBTVILLA: villareclist(dbname); break;
    default: break;
    }
  } else {
    htmlprintf("<p>The records are not shown now."
               " : <a href=\"%s?act=%d&amp;dbname=%?&amp;sure=1\">"
               "[SHOW RECORDS]</a></p>\n", scriptname, ACTRECLIST);
  }
}


/* chop the string */
void strchop(char *str){
  int i;
  for(i = strlen(str) - 1; i >= 0; i--){
    if(strchr(" \r\n\t", str[i])){
      str[i] = '\0';
    } else {
      break;
    }
  }
}


/* update the database of Depot */
void updatedepot(const char *dbname, int act, const char *key, const char *val){
  int put, dmode;
  DEPOT *depot;
  char *kbuf, *vbuf;
  put = FALSE;
  dmode = DP_DOVER;
  switch(act){
  case ACTRECPUTOVER:
    put = TRUE;
    dmode = DP_DOVER;
    break;
  case ACTRECPUTKEEP:
    put = TRUE;
    dmode = DP_DKEEP;
    break;
  case ACTRECPUTCAT:
    put = TRUE;
    dmode = DP_DCAT;
    break;
  case ACTRECOUTONE:
    break;
  default:
    return;
  }
  if(!key || !val){
    htmlprintf("<p>Parameters are lack to update the database.</p>\n");
    htmlprintf("<hr />\n");
    return;
  }
  if(!(depot = dpopen(dbname, DP_OWRITER, -1))){
    htmlprintf("<p>The database file could not open as a writer.</p>\n");
    htmlprintf("<hr />\n");
    return;
  }
  if(put){
    kbuf = cbmemdup(key, -1);
    vbuf = cbmemdup(val, -1);
    if(keychop) strchop(kbuf);
    if(valchop) strchop(vbuf);
    if(dpput(depot, kbuf, -1, vbuf, -1, dmode)){
      htmlprintf("<p>A record was stored.</p>\n");
    } else if(dpecode == DP_EKEEP){
      htmlprintf("<p>An existing record are kept.</p>\n");
    } else {
      htmlprintf("<p>The database has a fatal error.</p>\n");
    }
    free(vbuf);
    free(kbuf);
  } else {
    if(dpout(depot, key, -1)){
      htmlprintf("<p>A record was deleted.</p>\n");
    } else if(dpecode == DP_ENOITEM){
      htmlprintf("<p>There is no such record.</p>\n");
    } else {
      htmlprintf("<p>The database has a fatal error.</p>\n");
    }
  }
  if(!dpclose(depot)) htmlprintf("<p>The database has a fatal error.</p>\n");
  htmlprintf("<hr />\n");
}


/* update the database of Curia */
void updatecuria(const char *dbname, int act, const char *key, const char *val){
  int put, dmode;
  CURIA *curia;
  char *kbuf, *vbuf;
  put = FALSE;
  dmode = CR_DOVER;
  switch(act){
  case ACTRECPUTOVER:
    put = TRUE;
    dmode = CR_DOVER;
    break;
  case ACTRECPUTKEEP:
    put = TRUE;
    dmode = CR_DKEEP;
    break;
  case ACTRECPUTCAT:
    put = TRUE;
    dmode = CR_DCAT;
    break;
  case ACTRECOUTONE:
    break;
  default:
    return;
  }
  if(!key || !val){
    htmlprintf("<p>Parameters are lack to update the database.</p>\n");
    htmlprintf("<hr />\n");
    return;
  }
  if(!(curia = cropen(dbname, CR_OWRITER, -1, -1))){
    htmlprintf("<p>The database file could not open as a writer.</p>\n");
    htmlprintf("<hr />\n");
    return;
  }
  if(put){
    kbuf = cbmemdup(key, -1);
    vbuf = cbmemdup(val, -1);
    if(keychop) strchop(kbuf);
    if(valchop) strchop(vbuf);
    if(crput(curia, kbuf, -1, vbuf, -1, dmode)){
      htmlprintf("<p>A record was stored.</p>\n");
    } else if(dpecode == DP_EKEEP){
      htmlprintf("<p>An existing record are kept.</p>\n");
    } else {
      htmlprintf("<p>The database has a fatal error.</p>\n");
    }
    free(vbuf);
    free(kbuf);
  } else {
    if(crout(curia, key, -1)){
      htmlprintf("<p>A record was deleted.</p>\n");
    } else if(dpecode == DP_ENOITEM){
      htmlprintf("<p>There is no such record.</p>\n");
    } else {
      htmlprintf("<p>The database has a fatal error.</p>\n");
    }
  }
  if(!crclose(curia)) htmlprintf("<p>The database has a fatal error.</p>\n");
  htmlprintf("<hr />\n");
}


/* update the database of Villa */
void updatevilla(const char *dbname, int act, const char *key, const char *val){
  int put, dmode, olist;
  VILLA *villa;
  char *kbuf, *vbuf;
  put = FALSE;
  dmode = VL_DOVER;
  olist = FALSE;
  switch(act){
  case ACTRECPUTOVER:
    put = TRUE;
    dmode = VL_DOVER;
    break;
  case ACTRECPUTKEEP:
    put = TRUE;
    dmode = VL_DKEEP;
    break;
  case ACTRECPUTDUP:
    put = TRUE;
    dmode = VL_DDUP;
    break;
  case ACTRECOUTONE:
    olist = FALSE;
    break;
  case ACTRECOUTLIST:
    olist = TRUE;
    break;
  default:
    return;
  }
  if(!key || !val){
    htmlprintf("<p>Parameters are lack to update the database.</p>\n");
    htmlprintf("<hr />\n");
    return;
  }
  if(!(villa = vlopen(dbname, VL_OWRITER, VL_CMPLEX))){
    htmlprintf("<p>The database file could not open as a writer.</p>\n");
    htmlprintf("<hr />\n");
    return;
  }
  if(put){
    kbuf = cbmemdup(key, -1);
    vbuf = cbmemdup(val, -1);
    if(keychop) strchop(kbuf);
    if(valchop) strchop(vbuf);
    if(vlput(villa, kbuf, -1, vbuf, -1, dmode)){
      htmlprintf("<p>A record was stored.</p>\n");
    } else if(dpecode == DP_EKEEP){
      htmlprintf("<p>An existing record are kept.</p>\n");
    } else {
      htmlprintf("<p>The database has a fatal error.</p>\n");
    }
    free(vbuf);
    free(kbuf);
  } else {
    if(olist ? vloutlist(villa, key, -1) : vlout(villa, key, -1)){
      if(olist){
        htmlprintf("<p>Some records were deleted.</p>\n");
      } else {
        htmlprintf("<p>A record was deleted.</p>\n");
      }
    } else if(dpecode == DP_ENOITEM){
      htmlprintf("<p>There is no such record.</p>\n");
    } else {
      htmlprintf("<p>The database has a fatal error.</p>\n");
    }
  }
  if(!vlclose(villa)) htmlprintf("<p>The database has a fatal error.</p>\n");
  htmlprintf("<hr />\n");
}


/* list records of Depot */
void depotreclist(const char *dbname){
  DEPOT *depot;
  char *kbuf, *vbuf;
  int i, ksiz, vsiz;
  if(!(depot = dpopen(dbname, DP_OREADER, -1))){
    htmlprintf("<p>The database file could not open as a reader.</p>\n");
    return;
  }
  dpiterinit(depot);
  htmlprintf("<table summary=\"records\">\n");
  htmlprintf("<tr>\n");
  htmlprintf("<th abbr=\"number\">#</th>\n");
  htmlprintf("<th abbr=\"keys\">Keys</th>\n");
  htmlprintf("<th abbr=\"vals\">Values</th>\n");
  htmlprintf("</tr>\n");
  for(i = 1; (kbuf = dpiternext(depot, &ksiz)) != NULL; i++){
    if(!(vbuf = dpget(depot, kbuf, ksiz, 0, -1, &vsiz))){
      free(kbuf);
      continue;
    }
    htmlprintf("<tr>\n");
    htmlprintf("<td class=\"num\"><var>%d</var></td>\n", i);
    htmlprintf("<td class=\"raw\">%@</td>\n", kbuf);
    htmlprintf("<td class=\"raw\">%@</td>\n", vbuf);
    htmlprintf("</tr>\n");
    free(vbuf);
    free(kbuf);
  }
  htmlprintf("</table>\n");
  dpclose(depot);
}


/* list records of Curia */
void curiareclist(const char *dbname){
  CURIA *curia;
  char *kbuf, *vbuf;
  int i, ksiz, vsiz;
  if(!(curia = cropen(dbname, CR_OREADER, -1, -1))){
    htmlprintf("<p>The database file could not open as a reader.</p>\n");
    return;
  }
  criterinit(curia);
  htmlprintf("<table summary=\"records\">\n");
  htmlprintf("<tr>\n");
  htmlprintf("<th abbr=\"number\">#</th>\n");
  htmlprintf("<th abbr=\"keys\">Keys</th>\n");
  htmlprintf("<th abbr=\"vals\">Values</th>\n");
  htmlprintf("</tr>\n");
  for(i = 1; (kbuf = criternext(curia, &ksiz)) != NULL; i++){
    if(!(vbuf = crget(curia, kbuf, ksiz, 0, -1, &vsiz))){
      free(kbuf);
      continue;
    }
    htmlprintf("<tr>\n");
    htmlprintf("<td class=\"num\"><var>%d</var></td>\n", i);
    htmlprintf("<td class=\"raw\">%@</td>\n", kbuf);
    htmlprintf("<td class=\"raw\">%@</td>\n", vbuf);
    htmlprintf("</tr>\n");
    free(vbuf);
    free(kbuf);
  }
  htmlprintf("</table>\n");
  crclose(curia);
}


/* list records of Villa */
void villareclist(const char *dbname){
  VILLA *villa;
  char *kbuf, *vbuf;
  int i, ksiz, vsiz;
  if(!(villa = vlopen(dbname, VL_OREADER, VL_CMPLEX))){
    htmlprintf("<p>The database file could not open as a reader.</p>\n");
    return;
  }
  vlcurfirst(villa);
  htmlprintf("<table summary=\"records\">\n");
  htmlprintf("<tr>\n");
  htmlprintf("<th abbr=\"number\">#</th>\n");
  htmlprintf("<th abbr=\"keys\">Keys</th>\n");
  htmlprintf("<th abbr=\"vals\">Values</th>\n");
  htmlprintf("</tr>\n");
  for(i = 1; (kbuf = vlcurkey(villa, &ksiz)) != NULL; i++){
    if(!(vbuf = vlcurval(villa, &vsiz))){
      free(kbuf);
      vlcurnext(villa);
      continue;
    }
    htmlprintf("<tr>\n");
    htmlprintf("<td class=\"num\"><var>%d</var></td>\n", i);
    htmlprintf("<td class=\"raw\">%@</td>\n", kbuf);
    htmlprintf("<td class=\"raw\">%@</td>\n", vbuf);
    htmlprintf("</tr>\n");
    free(vbuf);
    free(kbuf);
    vlcurnext(villa);
  }
  htmlprintf("</table>\n");
  vlclose(villa);
}



/* END OF FILE */
