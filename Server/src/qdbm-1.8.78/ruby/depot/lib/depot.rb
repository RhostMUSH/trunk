#=================================================================================================
# Ruby API of Depot, the basic API of QDBM
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


require 'mod_depot'
require 'thread'



##
# require 'depot'
# The library `depot' should be included in application codes.
# An instance of the class `Depot' is used as a database handle.
# `Depot' performs Mix-in of `Enumerable'.
# Each method of `Depot' throws an exception of `Depot::EANY' or its sub classes when an error
# occurs: `Depot::ENOERR', `Depot::EFATAL', `Depot::EMODE', `Depot::EBROKEN', `Depot::EKEEP',
# `Depot::ENOITEM', `Depot::EALLOC', `Depot::EMAP', `Depot::EOPEN', `Depot::ECLOSE',
# `Depot::ETRUNC', `Depot::ESYNC', `Depot::ESTAT', `Depot::ESEEK', `Depot::EREAD',
# `Depot::EWRITE', `Depot::ELOCK', `Depot::EUNLINK', `Depot::EMKDIR', `Depot::ERMDIR' and
# `Depot::EMISC'.
#
class Depot
  include Mod_Depot, Enumerable
  #----------------------------------------------------------------
  # class constants
  #----------------------------------------------------------------
  MyMutex = Mutex::new()
  #----------------------------------------------------------------
  # class methods
  #----------------------------------------------------------------
  public
  ##
  # depot = Depot::new(name, omode, bnum)
  # Constructor: Get a database handle.
  # `name' specifies the name of a database file.
  # `omode' specifies the connection mode: `Depot::OWRITER' as a writer, `Depot::OREADER' as a
  # reader.  If the mode is `Depot::OWRITER', the following may be added by bitwise or:
  # `Depot::OCREAT', which means it creates a new database if not exist, `Depot::OTRUNC', which
  # means it creates a new database regardless if one exists.  Both of `Depot::OREADER' and
  # `Depot::OWRITER' can be added to by bitwise or: `Depot::ONOLCK', which means it opens a
  # database file without file locking, or `Depot::OLCKNB', which means locking is performed
  # without blocking.  `Depot::OCREAT' can be added to by bitwise or: `Depot::OSPARSE', which
  # means it creates a database file as a sparse file.  If it is omitted, `Depot::OREADER' is
  # specified.
  # `bnum' specifies the number of elements of the bucket array.  If it is omitted or not more
  # than 0, the default value is specified.  The size of a bucket array is determined on
  # creating, and can not be changed except for by optimization of the database.  Suggested
  # size of a bucket array is about from 0.5 to 4 times of the number of all records to store.
  # The return value is the database handle.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  # If a block parameter is given, this method works as an iterator.  A database handle is
  # opened and passed via the first argument of the block.  The database handle is surely
  # closed when the block is over.
  # While connecting as a writer, an exclusive lock is invoked to the database file.
  # While connecting as a reader, a shared lock is invoked to the database file.  The thread
  # blocks until the lock is achieved.  If `Depot::ONOLCK' is used, the application is
  # responsible for exclusion control.
  #
  #@ DEFINED IMPLICITLY
  ##
  # depot = Depot::open(name, omode, bnum)
  # Constructor: An alias of `new'.
  #
  #@ DEFINED OUTSIDE
  #----------------------------------------------------------------
  # private methods
  #----------------------------------------------------------------
  private
  #=
  # initialize(name, omode, bnum)
  # Method: Called implicitly by the constructor.
  #
  def initialize(name, omode = OREADER, bnum = -1)
    @silent = false
    MyMutex.synchronize() do
      @index = mod_open(name, omode, bnum)
      @name = name
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
    raise(DepotError)
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
  # bool = depot.close()
  # Method: Close the database handle.
  # The return value is always true.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  # Because the region of a closed handle is released, it becomes impossible to use the handle.
  # Updating a database is assured to be written when the handle is closed.  If a writer opens
  # a database but does not close it appropriately, the database will be broken.
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
  # depot.silent = bool
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
  # bool = depot.put(key, val, dmode)
  # Method: Store a record.
  # `key' specifies a key.  Although it must be an instance of String, binary data is okey.
  # `val' specifies a value.  Although it must be an instance of String, binary data is okey.
  # `dmode' specifies behavior when the key overlaps, by the following values: `Depot::DOVER',
  # which means the specified value overwrites the existing one, `Depot::DKEEP', which means
  # the existing value is kept, `Depot::DCAT', which means the specified value is concatenated
  # at the end of the existing value.  If it is omitted, `Depot::DOVER' is specified.
  # The return value is always true.  However, if the silent flag is true and replace is
  # cancelled, false is returned instead of exception.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs or replace
  # is cancelled.
  #
  def put(key, val, dmode = DOVER)
    mod_put(@index, key, val, dmode)
  end
  ##
  # bool = depot.store(key, val)
  # Method: An alias of `put'.
  #
  alias store put
  ##
  # depot[key] = val
  # Method: An alias of `put'.
  #
  alias []= put
  ##
  # bool = depot.out(key)
  # Method: Delete a record.
  # `key' specifies a key.  Although it must be an instance of String, binary data is okey.
  # The return value is always true.  However, if the silent flag is true and no record
  # corresponds, false is returned instead of exception.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs or no record
  # corresponds.
  #
  def out(key)
    mod_out(@index, key)
  end
  ##
  # bool = depot.delete(key)
  # Method: An alias of `out'.
  #
  alias delete out
  ##
  # bool = depot.clear()
  # Method: Delete all records.
  # The return value is always true.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  #
  def clear
    MyMutex.synchronize() do
      iterinit()
      while(rnum() > 0)
        out(iternext())
      end
    end
    true
  end
  ##
  # str = depot.get(key, start, max)
  # Method: Retrieve a record.
  # `key' specifies a key.  Although it must be an instance of String, binary data is okey.
  # `start' specifies the offset address of the beginning of the region of the value to be read.
  # If it is negative or omitted, the offset is specified as 0.
  # `max' specifies the max size to be read.  If it is negative or omitted, the size to read is
  # unlimited.
  # The return value is an instance of the value of the corresponding record.  If the silent flag
  # is true and no record corresponds, nil is returned instead of exception.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs, no record
  # corresponds, or the size of the value of the corresponding record is less than `max'.
  #
  def get(key, start = 0, max = -1)
    mod_get(@index, key, start, max)
  end
  ##
  # str = depot.fetch(key, defval)
  # Method: Retrieve a record.
  # `key' specifies a key.  Although it must be an instance of String, binary data is okey.
  # `defval' specifies the default value used when no record corresponds.  If it is omitted, nil
  # is specified.
  # The return value is an instance of the value of the corresponding record, or the default
  # value if no record corresponds.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  #
  def fetch(key, defval = nil)
    if @silent
      if val = mod_get(@index, key, 0, -1)
        val
      else
        defval
      end
    else
      begin
        mod_get(@index, key, 0, -1)
      rescue ENOITEM
        defval
      end
    end
  end
  ##
  # str = depot[key]
  # Method: An alias of `fetch'.
  #
  alias [] fetch
  ##
  # num = depot.vsiz(key)
  # Method: Get the size of the value of a record.
  # `key' specifies a key.  Although it must be an instance of String, binary data is okey.
  # The return value is the size of the value of the corresponding record.  If the silent flag
  # is true and no record corresponds, -1 is returned instead of exception.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs or no record
  # corresponds.
  # Because this method does not read the entity of a record, it is faster than `get'.
  #
  def vsiz(key)
    mod_vsiz(@index, key)
  end
  ##
  # bool = depot.iterinit()
  # Method: Initialize the iterator of the database handle.
  # The return value is always true.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  # The iterator is used in order to access the key of every record stored in a database.
  #
  def iterinit()
    mod_iterinit(@index)
  end
  ##
  # str = depot.iternext()
  # Method: Get the next key of the iterator.
  # The return value is the value of the next key.  If the silent flag is true and no record
  # corresponds, nil is returned instead of exception.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs or no record
  # is to be get out of the iterator.
  # It is possible to access every record by iteration of calling this method.  However, it is
  # not assured if updating the database is occurred while the iteration.  Besides, the order
  # of this traversal access method is arbitrary, so it is not assured that the order of
  # storing matches the one of the traversal access.
  #
  def iternext()
    mod_iternext(@index)
  end
  ##
  # bool = depot.setalign(align)
  # Method: Set alignment of the database handle.
  # `align' specifies the basic size of alignment.  If it is omitted, alignment is cleared.
  # The return value is always true.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  # If alignment is set to a database, the efficiency of overwriting values is improved.
  # The size of alignment is suggested to be average size of the values of the records to be
  # stored.  If alignment is positive, padding whose size is multiple number of the alignment
  # is placed.  If alignment is negative, as `vsiz' is the size of a value, the size of padding
  # is calculated with `(vsiz / pow(2, abs(align) - 1))'.  Because alignment setting is not
  # saved in a database, you should specify alignment every opening a database.
  #
  def setalign(align = 0)
    mod_setalign(@index, align)
  end
  ##
  # bool = depot.setfbpsiz(size);
  # Method: Set the size of the free block pool.
  # `size' specifies the size of the free block pool.  If it is undef, the free block pool is not
  # used.
  # The return value is always true.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  # The default size of the free block pool is 16.  If the size is greater, the space efficiency
  # of overwriting values is improved with the time efficiency sacrificed.
  #
  def setfbpsiz(size = 0)
    mod_setfbpsiz(@index, size)
  end
  ##
  # bool = depot.sync()
  # Method: Synchronize updating contents with the file and the device.
  # The return value is always true.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  # This method is useful when another process uses the connected database file.
  #
  def sync()
    mod_sync(@index)
  end
  ##
  # bool = depot.optimize(bnum)
  # Method: Optimize the database.
  # `bnum' specifies the number of the elements of the bucket array.  If it is omitted or not
  # more than 0, the default value is specified.
  # The return value is always true.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  # In an alternating succession of deleting and storing with overwrite or concatenate,
  # dispensable regions accumulate.  This method is useful to do away with them.
  #
  def optimize(bnum = -1)
    mod_optimize(@index, bnum)
  end
  ##
  # num = depot.fsiz()
  # Method: Get the size of the database file.
  # The return value is the size of the database file.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  #
  def fsiz()
    mod_fsiz(@index)
  end
  ##
  # num = depot.bnum()
  # Method: Get the number of the elements of the bucket array.
  # The return value is the number of the elements of the bucket array
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  #
  def bnum()
    mod_bnum(@index)
  end
  ##
  # num = depot.rnum()
  # Method: Get the number of the records stored in the database.
  # The return value is the number of the records stored in the database.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  #
  def rnum()
    mod_rnum(@index)
  end
  ##
  # num = depot.length()
  # Method: An alias of `rnum'.
  #
  alias length rnum
  ##
  # num = depot.size()
  # Method: An alias of `rnum'.
  #
  alias size rnum
  ##
  # bool = depot.writable()
  # Method: Check whether the database handle is a writer or not.
  # The return value is true if the handle is a writer, false if not.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  #
  def writable()
    mod_writable(@index)
  end
  ##
  # bool = depot.fatalerror()
  # Method: Check whether the database has a fatal error or not.
  # The return value is true if the database has a fatal error, false if not.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  #
  def fatalerror()
    mod_fatalerror(@index)
  end
  ##
  # depot.each() do |key, val| ... end
  # Iterator Method: Iterate a process with a pair of a key and a value of each record.
  #
  def each()
    MyMutex.synchronize() do
      iterinit()
      while(true)
        begin
          break unless key = iternext()
          val = get(key)
        rescue ENOITEM
          break
        end
        yield(key, val)
      end
      iterinit()
    end
    self
  end
  ##
  # depot.each_pair() do |key, val| ... end
  # Iterator Method: An alias of `each'.
  #
  alias each_pair each
  ##
  # depot.each_key() do |key| ... end
  # Iterator Method: Iterate a process with a key of each record.
  #
  def each_key()
    MyMutex.synchronize() do
      iterinit()
      while(true)
        begin
          break unless key = iternext()
        rescue ENOITEM
          break
        end
        yield(key)
      end
      iterinit()
    end
    self
  end
  ##
  # depot.each_value() do |val| ... end
  # Iterator Method: Iterate a process with a value of each record.
  #
  def each_value()
    MyMutex.synchronize() do
      iterinit()
      while(true)
        begin
          break unless key = iternext()
          val = get(key)
        rescue ENOITEM
          break
        end
        yield(val)
      end
      iterinit()
    end
    self
  end
  ##
  # ary = depot.keys()
  # Method: Get an array of all keys.
  # The return value is an array of all keys.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  #
  def keys()
    ary = Array::new(rnum())
    MyMutex.synchronize() do
      iterinit()
      0.upto(ary.length - 1) do |i|
        ary[i] = iternext()
      end
      iterinit()
    end
    ary
  end
  ##
  # ary = depot.values()
  # Method: Get an array of all values.
  # The return value is an array of all values.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs.
  #
  def values()
    ary = Array::new(rnum())
    MyMutex.synchronize() do
      iterinit()
      0.upto(ary.length - 1) do |i|
        ary[i] = get(iternext())
      end
      iterinit()
    end
    ary
  end
  ##
  # str = depot.index(val)
  # Method: Retrieve a record with a value.
  # `val' specifies a value.  Although it must be an instance of String, binary data is okey.
  # The return value is the key of the record with the specified value.
  # An exception of `Depot::EANY' or its sub classes is thrown if an error occurs or no record
  # corresponds.
  # If two or more records correspond, the first found record is selected.
  #
  def index(val)
    key = nil
    MyMutex.synchronize() do
      iterinit()
      while(true)
        break unless key = iternext()
        (get(key) == val) && break
      end
      iterinit()
    end
    key
  end
  ##
  # num = depot.to_int()
  # Method: An alias of `rnum'.
  #
  alias to_int rnum
  ##
  # num = depot.to_i()
  # Method: An alias of `to_int'.
  #
  alias to_i to_int
  ##
  # str = depot.to_str()
  # Method: Get string standing for the instance.
  #
  def to_str
    if(@index != -1)
      sprintf("#<Depot:%#x:name=%s:state=open:bnum=%d:rnum=%d>",
              object_id(), @name, bnum(), rnum())
    else
      sprintf("#<Depot:%#x:name=%s:state=closed>", object_id(), @name)
    end
  end
  ##
  # str = depot.to_s()
  # Method: An alias of `to_str'.
  #
  alias to_s to_str
  ##
  # ary = depot.to_ary()
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
  # ary = depot.to_a()
  # Method: An alias of `to_ary'.
  #
  alias to_a to_ary
  ##
  # hash = depot.to_hash()
  # Method: Get a hash storing all records.
  #
  def to_hash
    hash = Hash::new()
    each() do |key, val|
      hash[key] = val
    end
    hash
  end
  ##
  # hash = depot.to_h()
  # Method: An alias of `to_hash'.
  #
  alias to_h to_hash
  ##
  # str = depot.inspect()
  # Method: An alias of `to_str'.
  #
  alias inspect to_str
end


#----------------------------------------------------------------
# Alias definition of class methods
#----------------------------------------------------------------
class << Depot
  alias open new
end



# END OF FILE
