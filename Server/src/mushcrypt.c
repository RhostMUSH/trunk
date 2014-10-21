/* This file defines the function mush_crypt(key) used for password  *
 * encryption. It's got a whole messy bunch of #ifdef's in it 'cos   *
 * flexibility is good, right?!                                      *
 * By compiling with the right -D flag you can disable SHS or DES    *
 *                                                                   *
 * File based on the Pennmush 1.7.2 mycrypt.c                        */


#include <stdio.h>
#include <string.h>
#include "autoconf.h"
#include "config.h"
#include "externs.h"
#ifdef HAS_OPENSSL
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#endif
#include "debug.h"
#include "sha1.h"
#include "ansi.h"

#define FILENUM MUSHCRYPT_C

#ifdef SHS_REVERSE
int reverse_shs = 1;
#else
int reverse_shs = 0;
#endif

/* Validation check - you can't have two only's */
#ifdef CRYPT_SHS_ONLY
#ifdef CRYPT_DES_ONLY
#error Both CRYPT_SHS_ONLY and CRYPT_DES_ONLY are defined.. this makes no sense 
#endif
#endif

/* Makes the encrypt function (mush_crypt) easier to read 
 * Basically if both SHS and CRYPT are valid, we'll use crypt
 * If only crypt is valid, we'll use that.
 * If only SHS is valid, we'll use that.
 * else we'll spit out an error
*/

#define CRYPT_ENCRYPT_SHS 1
#define CRYPT_ENCRYPT_DES 1

#ifdef  CRYPT_DES_ONLY
#undef  CRYPT_ENCRYPT_SHS
#elif   CRYPT_SHS_ONLY
#undef  CRYPT_ENCRYPT_DES
#endif


#ifdef CRYPT_ENCRYPT_SHS
#include "shs.h"
#endif
#ifdef CRYPT_ENCRYPT_DES
char *crypt(const char *key, const char *salt);
#endif
#ifdef MUXCRYPT
int check_mux_password(const char *, const char *);
#endif

int mush_crypt_validate(dbref player, 
			const char *pUnencrypted, const char *pEncrypted,
			int flag) {
#ifdef CRYPT_ENCRYPT_SHS
  SHS_INFO shsInfo;
  static char crypt_buff[70];
#endif

  DPUSH; /* #95 */

#ifdef CRYPT_ENCRYPT_DES
  if (strcmp(pEncrypted, crypt(pUnencrypted, "XX")) == 0) {
    RETURN(1); /* #95 */
  }
#endif

#ifdef CRYPT_ENCRYPT_SHS
  shsInfo.reverse_wanted = (BYTE) reverse_shs;
  shsInit(&shsInfo);
  shsUpdate(&shsInfo, (BYTE *) pUnencrypted, strlen(pUnencrypted));
  shsFinal(&shsInfo);
  sprintf(crypt_buff, "XX%lu%lu", shsInfo.digest[0], shsInfo.digest[1]);
  if (strcmp(pEncrypted, crypt_buff) == 0) {
    RETURN(1); /* #95 */
  }
#endif

  if (strcmp(pUnencrypted, pEncrypted) == 0) {
    if (flag == 0) {
      s_Pass(player, mush_crypt(pEncrypted));    
    } else {
      s_MPass(player, mush_crypt(pEncrypted));
    }
    RETURN(1); /* #95 */
  }

/* At this stage, let's see if we're doing MUX encryption */
#ifdef MUXCRYPT
  if ( check_mux_password(pEncrypted, pUnencrypted) ) {
     RETURN(1); /* #95 */
  }
#endif
  RETURN(0); /* #95 */
}


char * mush_crypt(const char *key) {
#ifdef CRYPT_ENCRYPT_DES
  DPUSH; /* #96 */
  RETURN(crypt(key, "XX")); /* #96 */
#elif CRYPT_ENCRYPT_SHS
  SHS_INFO shsInfo;
  static char crypt_buff[70];
  DPUSH; /* #97 */
  shsInfo.reverse_wanted = (BYTE) reverse_shs;
  shsInit(&shsInfo);
  shsUpdate(&shsInfo, (BYTE *) key, strlen(key));
  shsFinal(&shsInfo);
  sprintf(crypt_buff, "XX%lu%lu", shsInfo.digest[0], shsInfo.digest[1]);
  RETURN(crypt_buff); /* #97 */
#else
#error mush_crypt() invalid state detected, no encryption scheme!
#endif
}

/* This used with permission from MUX2 */
const char Base64Table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define ENCODED_LENGTH(x) ((((x)+2)/3)*4)
#ifndef UINT32
#define UINT32 unsigned int
#endif
#ifndef UINT8
#define UINT8 unsigned short int
#endif


#define SHA1_PREFIX_LENGTH 6
const char szSHA1Prefix[SHA1_PREFIX_LENGTH+1] = "$SHA1$";
#define ENCODED_HASH_LENGTH ENCODED_LENGTH(5*sizeof(UINT32))

#define MD5_PREFIX_LENGTH 3
const char szMD5Prefix[MD5_PREFIX_LENGTH+1] = "$1$";

#define BLOWFISH_PREFIX_LENGTH 4
const char szBlowfishPrefix[BLOWFISH_PREFIX_LENGTH+1] = "$2a$";

