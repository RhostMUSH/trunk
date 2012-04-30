/*************************************************************************************************
 * The Java API of cursor functions of Villa
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
 * The Java API of cursor functions of Villa
 * This class depends on the native library `jqdbm'.
 */
public class VillaCursor {
  //----------------------------------------------------------------
  // instance fields
  //----------------------------------------------------------------
  /** pointer to the native object */
  private long coreptr;
  /** database handle */
  private Villa villa;
  //----------------------------------------------------------------
  // constructors and finalizers
  //----------------------------------------------------------------
  /**
   * Get a multiple cursor handle.
   * @param villa a database handle connected as a reader.
   * @throws VillaException if an error occurs.
   */
  VillaCursor(Villa villa) throws VillaException {
    this.villa = villa;
    synchronized(ADBM.class){
      int index = villa.getindex();
      if(index < 0) throw new VillaException();
      if(vlmulcurnew(index) == 0) throw new VillaException(vlmulcurecode());
    }
  }
  /**
   * Release resources.
   */
  protected void finalize(){
    vlmulcurdelete();
  }
  //----------------------------------------------------------------
  // public or protected methods
  //----------------------------------------------------------------
  /**
   * Move the multiple cursor to the first record.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throws VillaException if an error occurs or there is no record in the database.
   */
  public boolean first() throws VillaException {
    synchronized(ADBM.class){
      int index = villa.getindex();
      if(index < 0) throw new VillaException();
      if(vlmulcurfirst(index) == 0){
        if(villa.silent && vlmulcurecode() == Villa.ENOITEM) return false;
        throw new VillaException(vlmulcurecode());
      }
      return true;
    }
  }
  /**
   * Move the multiple cursor to the last record.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throws VillaException if an error occurs or there is no record in the database.
   */
  public boolean last() throws VillaException {
    synchronized(ADBM.class){
      int index = villa.getindex();
      if(index < 0) throw new VillaException();
      if(vlmulcurlast(index) == 0){
        if(villa.silent && vlmulcurecode() == Villa.ENOITEM) return false;
        throw new VillaException(vlmulcurecode());
      }
      return true;
    }
  }
  /**
   * Move the multiple cursor to the next record.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throws VillaException if an error occurs or there is no previous record.
   */
  public boolean prev() throws VillaException {
    synchronized(ADBM.class){
      int index = villa.getindex();
      if(index < 0) throw new VillaException();
      if(vlmulcurprev(index) == 0){
        if(villa.silent && vlmulcurecode() == Villa.ENOITEM) return false;
        throw new VillaException(vlmulcurecode());
      }
      return true;
    }
  }
  /**
   * Move the multiple cursor to the next record.
   * @return always true.  However, if the silent flag is true and no record corresponds, false
   * is returned instead of exception.
   * @throws VillaException if an error occurs or there is no next record.
   */
  public boolean next() throws VillaException {
    synchronized(ADBM.class){
      int index = villa.getindex();
      if(index < 0) throw new VillaException();
      if(vlmulcurnext(index) == 0){
        if(villa.silent && vlmulcurecode() == Villa.ENOITEM) return false;
        throw new VillaException(vlmulcurecode());
      }
      return true;
    }
  }
  /**
   * Move the multiple cursor to a position around a record.
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
  public boolean jump(byte[] key, int jmode) throws VillaException {
    synchronized(ADBM.class){
      int index = villa.getindex();
      if(index < 0) throw new VillaException();
      if(vlmulcurjump(index, key, key.length, jmode) == 0){
        if(villa.silent && vlmulcurecode() == Villa.ENOITEM) return false;
        throw new VillaException(vlmulcurecode());
      }
      return true;
    }
  }
  /**
   * Move the multiple cursor to a position around a record for stepping forward.
   * The same as `jump(key, Villa.JFORFARD)'.
   * @see #jump(byte[], int)
   */
  public boolean jump(byte[] key) throws VillaException {
    return jump(key, Villa.JFORWARD);
  }
  /**
   * Move the multiple cursor to a position around a record composed of serializable objects.
   * The same as `jump(qdbm.Util.serialize(key), jmode)'.
   * @see #jump(byte[], int)
   * @note If serialization is failed, an instance of `VillaException' is thrown.
   */
  public boolean jumpobj(Object key, int jmode) throws VillaException {
    byte[] kbuf = Util.serialize(key);
    if(kbuf == null) throw new VillaException();
    return jump(kbuf, jmode);
  }
  /**
   * Get the key of the record where the multiple cursor is.
   * @return a byte array of the key of the corresponding record.  If the silent flag is true and
   * no record corresponds, `null' is returned instead of exception.
   * @throws VillaException if an error occurs or no record corresponds to the cursor.
   */
  public byte[] key() throws VillaException {
    synchronized(ADBM.class){
      int index = villa.getindex();
      if(index < 0) throw new VillaException();
      byte[] val = vlmulcurkey(index);
      if(val == null){
        if(villa.silent && vlmulcurecode() == Villa.ENOITEM) return null;
        throw new VillaException(vlmulcurecode());
      }
      return val;
    }
  }
  /**
   * Get the key of the record composed of serializable objects, where the multiple cursor is.
   * The same as `qdbm.Util.deserialize(key())'.
   * @see #key()
   * @note If serialization is failed, an instance of `VillaException' is thrown.
   */
  public Object keyobj() throws VillaException {
    byte[] kbuf = key();
    if(kbuf == null) return null;
    Object key = Util.deserialize(kbuf);
    if(key == null) throw new VillaException();
    return key;
  }
  /**
   * Get the value of the record where the multiple cursor is.
   * @return a byte array of the value of the corresponding record.  If the silent flag is true
   * and no record corresponds, `null' is returned instead of exception.
   * @throws VillaException if an error occurs or no record corresponds to the cursor.
   */
  public byte[] val() throws VillaException {
    synchronized(ADBM.class){
      int index = villa.getindex();
      if(index < 0) throw new VillaException();
      byte[] val = vlmulcurval(index);
      if(val == null){
        if(villa.silent && vlmulcurecode() == Villa.ENOITEM) return null;
        throw new VillaException(vlmulcurecode());
      }
      return val;
    }
  }
  /**
   * Get the value of the record where the multiple cursor is.
   * The same as `qdbm.Util.deserialize(val())'.
   * @see #val()
   * @note If serialization is failed, an instance of `VillaException' is thrown.
   */
  public Object valobj() throws VillaException {
    byte[] vbuf = val();
    if(vbuf == null) return vbuf;
    Object val = Util.deserialize(vbuf);
    if(val == null) throw new VillaException();
    return val;
  }
  //----------------------------------------------------------------
  // native methods
  //----------------------------------------------------------------
  private static synchronized final native int vlmulcurecode();
  private synchronized final native int vlmulcurnew(int index);
  private synchronized final native void vlmulcurdelete();
  private synchronized final native int vlmulcurfirst(int index);
  private synchronized final native int vlmulcurlast(int index);
  private synchronized final native int vlmulcurprev(int index);
  private synchronized final native int vlmulcurnext(int index);
  private synchronized final native int vlmulcurjump(int index, byte[] key, int ksiz, int jmode);
  private synchronized final native byte[] vlmulcurkey(int index);
  private synchronized final native byte[] vlmulcurval(int index);
}



/* END OF FILE */
