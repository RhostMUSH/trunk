/*************************************************************************************************
 * Java API of Curia, the extended API of QDBM
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
 * The Java API of Curia, the extended API of QDBM.
 * This class depends on the native library `jqdbm'.
 */
public class Curia implements ADBM {
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
  /** open mode: create as sparse files */
  public static final int OSPARSE = 1 << 6;
  //----------------------------------------------------------------
  // write modes
  //----------------------------------------------------------------
  /** write mode: overwrite the existing value */
  public static final int DOVER = 0;
  /** write mode: keep the existing value */
  public static final int DKEEP = 1;
  /** write mode: concatenate values */
  public static final int DCAT = 2;
  //----------------------------------------------------------------
  // static initializer
  //----------------------------------------------------------------
  static {
    try {
      System.loadLibrary("jqdbm");
    } catch(UnsatisfiedLinkError e){
      e.printStackTrace();
    }
    crinit();
  }
  //----------------------------------------------------------------
  // public static methods
  //----------------------------------------------------------------
  /**
   * Get the version information.
   * @return a string of the version information.
   */
  public static synchronized String version(){
    return crversion();
  }
  /**
   * Get an error message.
   * @param ecode an error code.
   * @return the message string of the error code.
   */
  public static synchronized String errmsg(int ecode){
    return crerrmsg(ecode);
  }
  /**
   * Remove a database directory.
   * @param name the name of a database directory.
   * @throws CuriaException if an error occurs.
   */
  public static void remove(String name) throws CuriaException {
    synchronized(ADBM.class){
      if(crremove(name) == 0) throw new CuriaException(crecode());
    }
  }
  /**
   * Retrieve a record directly from a database directory.
   * @param name the name of a database directory.
   * @param key a byte array of a key.
   * @throws CuriaException if an error occurs or no record corresponds.
   * @note Although this method can be used even while the database directory is locked by
   * another process, it is not assured that recent updated is reflected.
   */
  public static byte[] snaffle(String name, byte[] key) throws CuriaException {
    synchronized(ADBM.class){
      byte[] val = crsnaffle(name, key, key.length);
      if(val == null) throw new CuriaException(crecode());
      return val;
    }
  }
  //----------------------------------------------------------------
  // instance fields
  //----------------------------------------------------------------
  /** Whether to repress frequent exceptions. */
  public boolean silent;
  /** Index of the native table for database handles. */
  private int index;
  //----------------------------------------------------------------
  // public or protected methods
  //----------------------------------------------------------------
  /**
   * Get the database handle.
   * @param name the name of a database directory.
   * @param omode the connection mode: `Curia.OWRITER' as a writer, `Curia.OREADER' as
   * a reader.  If the mode is `Curia.OWRITER', the following may be added by bitwise or:
   * `Curia.OCREAT', which means it creates a new database if not exist, `Curia.OTRUNC',
   * which means it creates a new database regardless if one exists.  Both of `Curia.OREADER'
   * and `Curia.OWRITER' can be added to by bitwise or: `Curia.ONOLCK', which means it opens a
   * database directory without file locking, or `Curia.OLCKNB', which means locking is performed
   * without blocking.  `Curia.OCREAT' can be added to by bitwise or: `Curia.OSPARSE', which
   * means it creates database files as sparse files.
   * @param bnum the number of elements of the each bucket array.  If it is not more than 0,
   * the default value is specified.  The size of each bucket array is determined on creating,
   * and can not be changed except for by optimization of the database.  Suggested size of each
   * bucket array is about from 0.5 to 4 times of the number of all records to store.
   * @param dnum the number of division of the database.  If it is not more than 0, the default
   * value is specified.  The number of division can not be changed from the initial value.  The
   * max number of division is 512.
   * @throws CuriaException if an error occurs.
   * @note While connecting as a writer, an exclusive lock is invoked to the database directory.
   * While connecting as a reader, a shared lock is invoked to the database directory.
   * The thread blocks until the lock is achieved.  If `Curia.ONOLCK' is used, the application
   * is responsible for exclusion control.
   */
  public Curia(String name, int omode, int bnum, int dnum) throws CuriaException {
    synchronized(ADBM.class){
      silent = false;
      if((index = cropen(name, omode, bnum, dnum)) == -1) throw new CuriaException(crecode());
    }
  }
  /**
   * Get the database handle as a reader.
   * The same as `Curia(name, Curia.OREADER, -1)'.
   * @see #Curia(java.lang.String, int, int, int)
   */
  public Curia(String name) throws CuriaException {
    this(name, OREADER, -1, -1);
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
        crclose(index);
        index = -1;
      }
    } finally {
      super.finalize();
    }
  }
  /**
   * Close the database handle.
   * @throws CuriaException if an error occurs.
   * @note Updating a database is assured to be written when the handle is closed.  If a
   * writer opens a database but does not close it appropriately, the database will be broken.
   */
  public void close() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      int rv = crclose(index);
      index = -1;
      if(rv == 0) throw new CuriaException(crecode());
    }
  }
  /**
   * Store a record.
   * @param key a byte array of a key.
   * @param val a byte array of a value.
   * @param dmode behavior when the key overlaps, by the following values: `Curia.DOVER',
   * which means the specified value overwrites the existing one, `Curia.DKEEP', which means the
   * existing value is kept, `Curia.DCAT', which means the specified value is concatenated at
   * the end of the existing value.
   * @return always true.  However, if the silent flag is true and replace is cancelled, false is
   * returned instead of exception.
   * @throws CuriaException if an error occurs or replace is cancelled.
   */
  public boolean put(byte[] key, byte[] val, int dmode) throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      if(crput(index, key, key.length, val, val.length, dmode) == 0){
        if(silent && crecode() == EKEEP) return false;
        throw new CuriaException(crecode());
      }
      return true;
    }
  }
  /**
   * Store a record with overwrite.
   * The same as `put(key, val, Curia.DOVER)'.
   * @see #put(byte[], byte[], int)
   */
  public boolean put(byte[] key, byte[] val) throws CuriaException {
    return put(key, val, DOVER);
  }
  /**
   * Delete a record.
   * @param key a byte array of a key.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throws CuriaException if an error occurs or no record corresponds.
   */
  public boolean out(byte[] key) throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      if(crout(index, key, key.length) == 0){
        if(silent && crecode() == ENOITEM) return false;
        throw new CuriaException(crecode());
      }
      return true;
    }
  }
  /**
   * Retrieve a record.
   * @param key a byte array of a key.
   * @param start the array index of the beginning of the value to be read.
   * @param max the max size to read with.  If it is negative, the size is unlimited.
   * @return a byte array of the value of the corresponding record.  If the silent flag is true
   * and no record corresponds, `null' is returned instead of exception.
   * @throws CuriaException if an error occurs, no record corresponds, or the size of the value
   * of the corresponding record is less than the index specified by the parameter `start'.
   */
  public byte[] get(byte[] key, int start, int max) throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      byte[] val = crget(index, key, key.length, start, max);
      if(val == null){
        if(silent && crecode() == ENOITEM) return null;
        throw new CuriaException(crecode());
      }
      return val;
    }
  }
  /**
   * Retrieve whole value of a record.
   * The same as `get(key, 0, -1)'.
   * @see #get(byte[], int, int)
   */
  public byte[] get(byte[] key) throws CuriaException {
    return get(key, 0, -1);
  }
  /**
   * Get the size of the value of a record.
   * @param key a byte array of a key.
   * @return the size of the value of the corresponding record.  If the silent flag is true and
   * no record corresponds, -1 is returned instead of exception.
   * @throws CuriaException if an error occurs or no record corresponds.
   * @note Because this method does not read the entity of a record, it is faster than `get'.
   */
  public int vsiz(byte[] key) throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      int rv = crvsiz(index, key, key.length);
      if(rv == -1){
        if(silent && crecode() == ENOITEM) return -1;
        throw new CuriaException(crecode());
      }
      return rv;
    }
  }
  /**
   * Initialize the iterator of the database handle.
   * @throws CuriaException if an error occurs.
   * @note The iterator is used in order to access the key of every record stored in a database.
   */
  public void iterinit() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      if(criterinit(index) == 0) throw new CuriaException(crecode());
    }
  }
  /**
   * Get the next key of the iterator.
   * @return a byte array of the key of the next record.  If the silent flag is true and no
   * record corresponds, `null' is returned instead of exception.
   * @throws CuriaException if an error occurs or no record corresponds.
   * @note It is possible to access every record by iteration of calling this method.
   * However, it is not assured if updating the database is occurred while the iteration.
   * Besides, the order of this traversal access method is arbitrary, so it is not assured
   * that the order of storing matches the one of the traversal access.
   */
  public byte[] iternext() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      byte[] val = criternext(index);
      if(val == null){
        if(silent && crecode() == ENOITEM) return null;
        throw new CuriaException(crecode());
      }
      return val;
    }
  }
  /**
   * Set alignment of the database handle.
   * @param align the basic size of alignment.
   * @throws CuriaException if an error occurs.
   * @note If alignment is set to a database, the efficiency of overwriting values is improved.
   * The size of alignment is suggested to be average size of the values of the records to be
   * stored.  If alignment is positive, padding whose size is multiple number of the alignment
   * is placed.  If alignment is negative, as `vsiz' is the size of a value, the size of padding
   * is calculated with `(vsiz / pow(2, abs(align) - 1))'.  Because alignment setting is not
   * saved in a database, you should specify alignment every opening a database.
   */
  public void setalign(int align) throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      if(crsetalign(index, align) == 0) throw new CuriaException(crecode());
    }
  }
  /**
   * Set the size of the free block pool.
   * @param size the size of the free block pool of a database.
   * @throws CuriaException if an error occurs.
   * @note The default size of the free block pool is 16.  If the size is greater, the space
   * efficiency of overwriting values is improved with the time efficiency sacrificed.
   */
  public void setfbpsiz(int size) throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      if(crsetfbpsiz(index, size) == 0) throw new CuriaException(crecode());
    }
  }
  /**
   * Synchronize updating contents with the files and the devices.
   * @throws CuriaException if an error occurs.
   * @note This method is useful when another process uses the connected database directory.
   */
  public void sync() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      if(crsync(index) == 0) throw new CuriaException(crecode());
    }
  }
  /**
   * Optimize the database.
   * @param bnum the number of the elements of each bucket array.  If it is not more than 0, the
   * default value is specified.
   * @throws CuriaException if an error occurs.
   * @note In an alternating succession of deleting and storing with overwrite or concatenate,
   * dispensable regions accumulate.  This method is useful to do away with them.
   */
  public void optimize(int bnum) throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      if(croptimize(index, bnum) == 0) throw new CuriaException(crecode());
    }
  }
  /**
   * Get the name of the database.
   * @return the string of the name of the database.
   * @throws CuriaException if an error occurs.
   */
  public String name() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      String buf = crname(index);
      if(buf == null) throw new CuriaException(crecode());
      return buf;
    }
  }
  /**
   * Get the total size of the database files.
   * @return the total size of the database files.
   * @throws CuriaException if an error occurs.
   */
  public long fsiz() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      long rv = (long)crfsizd(index);
      if(rv == -1) throw new CuriaException(crecode());
      return rv;
    }
  }
  /**
   * Get the total number of the elements of each bucket array.
   * @return the total number of the elements of each bucket array.
   * @throws CuriaException if an error occurs.
   */
  public int bnum() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      int rv = crbnum(index);
      if(rv == -1) throw new CuriaException(crecode());
      return rv;
    }
  }
  /**
   * Get the total number of the used elements of each bucket array.
   * @return the total number of the used elements of each bucket array.
   * @throws CuriaException if an error occurs.
   * @note This method is inefficient because it accesses all elements of each bucket array.
   */
  public int busenum() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      int rv = crbusenum(index);
      if(rv == -1) throw new CuriaException(crecode());
      return rv;
    }
  }
  /**
   * Get the number of the records stored in the database.
   * @return the number of the records stored in the database.
   * @throws CuriaException if an error occurs.
   */
  public int rnum() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      int rv = crrnum(index);
      if(rv == -1) throw new CuriaException(crecode());
      return rv;
    }
  }
  /**
   * Check whether the database handle is a writer or not.
   * @return true if the handle is a writer, false if not.
   * @throws CuriaException if an error occurs.
   */
  public boolean writable() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      return crwritable(index) == 0 ? false : true;
    }
  }
  /**
   * Check whether the database has a fatal error or not.
   * @return true if the database has a fatal error, false if not.
   * @throws CuriaException if an error occurs.
   */
  public boolean fatalerror() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      return crfatalerror(index) == 0 ? false : true;
    }
  }
  /**
   * Get the inode number of the database.
   * @return the inode number of the database directory.
   * @throws CuriaException if an error occurs.
   */
  public int inode() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      return crinode(index);
    }
  }
  /**
   * Get the last modified time of the database.
   * @return the last modified time of the database.
   * @throws CuriaException if an error occurs.
   */
  public long mtime() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      return crmtime(index);
    }
  }
  /**
   * Store a large object.
   * @param key a byte array of a key.
   * @param val a byte array of a value.
   * @param dmode behavior when the key overlaps, by the following values: `Curia.DOVER',
   * which means the specified value overwrites the existing one, `Curia.DKEEP', which means the
   * existing value is kept, `Curia.DCAT', which means the specified value is concatenated at
   * the end of the existing value.
   * @return always true.  However, if the silent flag is true and replace is cancelled, false is
   * returned instead of exception.
   * @throws CuriaException if an error occurs or replace is cancelled.
   */
  public boolean putlob(byte[] key, byte[] val, int dmode) throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      if(crputlob(index, key, key.length, val, val.length, dmode) == 0){
        if(silent && crecode() == EKEEP) return false;
        throw new CuriaException(crecode());
      }
      return true;
    }
  }
  /**
   * Store a large object with overwrite.
   * The same as `putlob(key, val, Curia.DOVER)'.
   * @see #put(byte[], byte[], int)
   */
  public boolean putlob(byte[] key, byte[] val) throws CuriaException {
    return putlob(key, val, DOVER);
  }
  /**
   * Delete a large object.
   * @param key a byte array of a key.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throws CuriaException if an error occurs or no large object corresponds.
   */
  public boolean outlob(byte[] key) throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      if(croutlob(index, key, key.length) == 0){
        if(silent && crecode() == ENOITEM) return false;
        throw new CuriaException(crecode());
      }
      return true;
    }
  }
  /**
   * Retrieve a large object.
   * @param key a byte array of a key.
   * @param start the array index of the beginning of the value to be read.
   * @param max the max size to be read.  If it is negative, the size to read is unlimited.
   * @return a byte array of the value of the corresponding large object.  If the silent flag is
   * true and no record corresponds, `null' is returned instead of exception.
   * @throws CuriaException if an error occurs, no large object corresponds or the size of the
   * value of the corresponding is less than the index specified by the parameter `start'.
   */
  public byte[] getlob(byte[] key, int start, int max) throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      byte[] val = crgetlob(index, key, key.length, start, max);
      if(val == null){
        if(silent && crecode() == ENOITEM) return null;
        throw new CuriaException(crecode());
      }
      return val;
    }
  }
  /**
   * Retrieve whole value of a large object.
   * The same as `get(key, 0, -1)'.
   * @see #get(byte[], int, int)
   */
  public byte[] getlob(byte[] key) throws CuriaException {
    return getlob(key, 0, -1);
  }
  /**
   * Get the size of the value of a large object.
   * @param key a byte array of a key.
   * @return the size of the value of the corresponding large object.
   * @throws CuriaException if an error occurs or no large object corresponds.
   * @note Because this method does not read the entity of a large object, it is faster
   * than `get'.
   */
  public int vsizlob(byte[] key) throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      int rv = crvsiz(index, key, key.length);
      if(rv == -1) throw new CuriaException(crecode());
      return rv;
    }
  }
  /**
   * Get the number of the large objects stored in the database.
   * @return the number of the large objects stored in the database.
   * @throws CuriaException if an error occurs.
   */
  public int rnumlob() throws CuriaException {
    if(index < 0) throw new CuriaException();
    synchronized(ADBM.class){
      int rv = crrnumlob(index);
      if(rv == -1) throw new CuriaException(crecode());
      return rv;
    }
  }
  /**
   * Store a record.
   * @param key a byte array of a key.
   * @param val a byte array of a value.
   * @param replace whether the existing value is to be overwritten or not.
   * @throws CuriaException if an error occurs or replace is cancelled.
   */
  public void store(byte[] key, byte[] val, boolean replace) throws CuriaException {
    if(!put(key, val, replace ? DOVER : DKEEP)) throw new CuriaException(EKEEP);
  }
  /**
   * Delete a record.
   * @param key a byte array of a key.
   * @throws CuriaException if an error occurs or no record corresponds.
   */
  public void delete(byte[] key) throws CuriaException {
    if(!out(key)) throw new CuriaException(ENOITEM);
  }
  /**
   * Fetch a record.
   * @param key a byte array of a key.
   * @return a byte array of the value of the corresponding record.
   * @throws CuriaException if an error occurs or no record corresponds.
   */
  public byte[] fetch(byte[] key) throws CuriaException {
    byte[] vbuf = get(key, 0, -1);
    if(vbuf == null) throw new CuriaException(ENOITEM);
    return vbuf;
  }
  /**
   * Get the first key.
   * @return a byte array of the key of the first record.
   * @throws CuriaException if an error occurs or no record corresponds.
   */
  public byte[] firstkey() throws CuriaException {
    iterinit();
    byte[] kbuf = iternext();
    if(kbuf == null) throw new CuriaException(ENOITEM);
    return kbuf;
  }
  /**
   * Get the next key.
   * @return a byte array of the key of the next record.
   * @throws CuriaException if an error occurs or no record corresponds.
   */
  public byte[] nextkey() throws CuriaException {
    byte[] kbuf = iternext();
    if(kbuf == null) throw new CuriaException(ENOITEM);
    return kbuf;
  }
  /**
   * Check whether a fatal error occured or not.
   * @return true if the database has a fatal error, false if not.
   * @throws CuriaException if an error occurs.
   */
  public boolean error() throws CuriaException {
    return fatalerror();
  }
  //----------------------------------------------------------------
  // native methods
  //----------------------------------------------------------------
  private static synchronized final native void crinit();
  private static synchronized final native String crversion();
  private static synchronized final native int crecode();
  private static synchronized final native String crerrmsg(int ecode);
  private static synchronized final native int cropen(String name, int omode, int bnum, int dnum);
  private static synchronized final native int crclose(int index);
  private static synchronized final native int crput(int index, byte[] key, int ksiz,
                                                     byte[] val, int vsiz, int dmode);
  private static synchronized final native int crout(int index, byte[] key, int ksiz);
  private static synchronized final native byte[] crget(int index, byte[] key, int ksiz,
                                                        int start, int max);
  private static synchronized final native int crvsiz(int index, byte[] key, int ksiz);
  private static synchronized final native int criterinit(int index);
  private static synchronized final native byte[] criternext(int index);
  private static synchronized final native int crsetalign(int index, int align);
  private static synchronized final native int crsetfbpsiz(int index, int size);
  private static synchronized final native int crsync(int index);
  private static synchronized final native int croptimize(int index, int bnum);
  private static synchronized final native String crname(int index);
  private static synchronized final native double crfsizd(int index);
  private static synchronized final native int crbnum(int index);
  private static synchronized final native int crbusenum(int index);
  private static synchronized final native int crrnum(int index);
  private static synchronized final native int crwritable(int index);
  private static synchronized final native int crfatalerror(int index);
  private static synchronized final native int crinode(int index);
  private static synchronized final native long crmtime(int index);
  private static synchronized final native int crputlob(int index, byte[] key, int ksiz,
                                                        byte[] val, int vsiz, int dmode);
  private static synchronized final native int croutlob(int index, byte[] key, int ksiz);
  private static synchronized final native byte[] crgetlob(int index, byte[] key, int ksiz,
                                                           int start, int max);
  private static synchronized final native int crvsizlob(int index, byte[] key, int ksiz);
  private static synchronized final native int crrnumlob(int index);
  private static synchronized final native int crremove(String name);
  private static synchronized final native byte[] crsnaffle(String name, byte[] key, int ksiz);
}



/* END OF FILE */
