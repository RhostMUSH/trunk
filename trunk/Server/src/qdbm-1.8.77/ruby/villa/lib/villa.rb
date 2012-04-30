#=================================================================================================
# Ruby API of Villa, the advanced API of QDBM
#                                                       Copyright (C) 2000-2006 Mikio Hirabayashi
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


require 'mod_villa'
require 'thread'


##
# require 'villa'
# The library `villa' should be included in application codes.
# An instance of the class `Villa' is used as a database handle.
# `Villa' performs Mix-in of `Enumerable'.
# Each method of `Villa' throws an exception of `Villa::EANY' or its sub classes when an error
# occurs: `Villa::ENOERR', `Villa::EFATAL', `Villa::EMODE', `Villa::EBROKEN', `Villa::EKEEP',
# `Villa::ENOITEM', `Villa::EALLOC', `Villa::EMAP', `Villa::EOPEN', `Villa::ECLOSE',
# `Villa::ETRUNC', `Villa::ESYNC', `Villa::ESTAT', `Villa::ESEEK', `Villa::EREAD',
# `Villa::EWRITE', `Villa::ELOCK', `Villa::EUNLINK', `Villa::EMKDIR', `Villa::ERMDIR' and
# `Villa::EMISC'.
#
class Villa
  include Mod_Villa, Enumerable
  #----------------------------------------------------------------
  # class constants
  #----------------------------------------------------------------
  MyMutex = Mutex::new()
  #----------------------------------------------------------------
  # class methods
  #----------------------------------------------------------------
  public
  ##
  # villa = Villa::new(name, omode, cmode)
  # Constructor: Get a database handle.
  # `name' specifies the name of a database file.
  # `omode' specifies the connection mode: `Villa::OWRITER' as a writer, `Villa::OREADER' as a
  # reader.  If the mode is `Villa::OWRITER', the following may be added by bitwise or:
  # `Villa::OCREAT', which means it creates a new database if not exist, `Villa::OTRUNC', which
  # means it creates a new database regardless if one exists, `Villa::OZCOMP', which means
  # leaves in the database are compressed with ZLIB, `Villa::OYCOMP', which means leaves in the
  # database are compressed with LZO, `Villa::OXCOMP', which means leaves in the database are
  # compressed with BZIP2.  Both of `Villa::OREADER' and `Villa::OWRITER' can be added to by
  # bitwise or: `Villa::ONOLCK', which means it opens a database file without file locking, or
  # `Villa::OLCKNB', which means locking is performed without blocking.  If it is omitted,
  # `Villa::OREADER' is specified.
  # `cmode' specifies the comparing function: `Villa::CMPLEX' comparing keys in lexical order,
  # `Villa::CMPDEC' comparing keys as decimal strings, `Villa::CMPOBJ' comparing keys as
  # serialized objects implementing the method `<=>'.  The comparing function should be kept
  # same in the life of a database.  If `Villa::CMPOBJ' is used, such methods as `put', `out',
  # `get' and so on can treat not only instances of `String' but also any serializable and
  # comparable object.
  # If so, objects being stored are serialized and objects being retrieved are deserialized.
  # The return value is the database handle.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  # If a block parameter is given, this method works as an iterator.  A database handle is
  # opened and passed via the first argument of the block.  The database handle is surely
  # closed when the block is over.
  # While connecting as a writer, an exclusive lock is invoked to the database file.
  # While connecting as a reader, a shared lock is invoked to the database file.  The thread
  # blocks until the lock is achieved.  `Villa::OZCOMP', `Villa::OYCOMP', and `Villa::OXCOMP'
  # are available only if QDBM was built each with ZLIB, LZO, and BZIP2 enabled.  If
  # `Villa::ONOLCK' is used, the application is responsible for exclusion control.
  #
  #@ DEFINED IMPLICITLY
  ##
  # villa = Villa::open(name, omode, cmode)
  # Constructor: An alias of `new'.
  #
  #@ DEFINED OUTSIDE
  #----------------------------------------------------------------
  # private methods
  #----------------------------------------------------------------
  private
  #=
  # initialize(name, omode, cmode)
  # Method: Called implicitly by the constructor.
  #
  def initialize(name, omode = OREADER, cmode = CMPLEX)
    @silent = false
    MyMutex.synchronize() do
      @index = mod_open(name, omode, cmode)
      @name = name
      @cmode = cmode
      @tranmutex = Mutex::new()
    end
    if(iterator?)
      begin
        yield(self)
      ensure
        close()
      end
    end
    self
  end
  #=
  # clone()
  # Method: Forbidden to use.
  #
  def clone
    raise(VillaError)
  end
  #=
  # dup()
  # Method: Forbidden to use.
  #
  alias dup clone
  #----------------------------------------------------------------
  # public methods
  #----------------------------------------------------------------
  public
  ##
  # bool = villa.close()
  # Method: Close the database handle.
  # The return value is always true.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  # Because the region of a closed handle is released, it becomes impossible to use the handle.
  # Updating a database is assured to be written when the handle is closed.  If a writer opens
  # a database but does not close it appropriately, the database will be broken.  If the
  # transaction is activated and not committed, it is aborted.
  #
  def close()
    MyMutex.synchronize() do
      begin
        mod_close(@index)
      ensure
        @index = -1
      end
    end
  end
  ##
  # villa.silent = bool
  # Method: Set the flag whether to repress frequent exceptions.
  # The return value is the assigned value.
  #
  def silent=(value)
    @silent = value ? true : false
    mod_setsilent(@index, silent ? 1 : 0)
    @silent
  end
  def silent
    @silent
  end
  ##
  # bool = villa.put(key, val, dmode)
  # Method: Store a record.
  # `key' specifies a key.  Although it must be an instance of String, binary data is okey.
  # `val' specifies a value.  Although it must be an instance of String, binary data is okey.
  # `dmode' specifies behavior when the key overlaps, by the following values: `Villa::DOVER',
  # which means the specified value overwrites the existing one, `Villa::DKEEP', which means the
  # existing value is kept, `Villa::DCAT', which means the specified value is concatenated at the
  # end of the existing value, `Villa::DDUP', which means duplication of keys is allowed and the
  # specified value is added as the last one, `Villa::DDUPR', which means duplication of keys is
  # allowed and the specified value is added as the first one.  If it is omitted, `Villa::DOVER'
  # is specified.
  # The return value is always true.  However, if the silent flag is true and replace is
  # cancelled, false is returned instead of exception.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs or replace
  # is cancelled.
  # The cursor becomes unavailable due to updating database.
  #
  def put(key, val, dmode = DOVER)
    if(@cmode == CMPOBJ)
      key = Marshal.dump(key)
      val = Marshal.dump(val)
    end
    mod_put(@index, key, val, dmode)
  end
  ##
  # bool = villa.store(key, val)
  # Method: An alias of `put'.
  #
  alias store put
  ##
  # villa[key] = val
  # Method: An alias of `put'.
  #
  alias []= put
  ##
  # bool = villa.out(key)
  # Method: Delete a record.
  # `key' specifies a key.  Although it must be an instance of String, binary data is okey.
  # The return value is always true.  However, if the silent flag is true and no record
  # corresponds, false is returned instead of exception.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs or no record
  # corresponds.
  # When the key of duplicated records is specified, the value of the first record of the same
  # key is selected.  The cursor becomes unavailable due to updating database.
  #
  def out(key)
    if(@cmode == CMPOBJ)
      key = Marshal.dump(key)
    end
    mod_out(@index, key)
  end
  ##
  # bool = villa.delete(key)
  # Method: An alias of `out'.
  #
  alias delete out
  ##
  # bool = villa.clear()
  # Method: Delete all records.
  # The return value is always true.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  #
  def clear
    MyMutex.synchronize() do
      begin
        while(rnum() > 0)
          curfirst();
          out(curkey())
        end
      rescue ENOITEM
      end
    end
    true
  end
  ##
  # str = villa.get(key)
  # Method: Retrieve a record.
  # `key' specifies a key.  Although it must be an instance of String, binary data is okey.
  # The return value is an instance of the value of the corresponding record.  If the silent flag
  # is true and no record corresponds, nil is returned instead of exception.
  # When the key of duplicated records is specified, the first record of the same key is
  # deleted.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs or no record
  # corresponds.
  #
  def get(key)
    if(@cmode == CMPOBJ)
      key = Marshal.dump(key)
    end
    val = mod_get(@index, key)
    if(@cmode == CMPOBJ && val)
      val = Marshal.load(val)
    end
    val
  end
  ##
  # str = villa.fetch(key, defval)
  # Method: Retrieve a record.
  # `key' specifies a key.  Although it must be an instance of String, binary data is okey.
  # `defval' specifies the default value used when no record corresponds.  If it is omitted, nil
  # is specified.
  # The return value is an instance of the value of the corresponding record, or the default
  # value if no record corresponds.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  #
  def fetch(key, defval = nil)
    if @silent
      if val = mod_get(@index, key)
        if(@cmode == CMPOBJ)
          val = Marshal.load(val)
        end
        val
      else
        defval
      end
    else
      begin
        val = mod_get(@index, key)
        if(@cmode == CMPOBJ)
          val = Marshal.load(val)
        end
        val
      rescue ENOITEM
        defval
      end
    end
  end
  ##
  # str = villa[key]
  # Method: An alias of `fetch'.
  #
  alias [] fetch
  ##
  # num = villa.vsiz(key)
  # Method: Get the size of the value of a record.
  # `key' specifies a key.  Although it must be an instance of String, binary data is okey.
  # The return value is the size of the value of the corresponding record.  If the silent flag
  # is true and no record corresponds, -1 is returned instead of exception.  If multiple records
  # correspond, the size of the first is returned.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  #
  def vsiz(key)
    if(@cmode == CMPOBJ)
      key = Marshal.dump(key)
    end
    mod_vsiz(@index, key)
  end
  ##
  # num = villa.vnum(key)
  # Method: Get the number of records corresponding a key.
  # `key' specifies a key.  Although it must be an instance of String, binary data is okey.
  # The return value is the size of the value of the corresponding record.  If no record
  # corresponds, 0 is returned.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  #
  def vnum(key)
    if(@cmode == CMPOBJ)
      key = Marshal.dump(key)
    end
    mod_vnum(@index, key)
  end
  ##
  # bool = villa.curfirst()
  # Method: Move the cursor to the first record.
  # The return value is always true.  However, if the silent flag is true and no record
  # corresponds, false is returned instead of exception.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs or there is
  # no record in the database.
  #
  def curfirst()
    mod_curfirst(@index)
  end
  ##
  # bool = villa.curlast()
  # Method: Move the cursor to the last record.
  # The return value is always true.  However, if the silent flag is true and no record
  # corresponds, false is returned instead of exception.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs or there is
  # no record in the database.
  #
  def curlast()
    mod_curlast(@index)
  end
  ##
  # bool = villa.curprev()
  # Method: Move the cursor to the previous record.
  # The return value is always true.  However, if the silent flag is true and no record
  # corresponds, false is returned instead of exception.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs or there is
  # no previous record.
  #
  def curprev()
    mod_curprev(@index)
  end
  ##
  # bool = villa.curnext()
  # Method: Move the cursor to the next record.
  # The return value is always true.  However, if the silent flag is true and no record
  # corresponds, false is returned instead of exception.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs or there is
  # no next record.
  #
  def curnext()
    mod_curnext(@index)
  end
  ##
  # bool = villa.curjump(key, jmode)
  # Method: Move the cursor to a position around a record.
  # `key' specifies a key.  Although it must be an instance of String, binary data is okey.
  # `jmode' specifies detail adjustment: `Villa::JFORWARD', which means that the cursor is set
  # to the first record of the same key and that the cursor is set to the next substitute if
  # completely matching record does not exist, `Villa::JBACKWARD', which means that the cursor
  # is set to the last record of the same key and that the cursor is set to the previous
  # substitute if completely matching record does not exist.  If it is omitted, `Villa::JFORWARD'
  # is specified.
  # The return value is always true.  However, if the silent flag is true and no record
  # corresponds, false is returned instead of exception.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs or there is
  # no record corresponding the condition.
  #
  def curjump(key, jmode = JFORWARD)
    if(@cmode == CMPOBJ)
      key = Marshal.dump(key)
    end
    mod_curjump(@index, key, jmode)
  end
  ##
  # str = villa.curkey()
  # Method: Get the key of the record where the cursor is.
  # The return value is the key of the corresponding record.  If the silent flag is true and no
  # record corresponds, nil is returned instead of exception.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs or no record
  # corresponds to the cursor.
  #
  def curkey()
    key = mod_curkey(@index)
    if(@cmode == CMPOBJ && key)
      key = Marshal.load(key)
    end
    key
  end
  ##
  # str = villa.curval()
  # Method: Get the value of the record where the cursor is.
  # The return value is the value of the corresponding record.  If the silent flag is true and no
  # record corresponds, nil is returned instead of exception.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs or no record
  # corresponds to the cursor.
  #
  def curval()
    val = mod_curval(@index)
    if(@cmode == CMPOBJ && val)
      val = Marshal.load(val)
    end
    val
  end
  ##
  # bool = villa.curput(val, cpmode);
  # Method: Insert a record around the cursor.
  # `val' specifies a value.  Although it must be an instance of String, binary data is okey.
  # `cpmode' specifies detail adjustment: `Villa::CPCURRENT', which means that the value of the
  # current record is overwritten, `Villa::CPBEFORE', which means that a new record is inserted
  # before the current record, `Villa::CPAFTER', which means that a new record is inserted after
  # the current record.  If it is omitted, `Villa::CPCURRENT' is specified.
  # The return value is always true.  However, if the silent flag is true and no record
  # corresponds to the cursor, false is returned instead of exception.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs or no record
  # corresponds to the cursor.
  # After insertion, the cursor is moved to the inserted record.
  #
  def curput(val, cpmode = CPCURRENT)
    if(@cmode == CMPOBJ)
      val = Marshal.dump(val)
    end
    mod_curput(@index, val, cpmode)
  end
  ##
  # bool = villa.curout();
  # Method: Delete the record where the cursor is.
  # The return value is always true.  However, if the silent flag is true and no record
  # corresponds to the cursor, false is returned instead of exception.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs or no record
  # corresponds to the cursor.
  # After deletion, the cursor is moved to the next record if possible.
  #
  def curout()
    mod_curout(@index)
  end
  ##
  # bool = villa.settuning(lrecmax, nidxmax, lcnum, ncnum)
  # Method: Set the tuning parameters for performance.
  # `lrecmax' specifies the max number of records in a leaf node of B+ tree.  If it is not more
  # than 0, the default value is specified.
  # `nidxmax' specifies the max number of indexes in a non-leaf node of B+ tree.  If it is not
  # more than 0, the default value is specified.
  # `lcnum' specifies the max number of caching leaf nodes.  If it is not more than 0, the
  # default value is specified.
  # `ncnum' specifies the max number of caching non-leaf nodes.  If it is not more than 0, the
  # default value is specified.
  # The return value is always true.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  # The default setting is equivalent to `vlsettuning(49, 192, 1024, 512)'.  Because tuning
  # parameters are not saved in a database, you should specify them every opening a database.
  #
  def settuning(lrecmax, nidxmax, lcnum, ncnum)
    mod_settuning(@index, lrecmax, nidxmax, lcnum, ncnum)
  end
  ##
  # bool = villa.sync()
  # Method: Synchronize updating contents with the file and the device.
  # The return value is always true.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  # This method is useful when another process uses the connected database file.  This method
  # should not be used while the transaction is activated.
  #
  def sync()
    mod_sync(@index)
  end
  ##
  # bool = villa.optimize()
  # Method: Optimize the database.
  # The return value is always true.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  # In an alternating succession of deleting and storing with overwrite or concatenate,
  # dispensable regions accumulate.  This method is useful to do away with them.  This method
  # should not be used while the transaction is activated.
  #
  def optimize()
    mod_optimize(@index)
  end
  ##
  # num = villa.fsiz()
  # Method: Get the size of the database file.
  # The return value is the size of the database file.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  # Because of the I/O buffer, the return value may be less than the real size.
  #
  def fsiz()
    mod_fsiz(@index)
  end
  ##
  # num = villa.rnum()
  # Method: Get the number of the records stored in the database.
  # The return value is the number of the records stored in the database.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  #
  def rnum()
    mod_rnum(@index)
  end
  ##
  # num = villa.length()
  # Method: An alias of `rnum'.
  #
  alias length rnum
  ##
  # num = villa.size()
  # Method: An alias of `rnum'.
  #
  alias size rnum
  ##
  # bool = villa.writable()
  # Method: Check whether the database handle is a writer or not.
  # The return value is true if the handle is a writer, false if not.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  #
  def writable()
    mod_writable(@index)
  end
  ##
  # bool = villa.fatalerror()
  # Method: Check whether the database has a fatal error or not.
  # The return value is true if the database has a fatal error, false if not.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  #
  def fatalerror()
    mod_fatalerror(@index)
  end
  ##
  # bool = villa.tranbegin()
  # Method: Begin the transaction.
  # The return value is always true.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  # If a thread is already in the transaction, the other threads block until the prius is out
  # of the transaction.  Only one transaction can be activated with a database handle at the
  # same time.
  #
  def tranbegin()
    @tranmutex.lock()
    MyMutex.synchronize() do
      begin
        mod_tranbegin(@index)
      rescue
        @tranmutex.unlock()
        raise()
      end
    end
  end
  ##
  # bool = villa.trancommit()
  # Method: Commit the transaction.
  # The return value is always true.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  # Updating a database in the transaction is fixed when it is committed successfully.  Any
  # other thread except for the one which began the transaction should not call this method.
  #
  def trancommit()
    begin
      mod_trancommit(@index)
    ensure
      @tranmutex.unlock()
    end
  end
  ##
  # bool = villa.tranabort()
  # Method: Abort the transaction.
  # The return value is always true.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  # Updating a database in the transaction is discarded when it is aborted.  The state of the
  # database is rollbacked to before transaction.  Any other thread except for the one which
  # began the transaction should not call this method.
  #
  def tranabort()
    begin
      mod_tranabort(@index)
    ensure
      @tranmutex.unlock()
    end
  end
  ##
  # villa.transaction() do ... end
  # Iterator Method: Perform an iterator block in the transaction.
  # The specified iterator block is performed in the transaction.  If the block returns true,
  # the transaction is committed.  If the block returns false or raises any exception, the
  # transaction is aborted.
  #
  def transaction()
    tranbegin()
    begin
      cmt = yield()
    rescue
      cmt = false
      raise()
    ensure
      if(cmt)
        trancommit()
      else
        tranabort()
      end
    end
  end
  ##
  # villa.each() do |key, val| ... end
  # Iterator Method: Iterate a process with a pair of a key and a value of each record.
  #
  def each()
    MyMutex.synchronize() do
      curfirst()
      while(true)
        begin
          break unless key = curkey()
          val = curval()
          yield(key, val)
          curnext()
        rescue ENOITEM
          break
        end
      end
      curfirst()
    end
    self
  end
  ##
  # villa.each_pair() do |key, val| ... end
  # Iterator Method: An alias of `each'.
  #
  alias each_pair each
  ##
  # villa.each_key() do |key| ... end
  # Iterator Method: Iterate a process with a key of each record.
  #
  def each_key()
    MyMutex.synchronize() do
      curfirst()
      while(true)
        begin
          break unless key = curkey()
          curnext()
          yield(key)
        rescue ENOITEM
          break
        end
      end
      curfirst()
    end
    self
  end
  ##
  # villa.each_value() do |val| ... end
  # Iterator Method: Iterate a process with a value of each record.
  #
  def each_value()
    MyMutex.synchronize() do
      curfirst()
      while(true)
        begin
          break unless val = curval()
          curnext()
          yield(val)
        rescue ENOITEM
          break
        end
      end
      curfirst()
    end
    self
  end
  ##
  # ary = villa.keys()
  # Method: Get an array of all keys.
  # The return value is an array of all keys.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  #
  def keys()
    ary = Array::new(rnum())
    MyMutex.synchronize() do
      begin
        curfirst()
        0.upto(ary.length - 1) do |i|
          ary[i] = curkey()
          curnext()
        end
        curfirst()
      rescue ENOITEM
      end
    end
    ary
  end
  ##
  # ary = villa.values()
  # Method: Get an array of all values.
  # The return value is an array of all values.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs.
  #
  def values()
    ary = Array::new(rnum())
    MyMutex.synchronize() do
      begin
        curfirst()
        0.upto(ary.length - 1) do |i|
          ary[i] = curval()
          curnext()
        end
        curfirst()
      rescue ENOITEM
      end
    end
    ary
  end
  ##
  # str = villa.index(val)
  # Method: Retrieve a record with a value.
  # `val' specifies a value.  Although it must be an instance of String, binary data is okey.
  # The return value is the key of the record with the specified value.
  # An exception of `Villa::EANY' or its sub classes is thrown if an error occurs or no record
  # corresponds.
  # If two or more records correspond, the first found record is selected.
  #
  def index(val)
    key = nil
    MyMutex.synchronize() do
      curfirst()
      while(true)
        break unless tmp = curval()
        if(tmp == val)
          key = curkey()
          break
        end
        curnext()
      end
      curfirst()
    end
    key
  end
  ##
  # num = villa.to_int()
  # Method: An alias of `rnum'.
  #
  alias to_int rnum
  ##
  # num = villa.to_i()
  # Method: An alias of `to_int'.
  #
  alias to_i to_int
  ##
  # str = villa.to_str()
  # Method: Get string standing for the instance.
  #
  def to_str
    if(@index != -1)
      sprintf("#<Villa:%#x:name=%s:state=open:rnum=%d>",
              object_id(), @name, rnum())
    else
      sprintf("#<Villa:%#x:name=%s:state=closed>", object_id(), @name)
    end
  end
  ##
  # str = villa.to_s()
  # Method: An alias of `to_str'.
  #
  alias to_s to_str
  ##
  # ary = villa.to_ary()
  # Method: Get an array of alternation of each pair of a key and a value.
  #
  def to_ary
    ary = Array::new(rnum())
    i = 0
    each() do |key, val|
      ary[i] = [key, val]
      i += 1
    end
    ary
  end
  ##
  # ary = villa.to_a()
  # Method: An alias of `to_ary'.
  #
  alias to_a to_ary
  ##
  # hash = villa.to_hash()
  # Method: Get a hash storing all records.
  #
  def to_hash
    hash = Hash::new()
    each() do |key, val|
      hash[key] || hash[key] = val
    end
    hash
  end
  ##
  # hash = villa.to_h()
  # Method: An alias of `to_hash'.
  #
  alias to_h to_hash
  ##
  # str = villa.inspect()
  # Method: An alias of `to_str'.
  #
  alias inspect to_str
end


#----------------------------------------------------------------
# Alias definition of class methods
#----------------------------------------------------------------
class << Villa
  alias open new
end



# END OF FILE
