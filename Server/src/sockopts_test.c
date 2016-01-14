#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

main()
{
   int i_mtulen, i_mtu, newsock;

   i_mtulen = sizeof(i_mtu);
   getsockopt(newsock, SOL_IP, IP_MTU, &i_mtu, (unsigned int *)&i_mtulen);
}
