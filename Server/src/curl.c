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


FUNCTION(local_fun_curl_get);
static size_t write_callback( void * contents, size_t size, size_t nmemb, void *userp );

char tempbuff[LBUF_SIZE];
char tempbuff2[LBUF_SIZE];
regex_t curl_has_protocol, curl_valid_url;
uint8_t overflow = 0;

static int ival(char *buff, char **bufcx, int result) {
   sprintf(tempbuff, "%d", result);
   safe_str(tempbuff, buff, bufcx);
   return 0;
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

FUNCTION(local_fun_curl_get)
{
   CURL * curl = NULL;
   CURLcode result;

   if( nfargs < 1 ) {
      safe_str( "#-1 CURL_GET EXPECTS 1 ARGUMENT [RECEIVED ", buff, bufcx );
      ival( buff, bufcx, nfargs );
      safe_str( "]", buff, bufcx );
      return;
   }

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
   curl_easy_setopt( curl, CURLOPT_TIMEOUT, mudconf.curl_request_limit );

   // truncate tempbuff2; this will hold our returned data
   tempbuff2[0] = '\0';
   overflow = 0;
   result = curl_easy_perform( curl );
   if( result == CURLE_OK ) {
      safe_str( tempbuff2, buff, bufcx );
   } else {
      if( overflow == 1 ) {
         strncpy( tempbuff, tempbuff2, LBUF_SIZE );
      } else {
         snprintf( tempbuff, LBUF_SIZE, "#-2 GET FAILED: %s", curl_easy_strerror( result ) );
      }
      safe_str( tempbuff, buff, bufcx );
   }

   curl_easy_cleanup( curl );
}

static size_t write_callback( void * contents, size_t size, size_t nmemb, void *userp ) {
	size_t realsize = size * nmemb;
	size_t bytesleft = LBUF_SIZE - strnlen( tempbuff2, LBUF_SIZE ) - 1;
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
