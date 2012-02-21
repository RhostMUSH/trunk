#=================================================================================================
# Perl API of Curia, the extended API of QDBM
#                                                       Copyright (C) 2000-2005 Mikio Hirabayashi
# This file is part of QDBM, Quick Database Manager.
# QDBM is free software; you can redistribute it and/or modify it under the terms of the GNU
# Lesser General Public License as published by the Free Software Foundation; either version
# 2.1 of the License or any later version.  QDBM is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
# You should have received a copy of the GNU Lesser General Public License along with QDBM; if
# not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA.
#=================================================================================================


package Curia;

use strict;
use warnings;

require Tie::Hash;
require Exporter;
require XSLoader;
our @ISA = qw(Tie::Hash Exporter);
our $VERSION = '1.0';
XSLoader::load('Curia', $VERSION);

use constant TRUE => 1;                  # boolean true
use constant FALSE => 0;                 # boolean false

use constant OREADER => 1 << 0;          # open as a reader
use constant OWRITER => 1 << 1;          # open as a writer
use constant OCREAT => 1 << 2;           # a writer creating
use constant OTRUNC => 1 << 3;           # a writer truncating
use constant ONOLCK => 1 << 4;           # open without locking
use constant OLCKNB => 1 << 5;           # lock without blocking
use constant OSPARSE => 1 << 6;          # create as sparse files

use constant DOVER => 0;                 # overwrite an existing value
use constant DKEEP => 1;                 # keep an existing value
use constant DCAT => 2;                  # concatenate values

my(%handles) = ();                       # table of database names
our($errmsg) = "no error";               # message of the last error



#=================================================================================================
# public objects
#=================================================================================================


##
# use Curia;
# Module `Curia' should be loaded in application codes.
# An instance of the class `Curia' is used as a database handle.
#


##
# $Curia::errmsg;
# Global Variable: The message of the last happened error.
#


##
# $curia = new Curia($name, $omode, $bnum, $dnum);
# Constructor: Get the database handle.
# `$name' specifies the name of a database directory.
# `$omode' specifies the connection mode: `Curia::OWRITER' as a writer, `Curia::OREADER' as a
# reader.  If the mode is `Curia::OWRITER', the following may be added by bitwise or:
# `Curia::OCREAT', which means it creates a new database if not exist, `Curia::OTRUNC', which
# means it creates a new database regardless if one exists.  Both of `Curia::OREADER' and
# `Curia::OWRITER' can be added to by bitwise or: `Curia::ONOLCK', which means it opens a
# database directory without file locking, or `Curia::OLCKNB', which means locking is performed
# without blocking.  `Curia::OCREAT' can be added to by bitwise or: `Curia::OSPARSE', which means
# it creates database files as sparse files.  If it is undef, `Curia::OREADER' is specified.
# `$bnum' specifies the number of elements of the bucket array.  If it is undef or not more
# than 0, the default value is specified.  The size of a bucket array is determined on creating,
# and can not be changed except for by optimization of the database.  Suggested size of a
# bucket array is about from 0.5 to 4 times of the number of all records to store.
# `$dnum' specifies the number of division of the database. If it is undef or not more than 0,
# the default value is specified.  The number of division can not be changed from the initial
# value.  The max number of division is 512.
# The return value is the database handle or undef if it is not successful.
# While connecting as a writer, an exclusive lock is invoked to the database directory.
# While connecting as a reader, a shared lock is invoked to the database directory.  The thread
# blocks until the lock is achieved.  If `Curia::ONOLCK' is used, the application is responsible
# for exclusion control.
#
sub new {
    my($class) = shift;
    my($name) = shift;
    my($omode) = shift;
    my($bnum) = shift;
    my($dnum) = shift;
    (defined($name) && length($name) > 0 && scalar(@_) == 0) || return undef;
    (!$handles{$name}) || return undef;
    (defined($omode)) || ($omode = OREADER);
    (defined($bnum)) || ($bnum = -1);
    (defined($dnum)) || ($dnum = -1);
    my($curia) = plcropen($name, $omode, $bnum, $dnum);
    $errmsg = plcrerrmsg();
    ($curia > 0) || return undef;
    $handles{$name} = $curia;
    my $self = [$name, TRUE, undef, undef, undef, undef];
    bless($self, $class);
    return $self;
}


##
# $bool = $curia->close();
# Method: Close the database handle.
# If successful, the return value is true, else, it is false.
# Because the region of a closed handle is released, it becomes impossible to use the handle.
# Updating a database is assured to be written when the handle is closed.  If a writer opens
# a database but does not close it appropriately, the database will be broken.
#
sub close {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    $$self[1] = FALSE;
    my($curia) = $handles{$$self[0]};
    my($rv) = plcrclose($curia);
    $errmsg = plcrerrmsg();
    delete($handles{$$self[0]});
    return $rv;
}


