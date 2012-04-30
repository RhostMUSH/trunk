/*************************************************************************************************
 * CGI script for file uploader
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

#undef TRUE
#define TRUE           1                 /* boolean true */
#undef FALSE
#define FALSE          0                 /* boolean false */

#define CONFFILE    "qupl.conf"          /* name of the configuration file */
#define DEFENC      "US-ASCII"           /* default encoding */
#define DEFLANG     "en"                 /* default language */
#define DEFTITLE    "File Uploader"      /* default title */
#define DEFDATADIR  "qupldir"            /* directory containing files */


/* global variables */
const char *scriptname;                  /* name of the script */
const char *enc;                         /* encoding of the page */
const char *lang;                        /* language of the page */
const char *title;                       /* title of the page */
int quota;                               /* limit of the total size */


/* function prototypes */
int main(int argc, char **argv);
const char *skiplabel(const char *str);
void htmlprintf(const char *format, ...);
int getdirsize(const char *path);
const char *datestr(time_t t);
const char *gettype(const char *path);


/* main routine */
int main(int argc, char **argv){
  CBLIST *lines, *parts, *files;
  CBMAP *params;
  FILE *ifp;
  const char *tmp, *datadir, *body, *sname;
  char *wp, *bound, *cdata, *filedata, *filename, *ebuf, *dbuf, *getname, *delname, numbuf[32];
  int i, clen, blen, filesize, c, sdir, ssize, total;
  time_t stime;
  /* set configurations */
  cbstdiobin();
  scriptname = argv[0];
  if((tmp = getenv("SCRIPT_NAME")) != NULL) scriptname = tmp;
  enc = NULL;
  lang = NULL;
  title = NULL;
  datadir = NULL;
  quota = 0;
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
      } else if(cbstrfwmatch(tmp, "quota:")){
        quota = atoi(skiplabel(tmp));
      }
    }
  }
  if(!enc) enc = DEFENC;
  if(!lang) lang = DEFLANG;
  if(!title) title = DEFTITLE;
  if(!datadir) datadir = DEFDATADIR;
  if(quota < 0) quota = 0;
  /* read parameters */
  filedata = NULL;
  filesize = 0;
  filename = NULL;
  getname = NULL;
  delname = NULL;
  if((tmp = getenv("REQUEST_METHOD")) != NULL && !strcmp(tmp, "POST") &&
     (tmp = getenv("CONTENT_LENGTH")) != NULL && (clen = atoi(tmp)) > 0 &&
     (tmp = getenv("CONTENT_TYPE")) != NULL && cbstrfwmatch(tmp, "multipart/form-data") &&
     (tmp = strstr(tmp, "boundary=")) != NULL){
    tmp += 9;
    if(*tmp == '"') tmp++;
    bound = cbmemdup(tmp, -1);
    if((wp = strchr(bound, ';')) != NULL) *wp = '\0';
    cdata = cbmalloc(clen + 1);
    for(i = 0; i < clen && (c = getchar()) != EOF; i++){
      cdata[i] = c;
    }
    parts = cbmimeparts(cdata, clen, bound);
    if(cblistnum(parts) > 0){
      body = cblistval(parts, 0, &blen);
      params = cbmapopen();
      filedata = cbmimebreak(body, blen, params, &filesize);
      if((tmp = cbmapget(params, "FILENAME", -1, NULL)) != NULL) filename = cbmemdup(tmp, -1);
      cbmapclose(params);
    }
    cblistclose(parts);
    free(cdata);
    free(bound);
  } else if((tmp = getenv("PATH_INFO")) != NULL){
    if(tmp[0] == '/') tmp++;
    getname = cburldecode(tmp, NULL);
    if(cbstrfwmatch(getname, "../") || strstr(getname, "/../") != NULL){
      free(getname);
      getname = NULL;
    }
  } else if((tmp = getenv("QUERY_STRING")) != NULL && (tmp = strstr(tmp, "delfile="))){
    tmp += 8;
    delname = cburldecode(tmp, NULL);
    if(cbstrfwmatch(delname, "../") || strstr(delname, "/../") != NULL){
      free(delname);
      delname = NULL;
    }
  }
  if(getname){
    /* send data of the file */
    if(chdir(datadir) == 0 && (ifp = fopen(getname, "rb")) != NULL){
      printf("Content-Type: %s\r\n", gettype(getname));
      printf("Cache-Control: no-cache, must-revalidate\r\n");
      printf("Pragma: no-cache\r\n");
      printf("\r\n");
      while((c = fgetc(ifp)) != EOF){
        putchar(c);
      }
      fclose(ifp);
    } else {
      printf("Status: 404 Not Found\r\n");
      printf("Content-Type: text/plain; charset=%s\r\n", enc);
      printf("Cache-Control: no-cache, must-revalidate\r\n");
      printf("Pragma: no-cache\r\n");
      printf("\r\n");
      printf("Not Found\n");
      printf("%s\n", getname);
    }
  } else {
    /* output headers */
    printf("Content-Type: text/html; charset=%s\r\n", enc);
    printf("Cache-Control: no-cache, must-revalidate\r\n");
    printf("Pragma: no-cache\r\n");
    printf("\r\n");
    htmlprintf("<?xml version=\"1.0\" encoding=\"%s\"?>\n", enc);
    htmlprintf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
               "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n");
    htmlprintf("<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"%s\" lang=\"%s\">\n",
               lang, lang);
    htmlprintf("<head>\n");
    htmlprintf("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%s\" />\n", enc);
    htmlprintf("<meta http-equiv=\"Content-Style-Type\" content=\"text/css\" />\n");
    htmlprintf("<link rel=\"contents\" href=\"./\" />\n");
    htmlprintf("<title>%@</title>\n", title);
    htmlprintf("<style type=\"text/css\">\n");
    htmlprintf("body { background-color: #eeeeee; color: #111111;"
               " margin: 1em 1em; padding: 1em 1em; }\n");
    htmlprintf("hr { margin: 1.5em 0em; height: 1pt;"
               " color: #999999; background-color: #999999; border: none; }\n");
    htmlprintf("a { color: #0022aa; text-decoration: none; }\n");
    htmlprintf("a:hover { color: #1144ff; text-decoration: underline; }\n");
    htmlprintf("table { border-collapse: collapse; }\n");
    htmlprintf("th,td { text-align: left; }\n");
    htmlprintf("th { padding: 0.1em 0.4em; border: none;"
               " font-size: smaller; font-weight: normal; }\n");
    htmlprintf("td { padding: 0.2em 0.5em; background-color: #ddddee;"
               " border: 1pt solid #888888; }\n");
    htmlprintf(".note { margin: 0.5em 0.5em; text-align: right; }\n");
    htmlprintf("</style>\n");
    htmlprintf("</head>\n");
    htmlprintf("<body>\n");
    htmlprintf("<h1>%@</h1>\n", title);
    htmlprintf("<hr />\n");
    htmlprintf("<form method=\"post\" enctype=\"multipart/form-data\" action=\"%s\">\n",
               scriptname);
    htmlprintf("<div>\n");
    htmlprintf("<input type=\"file\" name=\"file\" size=\"64\""
               " tabindex=\"1\" accesskey=\"0\" />\n");
    htmlprintf("<input type=\"submit\" value=\"UPLOAD\" tabindex=\"2\" accesskey=\"1\" />\n");
    htmlprintf("<a href=\"%s\">[RELOAD]</a>\n", scriptname);
    htmlprintf("</div>\n");
    htmlprintf("</form>\n");
    htmlprintf("<hr />\n");
    /* change the currnt directory */
    if(chdir(datadir) == -1){
      htmlprintf("<p>Changing the current directory was failed.</p>\n");
      htmlprintf("<hr />\n");
    } else {
      /* save the file */
      if(filedata && filename){
        sname = filename;
        if((ebuf = cbiconv(filename, -1, enc, "UTF-8", NULL, NULL)) != NULL) sname = ebuf;
        if((tmp = strrchr(sname, '/')) != NULL) sname = tmp + 1;
        if((tmp = strrchr(sname, '\\')) != NULL) sname = tmp + 1;
        if(ebuf){
          while((tmp = strstr(sname, "\xc2\xa5")) != NULL){
            sname = tmp + 2;
          }
        }
        dbuf = NULL;
        if(ebuf && (dbuf = cbiconv(sname, -1, "UTF-8", enc, NULL, NULL)) != NULL) sname = dbuf;
        if(getdirsize(".") + filesize > quota){
          htmlprintf("<p>Exceeding the quota. -- %@</p>\n", sname);
        } else if(!cbwritefile(sname, filedata, filesize)){
          htmlprintf("<p>Uploading was failed. -- %@</p>\n", sname);
        } else {
          htmlprintf("<p>Uploading was succeeded. -- %@</p>\n", sname);
        }
        htmlprintf("<hr />\n");
        free(dbuf);
        free(ebuf);
      } else if(delname){
        /* delete the file */
        if(unlink(delname) == 0){
          htmlprintf("<p>Deleting was succeeded. -- %@</p>\n", delname);
        } else {
          htmlprintf("<p>Deleting was failed. -- %@</p>\n", delname);
        }
        htmlprintf("<hr />\n");
      }
      /* show the file list */
      if((files = cbdirlist(".")) != NULL){
        cblistsort(files);
        htmlprintf("<table summary=\"files\">\n");
        htmlprintf("<tr>\n");
        htmlprintf("<th abbr=\"name\">Name</th>\n");
        htmlprintf("<th abbr=\"size\">Size</th>\n");
        htmlprintf("<th abbr=\"mtime\">Modified Time</th>\n");
        htmlprintf("<th abbr=\"act\">Actions</th>\n");
        htmlprintf("</tr>\n");
        for(i = 0; i < cblistnum(files); i++){
          sname = cblistval(files, i, NULL);
          if(!strcmp(sname, ".") || !strcmp(sname, "..")) continue;
          if(!cbfilestat(sname, &sdir, &ssize, &stime) || sdir) continue;
          htmlprintf("<tr>\n");
          htmlprintf("<td>%@</td>\n", sname);
          htmlprintf("<td>%d</td>\n", ssize);
          htmlprintf("<td>%@</td>\n", datestr(stime));
          htmlprintf("<td>\n");
          htmlprintf("<a href=\"%s/%?\">[GET]</a>", scriptname, sname);
          htmlprintf(" / ");
          htmlprintf("<a href=\"%s?delfile=%?\">[DEL]</a>", scriptname, sname);
          htmlprintf("</td>\n");
          htmlprintf("</tr>\n");
        }
        htmlprintf("</table>\n");
        cblistclose(files);
        total = getdirsize(".");
        sprintf(numbuf, "%.2f%%", (total * 100.0) / quota);
        htmlprintf("<div class=\"note\">Capacity: %@ (%d/%d)</div>\n", numbuf, total, quota);
      } else {
        htmlprintf("<p>Listing files in the data directory was failed.</p>\n");
      }
      htmlprintf("<hr />\n");
    }
    /* output footers */
    htmlprintf("<div class=\"note\">Powered by QDBM %@.</div>\n", dpversion);
    htmlprintf("</body>\n");
    htmlprintf("</html>\n");
  }
  /* release resources */
  if(getname) free(getname);
  if(delname) free(delname);
  if(filename) free(filename);
  if(filedata) free(filedata);
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


