#!/usr/bin/perl
#                                  8             8      8      8        
#                                  8             8      8      8        
#    o   o   o .oPYo. oPYo. .oPYo. 8oPYo. o    o 8oPYo. 8oPYo. 8 .oPYo. 
#    Y. .P. .P .oooo8 8  `' 8    8 8    8 8    8 8    8 8    8 8 8oooo8 
#    `b.d'b.d' 8    8 8     8    8 8    8 8    8 8    8 8    8 8 8.     
#     `Y' `Y'  `YooP8 8     8YooP' `YooP' `YooP' `YooP' `YooP' 8 `Yooo' 
#                           8
#                           8
#
#            A quick and dirty implimentation of telnet?!??!
#
use strict;                                              # required modules
use IO::Select;
use IO::Socket;
use File::Basename;
my %conf;

#
# usage
#    Provide a little help with how to use the script.
#
sub usage
{
   my $err = shift;

   printf(STDERR "%s\n\n",$err) if $err ne undef;
   printf(STDERR "%s",<<__EOF__);

usage: $0 [--conf=<file>]
       $0 <command> <host> <port>

       --conf=<file>  : Specifies the location and name of the 
                        configuration file instead of @conf{file}.

       <command>      : The command issued when first connecting to
                        the MU* server. The command is followed by a
                        space and the contents of the environment
                        variable REMOTE_HOST.
       <host>         : Hostname of the MU* server
       <port>         : Port of the MU* server

       If a configuration file is used, use the below format:
          host: <hostname>
          port: <port>
          command: <command>

       Example:

          host: teenymush.dynu.net
          port: 4096
          command: xyzzy 

__EOF__
   exit(1);
}

#
# read_configuration
#    Get the contents of the config file, and verify all options are set.
#
sub read_configuration
{
   my $file;

   open($file,@conf{file}) ||                            # open conf file
      usage("Unable to open config file '@conf{file}' for reading");

   while(<$file>) {
      s/\r|\n//g;
      if(/^\s*(host|port|command)\s*:\s*([^ ]+)\s*$/) {
         @conf{lc($1)} = $2;
      } elsif(!/^\s*#/) {
         die("Garbage in conf file: '" . $_ . "'");
      }
   }
   close($file);
  
   for my $item ("host","port","command") { 
      defined @conf{$item} ||
         die("Configuration file is missing a $item entry");
   }
}

#
# handle_command_line
#    Look for any options, or command/hostname/port.
#
sub handle_command_line
{
   my $pos = 0;
   for my $i ( 0 .. $#ARGV ) {
      if(@ARGV[$i] =~ /^--conf=(.*)$/) {
         @conf{file} = $1;
      } elsif(@ARGV[$i] =~ /^--([^=]+)=(.*)$/) {
         usage("Undefined option $1");
      } elsif(++$pos == 1) {
         @conf{command} = @ARGV[$i];
      } elsif($pos == 2) {
         @conf{host} = @ARGV[$i];
      } elsif($pos == 3) {
         @conf{port} = @ARGV[$i];
      } else {
         usage();
      }
   }
   usage($pos) if (!($pos == 3 || $pos == 0));
}
  

#
# main
#    Read configuration file, open socket, relay data till the socket
#    closes.
#
sub main
{
   my $listener = IO::Select->new();                    # initialize listener
   @conf{file} =(($0=~/([^\/\\]+)\.([^\.]+)$/) ? $1 : $0).".conf";
   my $buf;

   handle_command_line();

   read_configuration() if(!defined @conf{host});

   # open new connection, add to listener
   my $new = IO::Socket::INET->new(PeerAddr => "@conf{host}:@conf{port}", 
                                   blocking => 0) ||
         die("Unable to connect to @conf{host}:@conf{port}");
   $listener->add($new);

   printf($new "%s %s\n",@conf{command},@ENV{REMOTE_HOST});    # send command

   $listener->add(\*STDIN);                            # add stdin to listener
   $| = 1;                                                    # unbuffer stdin

   while(1) {

      my ($sockets) = IO::Select->select($listener,undef,undef,10);     # wait

      for my $sock (@$sockets) {
         if(sysread($sock,$buf,1024) <= 0) {             # socket disconnected
            $listener->remove($sock);                               # clean up
            exit(0);
         } elsif($sock eq $new) {                           # got socket input
            printf("%s",$buf);
         } else {                                            # got stdin input
            printf($new "%s",$buf);
         }
      }
   }
}

main();
