/*************************************************************************************************
 * Class of utility methods
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

import java.io.IOException;
import java.io.Serializable;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;



/**
 * Class of utility methods.
 * This class depends on the native library `jqdbm'.
 */
public class Util {
  //----------------------------------------------------------------
  // public static constants
  //----------------------------------------------------------------
  /** size of a long integer */
  private static final int LONGSIZE = 8;
  private static final int IOBUFSIZ = 8192;
  //----------------------------------------------------------------
  // static initializer
  //----------------------------------------------------------------
  static {
    try {
      System.loadLibrary("jqdbm");
    } catch(UnsatisfiedLinkError e){
      e.printStackTrace();
    }
  }
  //----------------------------------------------------------------
  // public static methods
  //----------------------------------------------------------------
  /**
   * Serialize an object.
   * @param obj a serializable object.
   * @return a byte array of the serialized object or null if an error occurs.
   */
  public static byte[] serialize(Object obj){
    ByteArrayOutputStream baos = new ByteArrayOutputStream();
    ObjectOutputStream oos = null;
    try {
      oos = new ObjectOutputStream(baos);
      oos.writeObject(obj);
      oos.flush();
      baos.flush();
      return baos.toByteArray();
    } catch(IOException e){
      return null;
    } finally {
      try {
        if(oos != null) oos.close();
      } catch(IOException e){}
    }
  }
  /**
   * Redintegrate a serialized object.
   * @param serial a byte array of the serialized object.
   * @return an original object or null if an error occurs.
   */
  public static Object deserialize(byte[] serial){
    ByteArrayInputStream bais = new ByteArrayInputStream(serial);
    ObjectInputStream ois = null;
    try {
      ois = new ObjectInputStream(bais);
      return ois.readObject();
    } catch(IOException e){
      return null;
    } catch(ClassNotFoundException e){
      return null;
    } finally {
      try {
        if(ois != null) ois.close();
      } catch(IOException e){}
    }
  }
  /**
   * Serialize a long integer.
   * @param num a long integer.
   * @return a byte array of the serialized long integer.
   */
  public static byte[] serializeLong(long num){
    byte[] serial = new byte[LONGSIZE];
    for(int i = 0; i < LONGSIZE; i++){
      serial[LONGSIZE-i-1] = (byte)(num & 0xff);
      num >>= 8;
    }
    return serial;
  }
  /**
   * Redintegrate a serialized long integer.
   * @param serial a byte array of a serialized long integer.
   * @return the long value.
   * @throws IllegalArgumentException thrown if the size of the array is invalid.
   */
  public static long deserializeLong(byte[] serial) throws IllegalArgumentException {
    if(serial.length != LONGSIZE) throw new IllegalArgumentException();
    long num = 0;
    for(int i = 0; i < LONGSIZE; i++){
      num *= 256;
      num += serial[i] + ((serial[i] < 0) ? 256 : 0);
    }
    return num;
  }
  /**
   * Get a formatted decimal string made from a number.
   * @param num a number.
   * @param cols the number of columns.  The result string may be longer than it.
   * @param padding a padding character to fulfil columns with.
   */
  public static String numstr(int num, int cols, char padding){
    StringBuffer sb = new StringBuffer(cols);
    boolean minus = false;
    if(num < 0){
      num *= -1;
      cols--;
      minus = true;
    }
    int i = 0;
    while(num > 0){
      sb.insert(0, num % 10);
      num /= 10;
      i++;
    }
    while(i < cols){
      sb.insert(0, padding);
      i++;
    }
    if(minus) sb.insert(0, '-');
    return sb.toString();
  }
  /**
   * Read whole data of a file.
   * @param path the path of a file.
   * @return while data of a file on success, or null on failure.
   */
  public static byte[] readFile(String path){
    InputStream is = null;
    ByteArrayOutputStream baos = null;
    try {
      is = new FileInputStream(path);
      baos = new ByteArrayOutputStream();
      byte[] buf = new byte[IOBUFSIZ];
      int len;
      while((len = is.read(buf)) != -1){
        baos.write(buf, 0, len);
      }
    } catch(IOException e){
      return null;
    } finally {
      try {
        if(baos != null) baos.close();
      } catch(IOException e){}
      try {
        if(is != null) is.close();
      } catch(IOException e){}
    }
    return baos.toByteArray();
  }
  /**
   * Write whole data to a file.
   * @param path the path of a file.
   * @param data data to write.
   * @return true if success, false on failure.
   */
  public static boolean writeFile(String path, byte[] data){
    OutputStream os = null;
    try {
      os = new FileOutputStream(path);
      os.write(data);
    } catch(IOException e){
      return false;
    } finally {
      try {
        if(os != null) os.close();
      } catch(IOException e){}
    }
    return true;
  }
  //----------------------------------------------------------------
  // public static native methods
  //----------------------------------------------------------------
  /**
   * Execute a shell command using the native function `system' defined in POSIX and ANSI C.
   * @param cmd a command line.
   * @return the return value of the function.  It depends on the native system.
   */
  public static synchronized native int system(String cmd);
  /**
   * Change current working directory using the native function `chdir' defined in POSIX.
   * @param path the path of a directory.
   * @return 0 on success, or -1 on failure.
   */
  public static synchronized native int chdir(String path);
  /**
   * Get current working directory using the native function `getcwd' defined in POSIX.
   * @return the path of the current working directory or null on failure.
   */
  public static synchronized native String getcwd();
  /**
   * Get process identification using the native function `getpid' defined in POSIX.
   * @return the process ID of the current process.
   */
  public static synchronized native int getpid();
  /**
   * Get an environment variable using the native function `getenv' defined in POSIX and ANSI C.
   * @param name the name of an environment variable.
   * @return the value of the variable, or null if it does not exist.
   */
  public static synchronized native String getenv(String name);
  //----------------------------------------------------------------
  // private methods
  //----------------------------------------------------------------
  /* Default constructor.
   * This should not be used. */
  private Util() throws Exception {
    throw new Exception();
  }
}



/* END OF FILE */
