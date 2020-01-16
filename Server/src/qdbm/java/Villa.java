/*************************************************************************************************
 * Java API of Villa, the advanced API of QDBM
 *                                                      Copyright (C) 2000-2006 Mikio Hirabayashi
 * This file is part of QDBM, Quick Database Manager.
 * QDBM is free software; you can redistribute it and/or modify it under the terms of the GNU
 * Lesser General Public License as published by the Free Software Foundation; either version
 * 2.1 of the License or any later version.  QDBM is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * You should have received a copy of the GNU Lesser General Public License along with QDBM; if
 * not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 USA.
 *************************************************************************************************/


package qdbm;



/**
 * The Java API of Villa, the advanced API of QDBM.
 * This class depends on the native library `jqdbm'.
 */
public class Villa implements ADBM {
  //----------------------------------------------------------------
  // error codes
  //----------------------------------------------------------------
  /** error code: no error */
  public static final int ENOERR = 0;
  /** error code: with fatal error */
  public static final int EFATAL = 1;
  /** error code: invalid mode */
  public static final int EMODE = 2;
  /** error code: broken database file */
  public static final int EBROKEN = 3;
  /** error code: existing record */
  public static final int EKEEP = 4;
  /** error code: no item found */
  public static final int ENOITEM = 5;
  /** error code: memory allocation error */
  public static final int EALLOC = 6;
  /** error code: memory mapping error */
  public static final int EMAP = 7;
  /** error code: open error */
  public static final int EOPEN = 8;
  /** error code: close error */
  public static final int ECLOSE = 9;
  /** error code: trunc error */
  public static final int ETRUNC = 10;
  /** error code: sync error */
  public static final int ESYNC = 11;
  /** error code: stat error */
  public static final int ESTAT = 12;
  /** error code: seek error */
  public static final int ESEEK = 13;
  /** error code: read error */
  public static final int EREAD = 14;
  /** error code: write error */
  public static final int EWRITE = 15;
  /** error code: lock error */
  public static final int ELOCK = 16;
  /** error code: unlink error */
  public static final int EUNLINK = 17;
  /** error code: mkdir error */
  public static final int EMKDIR = 18;
  /** error code: rmdir error */
  public static final int ERMDIR = 19;
  /** error code: miscellaneous error */
  public static final int EMISC = 20;
  //----------------------------------------------------------------
  // open modes
  //----------------------------------------------------------------
  /** open mode: open as a reader */
  public static final int OREADER = 1 << 0;
  /** open mode: open as a writer */
  public static final int OWRITER = 1 << 1;
  /** open mode: writer creating */
  public static final int OCREAT = 1 << 2;
  /** open mode: writer truncating */
  public static final int OTRUNC = 1 << 3;
  /** open mode: open without locking */
  public static final int ONOLCK = 1 << 4;
  /** open mode: lock without blocking */
  public static final int OLCKNB = 1 << 5;
  /** open mode: compress leaves with ZLIB */
  public static final int OZCOMP = 1 << 6;
  /** open mode: compress leaves with LZO */
  public static final int OYCOMP = 1 << 7;
  /** open mode: compress leaves with BZIP2 */
  public static final int OXCOMP = 1 << 8;
  //----------------------------------------------------------------
  // comparing modes
  //----------------------------------------------------------------
  /** comparing mode: compare by lexical order */
  public static final int CMPLEX = 0;
  /** comparing mode: compare as long integers */
  public static final int CMPNUM = 1;
  /** comparing mode: compare as decimal strings */
  public static final int CMPDEC = 2;
  /** comparing mode: compare as comparable objects */
  public static final int CMPOBJ = 3;
  //----------------------------------------------------------------
  // write modes
  //----------------------------------------------------------------
  /** write mode: overwrite the existing value */
  public static final int DOVER = 0;
  /** write mode: keep the existing value */
  public static final int DKEEP = 1;
  /** write mode: concatenate values */
  public static final int DCAT = 2;
  /** write mode: allow duplication of records */
  public static final int DDUP = 3;
  /** write mode: allow duplication with reverse order */
  public static final int DDUPR = 4;
  //----------------------------------------------------------------
  // jump modes
  //----------------------------------------------------------------
  /** jump mode: jump mode: step forward */
  public static final int JFORWARD = 0;
  /** jump mode: jump mode: step backward */
  public static final int JBACKWARD = 1;
  /** insertion mode: overwrite the current record */
  public static final int CPCURRENT = 0;
  /** insertion mode: insert before the current record */
  public static final int CPBEFORE = 1;
  /** insertion mode: insert after the current record */
  public static final int CPAFTER = 2;
  //----------------------------------------------------------------
  // static initializer
  //----------------------------------------------------------------
  static {
    try {
      System.loadLibrary("jqdbm");
    } catch(UnsatisfiedLinkError e){
      e.printStackTrace();
    }
    vlinit();
  }
  //----------------------------------------------------------------
  // public static methods
  //----------------------------------------------------------------
  /**
   * Get the version information.
   * @return the string of the version information.
   */
  public static synchronized String version(){
    return vlversion();
  }
  /**
   * Get an error message.
   * @param ecode an error code.
   * @return the message string of the error code.
   */
  public static synchronized String errmsg(int ecode){
    return vlerrmsg(ecode);
  }
  /**
   * Remove a database file.
   * @param name the name of a database file.
   * @throws VillaException if an error occurs.
   */
  public static void remove(String name) throws VillaException {
    synchronized(ADBM.class){
      if(vlremove(name) == 0) throw new VillaException(vlecode());
    }
  }
  //----------------------------------------------------------------
  // instance fields
  //----------------------------------------------------------------
  /** Whether to repress frequent exceptions. */
  public boolean silent;
  /** Index of the native table for database handles. */
  private int index;
  /** Whether the handle has the transaction or not. */
  private boolean tran;
  /** Monitor for mutual exclusion control. */
  private Object tranmonitor;
  //----------------------------------------------------------------
  // public or protected methods
  //----------------------------------------------------------------
  /**
   * Get the database handle.
   * @param name the name of a database file.
   * @param omode the connection mode: `Villa.OWRITER' as a writer, `Villa.OREADER' as
   * a reader.  If the mode is `Villa.OWRITER', the following may be added by bitwise or:
   * `Villa.OCREAT', which means it creates a new database if not exist, `Villa.OTRUNC',
   * which means it creates a new database regardless if one exists, `Villa.OZCOMP', which means
   * leaves in the database are compressed with ZLIB, `Villa.OYCOMP', which means leaves in the
   * database are compressed with LZO, `Villa.OXCOMP', which means leaves in the database are
   * compressed with BZIP2.  Both of `Villa.OREADER' and `Villa.OWRITER' can be added to by
   * bitwise or: `Villa.ONOLCK', which means it opens a database file without file locking, or
   * `Villa.OLCKNB', which means locking is performed without blocking.
   * @param cmode the comparing function: `Villa.CMPLEX' comparing keys in lexical order,
   * `Villa.CMPNUM' comparing keys as numbers of big endian, `Villa.CMPDEC' comparing keys as
   * decimal strings, `Villa.CMPOBJ' comparing keys as serialized objects implementing
   * `java.util.Comparable'.  The comparing function should be kept same in the life of a
   * database.
   * @note While connecting as a writer, an exclusive lock is invoked to the database file.
   * While connecting as a reader, a shared lock is invoked to the database file.  The thread
   * blocks until the lock is achieved.  `Villa.OZCOMP', `Villa.OYCOMP', and `Villa.OXCOMP' are
   * available only if QDBM was built each with ZLIB, LZO, and BZIP2 enabled.  If `Villa.ONOLCK'
   * is used, the application is responsible for exclusion control.
   */
  public Villa(String name, int omode, int cmode) throws VillaException {
    synchronized(ADBM.class){
      silent = false;
      if((index = vlopen(name, omode, cmode)) == -1) throw new VillaException(vlecode());
    }
    tran = false;
    tranmonitor = new Object();
  }
  /**
   * Get the database handle as a reader.
   * The same as `Villa(name, Villa.OREADER, Villa.CMPLEX)'.
   * @see #Villa(java.lang.String, int, int)
   */
  public Villa(String name) throws VillaException {
    this(name, OREADER, CMPLEX);
  }
  /**
   * Release the resources.
   * @note If the database handle is not closed yet, it is closed.  Every database should be
   * closed explicitly.  Do not cast the duty on the gerbage collection.
   */
  protected void finalize() throws Throwable {
    try {
      if(index < 0) return;
      synchronized(ADBM.class){
        vlclose(index);
        index = -1;
      }
    } finally {
      super.finalize();
    }
  }
  /**
   * Close the database handle.
   * @throws VillaException if an error occurs.
   * @note Updating a database is assured to be written when the handle is closed.  If a
   * writer opens a database but does not close it appropriately, the database will be broken.
   * If the transaction is activated and not committed, it is aborted.
   */
  public void close() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      int rv = vlclose(index);
      index = -1;
      if(rv == 0) throw new VillaException(vlecode());
    }
  }
  /**
   * Store a record.
   * @param key a byte array of a key.
   * @param val a byte array of a value.
   * @param dmode behavior when the key overlaps, by the following values: `Villa.DOVER',
   * which means the specified value overwrites the existing one, `Villa.DKEEP', which means
   * the existing value is kept, `Villa.DCAT', which means the specified value is concatenated
   * at the end of the existing value, `Villa.DDUP', which means duplication of keys is allowed
   * and the specified value is added as the last one, `Villa.DDUPR', which means duplication of
   * keys is allowed and the specified value is added as the first one.
   * @return always true.  However, if the silent flag is true and replace is cancelled, false is
   * returned instead of exception.
   * @throws VillaException if an error occurs or replace is cancelled.
   * @note The cursor becomes unavailable due to updating database.
   */
  public boolean put(byte[] key, byte[] val, int dmode) throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      if(vlput(index, key, key.length, val, val.length, dmode) == 0){
        if(silent && vlecode() == EKEEP) return false;
        throw new VillaException(vlecode());
      }
      return true;
    }
  }
  /**
   * Store a record with overwrite.
   * The same as `put(key, val, Villa.DOVER)'.
   * @see #put(byte[], byte[], int)
   */
  public boolean put(byte[] key, byte[] val) throws VillaException {
    return put(key, val, DOVER);
  }
  /**
   * Store a record composed of serializable objects.
   * The same as `put(qdbm.Util.serialize(key), qdbm.Util.serialize(val), dmode)'.
   * @see #put(byte[], byte[], int)
   * @note If serialization is failed, an instance of `VillaException' is thrown.
   */
  public boolean putobj(Object key, Object val, int dmode) throws VillaException {
    byte[] kbuf = Util.serialize(key);
    byte[] vbuf = Util.serialize(val);
    if(kbuf == null || vbuf == null) throw new VillaException();
    return put(kbuf, vbuf, dmode);
  }
  /**
   * Delete a record.
   * @param key a byte array of a key.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throws VillaException if an error occurs or no record corresponds.
   * @note When the key of duplicated records is specified, the first record of the same key
   * is deleted.  The cursor becomes unavailable due to updating database.
   */
  public boolean out(byte[] key) throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      if(vlout(index, key, key.length) == 0){
        if(silent && vlecode() == ENOITEM) return false;
        throw new VillaException(vlecode());
      }
      return true;
    }
  }
  /**
   * Delete a record composed of serializable objects.
   * The same as `out(qdbm.Util.serialize(key))'.
   * @see #out(byte[])
   * @note If serialization is failed, an instance of `VillaException' is thrown.
   */
  public boolean outobj(Object key) throws VillaException {
    byte[] kbuf = Util.serialize(key);
    if(kbuf == null) throw new VillaException();
    return out(kbuf);
  }
  /**
   * Retrieve a record.
   * @param key a byte array of a key.
   * @return a byte array of the value of the corresponding record.  If the silent flag is true
   * and no record corresponds, `null' is returned instead of exception.
   * @throws VillaException if an error occurs, no record corresponds.
   * @note When the key of duplicated records is specified, the value of the first record of
   * the same key is selected.
   */
  public byte[] get(byte[] key) throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      byte[] val = vlget(index, key, key.length);
      if(val == null){
        if(silent && vlecode() == ENOITEM) return null;
        throw new VillaException(vlecode());
      }
      return val;
    }
  }
  /**
   * Retrieve a record composed of serializable objects.
   * The same as `qdbm.Util.deserialize(get(qdbm.Util.serialize(key)))'.
   * @see #get(byte[])
   * @note If serialization is failed, an instance of `VillaException' is thrown.
   */
  public Object getobj(Object key) throws VillaException {
    byte[] kbuf = Util.serialize(key);
    if(kbuf == null) throw new VillaException();
    byte[] vbuf = get(kbuf);
    if(vbuf == null) return null;
    Object val = Util.deserialize(vbuf);
    if(val == null) throw new VillaException();
    return val;
  }
  /**
   * Get the size of the value of a record.
   * @param key a byte array of a key.
   * @return the size of the value of the corresponding record.  If multiple records correspond,
   * the size of the first is returned.  If the silent flag is true and no record corresponds, -1
   * is returned instead of exception.
   * @throws VillaException if an error occurs.
   */
  public int vsiz(byte[] key) throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      int rv = vlvsiz(index, key, key.length);
      if(rv == -1){
        if(silent && vlecode() == ENOITEM) return -1;
        throw new VillaException(vlecode());
      }
      return rv;
    }
  }
  /**
   * Get the size of the value of a record, composed of serializable objects.
   * The same as `vsiz(qdbm.Util.serialize(key))'.
   * @see #vnum(byte[])
   * @note If serialization is failed, an instance of `VillaException' is thrown.
   */
  public int vsizobj(Object key) throws VillaException {
    byte[] kbuf = Util.serialize(key);
    if(kbuf == null) throw new VillaException();
    return vsiz(kbuf);
  }
  /**
   * Get the number of records corresponding a key.
   * @param key a byte array of a key.
   * @return the number of corresponding records.  If no record corresponds, 0 is returned.
   * @throws VillaException if an error occurs.
   */
  public int vnum(byte[] key) throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      int rv = vlvnum(index, key, key.length);
      if(rv == -1) throw new VillaException(vlecode());
      return rv;
    }
  }
  /**
   * Get the number of records corresponding a key, composed of serializable objects.
   * The same as `vnum(qdbm.Util.serialize(key))'.
   * @see #vnum(byte[])
   * @note If serialization is failed, an instance of `VillaException' is thrown.
   */
  public int vnumobj(Object key) throws VillaException {
    byte[] kbuf = Util.serialize(key);
    if(kbuf == null) throw new VillaException();
    return vnum(kbuf);
  }
  /**
   * Move the cursor to the first record.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throws VillaException if an error occurs or there is no record in the database.
   */
  public boolean curfirst() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      if(vlcurfirst(index) == 0){
        if(silent && vlecode() == ENOITEM) return false;
        throw new VillaException(vlecode());
      }
      return true;
    }
  }
  /**
   * Move the cursor to the last record.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throws VillaException if an error occurs or there is no record in the database.
   */
  public boolean curlast() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      if(vlcurlast(index) == 0){
        if(silent && vlecode() == ENOITEM) return false;
        throw new VillaException(vlecode());
      }
      return true;
    }
  }
  /**
   * Move the cursor to the next record.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throws VillaException if an error occurs or there is no previous record.
   */
  public boolean curprev() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      if(vlcurprev(index) == 0){
        if(silent && vlecode() == ENOITEM) return false;
        throw new VillaException(vlecode());
      }
      return true;
    }
  }
  /**
   * Move the cursor to the next record.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throws VillaException if an error occurs or there is no next record.
   */
  public boolean curnext() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      if(vlcurnext(index) == 0){
        if(silent && vlecode() == ENOITEM) return false;
        throw new VillaException(vlecode());
      }
      return true;
    }
  }
  /**
   * Move the cursor to a position around a record.
   * @param key a byte array of a key.
   * @param jmode detail adjustment: `Villa.JFORWARD', which means that the cursor is set to
   * the first record of the same key and that the cursor is set to the next substitute if
   * completely matching record does not exist, `Villa.JBACKWARD', which means that the cursor
   * is set to the last record of the same key and that the cursor is set to the previous
   * substitute if completely matching record does not exist.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throws VillaException if an error occurs or there is no record corresponding the condition.
   */
  public boolean curjump(byte[] key, int jmode) throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      if(vlcurjump(index, key, key.length, jmode) == 0){
        if(silent && vlecode() == ENOITEM) return false;
        throw new VillaException(vlecode());
      }
      return true;
    }
  }
  /**
   * Move the cursor to a position around a record for stepping forward.
   * The same as `curjump(key, Villa.JFORFARD)'.
   * @see #curjump(byte[], int)
   */
  public boolean curjump(byte[] key) throws VillaException {
    return curjump(key, JFORWARD);
  }
  /**
   * Move the cursor to a position around a record composed of serializable objects.
   * The same as `curjump(qdbm.Util.serialize(key), jmode)'.
   * @see #curjump(byte[], int)
   * @note If serialization is failed, an instance of `VillaException' is thrown.
   */
  public boolean curjumpobj(Object key, int jmode) throws VillaException {
    byte[] kbuf = Util.serialize(key);
    if(kbuf == null) throw new VillaException();
    return curjump(kbuf, jmode);
  }
  /**
   * Get the key of the record where the cursor is.
   * @return a byte array of the key of the corresponding record.  If the silent flag is true and
   * no record corresponds, `null' is returned instead of exception.
   * @throws VillaException if an error occurs or no record corresponds to the cursor.
   */
  public byte[] curkey() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      byte[] val = vlcurkey(index);
      if(val == null){
        if(silent && vlecode() == ENOITEM) return null;
        throw new VillaException(vlecode());
      }
      return val;
    }
  }
  /**
   * Get the key of the record composed of serializable objects, where the cursor is.
   * The same as `qdbm.Util.deserialize(curkey())'.
   * @see #curkey()
   * @note If serialization is failed, an instance of `VillaException' is thrown.
   */
  public Object curkeyobj() throws VillaException {
    byte[] kbuf = curkey();
    if(kbuf == null) return null;
    Object key = Util.deserialize(kbuf);
    if(key == null) throw new VillaException();
    return key;
  }
  /**
   * Get the value of the record where the cursor is.
   * @return a byte array of the value of the corresponding record.  If the silent flag is true
   * and no record corresponds, `null' is returned instead of exception.
   * @throws VillaException if an error occurs or no record corresponds to the cursor.
   */
  public byte[] curval() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      byte[] val = vlcurval(index);
      if(val == null){
        if(silent && vlecode() == ENOITEM) return null;
        throw new VillaException(vlecode());
      }
      return val;
    }
  }
  /**
   * Get the value of the record where the cursor is.
   * The same as `qdbm.Util.deserialize(curval())'.
   * @see #curval()
   * @note If serialization is failed, an instance of `VillaException' is thrown.
   */
  public Object curvalobj() throws VillaException {
    byte[] vbuf = curval();
    if(vbuf == null) return null;
    Object val = Util.deserialize(vbuf);
    if(val == null) throw new VillaException();
    return val;
  }
  /**
   * Insert a record around the cursor.
   * @param val a byte array of a value.
   * @param cpmode detail adjustment: `Villa.CPCURRENT', which means that the value of the
   * current record is overwritten, `Villa.CPBEFORE', which means that a new record is inserted
   * before the current record, `Villa.CPAFTER', which means that a new record is inserted after
   * the current record.
   * @return always true.  However, if the silent flag is true and no record corresponds to the
   * cursor, false is returned instead of exception.
   * @throws VillaException if an error occurs or no record corresponds to the cursor.
   * @note After insertion, the cursor is moved to the inserted record.
   */
  public boolean curput(byte[] val, int cpmode) throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      if(vlcurput(index, val, val.length, cpmode) == 0){
        if(silent && vlecode() == ENOITEM) return false;
        throw new VillaException(vlecode());
      }
      return true;
    }
  }
  /**
   * Insert a record as the successor of the cursor.
   * The same as `curput(val, Villa.CPCURRENT)'.
   * @see #curput(byte[], int)
   */
  public boolean curput(byte[] val) throws VillaException {
    return curput(val, CPCURRENT);
  }
  /**
   * Insert a record around the cursor.
   * The same as `curput(qdbm.Util.serialize(val), cpmode)'.
   * @see #curput(byte[], int)
   */
  public boolean curputobj(Object val, int cpmode) throws VillaException {
    byte[] vbuf = Util.serialize(val);
    if(vbuf == null) throw new VillaException();
    return curput(vbuf, cpmode);
  }
  /**
   * Delete the record where the cursor is.
   * @return always true.  However, if the silent flag is true and no record corresponds to the
   * cursor, false is returned instead of exception.
   * @throws VillaException if an error occurs or no record corresponds to the cursor.
   * @note After deletion, the cursor is moved to the next record if possible.
   */
  public boolean curout() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      if(vlcurout(index) == 0){
        if(silent && vlecode() == ENOITEM) return false;
        throw new VillaException(vlecode());
      }
      return true;
    }
  }
  /**
   * Set the tuning parameters for performance.
   * @param lrecmax the max number of records in a leaf node of B+ tree.  If it is not more
   * than 0, the default value is specified.
   * @param nidxmax the max number of indexes in a non-leaf node of B+ tree.  If it is not more
   * than 0, the default value is specified.
   * @param lcnum the max number of caching leaf nodes.  If it is not more than 0, the default
   * value is specified.
   * @param ncnum the max number of caching non-leaf nodes.  If it is not more than 0, the
   * default value is specified.
   * @throws VillaException if an error occurs.
   * @note The default setting is equivalent to `settuning(49, 192, 1024, 512)'.  Because tuning
   * parameters are not saved in a database, you should specify them every opening a database.
   */
  public void settuning(int lrecmax, int nidxmax, int lcnum, int ncnum) throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      vlsettuning(index, lrecmax, nidxmax, lcnum, ncnum);
    }
  }
  /**
   * Synchronize updating contents with the file and the device.
   * @throws VillaException if an error occurs.
   * @note This method is useful when another process uses the connected database file.  This
   * method should not be used while the transaction is activated.
   */
  public void sync() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      if(vlsync(index) == 0) throw new VillaException(vlecode());
    }
  }
  /**
   * Optimize the database.
   * @throws VillaException if an error occurs.
   * @note In an alternating succession of deleting and storing with overwrite or concatenate,
   * dispensable regions accumulate.  This method is useful to do away with them.  This method
   * should not be used while the transaction is activated.
   */
  public void optimize() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      if(vloptimize(index) == 0) throw new VillaException(vlecode());
    }
  }
  /**
   * Get the name of the database.
   * @return the string of the name of the database.
   * @throws VillaException if an error occurs.
   */
  public String name() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      String buf = vlname(index);
      if(buf == null) throw new VillaException(vlecode());
      return buf;
    }
  }
  /**
   * Get the size of the database file.
   * @return the size of the database file.
   * @throws VillaException if an error occurs.
   * @note Because of the I/O buffer, the return value may be less than the real size.
   */
  public int fsiz() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      int rv = vlfsiz(index);
      if(rv == -1) throw new VillaException(vlecode());
      return rv;
    }
  }
  /**
   * Get the number of the leaf nodes of B+ tree.
   * @return the number of the leaf nodes.
   * @throws VillaException if an error occurs.
   */
  public int lnum() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      int rv = vllnum(index);
      if(rv == -1) throw new VillaException(vlecode());
      return rv;
    }
  }
  /**
   * Get the number of the non-leaf nodes of B+ tree.
   * @return the number of the non-leaf nodes.
   * @throws VillaException if an error occurs.
   */
  public int nnum() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      int rv = vlnnum(index);
      if(rv == -1) throw new VillaException(vlecode());
      return rv;
    }
  }
  /**
   * Get the number of the records stored in a database.
   * @return the number of the records stored in the database.
   * @throws VillaException if an error occurs.
   */
  public int rnum() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      int rv = vlrnum(index);
      if(rv == -1) throw new VillaException(vlecode());
      return rv;
    }
  }
  /**
   * Check whether the database handle is a writer or not.
   * @return true if the handle is a writer, false if not.
   * @throws VillaException if an error occurs.
   */
  public boolean writable() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      return vlwritable(index) == 0 ? false : true;
    }
  }
  /**
   * Check whether the database has a fatal error or not.
   * @return true if the database has a fatal error, false if not.
   * @throws VillaException if an error occurs.
   */
  public boolean fatalerror() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      return vlfatalerror(index) == 0 ? false : true;
    }
  }
  /**
   * Get the inode number of the database.
   * @return the inode number of the database file.
   * @throws VillaException if an error occurs.
   */
  public int inode() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      return vlinode(index);
    }
  }
  /**
   * Get the last modified time of the database.
   * @return the last modified time of the database.
   * @throws VillaException if an error occurs.
   */
  public long mtime() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      return vlmtime(index);
    }
  }
  /**
   * Begin the transaction.
   * @throws VillaException if an error occurs.
   * @note If a thread is already in the transaction, the other threads block until the prius
   * is out of the transaction.  Only one transaction can be activated with a database handle
   * at the same time.
   */
  public void tranbegin() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(tranmonitor){
      while(tran){
        try {
          tranmonitor.wait();
        } catch(InterruptedException e){
          e.printStackTrace();
          throw new VillaException();
        }
      }
      tran = true;
    }
    synchronized(ADBM.class){
      if(vltranbegin(index) == 0){
        tran = false;
        throw new VillaException(vlecode());
      }
    }
  }
  /**
   * Commit the transaction.
   * @throws VillaException if an error occurs.
   * @note Updating a database in the transaction is fixed when it is committed successfully.
   * Any other thread except for the one which began the transaction should not call this method.
   */
  public void trancommit() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      if(vltrancommit(index) == 0){
        synchronized(tranmonitor){
          tran = false;
          tranmonitor.notify();
        }
        throw new VillaException(vlecode());
      }
    }
    synchronized(tranmonitor){
      tran = false;
      tranmonitor.notify();
    }
  }
  /**
   * Abort the transaction.
   * @throws VillaException if an error occurs.
   * @note Updating a database in the transaction is discarded when it is aborted.  The state
   * of the database is rollbacked to before transaction.  Any other thread except for the one
   * which began the transaction should not call this method.
   */
  public void tranabort() throws VillaException {
    if(index < 0) throw new VillaException();
    synchronized(ADBM.class){
      if(vltranabort(index) == 0){
        synchronized(tranmonitor){
          tran = false;
          tranmonitor.notify();
        }
        throw new VillaException(vlecode());
      }
    }
    synchronized(tranmonitor){
      tran = false;
      tranmonitor.notify();
    }
  }
  /**
   * Store a record.
   * @param key a byte array of a key.
   * @param val a byte array of a value.
   * @param replace whether the existing value is to be overwritten or not.
   * @throws VillaException if an error occurs or replace is cancelled.
   */
  public void store(byte[] key, byte[] val, boolean replace) throws VillaException {
    if(!put(key, val, replace ? DOVER : DKEEP)) throw new VillaException(EKEEP);
  }
  /**
   * Delete a record.
   * @param key a byte array of a key.
   * @throws VillaException if an error occurs or no record corresponds.
   */
  public void delete(byte[] key) throws VillaException {
    if(!out(key)) throw new VillaException(ENOITEM);
  }
  /**
   * Fetch a record.
   * @param key a byte array of a key.
   * @return a byte array of the value of the corresponding record.
   * @throws VillaException if an error occurs or no record corresponds.
   */
  public byte[] fetch(byte[] key) throws VillaException {
    byte[] vbuf = get(key);
    if(vbuf == null) throw new VillaException(ENOITEM);
    return vbuf;
  }
  /**
   * Get the first key.
   * @return a byte array of the key of the first record.
   * @throws VillaException if an error occurs or no record corresponds.
   */
  public byte[] firstkey() throws VillaException {
    curfirst();
    byte[] kbuf = curkey();
    if(kbuf == null) throw new VillaException(ENOITEM);
    return kbuf;
  }
  /**
   * Get the next key.
   * @return a byte array of the key of the next record.
   * @throws VillaException if an error occurs or no record corresponds.
   */
  public byte[] nextkey() throws VillaException {
    curnext();
    byte[] kbuf = curkey();
    if(kbuf == null) throw new VillaException(ENOITEM);
    return kbuf;
  }
  /**
   * Check whether a fatal error occured or not.
   * @return true if the database has a fatal error, false if not.
   * @throws VillaException if an error occurs.
   */
  public boolean error() throws VillaException {
    return fatalerror();
  }
  /**
   * Get a multiple cursor.
   * @return a multiple cursor.
   * @note Even if plural cursors are fetched out of a database handle, they does not share the
   * locations with each other.  Note that this method can be used only if the database handle
   * is connected as a reader.
   */
  public VillaCursor mulcuropen() throws VillaException {
    return new VillaCursor(this);
  }
  //----------------------------------------------------------------
  // friendly methods
  //----------------------------------------------------------------
  /**
   * Get the index of the native table for database handles.
   * @return the index of the native table for database handles.
   */
  public int getindex(){
    return index;
  }
  //----------------------------------------------------------------
  // private static methods
  //----------------------------------------------------------------
  /**
   * Compare keys of two records as serialized objects implementing `java.util.Comparable'.
   * @param abuf serialized data of one object.
   * @param bbuf serialized data of the other object.
   * @return positive if the former is big, negative if the latter is big, 0 if both are
   * equivalent.
   */
  private static final int objcompare(byte[] abuf, byte[] bbuf){
    Object a = Util.deserialize(abuf);
    Object b = Util.deserialize(bbuf);
    if(a != null && b == null) return 1;
    if(a == null && b != null) return -1;
    if(a == null && b == null) return 0;
    try {
      return ((Comparable)a).compareTo(b);
    } catch(ClassCastException e){
      return 0;
    }
  }
  //----------------------------------------------------------------
  // native methods
  //----------------------------------------------------------------
  private static synchronized final native void vlinit();
  private static synchronized final native String vlversion();
  private static synchronized final native int vlecode();
  private static synchronized final native String vlerrmsg(int ecode);

  private static synchronized final native int vlopen(String name, int omode, int cmode);
  private static synchronized final native int vlclose(int index);
  private static synchronized final native int vlput(int index, byte[] key, int ksiz,
                                                     byte[] val, int vsiz, int dmode);
  private static synchronized final native int vlout(int index, byte[] key, int ksiz);
  private static synchronized final native byte[] vlget(int index, byte[] key, int ksiz);
  private static synchronized final native int vlvsiz(int index, byte[] key, int ksiz);
  private static synchronized final native int vlvnum(int index, byte[] key, int ksiz);
  private static synchronized final native int vlcurfirst(int index);
  private static synchronized final native int vlcurlast(int index);
  private static synchronized final native int vlcurprev(int index);
  private static synchronized final native int vlcurnext(int index);
  private static synchronized final native int vlcurjump(int index, byte[] key, int ksiz,
                                                         int jmode);
  private static synchronized final native byte[] vlcurkey(int index);
  private static synchronized final native byte[] vlcurval(int index);
  private static synchronized final native int vlcurput(int index, byte[] val, int vsiz,
                                                        int cpmode);
  private static synchronized final native int vlcurout(int index);
  private static synchronized final native void vlsettuning(int index, int lrecmax, int nidxmax,
                                                            int lcnum, int ncnum);
  private static synchronized final native int vlsync(int index);
  private static synchronized final native int vloptimize(int index);
  private static synchronized final native String vlname(int index);
  private static synchronized final native int vlfsiz(int index);
  private static synchronized final native int vllnum(int index);
  private static synchronized final native int vlnnum(int index);
  private static synchronized final native int vlrnum(int index);
  private static synchronized final native int vlwritable(int index);
  private static synchronized final native int vlfatalerror(int index);
  private static synchronized final native int vlinode(int index);
  private static synchronized final native long vlmtime(int index);
  private static synchronized final native int vltranbegin(int index);
  private static synchronized final native int vltrancommit(int index);
  private static synchronized final native int vltranabort(int index);
  private static synchronized final native int vlremove(String name);
}



/* END OF FILE */
