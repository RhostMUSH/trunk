#include <stdio.h>

main()
{
  char test[100];
  int d;

  sscanf("test 345","%s %d",test,&d);
  printf("%s %d\n",test,d);
}
