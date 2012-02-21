/*************************************************************************************************
 * Java API of Depot, the basic API of QDBM
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
 * The Java API of Depot, the basic API of QDBM.
 * This class depends on the native library `jqdbm'.
 */
public class Depot implements ADBM {
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
  /** open mode: create as a sparse file */
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
    dpinit();
  }
  //----------------------------------------------------------------
  // public static methods
  //----------------------------------------------------------------
  /**
   * Get the version information.
   * @return the string of the version information.
   */
  public static synchronized String version(){
    return dpversion();
  }
  /**
   * Get an error message.
   * @param ecode an error code.
   * @return the message string of the error code.
   */
  public static synchronized String errmsg(int ecode){
    return dperrmsg(ecode);
  }
  /**
   * Remove a database file.
   * @param name the name of a database file.
   * @throws DepotException if an error occurs.
   */
  public static void remove(String name) throws DepotException {
    synchronized(ADBM.class){
      if(dpremove(name) == 0) throw new DepotException(dpecode());
    }
  }
  /**
   * Retrieve a record directly from a database file.
   * @param name the name of a database file.
   * @param key a byte array of a key.
   * @throws DepotException if an error occurs or no record corresponds.
   * @note Although this method can be used even while the database file is locked by another
   * process, it is not assured that recent updated is reflected.
   */
  public static byte[] snaffle(String name, byte[] key) throws DepotException {
    synchronized(ADBM.class){
      byte[] val = dpsnaffle(name, key, key.length);
      if(val == null) throw new DepotException(dpecode());
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
   * @param name the name of a database file.
   * @param omode the connection mode: `Depot.OWRITER' as a writer, `Depot.OREADER' as
   * a reader.  If the mode is `Depot.OWRITER', the following may be added by bitwise or:
   * `Depot.OCREAT', which means it creates a new database if not exist, `Depot.OTRUNC',
   * which means it creates a new database regardless if one exists.  Both of `Depot.OREADER'
   * and `Depot.OWRITER' can be added to by bitwise or: `Depot.ONOLCK', which means it opens a
   * database file without file locking, or `Depot.OLCKNB', which means locking is performed
   * without blocking.  `Depot.OCREAT' can be added to by bitwise or: `Depot.OSPARSE', which
   * means it creates a database file as a sparse file.
   * @param bnum the number of elements of the bucket array.  If it is not more than 0,
   * the default value is specified.  The size of a bucket array is determined on creating,
   * and can not be changed except for by optimization of the database.  Suggested size of a
   * bucket array is about from 0.5 to 4 times of the number of all records to store.
   * @throws DepotException if an error occurs.
   * @note While connecting as a writer, an exclusive lock is invoked to the database file.
   * While connecting as a reader, a shared lock is invoked to the database file.  The thread
   * blocks until the lock is achieved.  If `Depot.ONOLCK' is used, the application is
   * responsible for exclusion control.
   */
  public Depot(String name, int omode, int bnum) throws DepotException {
    synchronized(ADBM.class){
      silent = false;
      if((index = dpopen(name, omode, bnum)) == -1) throw new DepotException(dpecode());
    }
  }
  /**
   * Get the database handle as a reader.
   * The same as `Depot(name, Depot.OREADER, -1)'.
   * @see #Depot(java.lang.String, int, int)
   */
  public Depot(String name) throws DepotException {
    this(name, OREADER, -1);
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
        dpclose(index);
        index = -1;
      }
    } finally {
      super.finalize();
    }
  }
  /**
   * Close the database handle.
   * @throws DepotException if an error occurs.
   * @note Updating a database is assured to be written when the handle is closed.  If a
   * writer opens a database but does not close it appropriately, the database will be broken.
   */
  public void close() throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      int rv = dpclose(index);
      index = -1;
      if(rv == 0) throw new DepotException(dpecode());
    }
  }
  /**
   * Store a record.
   * @param key a byte array of a key.
   * @param val a byte array of a value.
   * @param dmode behavior when the key overlaps, by the following values: `Depot.DOVER',
   * which means the specified value overwrites the existing one, `Depot.DKEEP', which means the
   * existing value is kept, `Depot.DCAT', which means the specified value is concatenated at
   * the end of the existing value.
   * @return always true.  However, if the silent flag is true and replace is cancelled, false is
   * returned instead of exception.
   * @throws DepotException if an error occurs or replace is cancelled.
   */
  public boolean put(byte[] key, byte[] val, int dmode) throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      if(dpput(index, key, key.length, val, val.length, dmode) == 0){
        if(silent && dpecode() == EKEEP) return false;
        throw new DepotException(dpecode());
      }
      return true;
    }
  }
  /**
   * Store a record with overwrite.
   * The same as `put(key, val, Depot.DOVER)'.
   * @see #put(byte[], byte[], int)
   */
  public boolean put(byte[] key, byte[] val) throws DepotException {
    return put(key, val, DOVER);
  }
  /**
   * Delete a record.
   * @param key a byte array of a key.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throws DepotException if an error occurs or no record corresponds.
   */
  public boolean out(byte[] key) throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      if(dpout(index, key, key.length) == 0){
        if(silent && dpecode() == ENOITEM) return false;
        throw new DepotException(dpecode());
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
   * @throws DepotException if an error occurs, no record corresponds, or the size of the value
   * of the corresponding record is less than the index specified by the parameter `start'.
   */
  public byte[] get(byte[] key, int start, int max) throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      byte[] val = dpget(index, key, key.length, start, max);
      if(val == null){
        if(silent && dpecode() == ENOITEM) return null;
        throw new DepotException(dpecode());
      }
      return val;
    }
  }
  /**
   * Retrieve whole value of a record.
   * The same as `get(key, 0, -1)'.
   * @see #get(byte[], int, int)
   */
  public byte[] get(byte[] key) throws DepotException {
    return get(key, 0, -1);
  }
  /**
   * Get the size of the value of a record.
   * @param key a byte array of a key.
   * @return the size of the value of the corresponding record.  If the silent flag is true and
   * no record corresponds, -1 is returned instead of exception.
   * @throws DepotException if an error occurs or no record corresponds.
   * @note Because this method does not read the entity of a record, it is faster than `get'.
   */
  public int vsiz(byte[] key) throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      int rv = dpvsiz(index, key, key.length);
      if(rv == -1){
        if(silent && dpecode() == ENOITEM) return -1;
        throw new DepotException(dpecode());
      }
      return rv;
    }
  }
  /**
   * Initialize the iterator of the database handle.
   * @throws DepotException if an error occurs.
   * @note The iterator is used in order to access the key of every record stored in a database.
   */
  public void iterinit() throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      if(dpiterinit(index) == 0) throw new DepotException(dpecode());
    }
  }
  /**
   * Get the next key of the iterator.
   * @return a byte array of the key of the next record.  If the silent flag is true and no
   * record corresponds, `null' is returned instead of exception.
   * @throws DepotException if an error occurs or no record corresponds.
   * @note It is possible to access every record by iteration of calling this method.
   * However, it is not assured if updating the database is occurred while the iteration.
   * Besides, the order of this traversal access method is arbitrary, so it is not assured
   * that the order of storing matches the one of the traversal access.
   */
  public byte[] iternext() throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      byte[] val = dpiternext(index);
      if(val == null){
        if(silent && dpecode() == ENOITEM) return null;
        throw new DepotException(dpecode());
      }
      return val;
    }
  }
  /**
   * Set alignment of the database handle.
   * @param align the basic size of alignment.
   * @throws DepotException if an error occurs.
   * @note If alignment is set to a database, the efficiency of overwriting values is improved.
   * The size of alignment is suggested to be average size of the values of the records to be
   * stored.  If alignment is positive, padding whose size is multiple number of the alignment
   * is placed.  If alignment is negative, as `vsiz' is the size of a value, the size of padding
   * is calculated with `(vsiz / pow(2, abs(align) - 1))'.  Because alignment setting is not
   * saved in a database, you should specify alignment every opening a database.
   */
  public void setalign(int align) throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      if(dpsetalign(index, align) == 0) throw new DepotException(dpecode());
    }
  }
  /**
   * Set the size of the free block pool.
   * @param size the size of the free block pool of a database.
   * @throws DepotException if an error occurs.
   * @note The default size of the free block pool is 16.  If the size is greater, the space
   * efficiency of overwriting values is improved with the time efficiency sacrificed.
   */
  public void setfbpsiz(int size) throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      if(dpsetfbpsiz(index, size) == 0) throw new DepotException(dpecode());
    }
  }
  /**
   * Synchronize updating contents with the file and the device.
   * @throws DepotException if an error occurs.
   * @note This method is useful when another process uses the connected database file.
   */
  public void sync() throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      if(dpsync(index) == 0) throw new DepotException(dpecode());
    }
  }
  /**
   * Optimize the database.
   * @param bnum the number of the elements of the bucket array.  If it is not more than 0, the
   * default value is specified.
   * @throws DepotException if an error occurs.
   * @note In an alternating succession of deleting and storing with overwrite or concatenate,
   * dispensable regions accumulate.  This method is useful to do away with them.
   */
  public void optimize(int bnum) throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      if(dpoptimize(index, bnum) == 0) throw new DepotException(dpecode());
    }
  }
  /**
   * Get the name of the database.
   * @return the string of the name of the database.
   * @throws DepotException if an error occurs.
   */
  public String name() throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      String buf = dpname(index);
      if(buf == null) throw new DepotException(dpecode());
      return buf;
    }
  }
  /**
   * Get the size of the database file.
   * @return the size of the database file.
   * @throws DepotException if an error occurs.
   */
  public int fsiz() throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      int rv = dpfsiz(index);
      if(rv == -1) throw new DepotException(dpecode());
      return rv;
    }
  }
  /**
   * Get the number of the elements of the bucket array.
   * @return the number of the elements of the bucket array.
   * @throws DepotException if an error occurs.
   */
  public int bnum() throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      int rv = dpbnum(index);
      if(rv == -1) throw new DepotException(dpecode());
      return rv;
    }
  }
  /**
   * Get the number of the used elements of the bucket array.
   * @return the number of the used elements of the bucket array.
   * @throws DepotException if an error occurs.
   * @note This method is inefficient because it accesses all elements of the bucket array.
   */
  public int busenum() throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      int rv = dpbusenum(index);
      if(rv == -1) throw new DepotException(dpecode());
      return rv;
    }
  }
  /**
   * Get the number of the records stored in the database.
   * @return the number of the records stored in the database.
   * @throws DepotException if an error occurs.
   */
  public int rnum() throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      int rv = dprnum(index);
      if(rv == -1) throw new DepotException(dpecode());
      return rv;
    }
  }
  /**
   * Check whether the database handle is a writer or not.
   * @return true if the handle is a writer, false if not.
   * @throws DepotException if an error occurs.
   */
  public boolean writable() throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      return dpwritable(index) == 0 ? false : true;
    }
  }
  /**
   * Check whether the database has a fatal error or not.
   * @return true if the database has a fatal error, false if not.
   * @throws DepotException if an error occurs.
   */
  public boolean fatalerror() throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      return dpfatalerror(index) == 0 ? false : true;
    }
  }
  /**
   * Get the inode number of the database.
   * @return the inode number of the database file.
   * @throws DepotException if an error occurs.
   */
  public int inode() throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      return dpinode(index);
    }
  }
  /**
   * Get the last modified time of the database.
   * @return the last modified time of the database.
   * @throws DepotException if an error occurs.
   */
  public long mtime() throws DepotException {
    if(index < 0) throw new DepotException();
    synchronized(ADBM.class){
      return dpmtime(index);
    }
  }
  /**
   * Store a record.
   * @param key a byte array of a key.
   * @param val a byte array of a value.
   * @param replace whether the existing value is to be overwritten or not.
   * @throws DepotException if an error occurs or replace is cancelled.
   */
  public void store(byte[] key, byte[] val, boolean replace) throws DepotException {
    if(!put(key, val, replace ? DOVER : DKEEP)) throw new DepotException(EKEEP);
  }
  /**
   * Delete a record.
   * @param key a byte array of a key.
   * @throws DepotException if an error occurs or no record corresponds.
   */
  public void delete(byte[] key) throws DepotException {
    if(!out(key)) throw new DepotException(ENOITEM);
  }
  /**
   * Fetch a record.
   * @param key a byte array of a key.
   * @return a byte array of the value of the corresponding record.
   * @throws DepotException if an error occurs or no record corresponds.
   */
  public byte[] fetch(byte[] key) throws DepotException {
    byte[] vbuf = get(key, 0, -1);
    if(vbuf == null) throw new DepotException(ENOITEM);
    return vbuf;
  }
  /**
   * Get the first key.
   * @return a byte array of the key of the first record.
   * @throws DepotException if an error occurs or no record corresponds.
   */
  public byte[] firstkey() throws DepotException {
    iterinit();
    byte[] kbuf = iternext();
    if(kbuf == null) throw new DepotException(ENOITEM);
    return kbuf;
  }
  /**
   * Get the next key.
   * @return a byte array of the key of the next record.
   * @throws DepotException if an error occurs or no record corresponds.
   */
  public byte[] nextkey() throws DepotException {
    byte[] kbuf = iternext();
    if(kbuf == null) throw new DepotException(ENOITEM);
    return kbuf;
  }
  /**
   * Check whether a fatal error occured or not.
   * @return true if the database has a fatal error, false if not.
   * @throws DepotException if an error occurs.
   */
  public boolean error() throws DepotException {
    return fatalerror();
  }
  //----------------------------------------------------------------
  // native methods
  //----------------------------------------------------------------
  private static synchronized final native void dpinit();
  private static synchronized final native String dpversion();
  private static synchronized final native int dpecode();
  private static synchronized final native String dperrmsg(int ecode);
  private static synchronized final native int dpopen(String name, int omode, int bnum);
  private static synchronized final native int dpclose(int index);
  private static synchronized final native int dpput(int index, byte[] key, int ksiz,
                                                     byte[] val, int vsiz, int dmode);
  private static synchronized final native int dpout(int index, byte[] key, int ksiz);
  private static synchronized final native byte[] dpget(int index, byte[] key, int ksiz,
                                                        int start, int max);
  private static synchronized final native int dpvsiz(int index, byte[] key, int ksiz);
  private static synchronized final native int dpiterinit(int index);
  private static synchronized final native byte[] dpiternext(int index);
  private static synchronized final native int dpsetalign(int index, int align);
  private static synchronized final native int dpsetfbpsiz(int index, int size);
  private static synchronized final native int dpsync(int index);
  private static synchronized final native int dpoptimize(int index, int bnum);
  private static synchronized final native String dpname(int index);
  private static synchronized final native int dpfsiz(int index);
  private static synchronized final native int dpbnum(int index);
  private static synchronized final native int dpbusenum(int index);
  private static synchronized final native int dprnum(int index);
  private static synchronized final native int dpwritable(int index);
  private static synchronized final native int dpfatalerror(int index);
  private static synchronized final native int dpinode(int index);
  private static synchronized final native long dpmtime(int index);
  private static synchronized final native int dpremove(String name);
  private static synchronized final native byte[] dpsnaffle(String name, byte[] key, int ksiz);
}



/* END OF FILE */
