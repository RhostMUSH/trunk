#include <time.h>
int
main()
{
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  return(0);
}
