#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  int x;

  if (argc != 2)
    exit(1);
  x = atoi(argv[1]);
  if (x < 1)
    exit(2);
  sleep(x);
  system("nohup Startmush &");
  return 0;
}
