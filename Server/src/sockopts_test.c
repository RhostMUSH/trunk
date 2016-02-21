#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

main()
{
   int i_mtulen, i_mtu, i_mss, i_msslen, newsock;

   i_mtulen = sizeof(i_mtu);
   i_mtulen = sizeof(i_mss);
   getsockopt(newsock, SOL_IP, IP_MTU, &i_mtu, (unsigned int *)&i_mtulen);
   getsockopt(newsock, IPPROTO_TCP, TCP_MAXSEG,  &i_mss, (unsigned int*)&i_msslen);
}