#define SALT_LENGTH 9
#define ENCODED_SALT_LENGTH ENCODED_LENGTH(SALT_LENGTH)

#define CRYPT_FAIL        0
#define CRYPT_SHA1        1
#define CRYPT_MD5         2
#define CRYPT_DES         3
#define CRYPT_DES_EXT     4
#define CRYPT_BLOWFISH    5
#define CRYPT_CLEARTEXT   6
#define CRYPT_OTHER       7

/* saved is the enrypted password, password is what is entered */
#ifdef HAS_OPENSSL


int
encode_base64(const char *input, int len, char *buff, char **bp)
{
  BIO *bio, *b64, *bmem;
  char *membuf;

  b64 = BIO_new(BIO_f_base64());
  if (!b64) {
    safe_str((char *)"#-1 ALLOCATION ERROR", buff, bp);
    return 0;
  }

  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

  bmem = BIO_new(BIO_s_mem());
  if (!bmem) {
    safe_str((char *)"#-1 ALLOCATION ERROR", buff, bp);
    BIO_free(b64);
    return 0;
  }

  bio = BIO_push(b64, bmem);

  if (BIO_write(bio, input, len) < 0) {
    safe_str((char *)"#-1 CONVERSION ERROR", buff, bp);
    BIO_free_all(bio);
    return 0;
  }

  (void) BIO_flush(bio);

  len = BIO_get_mem_data(bmem, &membuf);
  safe_copy_str(membuf, buff, bp, ((len > (LBUF_SIZE - 2)) ? (LBUF_SIZE - 2) : len));

  BIO_free_all(bio);

  return 1;
}

int
decode_base64(char *encoded, int len, char *buff, char **bp, int key)
{
  BIO *bio, *b64, *bmem;
  char decoded[LBUF_SIZE], *pdec;
  int dlen;

  memset(decoded, '\0', LBUF_SIZE);
  b64 = BIO_new(BIO_f_base64());
  if (!b64) {
    safe_str((char *)"#-1 ALLOCATION ERROR", buff, bp);
    return 0;
  }
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

  bmem = BIO_new_mem_buf(encoded, len);
  if (!bmem) {
    safe_str((char *)"#-1 ALLOCATION ERROR", buff, bp);
    BIO_free(b64);
    return 0;
  }
  //  len = BIO_set_close(bmem, BIO_NOCLOSE); This makes valgrind report a memory leak.

  bio = BIO_push(b64, bmem);

  dlen = BIO_read(bio, decoded, LBUF_SIZE -1);
  if ( dlen >= 0 )
     decoded[dlen]='\0';
  pdec = decoded;
  if ( !key ) {
     while ( pdec && *pdec ) {
        if ( !((*pdec == BEEP_CHAR) || isprint(*pdec) || isascii(*pdec)) )
           *pdec = '?';
        if ( !(*pdec == BEEP_CHAR) && (((int)*pdec > 255) || ((int)*pdec < 32)) )
           *pdec = '?';
        pdec++;
     }
  }
  safe_str(decoded, buff, bp);

  BIO_free_all(bio);

  return 1;
}

int
check_mux_password(const char *saved, const char *password)
{
   EVP_MD_CTX ctx;
   const EVP_MD *md;
   uint8_t hash[EVP_MAX_MD_SIZE];
   unsigned int rlen = EVP_MAX_MD_SIZE;
   char *decoded, *dp;
   char *start, *end;
   int return_chk;

   start = (char *) saved;

   /* MUX passwords start with a '$' */
   if (*start != '$')
      return 0;

   start++;
   /* The next '$' marks the end of the encryption algo */
   end = strchr(start, '$');
   if (end == NULL)
      return 0;

   *end++ = '\0';

   md = EVP_get_digestbyname(start);
   if (!md)
      return 0;

   start = end;
   /* Up until the next '$' is the salt. After that is the password */
   end = strchr(start, '$');
   if (end == NULL)
      return 0;

   *end++ = '\0';

   /* 'start' now holds the salt, 'end' the password.
   * Both are base64-encoded. */

   dp = decoded = alloc_lbuf("decode_buffer");
   /* decode the salt */
   decode_base64(start, strlen(start), decoded, &dp, 1);

   /* Double-hash the password */
   EVP_DigestInit(&ctx, md);
   EVP_DigestUpdate(&ctx, start, strlen(start));
   EVP_DigestUpdate(&ctx, password, strlen(password));
   EVP_DigestFinal(&ctx, hash, &rlen);

   /* Decode the stored password */
   dp = decoded;
   decode_base64(end, strlen(end), decoded, &dp, 1);

   /* Compare stored to hashed */
   return_chk = (memcmp(decoded, hash, rlen) == 0);
   free_lbuf(decoded);
   return (return_chk);
}
#else
int
encode_base64(const char *input, int len, char *buff, char **bp)
{
   safe_str((char *)"#-1 BASE64 disabled without openssl support.", buff, bp);
   return 0;
}

int
decode_base64(const char *input, int len, char *buff, char **bp, int key)
{
   safe_str((char *)"#-1 BASE64 disabled without openssl support.", buff, bp);
   return 0;
}

int
check_mux_password(const char *saved, const char *password) 
{
   return 0;
}
#endif

