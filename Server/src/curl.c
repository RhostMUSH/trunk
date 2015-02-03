#ifdef RHOST_CURL

#include <openssl/ssl.h>
#include <curl/curl.h>
#include <unistd.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

#include "copyright.h"
#include "autoconf.h"

#include "config.h"
#include "db.h"
#include "interface.h"
#include "mudconf.h"
#include "command.h"
#include "functions.h"
#include "externs.h"
#include "match.h"
#include "flags.h"
#include "alloc.h"
#include "vattr.h"


char tempbuff[LBUF_SIZE];
char tempbuff2[LBUF_SIZE];
regex_t curl_has_protocol, curl_valid_url;
uint8_t overflow = 0;

struct curl_options {
   uint8_t response_code;
   uint8_t response_headers;
};

FUNCTION(local_fun_curl_get);
static size_t write_callback( void * contents, size_t size, size_t nmemb, void *userp );
void curl_normalize_line_endings( char * buf, size_t buffer_length );
void curl_prepend_metadata( char * buf, struct curl_options * opts, CURL * curl );

static int ival(char *buff, char **bufcx, int result) {
   sprintf(tempbuff, "%d", result);
   safe_str(tempbuff, buff, bufcx);
   return 0;
}

void initialize_curl_options( struct curl_options * opts ) {
   memset( opts, 0, sizeof( struct curl_options ) );
}

void local_curl_init() {
   FUN *fp;
   char *buff, *cp, *dp;

   regcomp( &curl_has_protocol, ".*://.*", REG_EXTENDED | REG_NOSUB );
   regcomp( &curl_valid_url, "^(https?)://.*", REG_EXTENDED | REG_NOSUB );

   static FUN fun_table[] = {
      {"CURL_GET", local_fun_curl_get, 0, FN_VARARGS, CA_WIZARD, 0 },
      {NULL, NULL, 0, 0, 0, 0}
   };

   /* Register the functions */
   buff = alloc_sbuf("init_curl_functab");
   for (fp = fun_table ; fp->name ; fp++) {
      cp = (char *) fp->name;
      dp = buff;
      while (*cp) {
         *dp = ToLower(*cp);
         cp++;
         dp++;
      }
      *dp = '\0';
      hashadd2(buff, (int *) fp, &mudstate.func_htab, 1);
   }
}

uint8_t parse_curl_options( char ** fargs, size_t nfargs, struct curl_options * opts, char * buff, char ** bufcx ) {
   char * expressionlist_saveptr = NULL;
   char * expression_saveptr = NULL;
   char * expression = NULL;
   char * key = NULL;
   char * value = NULL;
   size_t i;
   for( i = 1; i < nfargs; i++ ) {
      expression = strtok_r( fargs[i], " ", &expressionlist_saveptr );
      while( expression != NULL ) {
         key = strtok_r( expression, "=", &expression_saveptr );
         if( key != NULL ) {
            if( strcasecmp( key, "response_code" ) == 0 ) {
               opts->response_code = 1;
            } else if( strcasecmp( key, "response_headers" ) == 0 ) {
               opts->response_headers = 1;
            } else {
               safe_str( "#-1 UNHANDLED OPTION ", buff, bufcx );
               safe_str( expression, buff, bufcx );
               return 1;
            }
            value = strtok_r( NULL, "=", &expression_saveptr );
         }
         expression = strtok_r( NULL, " ", &expressionlist_saveptr );
      }
   }
   return 0;
}

