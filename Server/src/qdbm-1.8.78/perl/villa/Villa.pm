#=================================================================================================
# Perl API of Villa, the basic API of QDBM
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


package Villa;

use strict;
use warnings;

require Tie::Hash;
require Exporter;
require XSLoader;
our @ISA = qw(Tie::Hash Exporter);
our $VERSION = '1.0';
XSLoader::load('Villa', $VERSION);

use constant TRUE => 1;                  # boolean true
use constant FALSE => 0;                 # boolean false

use constant OREADER => 1 << 0;          # open as a reader
use constant OWRITER => 1 << 1;          # open as a writer
use constant OCREAT => 1 << 2;           # a writer creating
use constant OTRUNC => 1 << 3;           # a writer truncating
use constant ONOLCK => 1 << 4;           # open without locking
use constant OLCKNB => 1 << 5;           # lock without blocking
use constant OZCOMP => 1 << 6;           # compress leaves with ZLIB
use constant OYCOMP => 1 << 7;           # compress leaves with LZO
use constant OXCOMP => 1 << 8;           # compress leaves with BZIP2

use constant CMPLEX => 0;                # compare in lexical order
use constant CMPDEC => 1;                # compare decimal strings

use constant DOVER => 0;                 # overwrite an existing value
use constant DKEEP => 1;                 # keep an existing value
use constant DCAT => 2;                  # concatenate values
use constant DDUP => 3;                  # allow duplication of records
use constant DDUPR => 4;                 # allow duplication with reverse order

use constant JFORWARD => 0;              # step forward
use constant JBACKWARD => 1;             # step backward

use constant CPCURRENT => 0;             # overwrite the current record
use constant CPBEFORE => 1;              # insert before the current record
use constant CPAFTER => 2;               # insert after the current record

my(%handles) = ();                       # table of database names
our($errmsg) = "no error";               # message of the last error



#=================================================================================================
# public objects
#=================================================================================================


##
# use Villa;
# Module `Villa' should be loaded in application codes.
# An instance of the class `Villa' is used as a database handle.
#


##
# $Villa::errmsg;
# Global Variable: The message of the last happened error.
#


##
# $villa = new Villa($name, $omode, $cmode);
# Constructor: Get the database handle.
# `$name' specifies the name of a database file.
# `$omode' specifies the connection mode: `Villa::OWRITER' as a writer, `Villa::OREADER' as a
# reader.  If the mode is `Villa::OWRITER', the following may be added by bitwise or:
# `Villa::OCREAT', which means it creates a new database if not exist, `Villa::OTRUNC', which
# means it creates a new database regardless if one exists, `Villa::OZCOMP', which means leaves
# in the database are compressed with ZLIB, `Villa::OYCOMP', which means leaves in the database
# are compressed with LZO, `Villa::OXCOMP', which means leaves in the database are compressed
# with BZIP2.  Both of `Villa::OREADER' and `Villa::OWRITER' can be added to by bitwise or:
# `Villa::ONOLCK', which means it opens a database file without file locking, or `Villa::OLCKNB',
# which means locking is performed without blocking.  If it is undef, `Villa::OREADER' is
# specified.
# `$cmode' specifies the comparing function: `Villa::CMPLEX' comparing keys in lexical order,
# `Villa::CMPDEC' comparing keys as decimal strings.  The comparing function should be kept
# same in the life of a database.
# The return value is the database handle or undef if it is not successful.
# While connecting as a writer, an exclusive lock is invoked to the database file.
# While connecting as a reader, a shared lock is invoked to the database file.  The thread
# blocks until the lock is achieved.  `Villa::OZCOMP', `Villa::OYCOMP', and `Villa::OXCOMP' are
# available only if QDBM was built each with ZLIB, LZO, and BZIP2 enabled.  If `Villa::ONOLCK'
# is used, the application is responsible for exclusion control.
#
sub new {
    my($class) = shift;
    my($name) = shift;
    my($omode) = shift;
    my($cmode) = shift;
    (defined($name) && length($name) > 0 && scalar(@_) == 0) || return undef;
    (!$handles{$name}) || return undef;
    (defined($omode)) || ($omode = OREADER);
    (defined($cmode)) || ($cmode = CMPLEX);
    my($villa) = plvlopen($name, $omode, $cmode);
    $errmsg = plvlerrmsg();
    ($villa > 0) || return undef;
    $handles{$name} = $villa;
    my $self = [$name, TRUE, undef, undef, undef, undef];
    bless($self, $class);
    return $self;
}