##
# $bool = $curia->put($key, $val, $dmode);
# Method: Store a record.
# `$key' specifies a key.  If it is undef, this method has no effect.
# `$val' specifies a value.  If it is undef, this method has no effect.
# `$dmode' specifies behavior when the key overlaps, by the following values: `Curia::DOVER',
# which means the specified value overwrites the existing one, `Curia::DKEEP', which means the
# existing value is kept, `Curia::DCAT', which means the specified value is concatenated at
# the end of the existing value.  If it is undef, `Curia::DOVER' is specified.
# If successful, the return value is true, else, it is false.
#
sub put {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    my($key) = shift;
    my($val) = shift;
    my($dmode) = shift;
    (scalar(@_) == 0) || return FALSE;
    (defined($key) && defined($val)) || return FALSE;
    (defined($dmode)) || ($dmode = DOVER);
    my($curia) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    if($$self[3]){
        local($_) = $val;
        $$self[3]();
        $val = $_;
    }
    my($rv) = plcrput($curia, $key, length($key), $val, length($val), $dmode);
    $errmsg = plcrerrmsg();
    return $rv;
}


##
# $bool = $curia->out($key);
# Method: Delete a record.
# `$key' specifies a key.  If it is undef, this method has no effect.
# If successful, the return value is true, else, it is false.  False is returned when no
# record corresponds to the specified key.
#
sub out {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    my($key) = shift;
    (scalar(@_) == 0) || return FALSE;
    (defined($key)) || return FALSE;
    my($curia) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    my($rv) = plcrout($curia, $key, length($key));
    $errmsg = plcrerrmsg();
    return $rv;
}


##
# $str = $curia->get($key, $start, $max);
# Method: Retrieve a record.
# `$key' specifies a key.  If it is undef, this method has no effect.
# `$start' specifies the offset address of the beginning of the region of the value to be read.
# If it is negative or undef, the offset is specified as 0.
# `$max' specifies the max size to be read.  If it is negative or undef, the size to read is
# unlimited.
# If successful, the return value is a scalar of the value of the corresponding record, else, it
# is undef.  undef is returned when no record corresponds to the specified key or the size of
# the value of the corresponding record is less than `$start'.
#
sub get {
    my($self) = shift;
    ($$self[1]) || return undef;
    my($key) = shift;
    my($start) = shift;
    my($max) = shift;
    (scalar(@_) == 0) || return undef;
    (defined($key)) || return undef;
    (defined($start) && $start >= 0) || ($start = 0);
    (defined($max) && $start >= 0) || ($max = -1);
    my($curia) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    my($rv) = plcrget($curia, $key, length($key), $start, $max);
    $errmsg = plcrerrmsg();
    if($rv && $$self[5]){
        local($_) = $rv;
        $$self[5]();
        $rv = $_;
    }
    return $rv;
}


##
# $num = $curia->vsiz($key);
# Method: Get the size of the value of a record.
# `$key' specifies a key.  If it is undef, this method has no effect.
# If successful, the return value is the size of the value of the corresponding record, else,
# it is -1.
# Because this method does not read the entity of a record, it is faster than `get'.
#
sub vsiz {
    my($self) = shift;
    ($$self[1]) || return -1;
    my($key) = shift;
    (scalar(@_) == 0) || return -1;
    (defined($key)) || return -1;
    my($curia) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    my($rv) = plcrvsiz($curia, $key, length($key));
    $errmsg = plcrerrmsg();
    return $rv;
}


##
# $bool = $curia->iterinit();
# Method: Initialize the iterator of the database handle.
# If successful, the return value is true, else, it is false.
# The iterator is used in order to access the key of every record stored in a database.
#
sub iterinit {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($curia) = $handles{$$self[0]};
    my($rv) = plcriterinit($curia);
    $errmsg = plcrerrmsg();
    return $rv;
}


##
# $str = $curia->iternext();
# Method: Get the next key of the iterator.
# If successful, the return value is a scalar of the value of the next key, else, it is undef.
# undef is returned when no record is to be get out of the iterator.
# It is possible to access every record by iteration of calling this method.  However, it is
# not assured if updating the database is occurred while the iteration.  Besides, the order of
# this traversal access method is arbitrary, so it is not assured that the order of storing
# matches the one of the traversal access.
#
sub iternext {
    my($self) = shift;
    ($$self[1]) || return undef;
    (scalar(@_) == 0) || return undef;
    my($curia) = $handles{$$self[0]};
    my($rv) = plcriternext($curia);
    $errmsg = plcrerrmsg();
    if($rv && $$self[4]){
        local($_) = $rv;
        $$self[4]();
        $rv = $_;
    }
    return $rv;
}


