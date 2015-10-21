#include <stdio.h>
#include <dirent.h>

static int file_select(const struct dirent *entry)
{
   return(1);
}

int
main()
{
   struct dirent **namelist;

   scandir((char *)"/var/tmp", &namelist, file_select, alphasort);
   return(0);
}
