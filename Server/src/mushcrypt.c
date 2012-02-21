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
#include "debug.h"
#include "sha1.h"
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
const char *mux_crypt(const char *, const char *, int *);
#endif

int mush_crypt_validate(dbref player, 
			const char *pUnencrypted, const char *pEncrypted,
			int flag) {
#ifdef MUXCRYPT
  int i_type;
#endif
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
  if ( strcmp(pEncrypted, mux_crypt(pUnencrypted, pEncrypted, &i_type)) == 0) {
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

static void EncodeBase64(size_t nIn, const char *pIn, char *pOut)
{
    size_t nTriples  = nIn/3;
    size_t nLeftover = nIn%3;
    UINT32 stage;

    const UINT8 *p = (const UINT8 *)pIn;
          UINT8 *q = (      UINT8 *)pOut;

    while (nTriples--)
    {
        stage = (p[0] << 16) | (p[1] << 8) | p[2];

        q[0] = Base64Table[(stage >> 18)       ];
        q[1] = Base64Table[(stage >> 12) & 0x3F];
        q[2] = Base64Table[(stage >>  6) & 0x3F];
        q[3] = Base64Table[(stage      ) & 0x3F];

        q += 4;
        p += 3;
    }

    switch (nLeftover)
    {
    case 1:
        stage = p[0] << 16;

        q[0] = Base64Table[(stage >> 18)       ];
        q[1] = Base64Table[(stage >> 12) & 0x3F];
        q[2] = '=';
        q[3] = '=';

        q += 4;
        break;

    case 2:
        stage = (p[0] << 16) | (p[1] << 8);

        q[0] = Base64Table[(stage >> 18)       ];
        q[1] = Base64Table[(stage >> 12) & 0x3F];
        q[2] = Base64Table[(stage >>  6) & 0x3F];
        q[3] = '=';

        q += 4;
        break;
    }
    q[0] = '\0';
}

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

const char szFail[] = "$FAIL$$";

// REMOVE: After 2006-JUL-23, remove support for DES-encrypted passwords on
// Win32 build.  This should allow support for DES-encrypted passwords to
// strattle three distinct versions of MUX.  After that, to convert the older
// passwords automatically would require going through one of these three
// older versions of the server.  Alternatively, since crypt and DES-encrypted
// passwords should be supported on Unix for even longer, converting the
// flatfile on a Unix box remains an option.
//
const char *mux_crypt(const char *szPassword, const char *szSetting, int *piType)
{
    static char buf[SHA1_PREFIX_LENGTH + ENCODED_SALT_LENGTH + 1 + ENCODED_HASH_LENGTH + 1 + 16];
    const char *pSaltField = NULL;
    size_t nSaltField = 0, nAlgo, nSetting;
    char *p;
    SHA1_CONTEXT shac;
    char szHashRaw[21];
    int i;

    memset(szHashRaw, '\0', sizeof(szHashRaw));
    *piType = CRYPT_FAIL;

    if (szSetting[0] == '$')
    {
        p = strchr(szSetting+1, '$');
        if (p)
        {
            p++;
            nAlgo = (size_t) (p - szSetting);
            if (  nAlgo == SHA1_PREFIX_LENGTH
               && memcmp(szSetting, szSHA1Prefix, SHA1_PREFIX_LENGTH) == 0)
            {
                // SHA-1
                //
                pSaltField = p;
                p = strchr(pSaltField, '$');
                if (p)
                {
                    nSaltField = p - pSaltField;
                }
                else
                {
                    nSaltField = strlen(pSaltField);
                }
                if (nSaltField <= ENCODED_SALT_LENGTH)
                {
                    *piType = CRYPT_SHA1;
                }
            }
            else if (  nAlgo == MD5_PREFIX_LENGTH
                    && memcmp(szSetting, szMD5Prefix, MD5_PREFIX_LENGTH) == 0)
            {
                *piType = CRYPT_MD5;
            }
            else if (  nAlgo == BLOWFISH_PREFIX_LENGTH
                    && memcmp(szSetting, szBlowfishPrefix, BLOWFISH_PREFIX_LENGTH) == 0)
            {
                *piType = CRYPT_BLOWFISH;
            }
            else
            {
                *piType = CRYPT_OTHER;
            }
        }
    }
    else if (szSetting[0] == '_')
    {
        *piType = CRYPT_DES_EXT;
    }
    else
    {
#if 0
        // Strictly speaking, we can say the algorithm is DES.
        //
        *piType = CRYPT_DES;
#else
        // However, in order to support clear-text passwords, we restrict
        // ourselves to only verifying an existing DES-encrypted password and
        // we assume a fixed salt of 'XX'.  If you have been using a different
        // salt, or if you need to generate a DES-encrypted password, the
        // following code won't work.
        //
        nSetting = strlen(szSetting);
        if (  nSetting == 13
           && memcmp(szSetting, "XX", 2) == 0)
        {
            *piType = CRYPT_DES;
        }
        else
        {
            *piType = CRYPT_CLEARTEXT;
        }
#endif
    }

    switch (*piType)
    {
    case CRYPT_FAIL:
        return szFail;

    case CRYPT_CLEARTEXT:
        return szPassword;

    case CRYPT_MD5:
    case CRYPT_BLOWFISH:
    case CRYPT_OTHER:
    case CRYPT_DES_EXT:
#ifdef WIN32
        // The WIN32 release only supports SHA1 and clear-text.
        //
        return szFail;
#endif // WIN32

    case CRYPT_DES:
#if defined(HAVE_LIBCRYPT) \
 || defined(HAVE_CRYPT)
        return crypt(szPassword, szSetting);
#else
        return szFail;
#endif
    }

    // Calculate SHA-1 Hash.

    SHA1_Init(&shac);
    SHA1_Compute(&shac, nSaltField, pSaltField);
    SHA1_Compute(&shac, strlen(szPassword), szPassword);
    SHA1_Final(&shac);

    // Serialize 5 UINT32 words into big-endian.
    //

    p = szHashRaw;
    for (i = 0; i <= 4; i++)
    {
        *p++ = (UINT8)(shac.H[i] >> 24);
        *p++ = (UINT8)(shac.H[i] >> 16);
        *p++ = (UINT8)(shac.H[i] >>  8);
        *p++ = (UINT8)(shac.H[i]      );
    }
    *p = '\0';

    //          1         2         3         4
    // 12345678901234567890123456789012345678901234567
    // $SHA1$ssssssssssss$hhhhhhhhhhhhhhhhhhhhhhhhhhhh
    //
    memset(buf, '\0', sizeof(buf));
    strncpy(buf, szSHA1Prefix, SHA1_PREFIX_LENGTH);
    memcpy(buf + SHA1_PREFIX_LENGTH, pSaltField, nSaltField);
    buf[SHA1_PREFIX_LENGTH + nSaltField] = '$';
    EncodeBase64(20, szHashRaw, buf + SHA1_PREFIX_LENGTH + nSaltField + 1);
    return buf;
}

