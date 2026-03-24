#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main()
{
   char *x;

   x = (char *)crypt("abcde", "XX012345");
   if ( x ) {
      printf("%s\n", "3DES");
   }
   x = (char *)crypt("abcde", "$sha1$0123456789$");
   if ( x ) {
      printf("%s\n", "SHA1");
   }
   x = (char *)crypt("abcde", "$md5$0123456789$");
   if ( x ) {
      printf("%s\n", "Sun-MD5");
   }
   x = (char *)crypt("abcde", "$1$0123456789$");
   if ( x ) {
      printf("%s\n", "MD5crypt");
   }
   x = (char *)crypt("abcde", "$2a$0123456789$");
   if ( x ) {
      printf("%s\n", "Blowfish");
   }
   x = (char *)crypt("abcde", "$6$12345$");
   if ( x ) {
      printf("%s\n", "SHA512");
   }
   x = (char *)crypt("abcde", "$6$rounds=50000$12345$");
   if ( x ) {
      printf("%s\n", "SHA512 (50000 rounds)");
   }
   x = (char *)crypt("abcde", "$y$12345$");
   if ( x ) {
      printf("%s\n", "YESCrypt");
   }
   x = (char *)crypt("abcde", "$gy$12345$");
   if ( x ) {
      printf("%s\n", "Gost-YESCrypt");
   }
   x = (char *)crypt("abcde", "$7$12345$");
   if ( x ) {
      printf("%s\n", "SCrypt");
   }
   x = (char *)crypt("abcde", "$2b$12345$");
   if ( x ) {
      printf("%s\n", "BCrypt");
   }

}