##
# $bool = $villa->close();
# Method: Close the database handle.
# If successful, the return value is true, else, it is false.
# Because the region of a closed handle is released, it becomes impossible to use the handle.
# Updating a database is assured to be written when the handle is closed.  If a writer opens
# a database but does not close it appropriately, the database will be broken.  If the
# transaction is activated and not committed, it is aborted.
#
sub close {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    $$self[1] = FALSE;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvlclose($villa);
    $errmsg = plvlerrmsg();
    delete($handles{$$self[0]});
    return $rv;
}


##
# $bool = $villa->put($key, $val, $dmode);
# Method: Store a record.
# `$key' specifies a key.  If it is undef, this method has no effect.
# `$val' specifies a value.  If it is undef, this method has no effect.
# `$dmode' specifies behavior when the key overlaps, by the following values: `Villa::DOVER',
# which means the specified value overwrites the existing one, `Villa::DKEEP', which means the
# existing value is kept, `Villa::DCAT', which means the specified value is concatenated at the
# end of the existing value, `Villa::DDUP', which means duplication of keys is allowed and the
# specified value is added as the last one, `Villa::DDUPR', which means duplication of keys is
# allowed and the specified value is added as the first one.  If it is undef, `Villa::DOVER' is
# specified.
# If successful, the return value is true, else, it is false.
# The cursor becomes unavailable due to updating database.
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
    my($villa) = $handles{$$self[0]};
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
    my($rv) = plvlput($villa, $key, length($key), $val, length($val), $dmode);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->out($key);