FUNCTION(local_fun_curl_get)
{
   CURL * curl = NULL;
   CURLcode result;
   struct curl_options opts;

   initialize_curl_options( &opts );

   if( nfargs < 1 ) {
      safe_str( "#-1 CURL_GET EXPECTS 1 OR MORE ARGUMENTS [RECEIVED ", buff, bufcx );
      ival( buff, bufcx, nfargs );
      safe_str( "]", buff, bufcx );
      return;
   }

   if( nfargs >= 2 )
      if( parse_curl_options( fargs, nfargs, &opts, buff, bufcx ) != 0 )
         return;

   if( curl == NULL )
      curl = curl_easy_init();
   if( !curl ) {
      STARTLOG(LOG_ALWAYS, "CURL", "FAIL")
         log_text( "an error occurred initializing the cURL agent" );
      ENDLOG
      safe_str( "#-1 INTERNAL ERROR", buff, bufcx );
      return;
   }

   if( regexec( &curl_has_protocol, fargs[0], 0, NULL, 0 ) == 0 ) {
      // Requested URL includes a protocol
      snprintf( tempbuff, LBUF_SIZE, "%s", fargs[0] );
   } else {
      // Requested URL does not include a protocol, assume http://
      snprintf( tempbuff, LBUF_SIZE, "http://%s", fargs[0] );
   }

   if( regexec( &curl_valid_url, tempbuff, 0, NULL, 0 ) != 0 ) {
      safe_str( "#-1 ONLY HTTP:// AND HTTPS:// ALLOWED [RECEIVED ", buff, bufcx );
      safe_str( tempbuff, buff, bufcx );
      safe_str( "]", buff, bufcx );
      curl_easy_cleanup( curl );
      return;
   }

   curl_easy_setopt( curl, CURLOPT_URL, tempbuff );
   curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );

   curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_callback );
   curl_easy_setopt( curl, CURLOPT_WRITEDATA, &opts );
   curl_easy_setopt( curl, CURLOPT_TIMEOUT, mudconf.curl_request_limit );
   if( opts.response_headers ) {
      curl_easy_setopt( curl, CURLOPT_HEADER, 1 );
   }

   // truncate tempbuff2; this will hold our returned data
   tempbuff2[0] = '\0';
   overflow = 0;
   result = curl_easy_perform( curl );
   if( result == CURLE_OK ) {
      curl_prepend_metadata( tempbuff2, &opts, curl );
      curl_normalize_line_endings( tempbuff2, LBUF_SIZE );
      safe_str( tempbuff2, buff, bufcx );
   } else {
      if( overflow == 1 ) {
         strncpy( tempbuff, tempbuff2, LBUF_SIZE );
         curl_prepend_metadata( tempbuff, &opts, curl );
         curl_normalize_line_endings( tempbuff, LBUF_SIZE );
      } else {
         snprintf( tempbuff, LBUF_SIZE, "#-2 GET FAILED: %s", curl_easy_strerror( result ) );
      }
      safe_str( tempbuff, buff, bufcx );
   }

   curl_easy_cleanup( curl );
}

void curl_prepend_metadata( char * buf, struct curl_options * opts, CURL * curl ) {
   long resp_code;
   char metabuff[LBUF_SIZE];
   char result[LBUF_SIZE];
   result[0] = '\0';
   if( opts->response_code ) {
      curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &resp_code );
      snprintf( metabuff, LBUF_SIZE, "%ld\n", resp_code );
      strncat( result, metabuff, LBUF_SIZE );
   }
   strncat( result, buf, LBUF_SIZE );
   strncpy( buf, result, LBUF_SIZE );
}

void curl_normalize_line_endings( char * buf, size_t buffer_length ) {
   size_t pos = 0, dpos=0;
   char normalized[LBUF_SIZE];
   size_t len = strlen( buf );
   for( pos = 0; pos < (len < LBUF_SIZE-1 ? len : LBUF_SIZE-1); pos++ ) {
      // Basically, discard all \r, replace with \n with \r\n.
      if( buf[pos] == '\r' )
         continue;
      if( buf[pos] == '\n' ) {
         normalized[dpos++] = '\r';
      }
      // Avoid chance of buffer overflow
      if( dpos == LBUF_SIZE-1 ) {
         normalized[dpos] = '\0';
         break;
      }
      normalized[dpos++] = buf[pos];
      normalized[dpos] = '\0';
   }
   strncpy( buf, normalized, LBUF_SIZE );
   return;
}

static size_t write_callback( void * contents, size_t size, size_t nmemb, void *userp ) {
   struct curl_options * opt = userp;
   size_t realsize;
   size_t bytesleft;
   realsize = size * nmemb;
   bytesleft = LBUF_SIZE - strnlen( tempbuff2, LBUF_SIZE ) - 1;
   if( realsize < bytesleft ) {
      strncat( tempbuff2, contents, realsize );
      return realsize;
   } else {
      strncat( tempbuff2, contents, bytesleft );
      overflow = 1;
      return bytesleft;
   }
}

#endif
