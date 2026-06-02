#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  int x;

  if (argc != 2)
    exit(1);
  for (x = 0; argv[1][x]; x++)
    if (argv[1][x] < '0' || argv[1][x] > '9')
      exit(2);
  x = atoi(argv[1]);
  if (x < 1)
    exit(3);
  sleep(x);
  system("nohup Startmush &");
  return 0;
}