##
# $bool = $curia->setalign($align);
# Method: Set alignment of the database handle.
# `$align' specifies the basic size of alignment.  If it is undef, alignment is cleared.
# If successful, the return value is true, else, it is false.
# If alignment is set to a database, the efficiency of overwriting values is improved.
# The size of alignment is suggested to be average size of the values of the records to be
# stored.  If alignment is positive, padding whose size is multiple number of the alignment
# is placed.  If alignment is negative, as `vsiz' is the size of a value, the size of padding
# is calculated with `(vsiz / pow(2, abs(align) - 1))'.  Because alignment setting is not
# saved in a database, you should specify alignment every opening a database.
#
sub setalign {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    my($align) = shift;
    (defined($align)) || ($align = 0);
    (scalar(@_) == 0) || return FALSE;
    my($curia) = $handles{$$self[0]};
    my($rv) = plcrsetalign($curia, $align);
    $errmsg = plcrerrmsg();
    return $rv;
}


##
# $bool = $curia->setfbpsiz($size);
# Method: Set the size of the free block pool.
# `$size' specifies the size of the free block pool.  If it is undef, the free block pool is not
# used.
# If successful, the return value is true, else, it is false.
# The default size of the free block pool is 16.  If the size is greater, the space efficiency of
# overwriting values is improved with the time efficiency sacrificed.
#
sub setfbpsiz {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    my($size) = shift;
    (defined($size)) || ($size = 0);
    (scalar(@_) == 0) || return FALSE;
    my($curia) = $handles{$$self[0]};
    my($rv) = plcrsetfbpsiz($curia, $size);
    $errmsg = plcrerrmsg();
    return $rv;
}


##
# $bool = $curia->sync();
# Method: Synchronize updating contents with the files and the devices.
# If successful, the return value is true, else, it is false.
# This method is useful when another process uses the connected database directory.
#
sub sync {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($curia) = $handles{$$self[0]};
    my($rv) = plcrsync($curia);
    $errmsg = plcrerrmsg();
    return $rv;
}


##
# $bool = $curia->optimize($bnum);
# Method: Optimize the database.
# `$bnum' specifies the number of the elements of the bucket array.  If it is undef or not more
# than 0, the default value is specified.
# If successful, the return value is true, else, it is false.
# In an alternating succession of deleting and storing with overwrite or concatenate,
# dispensable regions accumulate.  This method is useful to do away with them.
#
sub optimize {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    my($bnum) = shift;
    (defined($bnum)) || ($bnum = -1);
    (scalar(@_) == 0) || return FALSE;
    my($curia) = $handles{$$self[0]};
    my($rv) = plcroptimize($curia, $bnum);
    $errmsg = plcrerrmsg();
    return $rv;
}


##
# $num = $curia->fsiz();
# Method: Get the total size of the database files.
# If successful, the return value is the total size of the database files, else, it is -1.
# If the total size is more than 2GB, the return value overflows.
#
sub fsiz {
    my($self) = shift;
    ($$self[1]) || return -1;
    (scalar(@_) == 0) || return -1;
    my($curia) = $handles{$$self[0]};
    my($rv) = plcrfsiz($curia);
    $errmsg = plcrerrmsg();
    return $rv;
}


##
# $num = $curia->bnum();
# Method: Get the total number of the elements of each bucket array.
# If successful, the return value is the total number of the elements of each bucket array,
# else, it is -1.
#
sub bnum {
    my($self) = shift;
    ($$self[1]) || return -1;
    (scalar(@_) == 0) || return -1;
    my($curia) = $handles{$$self[0]};
    my($rv) = plcrbnum($curia);
    $errmsg = plcrerrmsg();
    return $rv;
}


##
# $num = $curia->rnum();
# Method: Get the number of the records stored in the database.
# If successful, the return value is the number of the records stored in the database, else,
# it is -1.
#
sub rnum {
    my($self) = shift;
    ($$self[1]) || return -1;
    (scalar(@_) == 0) || return -1;
    my($curia) = $handles{$$self[0]};
    my($rv) = plcrrnum($curia);
    $errmsg = plcrerrmsg();
    return $rv;
}


##
# $bool = $curia->writable();
# Method: Check whether the database handle is a writer or not.
# The return value is true if the handle is a writer, false if not.
#
sub writable {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($curia) = $handles{$$self[0]};
    my($rv) = plcrwritable($curia);
    $errmsg = plcrerrmsg();
    return $rv;
}


##
# $bool = $curia->fatalerror();
# Method: Check whether the database has a fatal error or not.
# The return value is true if the database has a fatal error, false if not.
#
sub fatalerror {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($curia) = $handles{$$self[0]};
    my($rv) = plcrfatalerror($curia);
    $errmsg = plcrerrmsg();
    return $rv;
}


##
#: Called automatically by the garbage collector.
# Destructor: DESTROY: Release the resources.
# If the database handle is not closed yet, it is closed.
#
sub DESTROY {
    my($self) = shift;
    $self->close();
}


