#!/usr/bin/perl
#
# Inspired by a similar script included with Pennmush
#

# These files wont be copied, but linked to the originals. useful for things
# like the binaries, and help files which are standard throughout any Rhost
# server.

%link = (
	'./game/netrhost' => '../bin/netrhost',
	'./game/netrhost.debugmon' => '../bin/netrhost.debugmon',
	'./game/mkindx' => '../bin/mkindx',
	'./game/mkhtml' => '../bin/mkhtml',
	'./game/dbconvert' => '../bin/dbconvert',
	'./game/txt/help.txt' => '../../game/txt/help.txt',
	'./game/txt/wizhelp.txt' => '../../game/txt/wizhelp.txt'
	);

# These wont be copied, or in the case of directories, recreated. Matches any
# path which begins with one of these. If you want a directory to be created,
# but not its contents copied, then insert: ./game/path/, if you dont want
# the directory to be created either use:   ./game/path
%dontcopy = (
	'./game/data/' => '1',
	'./game/oldflat/' => '1',
	'./game/prevflat/' => '1'
	);

$proceed = 0;
while(!$proceed) {

  print "Please enter a name for your game directory: ";
  chop($dir = <STDIN>);

  if ($dir =~ /[^A-Za-z0-9_]/) {
    print "Error: Directory name contains bad characters.\n";
    next;
  }

  if (-d $dir) {
    print "That directory already exists. Overwrite [n]? ";
    $yn = <STDIN>;
    unless ($yn =~ /^[yY]/) {
      next;
    }
  }

  # If reaching this point, everything's OK!
  $proceed = 1;
}

print "\tCreating directory ./$dir for new MUSH..";
mkdir($dir,0755) unless (-d $dir);
die "FAIL\n" unless (-d $dir);
print "OK\n";

print "\nPlease enter a name for your MUSH: ";
chop($mud_name = <STDIN>);
print "\nPlease enter a port for your MUSH: ";
chop($port = <STDIN>);

while($port !~ /^\d+$/) {
  print "Bad port ($port) entered, try again: ";
  chop($port = <STDIN>);
}

# Copy the stuff.

sub walk_tree {
    my @todo=@_;
        return sub {
	        return unless @todo;
	        my $dir=pop @todo;
	        if(-d $dir && ! -l $dir && opendir my $dh, $dir) {
		    my @subitems=
			grep {$_ ne '.' && $_ ne '..'} readdir $dh;
		    @subitems=map "$dir/$_",@subitems;
		    push @todo, @subitems;
	        }
	        return $dir;
       };
}

$files = walk_tree('./game');
													
while(defined($file = &$files)) {
    ($newfile = $file) =~ s!.\/game\/!.\/$dir\/!;

    $next = 0;
    foreach (sort keys %dontcopy) {
	$next = 1 if($file =~ /^$_/)
    }
    next if $next;
    
    if(defined $link{$file}) {
	symlink($link{$file}, $newfile); 
    }
    elsif(-d $file) {
	mkdir($newfile, 0755);
    }
    else {
	system("cp -p $file $newfile");
    }
}

# Now read the config file and replace the mud_name, port and debug_port.

system("rm ./$dir/netrhost.conf");

die "Unable to open ./game/netrhost.conf and ./$dir/netrhost.conf!\n"
  unless (open(FIN, './game/netrhost.conf') && open(FOUT, ">./$dir/$dir.conf"));

while(<FIN>) {
  if(/^mud_name/) {
    print FOUT "mud_name\t$mud_name\n";
  }
  elsif(/^port/) {
    print FOUT "port\t$port\n";
  }
  elsif(/^debug_id/) {
    print FOUT "debug_id\t$port", "0\n";
  }
  elsif(/^input_database/) {
    print FOUT "input_database\t$dir.db\n";
  }
  elsif(/^output_database/) {
    print FOUT "output_database\t$dir.db.new\n";
  }
  elsif(/^crash_database/) {
    print FOUT "crash_database\t$dir.db.CRASH\n";
  }
  elsif(/^gdbm_database/) {
    print FOUT "gdbm_database\t$dir.gdbm\n";
  }
  else
  {
    print FOUT $_;
  }
}

close FIN;
close FOUT;

die "Unable to open ./game/mush.config and ./$dir/mush.config!\n"
  unless (open(FIN, './game/mush.config') && open(FOUT, ">./$dir/mush.config"));

while(<FIN>) {
  if(/^GAMENAME/) {
    print FOUT "GAMENAME=$dir\n";
  }
  else {
    print FOUT $_;
  }
}

close FIN;
close FOUT;

print "\nCustomization completed for $mud_name, see ./$dir!\n";