/* get the total size of files in a directory */
int getdirsize(const char *path){
  CBLIST *files;
  const char *sname;
  int i, total, isdir, size;
  total = 0;
  if((files = cbdirlist(path)) != NULL){
    for(i = 0; i < cblistnum(files); i++){
      sname = cblistval(files, i, NULL);
      if(!strcmp(sname, ".") || !strcmp(sname, "..")) continue;
      if(!cbfilestat(sname, &isdir, &size, NULL) || isdir) continue;
      total += size;
    }
  }
  return total;
}


/* get static string of the date */
const char *datestr(time_t t){
  static char buf[32];
  struct tm *stp;
  if(!(stp = localtime(&t))) return "0000/00/00 00:00:00";
  sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d",
          stp->tm_year + 1900, stp->tm_mon + 1, stp->tm_mday,
          stp->tm_hour, stp->tm_min, stp->tm_sec);
  return buf;
}


/* get the media type of a file */
const char *gettype(const char *path){
  char *types[] = {
    ".txt", "text/plain", ".asc", "text/plain", ".html", "text/html", ".htm", "text/html",
    ".mht", "message/rfc822", ".sgml", "application/sgml", ".sgm", "application/sgml",
    ".xml", "application/xml", ".rtf", "application/rtf", ".pdf", "application/pdf",
    ".doc", "application/msword", ".xls", "application/vnd.ms-excel",
    ".ppt", "application/vnd.ms-powerpoint", ".xdw", "application/vnd.fujixerox.docuworks",
    ".zip", "application/zip", ".tar", "application/x-tar", ".gz", "application/x-gzip",
    ".png", "image/png", ".gif", "image/gif", ".jpg", "image/jpeg", ".jpeg", "image/jpeg",
    ".tif", "image/tiff", ".tiff", "image/tiff", ".bmp", "image/bmp", ".mid", "audio/midi",
    ".midi", "audio/midi", ".mp3", "audio/mpeg", ".wav", "audio/x-wav", ".mpg", "video/mpeg",
    ".mpeg", "video/mpeg", NULL
  };
  int i;
  for(i = 0; types[i]; i += 2){
    if(cbstrbwimatch(path, types[i])) return types[i+1];
  }
  return "application/octet-stream";
}



/* END OF FILE */