##
# $curia = tie(%hash, "Curia", $name, $omode, $bnum, $dnum);
# Tying Function: TIEHASH: Get the database handle.
#
sub TIEHASH {
    my($class, $name, $omode, $bnum, $dnum) = @_;
    (defined($name)) || return undef;
    (defined($omode)) || ($omode = OWRITER | OCREAT);
    (defined($bnum)) || ($bnum = -1);
    (defined($dnum)) || ($dnum = -1);
    return $class->new($name, $omode, $bnum, $dnum);
}


##
# $bool = ($hash{$key} = $val);
# Tying Function: STORE: Store a record with overwrite.
#
sub STORE {
    my($self, $key, $val) = @_;
    ($$self[1]) || return FALSE;
    (defined($key) && defined($val)) || return FALSE;
    my($curia) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    if($$self[3]){
        local($_) = $val;
        $$self[3]();
        $val = $_;
    }
    my($rv) = plcrput($curia, $key, length($key), $val, length($val), DOVER);
    ($rv == 0) && ($errmsg = plcrerrmsg());
    return $rv;
}


##
# $bool = delete($hash{$key});
# Tying Function: DELETE: Delete a record.
#
sub DELETE {
    my($self, $key) = @_;
    ($$self[1]) || return FALSE;
    (defined($key)) || return FALSE;
    my($curia) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    my($rv) = plcrout($curia, $key, length($key));
    $errmsg = plcrerrmsg();
    return $rv;
}


##
# $bool = (%hash = ());
# Tying Function: CLEAR: Delete all records.
#
sub CLEAR {
    my($self) = shift;
    ($self->iterinit()) || return FALSE;
    my($key);
    while(defined($key = $self->iternext())){
        ($self->out($key)) || return FALSE;
    }
    return TRUE;
}


##
# $str = $hash{$key};
# Tying Function: FETCH: Retrieve whole value of a record.
#
sub FETCH {
    my($self, $key) = @_;
    ($$self[1]) || return undef;
    (defined($key)) || return undef;
    my($curia) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    my($rv) = plcrget($curia, $key, length($key), 0, -1);
    $errmsg = plcrerrmsg();
    if($rv && $$self[5]){
        local($_) = $rv;
        $$self[5]();
        $rv = $_;
    }
    return $rv;
}


##
# $bool = exists($hash{$val});
# Tying Function: EXISTS: Check whether a record exists or not.
#
sub EXISTS {
    my($self) = shift;
    my($key) = shift;
    return $self->vsiz($key) >= 0 ? TRUE : FALSE;
}


##
#: Called automatically by keys(), each(), and so on.
# Tying Function: FIRSTKEY: Get the first key.
#
sub FIRSTKEY {
    my($self) = shift;
    ($self->iterinit()) || return undef;
    return $self->iternext();
}


##
#: Called automatically by keys(), each(), and so on.
# Tying Function: NEXTKEY: Get the next key.
#
sub NEXTKEY {
    my($self) = shift;
    return $self->iternext();
}


##
# $func = $curia->filter_store_key(\&nf);
# Method: set a filter invoked when writing a key.
# `\&nf' specifies the reference of a filter function proofing `$_'.  If it is undef, the
# current filter function is cleared.
# The return value is the old filter function.
#
sub filter_store_key {
    my($self) = shift;
    my($nf) = shift;
    my($of) = $$self[2];
    $$self[2] = $nf;
    return $of;
}


##
# $func = $curia->filter_store_value(\&nf);
# Method: set a filter invoked when writing a value.
# `\&nf' specifies the reference of a filter function proofing `$_'.  If it is undef, the
# current filter function is cleared.
# The return value is the old filter function.
#
sub filter_store_value {
    my($self) = shift;
    my($nf) = shift;
    my($of) = $$self[3];
    $$self[3] = $nf;
    return $of;
}


##
# $func = $curia->filter_fetch_key(\&nf);
# Method: set a filter invoked when reading a key.
# `\&nf' specifies the reference of a filter function proofing `$_'.  If it is undef, the
# current filter function is cleared.
# The return value is the old filter function.
#
sub filter_fetch_key {
    my($self) = shift;
    my($nf) = shift;
    my($of) = $$self[4];
    $$self[4] = $nf;
    return $of;
}


##
# $func = $curia->filter_fetch_value(\&nf);
# Method: set a filter invoked when reading a value.
# `\&nf' specifies the reference a filter function proofing `$_'.  If it is undef, the
# current filter function is cleared.
# The return value is the old filter function.
#
sub filter_fetch_value {
    my($self) = shift;
    my($nf) = shift;
    my($of) = $$self[5];
    $$self[5] = $nf;
    return $of;
}



TRUE;                                    # return success code


# END OF FILE
