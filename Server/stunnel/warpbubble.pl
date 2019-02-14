#!/usr/bin/env perl
#                                  8             8      8      8        
#                                  8             8      8      8        
#    o   o   o .oPYo. oPYo. .oPYo. 8oPYo. o    o 8oPYo. 8oPYo. 8 .oPYo. 
#    Y. .P. .P .oooo8 8  `' 8    8 8    8 8    8 8    8 8    8 8 8oooo8 
#    `b.d'b.d' 8    8 8     8    8 8    8 8    8 8    8 8    8 8 8.     
#     `Y' `Y'  `YooP8 8     8YooP' `YooP' `YooP' `YooP' `YooP' 8 `Yooo' 
#                           8
#                           8
#
#            A quick and dirty implementation of telnet?!??!
#
# Credit:  Adrick
#
use strict;                                           # required modules
use IO::Select;
use IO::Socket;
my $buf;
$| = 1;

# initialize listener
my $listener = IO::Select->new();                     # initialize listener

# open new connection, add to listener
my $new = IO::Socket::INET->new(PeerAddr => @ARGV[0], blocking => 0) ||
      die("Unable to connect to @ARGV[0]");
$listener->add($new);
printf($new "@ARGV[1] %s\n",@ENV{REMOTE_HOST});       # send command

# add stdin to listener
$listener->add(\*STDIN);

while(1) {

   my ($sockets) = IO::Select->select($listener,undef,undef,10);

   for my $sock (@$sockets) {
      if(sysread($sock,$buf,1024) <= 0) {             # socket disconnected
         $listener->remove($sock);                    # clean up
         exit(0);
      } elsif($sock == $new) {                        # handle socket input
         printf("%s",$buf);
      } else {                                        # stdin input
         printf($new "%s",$buf);
      }
   }
}

exit(1);