# Method: Delete a record.
# `$key' specifies a key.  If it is undef, this method has no effect.
# If successful, the return value is true, else, it is false.  False is returned when no
# record corresponds to the specified key.
# When the key of duplicated records is specified, the first record of the same key is deleted.
# The cursor becomes unavailable due to updating database.
#
sub out {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    my($key) = shift;
    (scalar(@_) == 0) || return FALSE;
    (defined($key)) || return FALSE;
    my($villa) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    my($rv) = plvlout($villa, $key, length($key));
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $str = $villa->get($key);
# Method: Retrieve a record.
# `$key' specifies a key.  If it is undef, this method has no effect.
# If successful, the return value is a scalar of the value of the corresponding record, else, it
# is undef.  undef is returned when no record corresponds to the specified key.
# When the key of duplicated records is specified, the value of the first record of the same key
# is selected.
#
sub get {
    my($self) = shift;
    ($$self[1]) || return undef;
    my($key) = shift;
    (scalar(@_) == 0) || return undef;
    (defined($key)) || return undef;
    my($villa) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    my($rv) = plvlget($villa, $key, length($key));
    $errmsg = plvlerrmsg();
    if($rv && $$self[5]){
        local($_) = $rv;
        $$self[5]();
        $rv = $_;
    }
    return $rv;
}


##
# $num = $villa->vsiz($key);
# Method: Get the size of the value of a record.
# `$key' specifies a key.  If it is undef, this method has no effect.
# The return value is the size of the value of the corresponding record.  If no record
# corresponds, -1 is returned.  If multiple records correspond, the size of the first is
# returned.
#
sub vsiz {
    my($self) = shift;
    ($$self[1]) || return 0;
    my($key) = shift;
    (scalar(@_) == 0) || return 0;
    (defined($key)) || return 0;
    my($villa) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    my($rv) = plvlvsiz($villa, $key, length($key));
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $num = $villa->vnum($key);
# Method: Get the number of records corresponding a key.
# `$key' specifies a key.  If it is undef, this method has no effect.
# If successful, the return value is the size of the value of the corresponding record, else,
# it is 0.
#
sub vnum {
    my($self) = shift;
    ($$self[1]) || return 0;
    my($key) = shift;
    (scalar(@_) == 0) || return 0;
    (defined($key)) || return 0;
    my($villa) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    my($rv) = plvlvnum($villa, $key, length($key));
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->curfirst();
# Method: Move the cursor to the first record.
# If successful, the return value is true, else, it is false.  False is returned if there is
# no record in the database.
#
sub curfirst {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvlcurfirst($villa);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->curlast();
# Method: Move the cursor to the last record.
# If successful, the return value is true, else, it is false.  False is returned if there is
# no record in the database.
#
sub curlast {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvlcurlast($villa);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->curprev();
# Method: Move the cursor to the previous record.
# If successful, the return value is true, else, it is false.  False is returned if there is
# no previous record.
#
sub curprev {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvlcurprev($villa);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->curnext();
# Method: Move the cursor to the next record.
# If successful, the return value is true, else, it is false.  False is returned if there is
# no next record.
#
sub curnext {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvlcurnext($villa);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->curjump($key, $jmode);
# Method: Move the cursor to a position around a record.
# `$key' specifies a key.  If it is undef, this method has no effect.
# `$jmode' specifies detail adjustment: `Villa::JFORWARD', which means that the cursor is set
# to the first record of the same key and that the cursor is set to the next substitute if
# completely matching record does not exist, `Villa::JBACKWARD', which means that the cursor
# is set to the last record of the same key and that the cursor is set to the previous
# substitute if completely matching record does not exist.  If it is undef, `Villa::JFORWARD'
# is specified.
# If successful, the return value is true, else, it is false.  False is returned if there is
# no record corresponding the condition.
#
sub curjump {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    my($key) = shift;
    my($jmode) = shift;
    (scalar(@_) == 0) || return FALSE;
    (defined($key)) || return FALSE;
    (defined($jmode)) || ($jmode = JFORWARD);
    my($villa) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    my($rv) = plvlcurjump($villa, $key, length($key), $jmode);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $str = $villa->curkey();
# Method: Get the key of the record where the cursor is.
# If successful, the return value is a scalar of the key of the corresponding record, else, it
# is undef.  undef is returned when no record corresponds to the cursor.
#
sub curkey {
    my($self) = shift;
    ($$self[1]) || return undef;
    (scalar(@_) == 0) || return undef;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvlcurkey($villa);
    $errmsg = plvlerrmsg();
    if($rv && $$self[4]){
        local($_) = $rv;
        $$self[4]();
        $rv = $_;
    }
    return $rv;
}


##
# $str = $villa->curval();
# Method: Get the value of the record where the cursor is.
# If successful, the return value is a scalar of the value of the corresponding record, else, it
# is undef.  undef is returned when no record corresponds to the cursor.
#
sub curval {
    my($self) = shift;
    ($$self[1]) || return undef;
    (scalar(@_) == 0) || return undef;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvlcurval($villa);
    $errmsg = plvlerrmsg();
    if($rv && $$self[5]){
        local($_) = $rv;
        $$self[5]();
        $rv = $_;
    }
    return $rv;
}


##
# $bool = $villa->curput($val, $cpmode);
# Method: Insert a record around the cursor.
# `$val' specifies a value.  If it is undef, this method has no effect.
# `$cpmode' specifies detail adjustment: `Villa::CPCURRENT', which means that the value of the
# current record is overwritten, `Villa::CPBEFORE', which means that a new record is inserted
# before the current record, `Villa::CPAFTER', which means that a new record is inserted after
# the current record.  If it is undef, `Villa::CPCURRENT' is specified.
# If successful, the return value is true, else, it is false.  False is returned when no record
# corresponds to the cursor.
# After insertion, the cursor is moved to the inserted record.
#
sub curput {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    my($val) = shift;
    my($cpmode) = shift;
    (scalar(@_) == 0) || return FALSE;
    (defined($val)) || return FALSE;
    (defined($cpmode)) || ($cpmode = CPCURRENT);
    my($villa) = $handles{$$self[0]};
    if($$self[3]){
        local($_) = $val;
        $$self[3]();
        $val = $_;
    }
    my($rv) = plvlcurput($villa, $val, length($val), $cpmode);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->curout();
# Method: Delete the record where the cursor is.
# If successful, the return value is true, else, it is false.  False is returned when no record
# corresponds to the cursor.
# After deletion, the cursor is moved to the next record if possible.
#
sub curout {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvlcurout($villa);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->settuning($lrecmax, $nidxmax, $lcnum, $ncnum);
# Method: Set alignment of the database handle.
# `$lrecmax' specifies the max number of records in a leaf node of B+ tree.  If it is undef or
# not more than 0, the default value is specified.
# `$nidxmax' specifies the max number of indexes in a non-leaf node of B+ tree.  If it is undef
# or not more than 0, the default value is specified.
# `$lcnum' specifies the max number of caching leaf nodes.  If it is undef or not more than 0,
# the default value is specified.
# `$ncnum' specifies the max number of caching non-leaf nodes.  If it is undef or not more than
# 0, the default value is specified.
# If successful, the return value is true, else, it is false.
# The default setting is equivalent to `vlsettuning(49, 192, 1024, 512)'.  Because tuning
# parameters are not saved in a database, you should specify them every opening a database.
#
sub settuning {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    my($lrecmax) = shift;
    my($nidxmax) = shift;
    my($lcnum) = shift;
    my($ncnum) = shift;
    (defined($lrecmax)) || ($lrecmax = -1);
    (defined($nidxmax)) || ($nidxmax = -1);
    (defined($lcnum)) || ($lcnum = -1);
    (defined($ncnum)) || ($ncnum = -1);
    (scalar(@_) == 0) || return FALSE;
    my($villa) = $handles{$$self[0]};
    plvlsettuning($villa, $lrecmax, $nidxmax, $lcnum, $ncnum);
    return TRUE;
}


##
# $bool = $villa->sync();
# Method: Synchronize updating contents with the file and the device.
# If successful, the return value is true, else, it is false.
# This method is useful when another process uses the connected database file.  This method
# should not be used while the transaction is activated.
#
sub sync {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvlsync($villa);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->optimize();
# Method: Optimize the database.
# If successful, the return value is true, else, it is false.
# In an alternating succession of deleting and storing with overwrite or concatenate,
# dispensable regions accumulate.  This method is useful to do away with them.  This method
# should not be used while the transaction is activated.
#
sub optimize {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvloptimize($villa);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $num = $villa->fsiz();
# Method: Get the size of the database file.
# If successful, the return value is the size of the database file, else, it is -1.
# Because of the I/O buffer, the return value may be less than the real size.
#
sub fsiz {
    my($self) = shift;
    ($$self[1]) || return -1;
    (scalar(@_) == 0) || return -1;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvlfsiz($villa);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $num = $villa->rnum();
# Method: Get the number of the records stored in the database.
# If successful, the return value is the number of the records stored in the database, else,
# it is -1.
#
sub rnum {
    my($self) = shift;
    ($$self[1]) || return -1;
    (scalar(@_) == 0) || return -1;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvlrnum($villa);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->writable();
# Method: Check whether the database handle is a writer or not.
# The return value is true if the handle is a writer, false if not.
#
sub writable {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvlwritable($villa);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->fatalerror();
# Method: Check whether the database has a fatal error or not.
# The return value is true if the database has a fatal error, false if not.
#
sub fatalerror {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvlfatalerror($villa);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->tranbegin();
# Method: Begin the transaction.
# If successful, the return value is true, else, it is false.
# Only one transaction can be activated with a database handle at the same time.
#
sub tranbegin {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvltranbegin($villa);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->trancommit();
# Method: Commit the transaction.
# If successful, the return value is true, else, it is false.
# Updating a database in the transaction is fixed when it is committed successfully.
#
sub trancommit {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvltrancommit($villa);
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = $villa->tranabort();
# Method: Abort the transaction.
# If successful, the return value is true, else, it is false.
# Updating a database in the transaction is discarded when it is aborted.  The state of the
# database is rollbacked to before transaction.
#
sub tranabort {
    my($self) = shift;
    ($$self[1]) || return FALSE;
    (scalar(@_) == 0) || return FALSE;
    my($villa) = $handles{$$self[0]};
    my($rv) = plvltranabort($villa);
    $errmsg = plvlerrmsg();
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
# $villa = tie(%hash, "Villa", $name, $omode, $cmp);
# Tying Function: TIEHASH: Get the database handle.
#
sub TIEHASH {
    my($class, $name, $omode, $cmp) = @_;
    (defined($name)) || return undef;
    (defined($omode)) || ($omode = OWRITER | OCREAT);
    return $class->new($name, $omode, $cmp);
}


##
# $bool = ($hash{$key} = $val);
# Tying Function: STORE: Store a record with overwrite.
#
sub STORE {
    my($self, $key, $val) = @_;
    ($$self[1]) || return FALSE;
    (defined($key) && defined($val)) || return FALSE;
    my($villa) = $handles{$$self[0]};
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
    my($rv) = plvlput($villa, $key, length($key), $val, length($val), DOVER);
    ($rv == 0) && ($errmsg = plvlerrmsg());
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
    my($villa) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    my($rv) = plvlout($villa, $key, length($key));
    $errmsg = plvlerrmsg();
    return $rv;
}


##
# $bool = (%hash = ());
# Tying Function: CLEAR: Delete all records.
#
sub CLEAR {
    my($self) = shift;
    my($key);
    while($self->curfirst()){
        ($key = $self->curkey()) || return FALSE;
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
    my($villa) = $handles{$$self[0]};
    if($$self[2]){
        local($_) = $key;
        $$self[2]();
        $key = $_;
    }
    my($rv) = plvlget($villa, $key, length($key));
    $errmsg = plvlerrmsg();
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
    return $self->vnum($key) > 0 ? TRUE : FALSE;
}


##
#: Called automatically by keys(), each(), and so on.
# Tying Function: FIRSTKEY: Get the first key.
#
sub FIRSTKEY {
    my($self) = shift;
    ($self->curfirst()) || return undef;
    return $self->curkey();
}


##
#: Called automatically by keys(), each(), and so on.
# Tying Function: NEXTKEY: Get the next key.
#
sub NEXTKEY {
    my($self) = shift;
    ($self->curnext()) || return undef;
    return $self->curkey();
}


##
# $func = $villa->filter_store_key(\&nf);
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
# $func = $villa->filter_store_value(\&nf);
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
# $func = $villa->filter_fetch_key(\&nf);
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
# $func = $villa->filter_fetch_value(\&nf);
# Method: set a filter invoked when reading a value.
# `\&nf' specifies the reference of a filter function proofing `$_'.  If it is undef, the
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
